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
    //    player.cpp's g_charMgrGlobal - this part is LIKELY, not broken: a
    //    3.5h session found a stable manager pointer whose list count
    //    tracked world population) and runs the SAME vtable-shared-class
    //    walk Player::TickResolveSelf already uses on TU 2.00.00. That walk
    //    always finds the same fixed set of six roster-slot bodies. LIVE
    //    FINDING (2026-09-06): the accessor's fixed owner is roster slot 0,
    //    and the aggregate round-trip check passed for the whole session -
    //    but that was only checked in aggregate, seeded by the same broken
    //    tag==1 test used for the accessor. This build logs round-trip and
    //    tag PER roster slot plus a whole-list round-trip total, to tell
    //    apart "round-trip is a per-object class invariant here too" from
    //    "round-trip is the actual possession signal, tag just wasn't."
    //
    // Neither probe hooks anything gameplay-relevant, feeds Player/Teleport,
    // or writes any game memory. Once the open question is answered - a
    // resolver promoted to VERIFIED in offsets.h and wired into Player, or
    // round-trip disproved as a possession signal too - delete the
    // corresponding probe; it has no other purpose.
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
