#include "friendly.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <MinHook.h>

#include "offsets.h"
#include "inventory.h"
#include "../mem/safe_memory.h"
#include "../mem/hooks.h"
#include "../core/logger.h"
#include "../core/state.h"

namespace trinity::game
{
    using mem::Read16;
    using mem::Read32;
    using mem::Read64;
    using mem::ReadPtr;
    using mem::Write64;

    namespace
    {
        // The two leaf trust-record setters. Both take the destination map owner
        // in rcx and the SOURCE 0x58-byte record in rdx, and copy the record into
        // the matching slot. Every trust write - gift, feed, AI, save-load, sync
        // - funnels through one of these (live-confirmed: an NPC trust write
        // broke at 0xDBE114F inside setNpc). See offsets.h (kSig_FriendlySet*).
        using FriendlySet_t = void*(__fastcall*)(void* mapOwner, void* record);
        FriendlySet_t oSetNpc = nullptr;
        FriendlySet_t oSetPet = nullptr;
        void* g_npcTarget = nullptr;
        void* g_petTarget = nullptr;
        size_t g_npcWhich = 0; // matched pattern index for each - see Friendly::Status()
        size_t g_petWhich = 0;

        // Last trust value we let through, per relationship. The setter writes an
        // absolute value, so we scale the increase over what we last allowed for
        // that (map, group, key) - a stable per-gift/feed multiplier. Keyed so an
        // NPC and a pet, or two different targets, never collide. First sight of
        // a key SEEDS unscaled; because the save-loader drives these same setters
        // at login, every relationship is pre-seeded there, so the first in-game
        // gift is already scaled and a loaded save is never re-scaled. Losses and
        // already-max targets pass through but keep the cache tracking the true
        // value. Self-healing: after a reload the stored value drops below our
        // cache, reads as a "loss", and reseeds.
        std::mutex g_cacheMx;
        std::unordered_map<uint64_t, int64_t> g_lastVal;

        int64_t ScaleGain(int64_t oldVal, int64_t newVal, float mult)
        {
            const double scaled = static_cast<double>(oldVal) +
                                  static_cast<double>(newVal - oldVal) * static_cast<double>(mult);
            if (scaled >= static_cast<double>(kFriendly_Max)) return kFriendly_Max;
            if (scaled <= 0.0) return 0;
            return static_cast<int64_t>(scaled + 0.5);
        }

        // mapId keeps the NPC (0) and pet (1) key spaces apart in the cache.
        void ScaleRecord(void* record, uint32_t mapId)
        {
            const uintptr_t r = reinterpret_cast<uintptr_t>(record);
            if (r < kMinPointer) return;

            uint32_t key = 0;
            uint16_t group = 0;
            int64_t  newVal = 0;
            if (!Read32(r + kOff_FriendlyRec_Key, &key)) return;
            if (!Read16(r + kOff_FriendlyRec_Group, &group)) return;
            if (!Read64(r + kOff_FriendlyRec_Value, &newVal)) return;

            // Ignore obviously-out-of-range values (a wrong offset or an
            // uninitialised insert) so we never write garbage into the record.
            if (newVal < 0 || newVal > kFriendly_Max) return;

            const uint64_t base   = (static_cast<uint64_t>(mapId) << 48) |
                                    (static_cast<uint64_t>(group) << 32);
            const uint64_t ckBase = base;        // key == 0: the persisted baseline
            const uint64_t ckLive = base | key;  // this live relationship

            std::lock_guard<std::mutex> lk(g_cacheMx);

            // key == 0 is the save-loader's persistent write - every NPC's stored
            // trust arrives this way at login, one record per group, before any
            // gameplay write. Record it as the group's baseline and NEVER scale
            // it, so a loaded save is never inflated.
            if (key == 0)
            {
                g_lastVal[ckBase] = newVal;
                return;
            }

            // Gameplay write. Old value = this relationship's last value if we've
            // seen it; else the group's persisted baseline (so the FIRST
            // interaction with an NPC - e.g. a greet, which may be the only one
            // it ever gets - still scales instead of being swallowed as a seed);
            // else 0 for a wholly new relationship.
            int64_t oldVal;
            auto itLive = g_lastVal.find(ckLive);
            if (itLive != g_lastVal.end())
            {
                oldVal = itLive->second;
            }
            else
            {
                auto itBase = g_lastVal.find(ckBase);
                oldVal = (itBase != g_lastVal.end()) ? itBase->second : 0;
            }

            const State& st = State::Get();
            const float mult = st.trustMultVal;
            const bool  on   = st.trustMult && mult > 1.0f;

            if (on && newVal > oldVal && oldVal < kFriendly_Max)
            {
                const int64_t s = ScaleGain(oldVal, newVal, mult);
                if (s != newVal && Write64(r + kOff_FriendlyRec_Value, s))
                {
                    LOG("friendly: trust gain key=%u group=%u: %lld -> %lld (mult=%.1fx)",
                        key, static_cast<unsigned>(group), oldVal, s, mult);
                    g_lastVal[ckLive] = s;
                    return;
                }
            }
            g_lastVal[ckLive] = newVal;
        }

