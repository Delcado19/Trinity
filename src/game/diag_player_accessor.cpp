#include "diag_player_accessor.h"

#include <Windows.h>
#include <cstdint>

#include "offsets.h"
#include "player.h"
#include "../mem/scanner.h"
#include "../mem/safe_memory.h"
#include "../mem/hooks.h"
#include "../core/logger.h"

namespace trinity::game
{
    namespace
    {
        // Resolved once at Install(); zeroed if the candidate ever proves
        // unsafe to call (see CallCandidateAccessor's SEH catch below), which
        // permanently stops further probing for the rest of the session.
        uintptr_t g_accessorAddr = 0;

        // The RIP-resolved manager-global slot the candidate accessor itself
        // reads (RVA 0x6C29C88, see Install()). Structurally this is exactly
        // what kCharMgrAnchors resolves on TU 2.00.00 - reading it once gives
        // P, reading P once more gives the manager - so it is a candidate
        // replacement for g_charMgrGlobal (player.cpp), independent of the
        // accessor-call probe above and NOT affected by that probe's tag==1
        // problem (see ManagerWalkTick).
        uintptr_t g_mgrSlot = 0;

        // The SECOND, independently-verified manager slot (RVA 0x6C29C68,
        // see offsets.h kSig_CharMgrSlot_TU20100_A/_B). Confirmed by direct
        // live memory read against a working third-party build to resolve
        // the SAME manager object as g_mgrSlot above - kept separate rather
        // than replacing g_mgrSlot so ManagerWalkTick's existing, already-
        // logged history stays exactly as it was. This slot's only
        // advantage is not depending on ever calling the accessor.
        uintptr_t g_selfChainMgrSlot = 0;

        using MoveUpdateProbe_t = uint64_t(__fastcall*)(uint64_t, uint64_t, uint64_t, uint64_t,
                                                         uint64_t, uint64_t, uint64_t);
        MoveUpdateProbe_t oMoveUpdateProbe = nullptr;
        void* g_moveUpdateProbeTarget = nullptr;

