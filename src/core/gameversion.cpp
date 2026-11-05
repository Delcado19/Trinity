#include "gameversion.h"

#include <Windows.h>
#include <cstdio>
#include <cstring>

#include "logger.h"

#pragma comment(lib, "Version.lib")

namespace trinity
{
    namespace
    {
        // Builds Trinity has actually been checked against, newest first. A
        // revision not in this table is not necessarily broken - most patches
        // move nothing this mod reads - but it is untested, and the log should
        // say so rather than imply a guarantee that was never made.
        struct KnownBuild { uint16_t revision; const char* tu; bool verified; };
        constexpr KnownBuild kKnown[] = {
            { 3549, "2.04.07",          true  },  // minor patch (2026-11-05), different exe (SHA256
                                                  // 25f24e06a1f886e9310fd290f9462f66b480a5e7763d1fe156bfddaa8d1992b9,
                                                  // audit.py). Full audit.py sweep against the live exe: DamageApply,
                                                  // MoveUpdate, LocStringGet, TodEngineGlobal and EquipEffectRefresh
                                                  // all still resolve unique - unchanged from 3527. Of the live-wired
                                                  // TU-candidate fallbacks, kSig_TravelToNode_TU3460_Candidate,
                                                  // kSig_InvGetHolder_TU3460_Candidate, kSig_InvCommit_TU20402_
                                                  // Candidate, kSig_MasterFrameUpdate_TU20402_Candidate,
                                                  // kSig_FieldTimeRealm_TU3460_Candidate, kSig_FieldTimeTick_
                                                  // TU3479_Candidate, kSig_StatCommit_TU3527_Candidate,
                                                  // kSig_TrItemValueCtor_TU3527_Candidate and kSig_InvCommitPlacement_
                                                  // TU3527_Candidate all still resolve unique - Fast Travel trigger,
                                                  // Inventory holder/commit/core-global, Game Speed, Advance Time/
                                                  // Time-of-Day freeze and God Mode/Infinite Stamina/Infinite Spirit
                                                  // should all keep working. kSig_StatCommit_TU3460_Candidate and
                                                  // kSig_StatCommit_TU3479_Candidate both stay at 0 matches (same
                                                  // regression already flagged on 3462, not yet re-derived).
                                                  // kSig_ParryVerdict stays at 0 (same regression already flagged
                                                  // on 3527) - Easy Parry stays dead. NEW regression this build:
                                                  // kSig_InvHolderInsert_TU3527_Candidate went from its documented
                                                  // unique match on 3527 to 26 matches here (no longer unique) -
                                                  // the per-feature "unique" safety gate correctly refuses it, so
                                                  // this changes nothing observable (Add Item was already blocked
                                                  // on missing kSig_InvFreePlacements regardless). Everything else
                                                  // already dead on 3527 stays dead. Same risk-accepted precedent
                                                  // as 3527/3479/3462/3460/3446 below - flipping this only lets
                                                  // Install() attempt each hook instead of blocking all of them
                                                  // wholesale. Our own live pass is pending.
            { 3527, "unconfirmed",      true  },  // follow-up patch (2026-11-04), different exe (SHA256
                                                  // dc4237f21d6ce1e49f8e5d6fa89bf803fbbe6549123078efdc393265c7bf6841,
                                                  // audit.py). Full audit.py sweep against the live exe: DamageApply,
                                                  // MoveUpdate, LocStringGet, TodEngineGlobal and EquipEffectRefresh
                                                  // all still resolve unique - unchanged from 3479. Of the live-wired
                                                  // TU-candidate fallbacks, kSig_TravelToNode_TU3460_Candidate,
                                                  // kSig_InvGetHolder_TU3460_Candidate, kSig_InvCommit_TU20402_
                                                  // Candidate, kSig_MasterFrameUpdate_TU20402_Candidate,
                                                  // kSig_FieldTimeRealm_TU3460_Candidate and kSig_FieldTimeTick_
                                                  // TU3479_Candidate all still resolve unique - Fast Travel trigger,
                                                  // Inventory holder/commit, Game Speed and Advance Time/Time-of-Day
                                                  // freeze should all keep working. kSig_StatCommit_TU3460_Candidate
                                                  // and kSig_StatCommit_TU3479_Candidate both stay at 0 matches (same
                                                  // regression already flagged on 3462, not yet re-derived) - God
                                                  // Mode/Infinite Stamina/Infinite Spirit stay dead. NEW regression
                                                  // this build: kSig_ParryEvaluator dropped to 0 matches (was unique
                                                  // through 3479) - Easy Parry is dead until re-derived, not yet
                                                  // done. Everything else already dead on 3479 stays dead. Same
                                                  // risk-accepted precedent as 3479/3462/3460/3446 below - flipping
                                                  // this only lets Install() attempt each hook instead of blocking
                                                  // all of them wholesale. Our own live pass is pending.
            { 3479, "unconfirmed",      true  },  // follow-up patch (2026-11-01, game 2.04.05), different exe
                                                  // (SHA256 changed, audit.py). Full audit.py sweep against the
                                                  // live exe: DamageApply, MoveUpdate, LocStringGet,
                                                  // TodEngineGlobal and EquipEffectRefresh all still resolve
                                                  // unique at drifted RVAs - same function, address moved, no
                                                  // new signature needed (audit.py's primary-only sweep flags a
                                                  // long list of MISSING entries, but every one of those was
                                                  // already dead since 3460/3446 via per-feature gating; not a
                                                  // new regression). Directly verified against the live exe, one
                                                  // of our own TU3460/TU20100/TU20402 fallback candidates each:
                                                  // kSig_InvGetHolder_TU3460_Candidate, kSig_FieldTimeRealm_
                                                  // TU3460_Candidate, kSig_TravelToNode_TU3460_Candidate,
                                                  // kSig_LocoStepper_TU20100_Candidate, kSig_PathingHelper_
                                                  // TU20100_Candidate, kSig_InvGetItemQty_TU20100_Candidate,
                                                  // kSig_InvCommit_TU20402_Candidate, kSig_InvCoreGlobal_
                                                  // TU20100_Candidate and kSig_MasterFrameUpdate_TU20402_
                                                  // Candidate all still resolve unique - Inventory holder/
                                                  // commit/core-global, Advance Time, Fast Travel trigger,
                                                  // locomotion/pathing hooks and Game Speed's frame-update hook
                                                  // should all keep working. kSig_StatCommit_TU3460_Candidate
                                                  // and kSig_StatCommit_TU20100_Candidate both stay at 0 matches
                                                  // (same regression already flagged on 3462) - but a new
                                                  // kSig_StatCommit_TU3479_Candidate, re-derived this session via
                                                  // helper-callee relocation diffing (see offsets.h), resolves
                                                  // unique at RVA 0xB6C4340 - God Mode/Infinite Stamina/Infinite
                                                  // Spirit should work again on this build specifically. Everything
                                                  // else already dead on 3460/3462 stays dead. Same risk-accepted
                                                  // precedent as 3462/3460/3446 below. Our own live pass is pending.
            { 3462, "unconfirmed",      true  },  // follow-up patch (2026-11-01), different exe (SHA256 changed,
                                                  // audit.py), not just a relabel. Full audit.py sweep against the
                                                  // live exe: DamageApply, MoveUpdate, LocStringGet, TodEngineGlobal,
                                                  // EquipEffectRefresh and the crime sigs all still resolve unique -
                                                  // unchanged from 3460/3446. Of the three TU3460 candidates added
                                                  // this session, kSig_InvGetHolder_TU3460_Candidate,
                                                  // kSig_FieldTimeRealm_TU3460_Candidate and the new
                                                  // kSig_TravelToNode_TU3460_Candidate still resolve unique
                                                  // (byte-identical RVAs to 3460) - Inventory/Advance Time/Fast
                                                  // Travel trigger should keep working. kSig_StatCommit_TU3460_
                                                  // Candidate regressed to 0 matches on THIS build specifically -
                                                  // God Mode/Infinite Stamina/Infinite Spirit are dead again until
                                                  // re-derived; not yet done. Everything else already dead on 3460
                                                  // stays dead. Same risk-accepted precedent as 3460/3446/2976
                                                  // below. Our own live pass is pending.
            { 3460, "unconfirmed",      true  },  // follow-up patch (2026-10-31), file version only one bump past
                                                  // 3446; TU label not yet confirmed by any third-party source. Full
                                                  // audit.py sweep against the live exe: DamageApply, MoveUpdate,
                                                  // InvCoreGlobal/InvGetItemQty (TU20100 candidate), PathingHelper,
                                                  // LocoStepper, AirMoverStep, TodEngineGlobal, EquipEffectRefresh
                                                  // and CharMgrSlot A/B all still resolve unique - unchanged from
                                                  // 3446. kSig_StatCommit (primary) and the TU20100 candidate both
                                                  // regressed to zero matches - a targeted recompile of just this
                                                  // one function - but a full Ghidra re-import of this build let us
                                                  // relocate it via its shared helper callee and re-derive a new
                                                  // kSig_StatCommit_TU3460_Candidate (offsets.h), one match, byte-
                                                  // identical field layout to the TU20100 candidate it replaces.
                                                  // God Mode/Infinite Stamina/Infinite Spirit (player.cpp's shared
                                                  // ResolvePlayerHooks hook) should work again on this build; not
                                                  // yet live-tested. Everything else from the 3446 sweep that was
                                                  // already dead (teleport, inventory commit path, Game Speed
                                                  // legacy, ToD, dye, Friendly trust) stays dead, same per-feature
                                                  // gating. Same risk-accepted precedent as 3446/2976 below -
                                                  // flipping this only lets Install() actually attempt each hook
                                                  // instead of blocking all of them wholesale (COMPATIBILITY.md
                                                  // "Build 3460"). Our own live pass is pending.
            { 3446, "2.04.02",          true  },  // the ocean DLC build (2026-10-30). Full audit.py sweep of every
                                                  // kSig_* (primary + TU candidates, 57+ patterns): core player/
                                                  // damage/char-manager pipeline (StatCommit, DamageApply, MoveUpdate,
                                                  // CharMgrSlot A/B, InvCoreGlobal) resolves unique; ~20 others
                                                  // (teleport, most of inventory holder/commit, Game Speed legacy
                                                  // path, time-of-day, dye, Friendly trust, crime) are confirmed
                                                  // dead - each gated independently per-feature, so flipping this
                                                  // true just lets the live log show which (COMPATIBILITY.md
                                                  // "Build 3446"), same risk-accepted precedent as 2976 below. Our
                                                  // own live pass is pending.
            { 2976, "2.03.02",          true  },  // gugi97 verified all 42 signatures resolve on 2976 and ran the
                                                  // damage/parry/flight features in-game (upstream d64e72c). Our
                                                  // build was stuck in diagnostics-only mode here (no damage
                                                  // multiplier / One-Hit Kill, live log 2026-09-29), so this
                                                  // enables our own gameplay hooks; our own live pass is pending.
            { 2949, "2.03.01",          false },  // fingerprinted; signature set re-audited offline (audit.py), not live-tested
            { 2944, "2.03.00",          false },  // fingerprinted; gameplay hooks not yet verified
            { 2760, "2.01.00",          false },  // fingerprinted; gameplay hooks not yet verified
            { 2625, "2.00.00",          true  },  // verified: every signature re-checked
            { 2474, "1.18.02",          true  },
            { 2435, "1.18.0",           true  },
            { 2330, "1.17.00",          true  },
        };

