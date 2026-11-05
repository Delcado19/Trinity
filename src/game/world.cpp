#include "world.h"

#include <cmath>
#include <atomic>
#include <limits>
#include <cstdint>
#include <cstring>
#include <initializer_list>

#include "offsets.h"
#include "../mem/scanner.h"
#include "../mem/safe_memory.h"
#include "../mem/hooks.h"
#include "../core/logger.h"
#include "../core/state.h"
#include "../core/gameversion.h"

namespace trinity::game
{
    using mem::Write8;
    using mem::Write32;
    using mem::Read32;

    namespace
    {
        // Reinterpret a float as its 32-bit pattern for a raw Write32.
        inline uint32_t FloatBits(float f)
        {
            uint32_t bits = 0;
            std::memcpy(&bits, &f, sizeof(bits));
            return bits;
        }

        inline float Clamp(float v, float lo, float hi)
        {
            return v < lo ? lo : (v > hi ? hi : v);
        }

        // Master frame updater hook (sub_140947B60):
        // Sets TimeManager.mode (+0x50) = 1 and TimeManager.timeScale (+0x54) = mult directly
        // on the live engine TimeManager before simulation steps are computed.
        using MasterFrameUpdate_t = void(__fastcall*)(void* ctx);
        MasterFrameUpdate_t oMasterFrameUpdate = nullptr;
        void*               g_masterFrameTarget = nullptr;
        size_t              g_masterFrameWhich  = 0; // matched pattern index - see World::Status()

        void __fastcall hkMasterFrameUpdate(void* ctx)
        {
            if (ctx)
            {
                uintptr_t timeMgr = 0;
                if (mem::ReadPtr(reinterpret_cast<uintptr_t>(ctx) + 0x60, &timeMgr) && timeMgr >= kMinPointer)
                {
                    // timeScale moved +8 on build 3446 (2.04.02) - see
                    // kOff_TimeManager_TimeScale_TU20402 in offsets.h.
                    const uintptr_t timeScaleOff = CurrentGameVersion().revision >= 3446
                        ? kOff_TimeManager_TimeScale_TU20402 : kOff_TimeManager_TimeScale;
                    const State& st = State::Get();
                    if (st.gameSpeed)
                    {
                        const float mult = Clamp(st.gameSpeedMult, 0.1f, 5.0f);
                        Write8(timeMgr + 0x50, 1);
                        Write32(timeMgr + timeScaleOff, FloatBits(mult));
                    }
                    else
                    {
                        Write8(timeMgr + 0x50, 0);
                        Write32(timeMgr + timeScaleOff, FloatBits(1.0f));
                    }
                }
            }
            oMasterFrameUpdate(ctx);
        }

        // Resolved addresses of the fixed-timestep override globals (BSS - zero
        // in the static image, so found via the override block's RIP operands).
        // Zero if the signature did not resolve, in which case Game Speed is
        // inert (Tick no-ops).
        uintptr_t g_flagAddr  = 0; // byte  byte_606B9CE : 1 forces the fixed step
        uintptr_t g_valueAddr = 0; // float dword_615A4F0 : forced seconds-per-frame
        uintptr_t g_speedPatchAddr = 0;
        uint8_t   g_speedOrigBytes[9] = {};

        // Whether we currently hold the override on. Lets Tick clear the flag
        // exactly once when the toggle is switched off, restoring the engine's
        // own real-time delta without fighting the game every frame afterwards.
        bool g_applied = false;

        // Master field-clock globals (client / server realm), each the base of
        // a 32-byte int32 time struct (day/hour/min/sec). Zero if the signature
        // did not resolve, in which case Advance Time is inert.
        uintptr_t g_timeClient = 0;
        uintptr_t g_timeServer = 0;

        // --- Freeze the visible SUN: render-manager clamp --------------------
        // Address of the engine-object global (qword_648F688). The render
        // TimeOfDay manager hangs off it and drives the sun independently of
        // the field clock, so the field-time freeze above does not hold it.
        // See offsets.h kSig_TodEngineGlobal. Zero if it did not resolve.
        uintptr_t g_todEngineGlobal = 0;