        // Calls the candidate accessor under SEH: an unverified calling
        // convention (assumed here: 0-arg __fastcall returning a pointer) can
        // fault instead of just returning garbage, and that must be caught
        // rather than crashing the game. Locals stay POD so __try/__except is
        // legal in this function.
        bool CallCandidateAccessor(uintptr_t fn, uintptr_t* outOwner)
        {
            using Accessor_t = void* (__fastcall*)();
            __try
            {
                *outOwner = reinterpret_cast<uintptr_t>(reinterpret_cast<Accessor_t>(fn)());
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        // Throttled so a 60-200Hz game-thread tick doesn't flood the console;
        // one line every couple of seconds is plenty to watch the result
        // stay stable (or not) across combat, mounting, and body transitions.
        void ProbeTick()
        {
            if (!g_accessorAddr) return;

            static unsigned long long s_nextAt = 0;
            const unsigned long long now = GetTickCount64();
            if (now < s_nextAt) return;
            s_nextAt = now + 2000;

            uintptr_t owner = 0;
            if (!CallCandidateAccessor(g_accessorAddr, &owner))
            {
                LOG_ERR("diag/accessor: SEH caught calling the candidate accessor - the 0-arg "
                        "__fastcall assumption is wrong, or it is unsafe to call off its native "
                        "call site. Do NOT promote this candidate. Disabling further probes.");
                g_accessorAddr = 0;
                return;
            }

            if (owner < kMinPointer)
            {
                LOG("diag/accessor: accessor returned null (not in world, or no body currently "
                    "satisfies the possessor round-trip).");
                return;
            }

            // Same identity proof documented in offsets.h for kCharMgrAnchors,
            // re-applied here as an independent sanity check on what the
            // candidate handed back - NOT a re-implementation of its walk.
            // Tag must be exactly 1: COMPATIBILITY.md found that the old
            // tag-1-or-9 test does not survive on this build.
            uint64_t td = 0, possessor = 0, pawn = 0;
            uint8_t tag = 0;
            const bool tagOk = mem::Read64(owner + kOff_Owner_TypeDesc, &td) && td >= kMinPointer &&
                               mem::Read8(static_cast<uintptr_t>(td) + 1, &tag) && tag == 1;
            const bool roundTripOk =
                mem::Read64(owner + kOff_Owner_Possessor, &possessor) && possessor >= kMinPointer &&
                mem::Read64(static_cast<uintptr_t>(possessor) + kOff_Possessor_Pawn, &pawn) &&
                pawn == owner;

            LOG_OK("diag/accessor: owner=0x%llX tag=%u(want 1: %s) round-trip=%s",
                   static_cast<unsigned long long>(owner), tag, tagOk ? "OK" : "FAIL",
                   roundTripOk ? "OK" : "FAIL");
        }

        // Read-only re-check of the OTHER candidate: does the manager global
        // resolved from g_mgrSlot support the SAME vtable-shared-class walk
        // Player::TickResolveSelf (player.cpp) already uses on TU 2.00.00 to
        // track EVERY active protagonist (not just "the" locally-possessed
        // body)? That walk was built specifically because the engine's own
        // accessor (the OTHER candidate above) stays pinned to whichever body
        // carries the permanent SelfPlayer tag - confirmed live: switching to
        // a different playable protagonist did not change the accessor's
        // returned owner at all. This function never calls into game code
        // and never touches a stat/vital offset - only the manager
        // list/count/vtable reads already used (and verified) on TU 2.00.00.
        void ManagerWalkTick()
        {
            if (!g_mgrSlot) return;

            static unsigned long long s_nextAt = 0;
            const unsigned long long now = GetTickCount64();
            if (now < s_nextAt) return;
            s_nextAt = now + 2500; // offset from ProbeTick's cadence so the two don't interleave

            uint64_t p = 0, mgr = 0, data = 0;
            if (!mem::Read64(g_mgrSlot, &p) || p < kMinPointer)
            {
                LOG("diag/mgrwalk: slot unreadable or null - not in world?");
                return;
            }
            if (!mem::Read64(static_cast<uintptr_t>(p), &mgr) || mgr < kMinPointer)
            {
                LOG("diag/mgrwalk: manager pointer unreadable or null.");
                return;
            }
            if (!mem::Read64(static_cast<uintptr_t>(mgr) + kOff_CharMgr_ListData, &data) ||
                data < kMinPointer)
            {
                LOG_WARN("diag/mgrwalk: list-data pointer unreadable - kOff_CharMgr_ListData "
                         "(0x%llX) may have moved on this build.",
                         static_cast<unsigned long long>(kOff_CharMgr_ListData));
                return;
            }
            uint32_t count = 0;
            if (!mem::Read32(static_cast<uintptr_t>(mgr) + kOff_CharMgr_ListCount, &count) ||
                count == 0 || count > kCharList_MaxCount)
            {
                LOG_WARN("diag/mgrwalk: list-count implausible (%u) - kOff_CharMgr_ListCount "
                         "may have moved on this build.", count);
                return;
            }

            // (A) Seed the shared protagonist-class vtable from ANY tag==1
            // (SelfPlayer) body - same seed Player.cpp uses. Only needs to
            // find one; which one does not matter for this step. While
            // walking anyway, also count how many list entries pass the
            // possessor round-trip (+0xA0 -> +0xD0) regardless of tag/vtable:
            // the live probe on 2026-09-06 found tag==1 matches 1-99 entries
            // depending on world population, not a rare "local player"
            // marker, so this checks whether round-trip fares any better as
            // a population-wide signal before trusting it per-member below.
            uint64_t anchorVt = 0;
            int nTag1 = 0;
            int rtTotal = 0;

            // Distinct-vtable histogram across the WHOLE list, not just the
            // tag==1-seeded pool: the live session on 2026-09-06 found a
            // confirmed protagonist switch never changed rtTotal or which
            // roster slot passed round-trip, which only proves the six
            // roster-slot bodies (anchorVt's class) are not the live
            // resolver - it does NOT prove the live body is even reachable
            // from this seed. If the list holds far more distinct vtables
            // than the ~6-member class anchorVt finds, the currently-played
            // body may sit under a DIFFERENT, never-inspected vtable.
            constexpr int kMaxVtBuckets = 48;
            constexpr int kMaxSmallAddrs = 4;
            uint64_t vtBucket[kMaxVtBuckets] = {};
            int vtBucketCount[kMaxVtBuckets] = {};
            uint64_t vtSmallAddrs[kMaxVtBuckets][kMaxSmallAddrs] = {};
            int nVtBuckets = 0;
            int nVtOverflow = 0;

            for (uint32_t i = 0; i < count; ++i)
            {
                uint64_t ch = 0;
                if (!mem::Read64(static_cast<uintptr_t>(data) + 8ull * i, &ch) || ch < kMinPointer)
                    continue;

                uint64_t possessor = 0, pawn = 0;
                if (mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_Possessor, &possessor) &&
                    possessor >= kMinPointer &&
                    mem::Read64(static_cast<uintptr_t>(possessor) + kOff_Possessor_Pawn, &pawn) &&
                    pawn == ch)
                    ++rtTotal;

                uint64_t vt = 0;
                if (mem::Read64(static_cast<uintptr_t>(ch), &vt) && vt >= kMinPointer)
                {
                    int slot = -1;
                    for (int b = 0; b < nVtBuckets; ++b)
                        if (vtBucket[b] == vt) { slot = b; break; }
                    if (slot < 0 && nVtBuckets < kMaxVtBuckets)
                    {
                        slot = nVtBuckets++;
                        vtBucket[slot] = vt;
                    }
                    if (slot >= 0)
                    {
                        // Track the first few members of every class, not just
                        // anchorVt's: if a class turns out small (<=4) once
                        // the whole list is walked, this is what lets the
                        // report show WHICH objects they are, not just a
                        // count - e.g. the single-member class discovered
                        // 2026-09-06 that no probe so far has ever inspected.
                        if (vtBucketCount[slot] < kMaxSmallAddrs)
                            vtSmallAddrs[slot][vtBucketCount[slot]] = ch;
                        ++vtBucketCount[slot];
                    }
                    else ++nVtOverflow;
                }

                uint64_t td = 0;
                uint8_t tag = 0;
                if (!mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_TypeDesc, &td) ||
                    td < kMinPointer)
                    continue;
                if (!mem::Read8(static_cast<uintptr_t>(td) + 1, &tag) || tag != 1)
                    continue;
                ++nTag1;
                if (!anchorVt) mem::Read64(static_cast<uintptr_t>(ch), &anchorVt);
            }

            // (B) Every character sharing that vtable, tag-agnostic - this is
            // what should track a played-companion swap even though the tag
            // does not (the played companion keeps the protagonist vtable
            // even after the engine re-tags it away from SelfPlayer). Unlike
            // the previous build, log round-trip and tag PER roster slot
            // instead of only the aggregate: the live session found the
            // accessor's fixed owner is roster slot 0 and it round-trips on
            // every sample, including across a real protagonist switch - if
            // that turns out true for all six slots always, round-trip is a
            // per-object class invariant here (same failure shape as tag==1),
            // not a possession signal. If exactly one slot round-trips and it
            // changes with the switch, that slot IS the resolver.
            int nClassMatch = 0;
            char list[384] = {};
            size_t listLen = 0;
            if (anchorVt)
            {
                for (uint32_t i = 0; i < count && nClassMatch < 6; ++i)
                {
                    uint64_t ch = 0;
                    if (!mem::Read64(static_cast<uintptr_t>(data) + 8ull * i, &ch) ||
                        ch < kMinPointer)
                        continue;
                    uint64_t vt = 0;
                    if (!mem::Read64(static_cast<uintptr_t>(ch), &vt) || vt != anchorVt) continue;
                    ++nClassMatch;

                    uint64_t td = 0, possessor = 0, pawn = 0;
                    uint8_t tag = 0;
                    mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_TypeDesc, &td);
                    if (td >= kMinPointer) mem::Read8(static_cast<uintptr_t>(td) + 1, &tag);
                    const bool rtOk =
                        mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_Possessor, &possessor) &&
                        possessor >= kMinPointer &&
                        mem::Read64(static_cast<uintptr_t>(possessor) + kOff_Possessor_Pawn, &pawn) &&
                        pawn == ch;
                    const int n = snprintf(list + listLen, sizeof(list) - listLen,
                                           "%s0x%llX:tag=%u:rt=%s", listLen ? "," : "",
                                           static_cast<unsigned long long>(ch), tag, rtOk ? "OK" : "no");
                    if (n > 0) listLen += static_cast<size_t>(n);
                }
            }