        GameVersion g_version;
        bool        g_resolved = false;
        char        g_text[32] = "unknown";

        void Resolve()
        {
            if (g_resolved) return;
            g_resolved = true;

            wchar_t path[MAX_PATH] = {};
            if (!GetModuleFileNameW(nullptr, path, MAX_PATH))
                return;

            DWORD ignored = 0;
            const DWORD size = GetFileVersionInfoSizeW(path, &ignored);
            if (!size) return;

            // The game exe is large but its version block is small; a heap
            // buffer keeps this off the loader thread's stack.
            void* data = ::operator new(size, std::nothrow);
            if (!data) return;

            if (GetFileVersionInfoW(path, 0, size, data))
            {
                VS_FIXEDFILEINFO* ffi = nullptr;
                UINT len = 0;
                if (VerQueryValueW(data, L"\\", reinterpret_cast<LPVOID*>(&ffi), &len) &&
                    ffi && len >= sizeof(VS_FIXEDFILEINFO) && ffi->dwSignature == 0xFEEF04BD)
                {
                    g_version.major    = HIWORD(ffi->dwFileVersionMS);
                    g_version.minor    = LOWORD(ffi->dwFileVersionMS);
                    g_version.build    = HIWORD(ffi->dwFileVersionLS);
                    g_version.revision = LOWORD(ffi->dwFileVersionLS);
                    g_version.known    = true;
                    snprintf(g_text, sizeof(g_text), "%u.%u.%u.%u",
                             g_version.major, g_version.minor,
                             g_version.build, g_version.revision);
                }
            }
            ::operator delete(data);
        }
    }

