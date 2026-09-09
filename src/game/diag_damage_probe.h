#pragma once

namespace trinity::game
{
    // Read-only, pass-through diagnostic hook on kSig_DamageApply. No new
    // signature was needed: COMPATIBILITY.md's "Damage application static
    // analysis" already found the TU 2.00.00 pattern gives a unique match on
    // TU 2.01.00 at RVA 0x1718500 (mod.cpp even calls this out by name: "TU
    // 2.01.00 demonstrates why a surviving unique match is not enough:
    // DamageApply still occurs once while StatCommit and every character-
    // manager anchor are gone").
    //
    // Why this probe exists: diag_player_accessor.*'s character-manager walk
    // (tag==1 / possessor round-trip on the roster-slot-0 class) was
    // confirmed dead for identifying "who is currently played" across two
    // full sessions including real protagonist switches - the result never
    // changes. But player.cpp's own production code never actually needs
    // that walk for damage-driven features: its hkDamageApply receives the
    // victim object directly as its first argument (`targetOwner`), with no
    // manager-list search at all. This probe logs that argument - plus the
    // same tag/possessor-round-trip identity check used elsewhere in this
    // file's sibling probe - every time a battle hit fires, to see whether
    // THAT value changes when the player takes a hit as a different
    // protagonist. If it does, no selection algorithm is needed at all: the
    // engine hands the right object to this call site for free.
    //
    // Purely observational: the original is always called unmodified, no
    // delta/value is altered, and nothing is written to game memory.
    class DamageProbe
    {
    public:
        // Refuses to install if the signature is missing or ambiguous. Safe
        // on an unverified game build - independent of Player/Teleport,
        // which stay uninstalled there.
        static bool Install();
        static void Remove();
    };
}