        // While frozen we pin the manager's lower==upper==g_todTargetHour so
        // the engine clamps the sun there. Captured once on enable; the
        // originals are restored on disable/unload.
        bool  g_todClampApplied = false;
        float g_todOrigLower    = 0.0f;
        float g_todOrigUpper    = 0.0f;
        float g_todTargetHour   = 0.0f; // 0..24; Advance steps this while frozen

        // --- Freeze Time of Day: the field-time tick hook --------------------
        // sub_871360 advances the whole day/night clock by adding the frame
        // delta to its float accumulator ([mgr+0x2C] += delta) before deriving
        // the realm clock globals + the sun from it. Freeze = force that delta
        // to 0 while engaged, so time simply stops accruing - no globals to
        // pin, no cross-thread race (see offsets.h kSig_FieldTimeTick). Only
        // the clock stops; physics, AI and combat keep running.
        //
        // The delta rides in xmm1 as a single float; the prototype declares it
        // so the register survives the trampoline untouched on pass-through.
        using FieldTimeTick_t = void(__fastcall*)(void* mgr, float delta);
        FieldTimeTick_t oFieldTimeTick = nullptr;
        void*  g_fieldTimeTickTarget = nullptr;
        size_t g_fieldTimeTickWhich  = 0; // matched pattern index - see World::TimeOfDayStatus()

        std::atomic<int> g_pendingHours{0};
        void ApplyPendingHours(void* mgr);

        void __fastcall hkFieldTimeTick(void* mgr, float delta)
        {
            ApplyPendingHours(mgr);
            if (State::Get().timeFrozen)
                delta = 0.0f; // clock stops accruing; sun + numeric clock hold
            oFieldTimeTick(mgr, delta);
        }



        bool ReadI32(uintptr_t addr, int* out)
        {
            uint32_t bits = 0;
            if (!Read32(addr, &bits)) return false;
            *out = static_cast<int>(bits);
            return true;
        }

        bool ReadF32(uintptr_t addr, float* out)
        {
            uint32_t bits = 0;
            if (!Read32(addr, &bits)) return false;
            std::memcpy(out, &bits, sizeof(*out));
            return true;
        }

        // Resolve the live render TimeOfDay manager: engine = *g_todEngineGlobal,
        // manager = *(engine + 0x2F8). Returns 0 until the engine is up (early
        // load) or if the global did not resolve - callers no-op on 0.
        uintptr_t ResolveTodManager()
        {
            if (!g_todEngineGlobal) return 0;
            uintptr_t engine = 0;
            if (!mem::ReadPtr(g_todEngineGlobal, &engine) || engine < kMinPointer)
                return 0;
            uintptr_t mgr = 0;
            if (!mem::ReadPtr(engine + kOff_Tod_Manager, &mgr) || mgr < kMinPointer)
                return 0;
            return mgr;
        }

        bool WriteI32(uintptr_t addr, int v)
        {
            return Write32(addr, static_cast<uint32_t>(v));
        }

        // Write day/hour to BOTH realm globals (min/sec left untouched). The
        // reading realm depends on the reader thread's TLS selector, so we
        // always write both. Guarded per field.
        void WriteClockDayHour(int day, int hour)
        {
            for (uintptr_t g : { g_timeClient, g_timeServer })
            {
                if (!g) continue;
                WriteI32(g + kOff_FieldTime_Day,  day);
                WriteI32(g + kOff_FieldTime_Hour, hour);
            }
        }