        // --- Fallback: signature-free relationship-array scan ------------------
        // Used only when both setter signatures above are dead. Same technique
        // the "Crimson Desert Enhanced Ultimate ASI" trainer (build 3209/10.8)
        // uses on 1.0.0.3479, where its own log calls the base pointer "trust
        // owner (yours)": starting from the player's own character pointer,
        // every 8-byte slot in a generous window is tried as an array pointer;
        // a candidate is accepted once several consecutive kFriendlyRecStride
        // records in a row pass the SAME field-range checks ScaleRecord already
        // applies (key/group fit their width, value in 0..kFriendly_Max). No
        // function byte pattern involved, so a game recompile cannot kill it.
        constexpr uintptr_t kFriendlyScanWindow     = 0x200; // bytes probed off the owner pointer
        constexpr uint32_t  kFriendlyScanMaxEntries = 32;    // observed cap on the real array
        constexpr uint32_t  kFriendlyScanMinValid   = 3;     // consecutive good records required to accept

        bool RecordLooksValid(uintptr_t rec)
        {
            uint32_t key = 0;
            uint16_t group = 0;
            int64_t  val = 0;
            if (!Read32(rec + kOff_FriendlyRec_Key, &key)) return false;
            if (!Read16(rec + kOff_FriendlyRec_Group, &group)) return false;
            if (!Read64(rec + kOff_FriendlyRec_Value, &val)) return false;
            (void)group; // any u16 is structurally valid; only key/val bound the record
            return val >= 0 && val <= kFriendly_Max;
        }

        bool ArrayLooksValid(uintptr_t array)
        {
            uint32_t streak = 0;
            for (uint32_t i = 0; i < kFriendlyScanMaxEntries; ++i)
            {
                if (!RecordLooksValid(array + i * kOff_FriendlyRec_Stride)) return false;
                if (++streak >= kFriendlyScanMinValid) return true;
            }
            return false;
        }

        uintptr_t FindRelationshipArray(uintptr_t owner)
        {
            if (owner < kMinPointer) return 0;
            for (uintptr_t off = 0; off <= kFriendlyScanWindow; off += 8)
            {
                uintptr_t candidate = 0;
                if (!ReadPtr(owner + off, &candidate) || candidate < kMinPointer) continue;
                if (ArrayLooksValid(candidate)) return candidate;
            }
            return 0;
        }

        std::atomic<uintptr_t> g_scanArray{ 0 };
        std::atomic<bool>      g_scanArmed{ false };

        void* __fastcall hkSetNpc(void* mapOwner, void* record)
        {
            ScaleRecord(record, 0);
            return oSetNpc(mapOwner, record);
        }