    const char* GameVersion::text() const { return g_text; }

    const char* GameVersion::titleUpdate() const
    {
        if (!known) return "unknown";
        for (const KnownBuild& k : kKnown)
            if (k.revision == revision) return k.tu;
        return "unrecognised";
    }

    bool GameVersion::isVerified() const
    {
        // A revision alone is not an executable identity: never grant gameplay
        // hooks to a different major/minor/build tuple reusing that revision.
        if (!known || major != 1 || minor != 0 || build != 0) return false;
        for (const KnownBuild& k : kKnown)
            if (k.revision == revision) return k.verified;
        return false;
    }

    const GameVersion& CurrentGameVersion()
    {
        Resolve();
        return g_version;
    }

    void LogGameVersion()
    {
        const GameVersion& v = CurrentGameVersion();
        if (!v.known)
        {
            LOG_WARN("version: could not read the game's version - if something "
                     "misbehaves, mention that in any report.");
            return;
        }
        if (v.isVerified())
            LOG("version: Crimson Desert %s (TU %s) - the build this release was checked against.",
                v.text(), v.titleUpdate());
        else
            LOG_WARN("version: Crimson Desert %s (TU %s) - Trinity has not been checked against "
                     "this build. Gameplay hooks will remain disabled; quote this line in a "
                     "bug report.", v.text(), v.titleUpdate());
    }
}