            // Summarize the histogram: how many distinct vtables exist at
            // all (nVtBuckets, capped at kMaxVtBuckets - nVtOverflow says if
            // that cap was hit), and the top few by member count. If the
            // list is dominated by a handful of huge classes (NPC/monster
            // archetypes) plus many small ones, a small anchorVt-sized class
            // is unremarkable noise; if most classes are large and anchorVt
            // is one of the SMALLEST, that is itself informative.
            int order[kMaxVtBuckets];
            for (int i = 0; i < nVtBuckets; ++i) order[i] = i;
            for (int i = 0; i < nVtBuckets; ++i)
                for (int j = i + 1; j < nVtBuckets; ++j)
                    if (vtBucketCount[order[j]] > vtBucketCount[order[i]])
                    {
                        int t = order[i]; order[i] = order[j]; order[j] = t;
                    }
            char top[256] = {};
            size_t topLen = 0;
            for (int i = 0; i < nVtBuckets && i < 5; ++i)
            {
                const int n = snprintf(top + topLen, sizeof(top) - topLen, "%s0x%llX:n=%d",
                                       topLen ? "," : "",
                                       static_cast<unsigned long long>(vtBucket[order[i]]),
                                       vtBucketCount[order[i]]);
                if (n > 0) topLen += static_cast<size_t>(n);
            }

