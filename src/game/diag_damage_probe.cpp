#include "diag_damage_probe.h"

#include <cstdint>

#include "offsets.h"
#include "../mem/scanner.h"
#include "../mem/safe_memory.h"
#include "../mem/hooks.h"
#include "../core/logger.h"

namespace trinity::game
{
    namespace
    {
        // Same 11-argument shape player.cpp's hkDamageApply already uses
        // (verified against the TU 2.00.00 source and cross-checked by
        // Ghidra's TU 2.01.00 decompile - see COMPATIBILITY.md's "Damage
        // application static analysis").
        using DamageApply_t = int64_t(__fastcall*)(void* targetOwner, uint16_t statusId,
                                                    int64_t time, int64_t delta, uintptr_t sourceCtx,
                                                    char a6, char a7, char a8, char a9, char a10,
                                                    void* out);
        DamageApply_t oDamageApply = nullptr;
        void* g_damageProbeTarget = nullptr;

        // Pass-through: forwards every argument and the original result
        // untouched, then logs what it saw. Never alters delta, never
        // writes to targetOwner or anywhere else.
        int64_t __fastcall hkDamageApplyProbe(void* targetOwner, uint16_t statusId,
                                              int64_t time, int64_t delta, uintptr_t sourceCtx,
                                              char a6, char a7, char a8, char a9, char a10,
                                              void* out)
        {
            const int64_t result = oDamageApply(targetOwner, statusId, time, delta, sourceCtx,
                                                a6, a7, a8, a9, a10, out);

            const uintptr_t owner = reinterpret_cast<uintptr_t>(targetOwner);

            // Same identity proof used in diag_player_accessor.cpp: tag
            // byte at owner+0x88 -> +1, and the possessor/pawn round trip
            // at +0xA0 -> +0xD0. Purely a read-only cross-check against
            // findings already logged elsewhere - not a re-implementation
            // of anything.
            uint64_t td = 0, possessor = 0, pawn = 0;
            uint8_t tag = 0;
            const bool haveTag = owner >= kMinPointer &&
                mem::Read64(owner + kOff_Owner_TypeDesc, &td) && td >= kMinPointer &&
                mem::Read8(static_cast<uintptr_t>(td) + 1, &tag);
            const bool roundTripOk = owner >= kMinPointer &&
                mem::Read64(owner + kOff_Owner_Possessor, &possessor) && possessor >= kMinPointer &&
                mem::Read64(static_cast<uintptr_t>(possessor) + kOff_Possessor_Pawn, &pawn) &&
                pawn == owner;

            LOG("diag/damage: targetOwner=0x%llX statusId=%u delta=%lld tag=%u(%s) round-trip=%s",
                static_cast<unsigned long long>(owner), statusId, static_cast<long long>(delta),
                tag, haveTag ? "ok" : "unreadable", roundTripOk ? "OK" : "no");

            return result;
        }
    }

    bool DamageProbe::Install()
    {
        // ponytail: no throttle - DamageApply only fires on an actual
        // battle hit (not per-frame like MoveUpdate), so a short deliberate
        // test session won't flood the log. Add one if a busy combat area
        // proves otherwise.
        if (!mem::InstallHook("diag/damage: damage-apply (read-only, pass-through)",
                              kSig_DamageApply, "damage probe disabled",
                              &hkDamageApplyProbe, &oDamageApply, &g_damageProbeTarget))
            return false;

        LOG_OK("diag/damage: probe armed - logs targetOwner/statusId/delta on every battle-hit "
               "call. Purely read-only: the original is always called unmodified, nothing is "
               "written anywhere.");
        return true;
    }

    void DamageProbe::Remove()
    {
        mem::RemoveHook(&g_damageProbeTarget);
    }
}
