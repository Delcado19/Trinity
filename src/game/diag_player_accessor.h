#pragma once

namespace trinity::game
{
    // Read-only runtime probes for TU 2.01.00 player resolution, built around
    // the candidate accessor found by static analysis (COMPATIBILITY.md:
    // "Character manager static analysis"; signature in offsets.h as
    // kSig_CharMgrAccessor_TU20100_Candidate). Two independent checks run off
    // the same read-only, pass-through hook on kSig_MoveUpdate (used solely
    // to get an execution point on the game thread; the original is always
    // called unmodified):
    //
    //  - ProbeTick (diag_player_accessor.cpp): calls the candidate accessor
    //    itself under SEH and logs what it returns. LIVE FINDING (2026-09-06):
    //    it returns the SAME owner across riding/running/flying AND across a
    //    genuine switch to a different playable protagonist - i.e. it is
    //    pinned to whichever body permanently carries the SelfPlayer tag
    //    (matches the exact limitation player.cpp already documents for the
    //    old build), not "the currently controlled body". Do not use it for
    //    that purpose.
    //  - ManagerWalkTick: re-resolves the character-manager global from the
    //    accessor's own RIP-relative slot (a candidate replacement for
    //    player.cpp's g_charMgrGlobal - this part is LIKELY, not broken: two
    //    live sessions totalling 5.5h found a stable manager pointer whose
    //    list count tracked world population) and runs the SAME
    //    vtable-shared-class walk Player::TickResolveSelf already uses on
    //    TU 2.00.00. That walk always finds the same fixed set of six
    //    roster-slot bodies. CONFIRMED (2026-09-06, two sessions, one incl.
    //    a real protagonist switch): the accessor's fixed owner is roster
    //    slot 0, and it is the ONLY list entry (rtTotal=1 out of ~200-1000)
    //    that ever passes the possessor round-trip, in every one of 2678
    //    samples across the whole session including the switch. Round-trip
    //    is therefore ALSO a dead end for player resolution here, same as
    //    tag==1 - both selection strategies tried on top of this manager
    //    are broken. Open question now: is the live body even reachable
    //    from anchorVt's seed at all, or does it carry a wholly different
    //    vtable never inspected? This build adds a full-list vtable
    //    histogram (diag/mgrwalk2) plus anchorVt's true uncapped member
    //    count, to check that before hunting Ghidra for a different anchor.
    //
    //  - SelfChainTick (added 2026-09-06, BREAKTHROUGH): resolves the
    //    manager from a SECOND, independently-verified slot signature
    //    (kSig_CharMgrSlot_TU20100_A/_B, RVA 0x6C29C68 - confirmed by direct
    //    live memory read to be the SAME manager object ManagerWalkTick's
    //    slot already reaches, so this is not a different list, just a
    //    safer way to reach it without ever calling the accessor) and runs
    //    TU 2.00.00's actual, previously-never-implemented
    //    TickResolveSelf/WalkSelfChain algorithm: seed the protagonist-class
    //    vtable from a tag==1 hit (as before), then for every member of that
    //    class, walk owner+0x68->actor, actor+0x20->marker, marker+0x18->
    //    root, root+0x58->statArray, and require statArray to type-check as
    //    Health - unlike tag/round-trip, this correctly excludes the pool's
    //    inactive slots and resolves exactly the SET of active protagonists
    //    (Kliff + whoever you're playing + any summoned companion), with
    //    ZERO offset changes from the TU 2.00.00 source. Confirmed externally
    //    (read-only ReadProcessMemory, no injection) against a working
    //    third-party TU 2.01.00 build before this was written - this tick
    //    re-does the same walk through OUR OWN signature-scanning
    //    infrastructure, which has not yet been live-tested. See
    //    COMPATIBILITY.md "BREAKTHROUGH (2026-09-06)".
    //
    // ProbeTick/ManagerWalkTick/SelfChainTick never hook anything gameplay-
    // relevant or write any game memory. This install ALSO now drives
    // Player::RefreshSelf() every tick (2026-09-07, after SelfChainTick
    // confirmed live across a real protagonist switch) - Teleport normally
    // owns that driver via its own hkMoveUpdate, but Teleport stays
    // uninstalled on an unverified build, so this probe is the substitute
    // game-thread tick. Player's own toggles (off by default) and its
    // WalkSelfChain-validated sets keep this safe; see player.cpp. Once
    // Teleport (or a real replacement driver) is verified for this build,
    // remove this call and every probe in this file - they have no other
    // purpose.
    class PlayerAccessorProbe
    {
    public:
        // Resolves the candidate accessor (refuses to proceed if the
        // signature is missing or ambiguous) and arms the read-only,
        // pass-through move-update tick that drives the probe. Safe to call
        // on an unverified game build - independent of Player/Teleport,
        // which stay uninstalled there.
        static bool Install();
        static void Remove();
    };
}