        void ApplyPendingHours(void* fieldMgr)
        {
            const int hours = g_pendingHours.exchange(0);
            if (!hours || !fieldMgr) return;
            int day = 0, hour = 0;
            float seconds = 0.0f;
            const uintptr_t mgr = reinterpret_cast<uintptr_t>(fieldMgr);
            if (!ReadI32(g_timeClient + kOff_FieldTime_Day, &day) ||
                !ReadI32(g_timeClient + kOff_FieldTime_Hour, &hour) ||
                !ReadF32(mgr + 0x2C, &seconds) || !std::isfinite(seconds) || seconds < 0 ||
                day < 0 || hour < 0 || hour > 23) return;
            int newDay = 0, newHour = 0;
            if (!detail::ShiftClockHours(day, hour, hours, newDay, newHour)) return;
            // Lian v1.4.2 updates +0x2C as well as the realm mirrors. Otherwise
            // the next field tick reconstructs the old hour. Preserve sub-hour time.
            const float nextSeconds = newHour * 3600.0f + std::fmod(seconds, 3600.0f);
            if (!Write32(mgr + 0x2C, FloatBits(nextSeconds))) return;
            WriteClockDayHour(newDay, newHour);
            const float renderHour = nextSeconds / 3600.0f;
            if (const uintptr_t tod = ResolveTodManager())
                Write32(tod + kOff_Tod_CurrentHour, FloatBits(renderHour));
            if (g_todClampApplied) g_todTargetHour = renderHour;
            LOG("world: time step %+d requested; day=%d hour=%d (runtime persistence requires verification).",
                hours, newDay, newHour);
        }

        // --- Master field-clock discovery ------------------------------------
        // One signature over sub_1CA3890's realm-select read yields both realm
        // globals: resolve the RIP operands of the two `vmovups` (server, then
        // client). See offsets.h kSig_FieldTimeRealm.
        bool ResolveFieldTimeGlobals()
        {
            // Falls back to the build 3460 candidate (offsets.h: cross-validated
            // by a third-party trainer's own runtime capture of these same two
            // globals) when the verified source pattern does not match.
            size_t which = 0;
            const uintptr_t m = mem::FindPatternAny(
                {kSig_FieldTimeRealm, kSig_FieldTimeRealm_TU3460_Candidate}, &which);
            if (!m || mem::CountMatches(which == 0 ? kSig_FieldTimeRealm
                                                    : kSig_FieldTimeRealm_TU3460_Candidate, 2) != 1)
                return false;
            if (which > 0)
                LOG_WARN("world: field-clock matched fallback pattern #%zu - this game "
                         "build moved the current one, so please report it.", which);
            g_timeServer = mem::ResolveRipAt(m + kOff_FieldTime_ServerVmovups, kLen_FieldTime_Vmovups);
            g_timeClient = mem::ResolveRipAt(m + kOff_FieldTime_ClientVmovups, kLen_FieldTime_Vmovups);
            if (g_timeClient < kMinPointer || g_timeServer < kMinPointer)
            {
                g_timeClient = g_timeServer = 0;
                return false;
            }
            return true;
        }
    }