            // classMatches above stops counting at 6 (it only needed enough
            // to log); look up anchorVt's TRUE member count from the
            // histogram so a silently-truncated class size doesn't get
            // mistaken for "the class has exactly 6 members".
            int anchorTrueCount = -1;
            for (int i = 0; i < nVtBuckets; ++i)
                if (vtBucket[i] == anchorVt) { anchorTrueCount = vtBucketCount[i]; break; }

            LOG("diag/mgrwalk: manager=0x%llX count=%u tag1=%d rtTotal=%d classMatches=%d(true=%d) [%s]",
                static_cast<unsigned long long>(mgr), count, nTag1, rtTotal, nClassMatch,
                anchorTrueCount, list);
            LOG("diag/mgrwalk2: distinct vtables=%d%s top5(vt:memberCount)=[%s]",
                nVtBuckets, nVtOverflow ? "+ (capped)" : "", top);

            // Every class with <=4 members, in full: which object(s), tag,
            // round-trip. A class this small that anchorVt's seed never
            // reaches (it only ever seeds from a tag==1 hit, and this build
            // found anchorVt's own class has 100 members, all tag==1 - not a
            // small "player" class at all) is the concrete candidate for
            // "the live body sits under a vtable this walk never inspected".
            char small[512] = {};
            size_t smallLen = 0;
            for (int b = 0; b < nVtBuckets; ++b)
            {
                if (vtBucketCount[b] > kMaxSmallAddrs || vtBucket[b] == anchorVt) continue;
                for (int m = 0; m < vtBucketCount[b]; ++m)
                {
                    const uint64_t ch = vtSmallAddrs[b][m];
                    uint64_t td = 0, possessor = 0, pawn = 0;
                    uint8_t tag = 0;
                    mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_TypeDesc, &td);
                    if (td >= kMinPointer) mem::Read8(static_cast<uintptr_t>(td) + 1, &tag);
                    const bool rtOk =
                        mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_Possessor, &possessor) &&
                        possessor >= kMinPointer &&
                        mem::Read64(static_cast<uintptr_t>(possessor) + kOff_Possessor_Pawn, &pawn) &&
                        pawn == ch;
                    const int n = snprintf(small + smallLen, sizeof(small) - smallLen,
                                           "%svt=0x%llX:0x%llX:tag=%u:rt=%s", smallLen ? "," : "",
                                           static_cast<unsigned long long>(vtBucket[b]),
                                           static_cast<unsigned long long>(ch), tag,
                                           rtOk ? "OK" : "no");
                    if (n > 0) smallLen += static_cast<size_t>(n);
                }
            }
            if (smallLen)
                LOG("diag/mgrwalk3: small classes (<=%d members, excl. anchorVt's own) [%s]",
                    kMaxSmallAddrs, small);
        }

        // The TU 2.00.00 WalkSelfChain identity proof, unchanged: owner ->
        // actor(+0x68) -> marker(+0x20) -> root(+0x18) -> statArray(+0x58),
        // require the array's first entry to type-check as Health. Read-only;
        // mirrors player.cpp's WalkSelfChain exactly (see offsets.h comments
        // next to kOff_Owner_Actor etc.) so a future promotion to VERIFIED
        // can copy this logic instead of re-deriving it.
        bool WalkSelfChainRO(uintptr_t owner, uintptr_t* outRoot, uintptr_t* outArr)
        {
            uint64_t actor = 0, marker = 0, root = 0, arr = 0;
            if (!mem::Read64(owner + kOff_Owner_Actor, &actor) || actor < kMinPointer) return false;
            if (!mem::Read64(static_cast<uintptr_t>(actor) + kOff_Actor_StatusMarker, &marker) ||
                marker < kMinPointer)
                return false;
            if (!mem::Read64(static_cast<uintptr_t>(marker) + kOff_Marker_TargetOwner, &root) ||
                root < kMinPointer)
                return false;
            if (!mem::Read64(static_cast<uintptr_t>(root) + kOff_Root_StatArray, &arr) ||
                arr < kMinPointer)
                return false;
            uint32_t type = 0;
            if (!mem::Read32(static_cast<uintptr_t>(arr) + kOff_StatEntry_Type, &type) ||
                static_cast<int32_t>(type) != StatType_Health)
                return false;
            *outRoot = static_cast<uintptr_t>(root);
            *outArr = static_cast<uintptr_t>(arr);
            return true;
        }

        // Re-does ManagerWalkTick's manager resolve through the SECOND slot
        // (g_selfChainMgrSlot - confirmed the same underlying manager, see
        // diag_player_accessor.h), then runs the actual TU 2.00.00 selection
        // algorithm instead of tag/round-trip: every member of the
        // protagonist-class vtable whose WalkSelfChain resolves is an active
        // protagonist. Read-only throughout - no write, no call into game
        // code, same offsets already used (and confirmed externally) elsewhere
        // in this file.
        void SelfChainTick()
        {
            if (!g_selfChainMgrSlot) return;

            static unsigned long long s_nextAt = 0;
            const unsigned long long now = GetTickCount64();
            if (now < s_nextAt) return;
            s_nextAt = now + 3000; // offset from the other two ticks' cadence

            uint64_t p = 0, mgr = 0, data = 0;
            if (!mem::Read64(g_selfChainMgrSlot, &p) || p < kMinPointer) return;
            if (!mem::Read64(static_cast<uintptr_t>(p), &mgr) || mgr < kMinPointer) return;
            if (!mem::Read64(static_cast<uintptr_t>(mgr) + kOff_CharMgr_ListData, &data) ||
                data < kMinPointer)
                return;
            uint32_t count = 0;
            if (!mem::Read32(static_cast<uintptr_t>(mgr) + kOff_CharMgr_ListCount, &count) ||
                count == 0 || count > kCharList_MaxCount)
                return;

            uint64_t anchorVt = 0;
            for (uint32_t i = 0; i < count && !anchorVt; ++i)
            {
                uint64_t ch = 0;
                if (!mem::Read64(static_cast<uintptr_t>(data) + 8ull * i, &ch) || ch < kMinPointer)
                    continue;
                uint64_t td = 0;
                uint8_t tag = 0;
                if (!mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_TypeDesc, &td) ||
                    td < kMinPointer)
                    continue;
                if (!mem::Read8(static_cast<uintptr_t>(td) + 1, &tag) || tag != 1) continue;
                mem::Read64(static_cast<uintptr_t>(ch), &anchorVt);
            }
            if (!anchorVt)
            {
                LOG("diag/selfchain: no tag==1 seed found this tick - not in world?");
                return;
            }

            char list[512] = {};
            size_t listLen = 0;
            int nResolved = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                uint64_t ch = 0;
                if (!mem::Read64(static_cast<uintptr_t>(data) + 8ull * i, &ch) || ch < kMinPointer)
                    continue;
                uint64_t vt = 0;
                if (!mem::Read64(static_cast<uintptr_t>(ch), &vt) || vt != anchorVt) continue;

                uintptr_t root = 0, arr = 0;
                if (!WalkSelfChainRO(static_cast<uintptr_t>(ch), &root, &arr)) continue;

                uint64_t td = 0;
                uint8_t tag = 0;
                mem::Read64(static_cast<uintptr_t>(ch) + kOff_Owner_TypeDesc, &td);
                if (td >= kMinPointer) mem::Read8(static_cast<uintptr_t>(td) + 1, &tag);

                ++nResolved;
                const int n = snprintf(list + listLen, sizeof(list) - listLen,
                                       "%s0x%llX:tag=%u", listLen ? "," : "",
                                       static_cast<unsigned long long>(ch), tag);
                if (n > 0) listLen += static_cast<size_t>(n);
            }

            LOG_OK("diag/selfchain: anchorVt=0x%llX resolved=%d active protagonist(s) [%s]",
                   static_cast<unsigned long long>(anchorVt), nResolved, list);
        }

        // Pass-through: every argument and the original result are forwarded
        // untouched. This hook's only job is a call on the game thread - it
        // deliberately never reads or writes the moved object's own fields
        // (contrast Teleport's hook, which does), so it cannot entangle this
        // probe's result with the ALSO-unverified movement candidate.
        uint64_t __fastcall hkMoveUpdateProbe(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
                                              uint64_t a5, uint64_t a6, uint64_t a7)
        {
            const uint64_t result = oMoveUpdateProbe(a1, a2, a3, a4, a5, a6, a7);
            ProbeTick();
            ManagerWalkTick();
            SelfChainTick();

            // Fallback driver: as of 2026-09-09, mod.cpp installs Teleport
            // BEFORE this probe specifically so Teleport's own hkMoveUpdate
            // (a strict superset - it also drives g_playerMoveOwner, Super
            // Jump, Game Speed) claims kSig_MoveUpdate first; this hook then
            // fails to install (MinHook refuses a second hook on an
            // already-hooked target) and this line never runs. This only
            // takes over for a TRANSIENT failure in Teleport's own
            // MH_CreateHook/MH_EnableHook call (kSig_MoveUpdate itself
            // still resolves, the hook API call fails) - Teleport::Install()
            // returns false whenever kSig_MoveUpdate fails to resolve at
            // all, which means this hook would fail identically (same dead
            // signature) and there is no driver either way in that case.
            // Player itself gates every write behind its own toggles (off
            // by default) and behind WalkSelfChain resolving successfully,
            // so this is safe to drive unconditionally when it does run.
            Player::RefreshSelf();
            return result;
        }
    }

    bool PlayerAccessorProbe::Install()
    {
        const uintptr_t addr = mem::FindPattern(kSig_CharMgrAccessor_TU20100_Candidate);
        if (!addr)
        {
            LOG_ERR("diag/accessor: candidate signature NOT FOUND - this build moved again; "
                    "nothing to probe.");
            return false;
        }
        const size_t matches = mem::CountMatches(kSig_CharMgrAccessor_TU20100_Candidate);
        if (matches != 1)
        {
            LOG_ERR("diag/accessor: candidate signature ambiguous (%zu matches) - refusing to "
                    "call it.", matches);
            return false;
        }

        const mem::ModuleRegion& mod = mem::GameModule();
        LOG("diag/accessor: candidate accessor resolved at RVA 0x%llX (COMPATIBILITY.md "
            "documents 0x2837940 for the 2026-09-03 build; a different RVA here just means a "
            "newer build - re-confirm the pattern still means the same thing before trusting it).",
            static_cast<unsigned long long>(addr - mod.base));

        const uintptr_t slot = mem::ResolveRipAt(addr + kRipOff_CharMgrAccessor_TU20100_Candidate, 7);
        if (slot)
            LOG("diag/accessor: manager-global slot at RVA 0x%llX (COMPATIBILITY.md documents "
                "0x6C29C88).", static_cast<unsigned long long>(slot - mod.base));

        g_accessorAddr = addr;
        g_mgrSlot = slot;

        // The second, independently-verified manager slot (see offsets.h
        // kSig_CharMgrSlot_TU20100_A/_B) - confirmed by direct live memory
        // read to resolve the SAME manager `slot` above already reaches, so
        // this does not need to succeed for the probe overall to work; it
        // only feeds SelfChainTick, which no-ops without it.
        {
            const uintptr_t a = mem::FindPattern(kSig_CharMgrSlot_TU20100_A);
            if (a && mem::CountMatches(kSig_CharMgrSlot_TU20100_A) == 1)
            {
                const uintptr_t s = mem::ResolveRipAt(a + kRipOff_CharMgrSlot_TU20100_A, 7);
                const uintptr_t b = mem::FindPattern(kSig_CharMgrSlot_TU20100_B);
                const uintptr_t sB = (b && mem::CountMatches(kSig_CharMgrSlot_TU20100_B) == 1)
                                         ? mem::ResolveRipAt(b + kRipOff_CharMgrSlot_TU20100_B, 7)
                                         : 0;
                if (s && sB && s == sB)
                {
                    g_selfChainMgrSlot = s;
                    LOG_OK("diag/selfchain: manager slot at RVA 0x%llX, confirmed by two "
                           "independent anchors (COMPATIBILITY.md documents 0x6C29C68).",
                           static_cast<unsigned long long>(s - mod.base));
                }
                else
                {
                    LOG_ERR("diag/selfchain: anchors A/B disagree or one failed (A=0x%llX "
                            "B=0x%llX) - refusing to trust either; SelfChainTick will no-op.",
                            static_cast<unsigned long long>(s), static_cast<unsigned long long>(sB));
                }
            }
            else
            {
                LOG_ERR("diag/selfchain: anchor A not found or ambiguous - this build moved "
                        "again; SelfChainTick will no-op.");
            }
        }

        if (!mem::InstallHook("diag/accessor: move-update tick (read-only, pass-through)",
                              kSig_MoveUpdate, "player-accessor probe disabled",
                              &hkMoveUpdateProbe, &oMoveUpdateProbe, &g_moveUpdateProbeTarget))
        {
            g_accessorAddr = 0;
            return false;
        }

        LOG_OK("diag/accessor: probe armed - logs the candidate accessor's result every ~2s "
               "in-game. Purely read-only: no hook on the accessor itself, no writes anywhere.");
        return true;
    }

    void PlayerAccessorProbe::Remove()
    {
        mem::RemoveHook(&g_moveUpdateProbeTarget);
        g_accessorAddr = 0;
        g_mgrSlot = 0;
        g_selfChainMgrSlot = 0;
    }
}