        void* __fastcall hkSetPet(void* mapOwner, void* record)
        {
            ScaleRecord(record, 1);
            return oSetPet(mapOwner, record);
        }
    }

    bool Friendly::Install()
    {
        const bool npc = mem::InstallHookAny("friendly: NPC trust setter",
                                             { kSig_FriendlySetNpc, kSig_FriendlySetNpc_1180, kSig_FriendlySetNpc_TU2030 },
                                             "NPC gift Trust Multiplier disabled",
                                             &hkSetNpc, &oSetNpc, &g_npcTarget, &g_npcWhich);
        const bool pet = mem::InstallHookAny("friendly: pet trust setter",
                                             { kSig_FriendlySetPet, kSig_FriendlySetPet_TU2030 },
                                             "pet Trust Multiplier disabled",
                                             &hkSetPet, &oSetPet, &g_petTarget, &g_petWhich);
        // Both setter signatures dead (e.g. 1.0.0.3479) - arm the scan fallback.
        // Tick() does the actual work once a game-thread frame has run.
        if (!npc && !pet)
        {
            g_scanArmed.store(true, std::memory_order_relaxed);
            LOG("friendly: both trust setters dead - falling back to a relationship-array scan");
        }
        return npc || pet || g_scanArmed.load(std::memory_order_relaxed);
    }

    void Friendly::Remove()
    {
        mem::RemoveHook(&g_npcTarget);
        mem::RemoveHook(&g_petTarget);
        g_scanArmed.store(false, std::memory_order_relaxed);
        g_scanArray.store(0, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(g_cacheMx);
        g_lastVal.clear();
    }

    void Friendly::Tick()
    {
        if (!g_scanArmed.load(std::memory_order_relaxed)) return;

        uintptr_t array = g_scanArray.load(std::memory_order_relaxed);
        if (array < kMinPointer || !ArrayLooksValid(array))
        {
            array = FindRelationshipArray(Inventory::ClientCharacterAddr());
            g_scanArray.store(array, std::memory_order_relaxed);
            if (array < kMinPointer) return;
            LOG("friendly: relationship array found @ %llX via scan fallback", (unsigned long long)array);
        }

        // mapId 2 keeps the scan path's cache keys apart from the hook path's
        // 0 (NPC) / 1 (pet); the two never run at the same time (Install()
        // only arms the scan when both hooks failed), but keeping them
        // separate costs nothing and avoids any ambiguity if that changes.
        for (uint32_t i = 0; i < kFriendlyScanMaxEntries; ++i)
            ScaleRecord(reinterpret_cast<void*>(array + i * kOff_FriendlyRec_Stride), 2);
    }

    bool Friendly::Ready()
    {
        return g_npcTarget != nullptr || g_petTarget != nullptr ||
               g_scanArray.load(std::memory_order_relaxed) >= kMinPointer;
    }

    mem::SigStatus Friendly::Status()
    {
        // Ready() is an OR (either half gifting still raises trust), so Dead
        // here means both failed; Fallback means at least one works but not
        // both on their current-build pattern - Ok needs both exact.
        if (g_scanArmed.load(std::memory_order_relaxed))
            return g_scanArray.load(std::memory_order_relaxed) >= kMinPointer ? mem::SigStatus::Fallback
                                                                               : mem::SigStatus::Dead;
        if (!Ready()) return mem::SigStatus::Dead;
        const mem::SigStatus npc = g_npcTarget ? mem::StatusOf(true, g_npcWhich) : mem::SigStatus::Dead;
        const mem::SigStatus pet = g_petTarget ? mem::StatusOf(true, g_petWhich) : mem::SigStatus::Dead;
        return (npc == mem::SigStatus::Ok && pet == mem::SigStatus::Ok) ? mem::SigStatus::Ok
                                                                         : mem::SigStatus::Fallback;
    }
}