    namespace
    {
        // Idempotent (guarded by "if resolved, skip") even though nothing
        // currently calls this twice - see the shortcut note below Install().
        void ResolveWorldHooks()
        {
            // Falls back to the TU 2.01.00 candidate (COMPATIBILITY.md:
            // "BREAKTHROUGH 2026-09-08") when the primary (TU 2.00.00) pattern
            // does not match - same recompiled-prologue shape as the pathing-
            // helper/dye-upsert fixes from the same session.
            if (!oMasterFrameUpdate &&
                mem::InstallHookAny("world: master frame update",
                                    {kSig_MasterFrameUpdate, kSig_MasterFrameUpdate_TU20100_Candidate,
                                     kSig_MasterFrameUpdate_TU20402_Candidate},
                                    "Game Speed timescale unavailable",
                                    reinterpret_cast<void*>(hkMasterFrameUpdate),
                                    reinterpret_cast<void**>(&oMasterFrameUpdate),
                                    &g_masterFrameTarget, &g_masterFrameWhich))
            {
                LOG("world: master frame update hook installed @ %p - Game Speed ready.", g_masterFrameTarget);
            }

            if (!g_flagAddr || !g_valueAddr)
            {
                const uintptr_t m = mem::FindPattern(kSig_GameSpeed);
                const size_t gameSpeedMatches = mem::CountMatches(kSig_GameSpeed, 2);
                if (!m || gameSpeedMatches != 1)
                {
                    LOG_ERR("world: game-speed signature %s - Game Speed disabled.",
                            m ? "ambiguous" : "NOT FOUND");
                }
                else
                {
                    // Flag: disp32 of `cmp cs:byte_606B9CE, 1` (an imm follows the disp,
                    // so resolve from the explicit disp/next-instr rather than ResolveRipAt).
                    g_flagAddr = mem::ResolveRip(m + kOff_GameSpeed_FlagDisp, m + kOff_GameSpeed_FlagEnd);
                    // Value: `vmovss xmm0, cs:dword_615A4F0` - a standard RIP instr (disp
                    // at its tail), so ResolveRipAt handles it.
                    g_valueAddr = mem::ResolveRipAt(m + kOff_GameSpeed_ValueVmovss, kLen_GameSpeed_Vmovss);

                    if (g_flagAddr < kMinPointer || g_valueAddr < kMinPointer)
                    {
                        LOG_ERR("world: game-speed globals resolved out of range - Game Speed disabled.");
                        g_flagAddr = g_valueAddr = 0;
                    }
                    else
                    {
                        // Patch the one-shot conditional check in the override block (offset +28)
                        // so forced delta applies continuously on every sub-tick.
                        g_speedPatchAddr = m + 28;
                        memcpy(g_speedOrigBytes, reinterpret_cast<const void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes));
                        DWORD oldProt = 0;
                        if (VirtualProtect(reinterpret_cast<void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes), PAGE_EXECUTE_READWRITE, &oldProt))
                        {
                            memset(reinterpret_cast<void*>(g_speedPatchAddr), 0x90, sizeof(g_speedOrigBytes));
                            VirtualProtect(reinterpret_cast<void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes), oldProt, &oldProt);
                            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes));
                        }
                        else
                            g_speedPatchAddr = 0;
                        LOG_OK("world: game-speed globals resolved - Game Speed ready.");
                    }
                }
            }

            // Time of Day resolves independently - Game Speed still works if this
            // signature drifts, and vice versa. Advance needs the clock globals...
            if (!g_timeClient && !ResolveFieldTimeGlobals())
                LOG_WARN("world: field-clock signature NOT FOUND - Advance Time disabled.");

            // ...Freeze needs the field-time tick hook (zeroes the clock's delta,
            // which holds the numeric clock)...
            // Independent of both the globals above and Game Speed - each can drift
            // without disabling the others.
            if (!oFieldTimeTick)
                mem::InstallHookAny("world: field-time tick",
                                    {kSig_FieldTimeTick, kSig_FieldTimeTick_TU3479_Candidate},
                                    "Freeze Time of Day and Shift Time disabled",
                                    reinterpret_cast<void*>(hkFieldTimeTick),
                                    reinterpret_cast<void**>(&oFieldTimeTick),
                                    &g_fieldTimeTickTarget, &g_fieldTimeTickWhich);

            // ...and the render-manager clamp holds the visible SUN (the field-time
            // tick alone does not - the sun rides its own accumulator). Resolve the
            // engine-object global here; the manager itself is read live each Tick.
            if (!g_todEngineGlobal)
            {
                const uintptr_t g = mem::FindPattern(kSig_TodEngineGlobal);
                if (!g || mem::CountMatches(kSig_TodEngineGlobal, 2) != 1)
                    LOG_WARN("world: TOD engine-global signature NOT FOUND - sun freeze disabled.");
                else
                {
                    g_todEngineGlobal = mem::ResolveRipAt(g + kOff_TodEngineGlobal_Mov,
                                                          kLen_TodEngineGlobal_Mov);
                    if (g_todEngineGlobal < kMinPointer)
                    {
                        LOG_ERR("world: TOD engine-global resolved out of range - sun freeze disabled.");
                        g_todEngineGlobal = 0;
                    }
                }
            }
        }

        // shortcut: no Tick()-driven retry here (removed 2026-11-02) - a
        // live run proved these never actually resolve on retry on this
        // build (3+ minutes, zero successes), so re-scanning every ~2-15s
        // only burned game-thread time re-running several multi-candidate
        // AOB scans back to back, which is what froze the game. Re-add a
        // retry only once a specific signature is confirmed to be a
        // transient packing-timing miss (like player.cpp's damage-apply),
        // not a permanently moved one.
    }

    bool World::Install()
    {
        ResolveWorldHooks();
        return oMasterFrameUpdate || (g_flagAddr && g_valueAddr) || oFieldTimeTick;
    }

    void World::Tick()
    {
        const State& st = State::Get();

        // Game Speed - only if its globals resolved; independent of the sun
        // freeze below, so a Game Speed signature drift never disables Freeze.
        if ((g_flagAddr && g_valueAddr) && st.gameSpeed)
        {
            // Forced frame delta = mult / 60: the engine's own fixed-timestep
            // reference is the 60-FPS step (1/60 s), so 1.00x reproduces it and
            // the multiplier scales sim time from there. Clamp both the factor
            // (to the slider's range) and the resulting delta (defensively, so a
            // bad value can never feed the sim an absurd timestep).
            const float mult  = Clamp(st.gameSpeedMult, 0.1f, 5.0f);
            const float delta = Clamp(mult / kGameSpeed_BaselineFps, 1.0e-5f, 1.0f);

            // Value first, then arm both flags (flag 1 = fixed step, flag 2 = high delta / fast step),
            // so the timing update never reads a stale delta on the frame we switch it on.
            // Re-armed every tick so it self-heals if the game clears the flag.
            Write32(g_valueAddr, FloatBits(delta));
            Write8(g_flagAddr, 1);
            Write8(g_flagAddr + 1, 1);
            g_applied = true;
        }
        else if (g_applied)
        {
            // Back to the engine's own measured real-time delta.
            Write8(g_flagAddr, 0);
            Write8(g_flagAddr + 1, 0);
            g_applied = false;
        }

        // Freeze Time of Day: the field-time tick hook (hkFieldTimeTick) holds
        // the NUMERIC clock, but the visible SUN rides the render manager's own
        // accumulator - so pin its clamp here to hold the sun too. Force
        // lower == upper == the captured hour every tick while frozen; the
        // engine clamps currentTimeOfDay to it (real time keeps flowing).
        const uintptr_t mgr = ResolveTodManager();
        if (st.timeFrozen && mgr)
        {
            if (!g_todClampApplied)
            {
                // Capture the originals + the hour to hold, once on enable.
                if (!ReadF32(mgr + kOff_Tod_LowerLimit, &g_todOrigLower)) g_todOrigLower = 0.0f;
                if (!ReadF32(mgr + kOff_Tod_UpperLimit, &g_todOrigUpper)) g_todOrigUpper = 24.0f;
                float cur = 0.0f;
                if (ReadF32(mgr + kOff_Tod_CurrentHour, &cur) && cur >= 0.0f && cur <= 24.0f)
                    g_todTargetHour = cur;
                g_todClampApplied = true;
            }
            const uint32_t bits = FloatBits(g_todTargetHour);
            Write32(mgr + kOff_Tod_LowerLimit, bits);
            Write32(mgr + kOff_Tod_UpperLimit, bits);
        }
        else if (g_todClampApplied)
        {
            // Disabled (or manager lost): restore the engine's own limits once.
            if (mgr)
            {
                Write32(mgr + kOff_Tod_LowerLimit, FloatBits(g_todOrigLower));
                Write32(mgr + kOff_Tod_UpperLimit, FloatBits(g_todOrigUpper));
            }
            g_todClampApplied = false;
        }
    }

    void World::Remove()
    {
        // Leave the game at normal speed on unload.
        if (g_applied && g_flagAddr)
        {
            Write8(g_flagAddr, 0);
            Write8(g_flagAddr + 1, 0);
        }
        if (g_speedPatchAddr)
        {
            DWORD oldProt = 0;
            if (VirtualProtect(reinterpret_cast<void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes), PAGE_EXECUTE_READWRITE, &oldProt))
            {
                memcpy(reinterpret_cast<void*>(g_speedPatchAddr), g_speedOrigBytes, sizeof(g_speedOrigBytes));
                VirtualProtect(reinterpret_cast<void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes), oldProt, &oldProt);
                FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(g_speedPatchAddr), sizeof(g_speedOrigBytes));
            }
            g_speedPatchAddr = 0;
        }
        g_applied  = false;
        g_flagAddr = g_valueAddr = 0;

        // Restore the render manager's time-of-day limits if we were holding
        // the sun, then forget the engine global.
        if (g_todClampApplied)
        {
            const uintptr_t mgr = ResolveTodManager();
            if (mgr)
            {
                Write32(mgr + kOff_Tod_LowerLimit, FloatBits(g_todOrigLower));
                Write32(mgr + kOff_Tod_UpperLimit, FloatBits(g_todOrigUpper));
            }
            g_todClampApplied = false;
        }
        g_todEngineGlobal = 0;

        // Unhook the master frame update hook
        mem::RemoveHook(&g_masterFrameTarget);
        oMasterFrameUpdate = nullptr;

        // Unhook the field-time tick (freeze) and forget the clock globals.
        mem::RemoveHook(&g_fieldTimeTickTarget);
        oFieldTimeTick = nullptr;
        g_timeClient = g_timeServer = 0;
        g_pendingHours.store(0);
    }

    bool World::Ready()
    {
        return g_masterFrameTarget != nullptr || (g_flagAddr >= kMinPointer && g_valueAddr >= kMinPointer);
    }

    bool World::TimeOfDayReady()
    {
        return g_fieldTimeTickTarget && g_timeClient >= kMinPointer && g_timeServer >= kMinPointer;
    }

    mem::SigStatus World::Status()
    {
        // Two independent mechanisms both drive Game Speed (see Install()) -
        // Ready() is their OR, so Dead here means both failed; Ok needs the
        // hook path clean on its current-build pattern AND the globals found.
        if (!Ready()) return mem::SigStatus::Dead;
        const mem::SigStatus hook    = oMasterFrameUpdate ? mem::StatusOf(true, g_masterFrameWhich)
                                                           : mem::SigStatus::Dead;
        const mem::SigStatus globals = (g_flagAddr >= kMinPointer && g_valueAddr >= kMinPointer)
                                            ? mem::SigStatus::Ok : mem::SigStatus::Dead;
        return (hook == mem::SigStatus::Ok && globals == mem::SigStatus::Ok) ? mem::SigStatus::Ok
                                                                              : mem::SigStatus::Fallback;
    }

    mem::SigStatus World::TimeOfDayStatus()
    {
        // The numeric clock (tick hook + globals) is required outright; the
        // separate sun-clamp lever (g_todEngineGlobal) only holds the VISIBLE
        // sun - missing it freezes the clock but lets the sun keep moving, so
        // that is a Fallback, not a Dead.
        if (!TimeOfDayReady()) return mem::SigStatus::Dead;
        const mem::SigStatus hook = mem::StatusOf(true, g_fieldTimeTickWhich);
        const mem::SigStatus sun  = g_todEngineGlobal >= kMinPointer ? mem::SigStatus::Ok : mem::SigStatus::Fallback;
        return mem::Worse(hook, sun);
    }

    bool World::AdvanceTimeOfDayHours(int hours)
    {
        // Buttons queue one bounded step; the field tick owns all clock writes.
        if (!TimeOfDayReady() || !hours || hours < -240 || hours > 240) return false;
        int expected = 0;
        return g_pendingHours.compare_exchange_strong(expected, hours);
    }
}
