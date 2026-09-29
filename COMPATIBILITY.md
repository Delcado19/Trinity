# Crimson Desert Compatibility Matrix

Target executable fingerprint:

```text
Game version: 2.01.00
File version: 1.0.0.2760
SHA-256: 4d99c15c58bd20a94d354d10ae395d1fac777d59ef52cba8080dc3fc8dc6f454
PE timestamp: 0x6A998DC4 (2026-09-03T15:09:56Z)
SizeOfImage: 0x16F1F000
```

The current port branch may be loaded only in its diagnostics-only mode. The
table below combines the offline signature audit, targeted static analysis, and
the explicitly identified smoke test. No gameplay behavior has been tested. A
single match means only that the byte sequence exists once.

| Feature | TU 2.01.00 status | Evidence / next gate |
| --- | --- | --- |
| Unmodified TU 2.00.00 source build | PASS | Release ASI built; this does not establish game compatibility |
| DLL loading/injection | PASS | DMM-deployed ASI initialized in `CrimsonDesert.exe` during the 2026-09-05 smoke test |
| Overlay/UI | PASS | User opened the menu; log confirms rendering at 2560x1440 with 6 back buffers |
| Version detection | PASS | Revision 2760 was identified as TU 2.01.00 and remained unverified |
| Process gate | PASS | Runtime log contains only `CrimsonDesert.exe`; helper processes cannot start Trinity initialization |
| Gameplay hook gate | PASS, narrowed 2026-09-07, widened 2026-09-08/09/10 | `mod.cpp`'s unverified-build branch calls `game::Player::Install()`, `game::World::Install()` (narrowly), `game::Parry::Install()`, `game::Teleport::Install()`, and `game::Inventory::Install()` (browse-only); Dye, Equipment, Friendly still do not run on this build |
| Character/player resolution | VERIFIED (2026-09-07) | `WalkSelfChain` (unchanged TU 2.00.00 offsets: `+0x68/+0x20/+0x18/+0x58`) confirmed live on our own signature-scanning build across a real protagonist switch (prior session) and now end-to-end through the actual write path in real play (this session). The manager itself was never broken - both known slots (`0x6C29C88`, `0x6C29C68`) resolve the identical object; `kCharMgrAnchors` now includes two live-verified TU 2.01.00 entries (RVA `0x6C29C68`). See "BREAKTHROUGH" under "Player-accessor live probe" |
| Stat commit | VERIFIED (2026-09-07) | `kSig_StatCommit_TU20100_Candidate` fired correctly in a long real play session (`stat-commit matched fallback pattern #1`, no errors/crashes) and its God Mode consumer was confirmed working by the user |
| Damage application | BROKEN at runtime despite offline match | `kSig_DamageApply` matches uniquely offline (RVA `0x1718500`, re-confirmed against the installed exe) but never resolves live - retried 753 times over 5 real hours, still NOT FOUND every time. Not a timing issue (ruled out by the retry test); root cause unknown, needs a live memory-read comparison next session. One-Hit Kill / damage multipliers / Fire-Cold immunity / No Fall Damage all depend on this and stay non-functional. God Mode / Infinite Stamina / Infinite Spirit are unaffected (different hook, confirmed working) |
| Health/God Mode | VERIFIED (2026-09-07) | Confirmed working by the user in real play (long session, build 12:08:08) |
| Stamina/spirit | VERIFIED (2026-09-08) | Confirmed working by the user in a follow-up long session, including a real (non-combat) protagonist switch |
| Respawn/revive | REQUIRED TEST | Must wait for isolated health/state validation |
| Boss death/quest completion | REQUIRED TEST | Must wait for isolated damage/death validation |
| Position tracking | LIKELY-VERIFIED (2026-09-10) | Unique match retains the seven-argument integrator and `+0x90`/`+0xC0`/`+0xD0` roles; player exclusivity is unverified. `kSig_MoveUpdate` (the fatal signature) installed cleanly (log: `teleport: pathing helper hook installed @ 0x1435C8000`); a live coordinate warp completed with target-world and observed-world matching exactly (`teleport: warp complete target-world -9406.24 564.77 -4562.35 ... observed-world -9406.24 564.77 -4562.35`). Super Jump also confirmed working by the user in the same session. |
| Locomotion/Super Run | VERIFIED (2026-09-10) | Old `kSig_LocoStepper` dead; `kSig_LocoStepper_TU20100_Candidate` (found by walking the airborne-mover anchor to its callee, see "LocoStepper" section) installed live at exactly the predicted RVA (log: `teleport: locomotion-stepper hook installed @ 0x1435BE940`, matching Ghidra's `ENTRY=1435be940`). Confirmed working in real play by the user. |
| Fast travel | LIVE-CONFIRMED with our own build (2026-09-29, bell, v0.18.0 on 2976). Was: OFFLINE-FIXED, NOT YET LIVE-TESTED. 2026-09-29 user teleports (shops + a bell, all worked on 2976) ran on the THIRD-PARTY vTweak v1.4.2 `Trinity.asi` (Trinity.log: "built Sep 20 2026", 2.4 MB, dated 09-25), not on our build (`build\Release\Trinity.asi`, 1.1 MB, 09-28 23:46, not deployed) - so it does not validate our port | `kSig_TravelToNode`'s old three-argument pattern is permanently dead since TU 2.03.00 recompiled the function with a new fourth (mode) argument; ported gugi97's fixed pattern + `kTravelMode_Node` (upstream commit e5b3303, live-tested by gugi on 2944). Re-checked offline against our own installed 2976 exe: one match @ RVA `0x655110` (`0x655110-0x6550B0=0x60` above gugi's 2944 address, plausible build drift). The menu also needs `ResolveTableResolver(kStr_GimmickSceneTable, ...)` (scene-registry) to populate the catalog - unchanged since our shared history with gugi (0-line diff, no commit on either side touched it since merge-base `7ad5fb1`), so it should resolve the same way it did in gugi's live-tested 2944 session, but this is inference, not our own re-check; an empty menu on live test means this, not `TravelToNode`, is the problem. `kSig_DestinationUpdate` (a separate capability, "Teleport to Destination" via map marker) stays at 0 matches on 2976 (confirmed via `audit.py`, 2026-09-28) - inert, not a live bug. See "Fast travel port from upstream gugi" below |
| Inventory read/write | LIKELY (browse-only), re-derived 2026-09-10 | Both FATAL signatures (`kSig_InvGetItemQty`, `kSig_InvGetHolder`) now resolve; `kSig_InvCoreGlobal` also fixed, so the durable container walk populates the list without waiting on the HUD. Quantity edits do NOT persist yet (`kSig_InvCommit`/`kSig_InvHolderInsert`, the server-holder capture paths, still dead) and Add Item/Slot Size are still refused (their own signatures still dead). See "Inventory re-derived via a Cheat Engine table" below. NOT yet live-tested |
| Localization lookup | UNKNOWN | `kSig_LocStringGet` has one match; semantics unverified |
| Time of day | BROKEN | ToD global survives; master/tick/realm paths do not |
| Equipment/dye | BROKEN | Refresh survives; batch/dye operations do not |
| Trust/friendly | BROKEN | Setter signatures have zero matches |
| Easy Parry | LIKELY-VERIFIED (2026-09-09) | Old signature dead; `kSig_ParryVerdict_TU20100_Candidate` re-derived, offline-verified unique, wired live. Multi-hour real-play session: zero errors, normal block behavior, perfect-parry green flash/stagger observed. Confirmation informal (no strict A/B toggle comparison) - see "World / Dye / Parry" section |
| Weather | BROKEN | Deserializer signature has zero matches |

Full mechanical results are in
[`reports/crimson-desert-2.01.00-signatures.md`](reports/crimson-desert-2.01.00-signatures.md).

## Pre-test local evidence

The 2026-09-04 installation state contains Ultimate ASI Loader 9.7.1 as
`winmm.dll` and three other ASIs: `CharacterCreatorHead.asi`,
`FreedomFlyer.asi`, and `LETMESLEEP.asi`.

Historical Trinity v1.3.3 logs show that the loader injected it into both
`CrimsonDesert.exe` and `crashpad_handler.exe`. The old build mislabeled PE
revision 2760 as "TU 2.00.02 (Active)" and installed its `DamageApply` hook at
RVA `0x1718500`; this confirms only the observed location, not semantic safety.
The crash report's first recorded mod fault is in `CharacterCreatorHead.asi`,
followed by repeated null dereferences in the game executable. That evidence is
not sufficient to attribute the crashes to Trinity. A controlled Trinity test
must isolate the other ASIs to avoid confounded results.

## Diagnostics-only runtime smoke test

On 2026-09-05 the freshly built ASI was installed through DMM after the other
ASI mods and old Trinity diagnostics were removed. Trinity v0.18.0 initialized
in `CrimsonDesert.exe`, recognized file version 1.0.0.2760 as TU 2.01.00, and
reported diagnostics-only mode before the overlay became ready. The user opened
the in-game menu successfully. The session ended without generating a
`Trinity_Crash.dmp` or `Trinity_Crash.txt` file.

This smoke test validates loading, version gating, and overlay rendering only.
It does not validate any gameplay hook, offset, structure, or feature behavior.

## Stat commit static analysis

The TU 2.00.00 `kSig_StatCommit` remains `BROKEN` with zero matches. Following
the confirmed TU 2.01.00 damage data flow identifies a likely equivalent at RVA
`0xC4E6A80`, reached through the live thunk at RVA `0x171E630`:

```text
DamageApply 0x1718500
  -> generic status path 0x1718930
  -> ApplyDelta thunk 0x171D6B0
  -> ApplyDelta implementation 0xC4E3E70
  -> StatCommit thunk 0x171E630
  -> StatCommit implementation 0xC4E6A80
```

The implementation accepts the same four argument roles documented by the old
source: entry, time, clamped target, and 16-bit flag. It reconstructs the upper
bound from entry fields `+0x18` and `+0x20`, applies the floor at `+0x28`, and
writes the normalized value at `+0x20` plus the current value at `+0x08`. It
also updates fields at `+0x38`, `+0x48`, `+0x50`, and `+0x52`. These accesses
are verified instruction behavior; their higher-level field names beyond the
old documented fields remain unverified.

Candidate pattern:

```text
66 44 89 4C 24 ?? 48 89 54 24 ?? 53 55 56 57 41 56 48 83 EC ??
4C 8D 71 18 48 89 CF 48 8B 49 20 4C 89 C3 49 03 0E 4C 89 F6 4C 39 C1
```

Expected and observed count: one match at RVA `0xC4E6A80` in executable
`.debug$P`. The function has nine direct callers through its thunk. This is
still `LIKELY/HIGH RISK`, not `VERIFIED`: Ghidra did not recover the calling
convention, player-entry ownership and live layouts are unverified, and the
post-commit hook can affect lethal-state observation. No source signature or
hook is enabled yet.

## Damage application static analysis

Ghidra identifies the unique `kSig_DamageApply` match at RVA `0x1718500`
(VA `0x141718500`) as a 347-byte dispatcher with 39 direct call sites. Its
prologue and decompilation retain the source hook's 11-argument shape, including
a 16-bit status identifier and signed 64-bit delta. Ghidra did not recover a
calling convention, so this is ABI-shape evidence rather than ABI verification.

The dispatcher compares the status identifier with a realm-selected identifier
and routes that case to RVA `0x17164F0`; other statuses go to RVA `0x1718930`.
The special path contains zero-value and negative-delta branches plus state
flags at `+0x272` and `+0x273`. The generic path clamps the resulting value,
increments a change counter, and invokes multiple notification/write-back
callbacks. RVA `0x1717B20`, called by both paths, resolves the current value for
a 16-bit status identifier.

This evidence supports `LIKELY`, not `VERIFIED`: status identities, structure
meanings, caller intent, calling convention, and death/respawn/quest side
effects are not yet established for TU 2.01.00. The hook therefore remains
disabled.

## Character manager static analysis

All four TU 2.00.00 character-manager anchors remain `BROKEN` with zero
matches. The old accessor RVA maps to unrelated parser logic in TU 2.01.00, and
the old anchor suffixes also have zero matches, so neither the old RVA nor a
shortened old pattern is reusable.

The old source documents a manager vector at `+0xB8`/`+0xC0`, a type descriptor
at owner `+0x88`, its tag byte at `+1`, a player-tag mask of `0xF7`, and the
possessor/back-reference pair at `+0xA0`/`+0xD0`. A broad Ghidra operand search
for those values produced 1,148 functions; adding `0xF7` reduced the set to 43,
but reviewed compact candidates did not contain the documented manager-vector,
tag, and possessor data flow.

A narrower search for nearby `+0x88`, `+1`, and `AND 0xF7` instructions produced
two candidates. RVA `0x16E4650` compares the byte behind a pointer loaded from
`+0x88` with 7, but applies `0xF7` to a separate output flag at `+0x132`. RVA
`0x31541B0` repeatedly sets and clears bits in the byte at `+0x88`; its `+1`
operand belongs to an unrelated indexed access. Neither is the old player-tag
check, and neither exposes the manager-vector/possessor round trip.

The stricter round-trip search then identified the likely TU 2.01.00
local-player accessor at RVA `0x2837940`. It loads the slot at RVA `0x6C29C88`,
dereferences it once, and passes the resulting manager to the enumeration helper
at RVA `0x29B5310`. It walks the helper's temporary 0x20-byte entries, reads the
owner pointer at entry `+0x08`, requires type descriptor `owner+0x88` tag byte
`+1` to equal 1, and requires the exact `owner+0xA0` to `possessor+0xD0`
round trip. It returns the matching owner or zero and destroys the temporary
entries before returning.

Twelve functions contain the same `+0xA0/+0xD0` ownership round trip, providing
strong static evidence that this relationship survives TU 2.01.00. Nine of
those functions also use the type descriptor at `+0x88`; several independently
combine tag 1 with the round trip after calling the same enumeration helper.
The old `((tag - 1) & 0xF7) == 0` test does not survive: no round-trip function
contains the `0xF7` mask, and the new accessor accepts tag 1 only. Code that
still assumes the old tag-1-or-9 player set is therefore unsafe.

Candidate pattern:

```text
4C 8B DC 49 89 5B 08 49 89 73 10 57 48 83 EC 40 33 F6
49 89 73 D8 49 89 73 E0 49 89 73 E8 49 8D 43 D8 49 89 43 F0
49 8D 53 E8 48 8B 0D ?? ?? ?? ?? 48 8B 09 E8 ?? ?? ?? ??
48 8B 4C 24 ??
44 8B 44 24 ?? 49 C1 E0 05 4C 03 C1 49 3B C8 74 ?? 90 48 8B 51 08
48 8B 82 88 00 00 00 80 78 01 01 75 ?? 48 8B 9A A0 00 00 00
48 85 DB 74 ?? 48 8B 9B D0 00 00 00 48 85 DB 74 ?? 48 3B DA 74 ??
```

Expected and observed count: one match at accessor entry RVA `0x2837940` in
executable `.data2`. The RIP-relative load at pattern offset `+0x2A` resolves
the manager slot to RVA `0x6C29C88` without embedding that RVA in runtime code.
The sibling slot at RVA `0x6C29C68` is used by other accessors and must not be
substituted merely because its surrounding logic looks similar.

Character/player resolution is `BROKEN`, not `LIKELY`. The accessor has no
recovered direct callers, Ghidra labels its calling convention unknown, and
the old direct manager `+0xB8/+0xC0` container layout has not been established
for TU 2.01.00. The live probe below (run to completion, not just tested for
crash-safety) shows the candidate does not track the currently-played
protagonist at all, so it must not be wired into `player.cpp`.

Classification at this stage: type descriptor `+0x88`, tag byte `+1`, and
possessor/pawn `+0xA0/+0xD0` remain `LIKELY` as a general identity-proof
pattern (reused successfully elsewhere in this document), but the specific
accessor/manager-global candidate built on top of them is `BROKEN` for player
resolution; direct manager fields `+0xB8/+0xC0` remain `UNKNOWN`.

### Player-accessor live probe (read-only) - accessor dropped, manager global kept, selection open

Two independent read-only probes were run live in `CrimsonDesert.exe` against
`kSig_CharMgrAccessor_TU20100_Candidate` and the RIP-relative manager-global
slot it reads (`src/game/diag_player_accessor.*`), both driven off a
read-only, pass-through hook on `kSig_MoveUpdate` (no position, velocity, or
other field is read or written by the hook itself):

- **ProbeTick** calls the accessor directly under SEH and checks the
  returned owner against the type-tag (`+0x88` tag `== 1`) and possessor
  round-trip (`+0xA0` -> `+0xD0`) identity proof documented above.
- **ManagerWalkTick** re-resolves the manager from the accessor's own
  manager-global slot and re-runs the same vtable-shared-class walk
  `Player::TickResolveSelf` (player.cpp) already uses on the verified
  TU 2.00.00 baseline, tag-agnostic, to find every body sharing a
  protagonist's vtable.

A first short session (2026-09-06, ~29s, idle/light movement only) looked
promising: the accessor returned one stable owner with tag/round-trip OK on
every sample. That result was provisional by construction - it never
exercised a real body swap - and a full session was run to close that gap.

**Full session, 2026-09-06, 08:26-11:55 (`Trinity.log`, 7897 lines, 4373
ProbeTick + 3509 ManagerWalkTick samples), covering riding, running, flying,
combat, cooking, questing, and a genuine switch to a different playable
protagonist:**

- `ProbeTick`: `owner=0x2DB7A0F0200` in **all 4370** valid samples, tag `1`
  and round-trip `OK` throughout. Confirms the first session's result was not
  luck - it is pinned to one specific body for the entire session, unmoved by
  the protagonist switch. This matches the known limitation already
  documented in `player.cpp` for the verified baseline: the permanent
  SelfPlayer tag stays on one body even while a different protagonist is
  being played. **The accessor cannot answer "who is currently controlled."**
- `ManagerWalkTick`: `classMatches=6` in **all 3509** samples, and every
  single time the *same six addresses in the same order*,
  `0x2DB7A0F0200,...0300,...0400,...0500,...0600,...0700` - a contiguous
  block spaced exactly `0x100` apart (one 256-byte object slot each; every
  offset this code reads, `+0x88`/`+0xA0`/`+0xD0`, fits inside that), with
  the first entry being the exact address `ProbeTick` also returns. This
  never changed across three and a half hours of varied play, including the
  character switch. Six permanently-resident roster bodies drawn from a pool
  allocator would look exactly like this too, so "static template array" was
  an unverified guess, not a conclusion - see the open question below before
  assuming these six are inert.
- The seed step for that walk (`nTag1`, how many characters in the manager's
  list currently carry tag `1`) ranged from 1 to 99 across the session
  (average ~49), tracking the manager's own live population count (187 to
  1016) rather than staying near 1-2 as "the local player(s)" would. Tag `1`
  is therefore not a rare, player-specific marker on this build; it matches a
  double-digit percentage of all characters, so seeding a vtable from "any
  tag==1 body" is seeding from an arbitrary match, not the player.
- The manager pointer itself (`0x2DB75C12C00`) and its list-count field
  stayed sane and stable in shape the whole session, and neither probe ever
  hit the SEH handler or logged a warning beyond the expected
  diagnostics-only banner - so the memory layout being read
  (`kOff_CharMgr_ListData`/`ListCount`, `kOff_Owner_TypeDesc`,
  `kOff_Owner_Possessor`/`Pawn`) is not itself unsafe or wrong, only
  insufficient to answer "who is playable right now."

**Verdict, split by piece, not lumped together:**

- The **accessor call** (`ProbeTick`) is a dead end for player resolution:
  pinned to a fixed body, unaffected by a real control swap. Do not promote
  it; it has no purpose left beyond staying as a probe.
- The **manager global** the accessor's RIP-relative slot exposes is NOT
  shown to be broken - it is structurally the same two-dereference shape as
  `player.cpp`'s verified `g_charMgrGlobal`, stayed at one stable address the
  whole session, and its list count (187 to 1016) tracked visible world
  population the way a real character list should. It is a working
  `LIKELY` candidate replacement for the broken `kCharMgrAnchors`, on a
  single anchor with no independent cross-check yet.
- The **selection logic** stacked on top of that manager (seed a vtable from
  any tag`==1` body, then collect everything sharing it) is BROKEN: `nTag1`
  tracked world population (1 to 99, matching the double-digit-percent share
  of all characters that carry tag `1`), not "the local player(s)", so the
  vtable it seeded from was an arbitrary match, not the player's.

### Per-roster-slot round-trip test - CONFIRMED negative, round-trip also broken

A second live session (2026-09-06, 12:27-14:37, ~2h10m, `Trinity.log`, 6037
lines, 2678 `diag/mgrwalk` samples) logged round-trip and tag per roster-slot
address individually (not just the tag-seeded aggregate), plus `rtTotal` -
the round-trip pass count across the ENTIRE character list (187-604 entries
that session), not just the six roster slots. The user confirmed a genuine
protagonist switch happened during this session.

Result: `rtTotal=1` in **all 2678** samples, and it was **always** roster
slot 0 (`0x...0200`, the same address the accessor call returns) - never any
other slot, never zero, never more than one, across the whole session
including the switch. The other five roster slots never round-tripped even
once. This directly answers the open question from the first session: round
trip is not a rare-but-real possession signal being masked by a bad seed -
whatever makes roster slot 0 round-trip is a fixed property of that specific
object, unmoved by which body the player actually controls. **Round-trip,
read through this roster pool, is a dead end for player resolution, the same
way tag `==1` was.**

**Verdict, updated:** the accessor call is dead (confirmed twice). Both
selection strategies tried on top of the manager-global's list (tag-seeded
vtable class, and per-member round-trip within that same class) are dead.
The manager-global chain itself is still not shown to be broken - the open
question is now sharper: **is the actively-controlled body even reachable
from `anchorVt`'s seed at all?** `anchorVt` is seeded from an arbitrary
tag`==1` hit that happens to land in the six-member roster pool; if the live
protagonist's object carries a *different* vtable entirely, every probe
built so far is structurally blind to it, independent of round-trip or tag.

The diagnostic (`ManagerWalkTick` in `diag_player_accessor.cpp`) now also
logs a full-list vtable histogram (`diag/mgrwalk2`: distinct vtable count,
top 5 by member count) and the *true*, uncapped member count of `anchorVt`'s
own class. If the roster pool's class turns out to be one of many
similarly-sized classes (or the smallest), the six-member pool was likely
never the right place to look for the played body in the first place - built
2026-09-06, not yet live-tested.

A first-pass static hunt (2026-09-06) for a `+0xA0` possessor SETTER
(the actual "Possess" call, expected to be a much stronger anchor than a
read-side snapshot, since it fires exactly at the control-swap transition)
via `FindFunctionsByScalars.java A0 D0` against the analyzed TU 2.01.00
Ghidra project found 4731 functions containing either scalar (far too broad
- `0xA0`/`0xD0` are common small immediates) and 511 functions writing to
both `+0xA0` and `+0xD0` on the *same* object (unrelated adjacent-field
writes, not a cross-object possess link). Zero functions matched the
specific bidirectional pattern `X->0xA0 = Y` and `Y->0xD0 = X` with exact
register identity - either that exact textual shape does not exist in this
build (compiler reordering / an intervening call could break simple
register-name matching), or the setter lives behind a shared helper that
takes both pointers as arguments rather than inlining both stores at the
call site. Not pursued further yet; the vtable-histogram live test is
cheaper and should run first.

### Vtable histogram live result - anchorVt's class is not small at all

A short live session (2026-09-06, 15:36-15:39, `Trinity.log`, 113 lines, 29
`diag/mgrwalk2` samples) ran the histogram test. The whole character list
(299-873 entries across the session) resolves to only **three** distinct
vtables:

- one growing class (300-870 members, tracks world population) - the
  generic NPC/monster bulk class.
- **`anchorVt`'s own class: a constant 100 members**, not six. `classMatches`
  was silently truncated at 6 the whole time; the roster pool investigated
  in both prior sessions was never "six special slots" - it is 100 objects
  of one archetype, and `nTag1` (~97-99) is now explained: essentially the
  *entire* 100-member class carries tag `==1`, so tag `1` is that archetype's
  class tag, not a "self player" marker at all on this build.
- **one singleton class: exactly 1 member**, never inspected by any probe so
  far (`anchorVt` only ever seeds from a tag`==1` hit, which always lands in
  the 100-member class since that class defines what tag `==1` means here).

The singleton is the concrete answer to the open question from the round-trip
test: a body under a vtable this walk never reached. `ManagerWalkTick` was
extended (`diag/mgrwalk3`, built 2026-09-06, not yet live-tested with a
protagonist switch) to log every class with <=4 members in full - address,
tag, round-trip - specifically to inspect this singleton and watch whether
its address, tag, or round-trip status changes across a Kliff/Damiane switch.

Character/player resolution stays `LIKELY (manager only) / BROKEN
(accessor + both selection strategies tried on the 100-member class)`
pending the singleton-class result.

### Community cross-check (Cheat Engine tables) - no dynamic "current player" pointer exists either

Checked the two active FearlessRevolution Crimson Desert CE-table threads
(forum topics 38679 "MPElite" table, now at v21/"final release", and 38655
"N3rveMods" table) plus the "Marcus101RR" table on Nexus Mods
(nexusmods.com/crimsondesert/mods/64) as an external cross-reference, since
none of this project's own static analysis has found a live "currently
controlled body" resolver on this build.

Result: none of the three community tables expose a dynamic "current
player" pointer either. The Nexus table's feature list is explicit:
**"Character Pointers: Kliff, Damiane, Oongka"** - three separate, fixed,
per-name pointers (one per party member), not one pointer that follows
whichever body is under player control. Its own description calls this
"the basic party data for all 3 characters." The two FearlessRevolution
tables (health/stamina/damage/inventory cheats) likewise key off a
one-time "load player pointers" resolve step (mentioned by a user hitting
a crash on it after a game update) rather than a per-tick re-resolve of
"whoever is active right now."

This cross-checks our own dead-end series rather than shortcutting it:
it's independent evidence that this engine keeps persistent, named
per-character objects (matching our 100-member roster/party class and the
fixed roster-slot-0 object the accessor/round-trip probes kept finding),
and that nobody else has published a working "follow the active
protagonist" resolver for this game either. Reframes the open question:
the right target is probably not "a global that swaps to the active body"
but a **per-object flag** (e.g. an `IsPossessed`/input-control bool on the
character struct itself) that changes on the two known, fixed Kliff/
Damiane objects when the player switches - not a search for a different
object entirely. The still-untested singleton class (see above) remains
worth checking, but this finding lowers confidence that it *is* the
answer, since the community tables suggest the live-controlled distinction
is a field on a known party member, not a separate hidden object.

Not pursued further this session: the .CT files themselves (which likely
contain the actual AOB/pointer-chain definitions CE uses to resolve each
named character) were not downloaded/parsed - would need explicit
sign-off before fetching a third-party binary file, and the description
text already gave the structurally relevant answer.

### First-ever null accessor return (2026-09-06, 17:20-17:22, short session)

A short (~2m45s, 107-line) session produced the first null result from
`ProbeTick` in any session so far - every prior sample, across three
sessions, always returned the fixed roster-slot-0 owner
(`0x...F0200`). At `17:22:45` it logged `accessor returned null (not in
world, or no body currently satisfies the possessor round-trip)`, and the
log ends immediately after (session likely ended right there - loading
screen, death, area transition, or app close, not confirmed).

**Confirmed by the user: this was just session end (game/app closed
shortly after a brief play period), not a protagonist switch or any other
in-game transition.** The null return is fully explained by that - the
hook's last sample landed after the game world had already torn down (or
was tearing down), so no body satisfied the round-trip check any more.
Not a data point about live body resolution; discard it as a switch
signal. The singleton class's non-reaction (`tag=0:rt=no`, unchanged) is
therefore also not informative here. No conclusions drawn from this
session beyond re-confirming the steady-state values already established.

### New direction: DamageApply already hands us the victim directly (2026-09-06)

Re-reading `player.cpp` after the community-table cross-check above revealed
something this whole investigation had missed: **TU 2.00.00's own production
code never resolves "who is currently played" via the character-manager list
for damage-driven features at all.** `hkDamageApply` (player.cpp) receives
the victim object as its literal first argument (`targetOwner`) - no
manager walk, no tag check, no round-trip needed. The character-manager walk
(`TickResolveSelf`/`WalkSelfChain`) exists for a *different* purpose: finding
the full set of currently-active protagonists (Kliff + whoever you're
playing + any summoned companion) so God Mode/Infinite Stamina/Infinite
Spirit can pin ALL of them continuously, every frame, independent of any
damage event. It also documents, in its own comments, the exact problem this
project rediscovered from scratch over three sessions: "the body you
actively control is re-tagged (Mercenary=4) when playing a secondary
protagonist... observed live: the played character carries tag 4 /
Mercenary, not SelfPlayer, while Kliff-as-companion keeps SelfPlayer."

Better still, `mod.cpp` already notes `kSig_DamageApply` (the unmodified
TU 2.00.00 signature) gives a unique match on TU 2.01.00 at RVA `0x1718500`
(see "Damage application static analysis" above) - no new candidate pattern
needed. A new read-only, pass-through probe (`src/game/diag_damage_probe.*`,
`DamageProbe`, wired into `mod.cpp` alongside `PlayerAccessorProbe`) hooks it
and logs `targetOwner` (plus a tag/round-trip cross-check) on every battle
hit, unmodified. If `targetOwner` changes to a different address when the
player takes a hit as a different protagonist, no character-manager
selection logic is needed at all for this purpose - the engine hands the
right object to this call site for free. Built 2026-09-06 17:47, not yet
live-tested. Needed: a short session where the user takes at least one hit
(or any negative stat delta) as Kliff, switches, and takes at least one hit
as Damiane, so `diag/damage` lines can be compared across the switch.

The still-unresolved `WalkSelfChain` offsets (`kOff_Owner_Actor` 0x68,
`kOff_Actor_StatusMarker` 0x20, `kOff_Marker_TargetOwner` 0x18,
`kOff_Root_StatArray` 0x58) have never been checked against TU 2.01.00 at
all - if the damage probe doesn't pan out, re-deriving that chain (the
verified TU 2.00.00 mechanism for tracking the *set* of active protagonists,
not a single "current" one) is the next static-analysis target, not more
character-manager histogram work.

### BREAKTHROUGH (2026-09-06 22:xx): character-manager slot found, WalkSelfChain confirmed unchanged - live-verified against a working third-party build

The upstream maintainer (`gugi97`) published a compiled TU 2.01.00-verified
build (Trinity v0.19.0) on Nexus Mods (crimsondesert/mods/3252) without
publishing the corresponding source (the public `upstream-gugi` remote still
tops out at v0.17.0/TU 2.00.00). The user installed it via DMM to test, and
its `Trinity.log` showed every gameplay feature (Teleport, Inventory, World,
Dye, Friendly, Parry) resolving successfully against this exact game build.

Rather than reverse-engineer the third-party binary or its source, the
already-running process was used as a live oracle for our OWN static
analysis, via a read-only external `ReadProcessMemory` probe (no injection,
no disassembly of gugi's code - just following our own already-documented
offsets against a process our own analysis independently targets):

1. `CrimsonDesert.exe` (pid confirmed from `Trinity.log`) has image base
   `0x140000000` on this run (no ASLR slide happened to occur), so every
   absolute VA gugi's log printed converts to an RVA by subtracting that
   base directly - e.g. `inventory: server character manager @
   0000000146C29C68` is RVA **`0x6C29C68`**.
2. That RVA is the exact "sibling slot... used by other accessors" this
   document already warned itself not to substitute for our own (dead)
   candidate at `0x6C29C88` (see "Character manager static analysis"
   above) - i.e. the correct manager slot was already spotted and
   deliberately set aside as a false lead.
3. An external read of `*(0x146C29C68)` -> `P`, `*P` -> `mgr`, then
   `mgr+0xB8`/`mgr+0xC0` (the SAME `kOff_CharMgr_ListData`/
   `kOff_CharMgr_ListCount` offsets already used elsewhere in this
   document, unchanged) gave a live list of 430 entries - the identical
   roster-slot address family (`...F0200`, `F0300`, `F0400`, ...) every
   prior session's `diag/mgrwalk` already found via the OTHER (0x6C29C88)
   slot. **Both manager slots resolve the same underlying character list on
   this build** - the character-manager/list-data/list-count chain was
   never broken; only the wrong slot was being chased for player
   *identification* purposes.
4. `TickResolveSelf`'s exact TU 2.00.00 algorithm was then reconstructed
   externally against the live list: seed `anchorVt` from any tag==1 hit
   (as before), then for every member of that same vtable class, walk
   `WalkSelfChain` (`owner+0x68` -> actor, `actor+0x20` -> marker,
   `marker+0x18` -> root, `root+0x58` -> statArray, require
   `statArray+0x00 == 0` i.e. `StatType_Health`) with **zero offset
   changes from the TU 2.00.00 source**. Result: exactly 3 of the
   ~100-member class resolve -
   `0x...F0200 tag=1` (Kliff), `0x...F0300 tag=4` ("Mercenary" - the
   played secondary protagonist, matching the exact tag player.cpp's own
   comment already predicted), and `0x...F0600 tag=5` (a third active
   body, presumably the summoned companion Oongka). This is precisely
   "Kliff + whoever you're playing + any summoned companion", matching
   the TU 2.00.00 algorithm's documented intent exactly.

**Correction after further live cross-checking:** a direct external read of
both slots against the same live process shows `0x6C29C68` and `0x6C29C88`
resolve to the **identical** manager object (`*(*slot)` matched exactly for
both). The character-manager/list-data/list-count chain was never actually
broken and never needed a different slot - `diag_player_accessor`'s own
`g_mgrSlot` (derived from the accessor's RIP load, RVA `0x6C29C88`) was
already reaching the correct manager in every prior session. **The entire
fix is `WalkSelfChain`: this project simply never implemented it, using
tag==1 seeding and possessor round-trip instead - both structurally wrong
selection criteria that happened to run against a perfectly fine list.**
`0x6C29C68` is still worth having as a second, independent, directly-hookable
anchor (see below) - it does not depend on ever calling the accessor, an
unverified 100+ byte function with an unknown calling convention - but it
is a robustness improvement, not the fix itself.

**Conclusion: `WalkSelfChain` and its four offsets are unchanged on
TU 2.01.00 and are the missing piece. The manager slot was never the
problem - either existing slot reaches the same manager.** Character/player
resolution moves from `BROKEN` to `LIKELY, live-verified` pending only a
static (Ghidra) signature for RVA `0x6C29C68` itself, so it can be found by
pattern instead of a hardcoded, rebase-fragile constant - the accessor
search technique already used for `0x6C29C88` (RIP-relative load, unique
match) should locate it directly now that the target RVA is known. The
tag==1-seeded 100-member class, the possessor round-trip at `+0xA0/+0xD0`,
and three full sessions of "it never reacts to a switch" were never wrong
observations - they were correctly describing a manager reached via the
WRONG slot's selection criterion (tag/round-trip), not the RIGHT slot's
correct one (`WalkSelfChain`'s vital-chain resolution). The `diag_damage_probe`
built earlier this session is no longer the primary lead; the character-
manager path is now known-correct and should be finished first.

**Static signature found for `0x6C29C68` (2026-09-06, offline-verified):**
Ghidra's `FindReferencesToAddress.java` (new script, `tools/ghidra/`) found
209 reads of that RVA across ~50 call sites, all funneling into one of two
shared char-manager API helpers (`0x1429b39d0` / `0x1429b3e10` in the
analyzed image) as `arg1` (RCX) - matching the exact "read manager, pass as
ARG1" shape `kCharMgrAnchors` already documents for TU 2.00.00. Two
independent call sites were turned into anchors
(`tools/TrinitySignatureAudit/candidates-2.01.00.json`:
`kCharMgrSlot_0x6C29C68_TU20100_Candidate_A/_B`,
now also in `offsets.h` as `kSig_CharMgrSlot_TU20100_A/_B`), each keyed on
the ABI-fixed dereference-into-arg1 step plus a couple of that call site's
literal stack offsets. `tools/TrinitySignatureAudit/audit.py` confirms
BOTH give exactly one raw match in the real executable and BOTH RIP-resolve
to exactly `0x6C29C68` - offline-mechanical confirmation, not yet
re-verified with the runtime `CountMatches`/`FindPattern` path.

`WalkSelfChain` is now wired into the read-only diagnostic probe
(`SelfChainTick`, `diag_player_accessor.cpp`, built 2026-09-06 23:10:02) -
resolves the manager from the new, independently-verified slot (both
`kSig_CharMgrSlot_TU20100_A` and `_B` are required to agree before it's
trusted), seeds `anchorVt` the same way `ManagerWalkTick` already does, then
walks the real `WalkSelfChain` (not tag/round-trip) over every member of
that class and logs every one that resolves (`diag/selfchain`). Not yet
live-tested. Needed: deploy via DMM, confirm `diag/selfchain: manager slot
at RVA 0x6C29C68, confirmed by two independent anchors` appears at startup
(proves both new anchors still agree at runtime, not just offline), then a
short session including a real Kliff/Damiane switch - the resolved set
should stay 2-3 entries throughout and their tags should match the pattern
already found externally (1, 4, 5).

**Live-tested on OUR OWN build, 2026-09-06 23:10 build, session 2026-09-07
11:52-11:56 (confirmed by user: save loaded as Kliff, picked up an item,
mounted a horse, rode, switched to Damiane WHILE STILL MOUNTED, walked,
re-mounted, rode again):**

- Startup confirms both new anchors agree at RUNTIME (not just offline):
  `diag/selfchain: manager slot at RVA 0x6C29C68, confirmed by two
  independent anchors`.
- `diag/selfchain` is no longer frozen (contrast every prior session's
  tag/round-trip result, which never changed once in three full sessions).
  Over ~4 minutes the resolved set moved 4 -> 2 -> 3 -> 1 members, tracking
  real world-state changes instead of a static class invariant. Roster slot
  `...F0200` (tag=1, Kliff) is present in literally every sample, all
  session - exactly the "Kliff is always tracked" behavior the TU 2.00.00
  source predicts.
- User confirmed Kliff and Damiane were NOT at the same location - the
  switch was a genuine area transition, which explains the 17-19s gaps
  preceding each count change (loading screens), and ties the 4->2->3
  sequence directly to the switch: the set shrank right after switching
  (fewer nearby characters resolved in the new area yet) then grew back as
  the new area's population streamed in. Roster slot `...F0200` (tag=1,
  Kliff) stayed resolvable in every sample even while the user was
  physically playing as Damiane in a different location entirely -
  confirms Kliff's object is tracked independent of camera/controlled-body
  location, matching "Kliff is always tracked" exactly.
- **Caveat: `WalkSelfChain` returns the SET of active protagonists
  (matching its documented TU 2.00.00 purpose - apply God Mode/Infinite
  Stamina/Infinite Spirit/damage multipliers to all of them uniformly), not
  a single "the exact body I am piloting right now" identity.** This is
  sufficient for `Player::Install()`'s stat/damage features (which never
  needed per-body identity on TU 2.00.00 either) but NOT yet proof that a
  single-body resolver (needed for Teleport/Free-Flight) exists on this
  build.

**Conclusion: `WalkSelfChain` is confirmed live, on our own signature-scanning
infrastructure, doing exactly what the TU 2.00.00 source describes.**
Character/player resolution can move to `LIKELY, live-verified` for the
God-Mode-shaped feature set.

### Player stat features wired in for TU 2.01.00 (2026-09-06/07, build 2026-09-07 12:08:08)

Rather than flip `isVerified()` for revision 2760 (which would also enable
Teleport/Inventory/World/Dye/Equipment/Friendly/Parry - none independently
verified this session), `mod.cpp`'s unverified-build branch now ALSO calls
`game::Player::Install()` directly, narrowly enabling only God Mode /
Infinite Stamina / Infinite Spirit / damage multipliers:

- `kCharMgrAnchors` (offsets.h) gained the two live-verified TU 2.01.00
  anchors (RVA `0x6C29C68`) as two more voting entries. The four TU 2.00.00
  anchors simply return zero matches on this build and get skipped by the
  existing vote logic - no version branching needed, `ResolveCharMgrGlobal`
  is unchanged.
- `WalkSelfChain`, `IsPlayerClass`, and every offset they use are
  confirmed unchanged (see above) - `player.cpp`'s own logic needed zero
  changes beyond the anchor table.
- `kSig_StatCommit` (TU 2.00.00) is BROKEN on this build; added
  `kSig_StatCommit_TU20100_Candidate` (the RVA `0xC4E6A80` candidate this
  document already found, previously documented but never actually added
  as a signature constant) as a fallback via `InstallHookAny`. Still
  LIKELY/HIGH RISK per its own note (calling convention not Ghidra-
  recovered) - accepted because `hkStatCommit` only writes through
  `PinEntry`, which only ever touches an entry already validated by
  `WalkSelfChain`, so a wrong guess can do nothing, not corrupt unrelated
  memory.
- `kSig_DamageApply` needed no fallback - the unmodified TU 2.00.00 pattern
  already matches uniquely on this build (RVA `0x1718500`, already noted
  in `mod.cpp`'s own comment before this session).
- The one genuinely risky untouched piece, `kMountVtableOffset_TU20000` (a
  hardcoded image-relative offset, not a signature - degrades to "matches
  whatever unrelated class sits there now" instead of "resolves nothing"),
  is now gated behind `CurrentGameVersion().isVerified()` so mount-stamina
  pinning simply does not run on TU 2.01.00 rather than risk pinning the
  wrong object.
- `Player::RefreshSelf()` originally ran only from
  `diag_player_accessor.cpp`'s own `kSig_MoveUpdate` pass-through hook
  (Teleport was still uninstalled at the time this note was first written).
  **Superseded 2026-09-09:** Teleport is now wired in (see "LocoStepper"
  breakthrough) and installs FIRST in `mod.cpp`'s unverified branch
  specifically so its own `hkMoveUpdate` - a strict superset that also
  drives `g_playerMoveOwner`, Super Jump, and Game Speed - claims
  `kSig_MoveUpdate` before the probe's competing hook can (MinHook refuses
  a second hook on an already-hooked target). The probe's substitute now
  fails to install and stays a fallback only, in case a future build ever
  breaks Teleport's own hook again.

Every write remains gated behind its own `State` toggle (all off by
default). As of 2026-09-08/09, God Mode/Infinite Stamina/Infinite Spirit,
Easy Parry, Game Speed (partially), and Teleport (position tracking, warps,
Super Jump, Super Run) all have live hooks on this build; Free Flight and
Trust Multiplier remain non-functional (menu toggles exist but no hook
backs them yet).

**LIVE-TESTED, confirmed working (2026-09-07, long session, build
12:08:08):** startup log confirms both new anchors matched cleanly -
`player: char-manager resolved from 2/6 anchors (the others need
re-deriving; features are unaffected)` (the 4 dead TU 2.00.00 anchors
correctly contributed nothing, no "DISAGREE" warning, so the two new ones
agree with each other) - and `player: stat-commit matched fallback
pattern #1` confirms `kSig_StatCommit_TU20100_Candidate` resolved and was
used. No ERROR/crash lines in the portion read. **User confirmed God Mode
actually works in-game.** A character switch was not exercised this session
(story-gated at the user's current progress), so this does not re-confirm
the multi-body live-selfchain result from the prior session, but the
God-Mode write path itself is now proven end-to-end: anchor resolution ->
`WalkSelfChain` -> `hkStatCommit` -> `PinEntry` write, on a real game
session with the toggle on. Character/player resolution AND the God Mode
write path can move to `VERIFIED` for TU 2.01.00; `kSig_StatCommit_TU20100_Candidate`
specifically can drop its "not VERIFIED" caveat now that it has fired
correctly in live play.

Infinite Stamina and Infinite Spirit confirmed working by the user in a
follow-up long session (2026-09-08, build 12:08:08) - "afaik...zumindest
kam keine Meldung im Spiel" (no error surfaced in play). A real protagonist
switch also happened in that session (without combat) - further live
confirmation of `WalkSelfChain` beyond the read-only SelfChainTick result.

### Bug found and fixed: damage-apply consistently failed to resolve at Install() time (2026-09-08)

The same session reported **One-Hit Kill definitely does not work**. The
(huge, ~150MB, later overwritten by a subsequent launch) log showed why:

```text
player: char-manager resolved from 2/6 anchors (...)
player: stat-commit matched fallback pattern #1 (...)
player: damage-apply signature NOT FOUND - damage multipliers disabled.
```

`kSig_DamageApply` (unmodified, confirmed unique offline at RVA `0x1718500`,
section `.data2` - re-verified with `audit.py` against the exact same
installed executable, same SHA-256, same result) simply was not found by
the RUNTIME scanner at the moment `Player::Install()` ran. Reproduced
across at least two separate game launches the same morning (07:36 and
08:23 starts), both showing the identical failure while char-manager and
stat-commit resolved fine within ~5 seconds every time - not a one-off
fluke.

Root cause: `scanner.cpp` already documents that this game ships packed and
some code regions are not yet unpacked/executable (or even present as a
readable page at all) at the moment `Mod::Initialize()` runs
`Player::Install()`, which is very early - well before the game world, and
possibly before the main menu, finishes loading. `kSig_DamageApply` lives
in one of those regions and evidently was not ready yet on every observed
launch. Previously there was no retry - a hook that failed to resolve at
Install() time stayed disabled for the entire session, silently, with the
error logged only once, easy to miss.

**Fix attempted (build 2026-09-08 08:29:00):** `player.cpp`'s three
hook/global resolutions (char-manager vote, stat-commit `InstallHookAny`,
damage-apply `InstallHook`) were extracted into `ResolvePlayerHooks()`, now
idempotent (skips whatever already resolved). `RetryUnresolvedHooks()`
calls it again from `Player::RefreshSelf()` every tick, retrying every 2s
for the first ~60s of actual gameplay ticks, then backing off to every 15s
for the rest of the session rather than ever fully giving up.

**Live-tested, 2026-09-08, ~5 hour session: the retry mechanism itself
works exactly as designed, but disproves the "just needs to wait for
unpacking" theory.** The slow-phase transition logged once, on schedule
(`still missing hook(s) after ~60s ... damage-apply=0 ... Slowing retries
to every 15s`), and then retried roughly every 15-75s (gaps consistent with
loading screens/menus, when `Player::RefreshSelf()`'s driver - the
move-update hook - does not tick) for the remainder of the session - 753
attempts total, every single one still `NOT FOUND`. No crash, only this one
distinct error message the entire session. **Root cause is therefore NOT
simply "not unpacked yet at Install() time" - something about this specific
signature never becomes resolvable at runtime at all on this build, despite
matching uniquely offline (re-confirmed against the exact installed
executable, same SHA-256, same RVA `0x1718500`, section `.data2`) even
after 5 real hours including presumably plenty of combat (so the actual
function obviously executes - our hook just never attaches to it).**

**Further live investigation, 2026-09-08 afternoon (with CDLoot/CrimsonRoute/
FreedomFlyer/LETMESLEEP all disabled - only `Trinity.asi` loaded):** the
"another mod's hook collides here" theory is now RULED OUT - the JMP is
still there with zero other ASI mods present. External `ReadProcessMemory`
at `moduleBase + 0x1718500` consistently shows a 5-byte `E9` (JMP rel32)
where the expected `48 89 5C 24 ??` prologue should start; the remaining
bytes from offset 5 onward match the rest of the documented pattern
exactly. Following the JMP: it lands in a genuine, committed,
`PAGE_EXECUTE_READWRITE`, privately-allocated trampoline page containing
`FF 25 00000000 <8-byte absolute pointer>` - and that absolute pointer
resolves to an address INSIDE `Trinity.asi`'s own loaded module range.
**This directly contradicts `Trinity.log`, which shows `damage-apply
signature NOT FOUND` continuously and repeatedly (every retry, 15s apart,
for the full length of every session tested) - i.e., Trinity's own code
insists it never successfully created this hook, while live memory shows a
working hook whose destination is inside Trinity's own module.**
Re-verified multiple times, same result each time, not a stale read.

Not yet resolved - added a temporary ground-truth diagnostic instead of
more external guessing: `player.cpp`'s `ResolvePlayerHooks()` now logs
(`player/diag: damage-apply GT`) the raw bytes Trinity itself reads at that
exact address, plus `oDamageApply`'s actual pointer value, on every retry
attempt. Built 2026-09-08 15:39:49. This will show directly, from inside
the process, whether `oDamageApply` really is null when the bytes show a
hook, resolving the contradiction with certainty instead of inference. Not
yet live-tested.

Plausible explanations, not yet distinguished: (a) that specific address
range is genuinely never `MEM_COMMIT`+readable during this session (odd for
code that must execute, but not impossible if it is relocated/copied
elsewhere at load time and the original bytes stay unmapped or become
different unrelated data); (b) the loaded, running bytes at that location
differ from the as-shipped file bytes the offline audit reads - e.g.
load-time patching, a checksum/anti-tamper guard, or lightweight
obfuscation specifically on damage-related code (a common anti-cheat
target), which audit.py's raw-file read cannot see. Needs a live read
comparison (external `ReadProcessMemory` at `moduleBase + 0x1718500`,
same technique used to find RVA `0x6C29C68`) the next time the game is
running, to see directly whether that address is even mapped/readable and,
if so, what bytes are actually there - before spending more Ghidra time on
a signature that may never have been the real function.

Kept the retry (no cost beyond a full-image pattern scan every 15s, and it
correctly stopped spamming after the first minute) since it is harmless and
would still help if the true cause turns out to be session-specific timing
after all - but do not expect it to fix this until the root cause above is
actually identified. God Mode / Infinite Stamina / Infinite Spirit are
unaffected (their hooks resolved fine, confirmed working) - only the
damage-apply-dependent features (damage multipliers, One-Hit Kill, Fire/Cold
immunity, No Fall Damage) remain broken.

Not yet done: re-verifying
`kOff_CharMgr_ListData`/`ListCount` (0xB8/0xC0) are exactly right (they
produced a plausible list, but were not independently re-derived, only
reused - unsurprising now that both slots are confirmed to be the same
manager already used successfully elsewhere); a live test on OUR OWN built
`Trinity.asi` (everything so far was validated by reading a THIRD PARTY's
already-running process, not by running our own code); then, only after
that live-verifies, wiring `TickResolveSelf`/`WalkSelfChain` into
`player.cpp` itself and marking this game version verified; confirming God
Mode/Infinite Stamina/Infinite Spirit correctly apply to all three resolved
bodies once wired up.

## Movement update static analysis

The unchanged `kSig_MoveUpdate` has one match at RVA `0x418EFC0`. Ghidra
recovers seven parameters, matching the old hook shape. The function reads the
desired-motion vector at first-argument `+0xC0`, integrates and notifies using
the position vector at `+0x90`, and writes the resulting velocity vector at
`+0xD0`. These are the same three field roles documented for TU 2.00.00.

There are two direct callers at RVAs `0x35BE940` and `0x35C1EE0`. Both pass the
object loaded from caller component `+0x2B8` as the integrator's first argument
and supply the same seven argument roles. This supports functional continuity,
but static analysis does not establish that the dispatch remains exclusive to
the local player in TU 2.01.00. Ghidra also reports the calling convention as
unknown.

**Note (2026-09-10):** `0x35BE940` - found here via `kSig_MoveUpdate`'s own
caller list, in an earlier session, for an unrelated reason - is the exact
same address independently re-derived as `kSig_LocoStepper_TU20100_Candidate`
this session (see "LocoStepper" breakthrough below), by a completely
different method (walking the airborne-mover anchor to its callee). Two
independent approaches converging on the same function is further
corroboration, on top of the live confirmation already recorded there.

Position tracking was `LIKELY`, not `VERIFIED`, while the hook stayed
disabled. **Superseded 2026-09-09/10:** `Teleport::Install()` is now wired
in and live-tested - see the "LocoStepper" and Position-tracking matrix
entries below for current status.

### Teleport signatures: started re-deriving via the same live cross-reference (2026-09-08)

Following the character-manager breakthrough's method: gugi97's working
v0.19.0 build (Nexus mods/3252, confirmed by the user to be running) logs
live addresses for several teleport pieces at startup. Converted to RVAs
(image base `0x140000000`, unchanged all session):

- `teleport: marker world-origin resolved @ ...46C16B80` → RVA `0x6C16B80`.
  Cross-checked against our own (unchanged) `kSig_MarkerOriginPrefix`
  broad-prefix search (`C5 F8 5C 05`, 26 raw matches offline) via Ghidra's
  `FindReferencesToAddress.java`: many code sites (`VSUBPS`/`VADDPS`/
  `VMOVUPS` against this exact xmmword) exist, the same general shape our
  existing prefix search + `CollectMarkerOrigin` validator already look
  for. Not proven our validator lands on this exact candidate among its 26
  hits, but the underlying technique is confirmed still valid on this
  build - **no code change made**, this needs a live test (not a
  Ghidra-only one) before concluding either way.
- `teleport: airborne mover @ ...435C3290..435C3C5C - Free Flight ready` →
  RVA range `0x35C3290..0x35C3C5C`. This is `kAirMover_Lo/Hi`
  (`teleport.cpp`) - a hardcoded, manually-tracked range (not a signature),
  historically updated per verified version already. Updated to this value
  (build 2026-09-08 17:13:08) - **copied from another build's already-
  working value, not independently re-derived or live-tested through our
  own code.** Has no live effect yet: `Teleport::Install()` is not called
  on this game version (see `mod.cpp`) until Teleport as a whole is
  verified.
- `teleport: pathing helper hook installed @ ...435C8000` → RVA
  `0x35C8000`. The old `kSig_PathingHelper` (TU 2.00.00) is BROKEN on this
  build (0 matches). Ghidra confirms RVA `0x35C8000` is a real function
  entry point (`ExportFunctionContext.java`: `ENTRY=0x1435c8000`,
  3 callers) whose prologue differs from the old signature only in HOW the
  shadow-space registers are saved (RAX-copy-of-RSP instead of RSP-direct -
  a compiler prologue-encoding choice) - everything from the R14/RSI setup
  onward is byte-identical to the old signature's tail. Built a new
  candidate from the TRUE function entry
  (`kSig_PathingHelper_TU20100_Candidate`, offsets.h) and verified offline:
  one match, exactly RVA `0x35C8000`
  (`tools/TrinitySignatureAudit/candidates-2.01.00.json`). Wired into
  `teleport.cpp` via `InstallHookAny` (primary TU 2.00.00 pattern, this as
  fallback) - same pattern as `player.cpp`'s stat-commit fallback. **Not
  yet live-tested through our own hook plumbing** (only offline-verified
  bytes + gugi's own, differently-coded build proved the underlying
  function works).

### LocoStepper/TravelToNode/DestinationUpdate/scene-registry: the log-conversion technique does NOT apply here (2026-09-08/09)

Attempted the same live-log-to-RVA conversion that solved the character
manager, pathing helper, master-frame-update, dye-upsert, and parry-verdict
signatures - it does not work for these four. **Root cause: gugi97's
v0.19.0 is NOT running our source code.** Proof: its startup log prints
`teleport: airborne mover @ ... - Free Flight ready.` - a line that does
not exist anywhere in this repo's `teleport.cpp` (checked: `grep -rn
"airborne mover" src/` finds nothing). gugi's build is therefore a
divergent fork/rewrite that happens to share this project's log-message
style for the pieces it kept recognizable, not a drop-in oracle for every
piece. **Consequence: "no ERROR line for X in gugi's log" is NOT evidence
that X resolved successfully on gugi's build** - our own `InstallHook`/
`InstallHookAny` only log on failure or fallback, so silence in OUR code
would mean success, but gugi's code may log differently, consolidate
messages, or not log some pieces at all. This reasoning was caught and
rejected before being written down as a false conclusion - do not revisit
it without a printed address to anchor on, the same bar every other
breakthrough this session actually met.

Static-only fallback attempted instead: `kSig_LocoStepper`'s distinguishing
frame-setup literals (`LEA RBP,[RAX-0x798]; SUB RSP,0x860`) were searched
directly (Ghidra `FindFunctionsByScalars`-style raw byte scan) since they
survived a full prologue reshape for `DyeUpsert` earlier. Found exactly 4
matches offline. Ghidra `ExportFunctionContext` on all 4:

- RVA `0x2F09060` (closest structural match: same 7-register push set, a
  REX-prefixed BYTE parameter store near the prologue like the old
  signature's `44 88 48 20` = `MOV [rax+0x20],R9B`) - **checked and
  REJECTED.** Its decompiled parameter shape
  (`longlong, longlong, byte, ulonglong, undefined8, undefined8, char,
  undefined1`) does not match the required hook shape
  (`comp: uintptr_t, dt: float, vel: float*, a4..a7: uint64_t`) - a `float`
  second argument would occupy XMM1 while leaving the RDX general-purpose
  slot unused, but this candidate clearly uses RDX/param_2 as a plain
  integer. `BODY_ADDRESSES=4594` is also far larger than a per-frame
  locomotion substep should plausibly be. Installing a MinHook detour with
  a wrong calling convention on a function that fires every frame for every
  character is the highest-risk action this whole investigation could take
  - much higher than any read-only probe or Parry's pre-verified 3-byte
  patch - so this was NOT wired in without stronger confirmation.
- RVA `0x14252d8a0`, `0x14273c0c0`, `0x142abdc40` - not evaluated in depth;
  their register-save/XMM-save shapes diverge further from the old
  signature (see the raw instruction dumps if resuming this).

**RESOLVED (2026-09-09).** User confirmed Super Run and Trust Multiplier
both actually work in gugi's v0.19.0 build - worth the dedicated static
effort. Solved by walking a known-good anchor instead of a blind literal
search: `kAirMover_Lo`/`Hi` (RVA `0x35C3290`-`0x35C3C5C`) is itself the
airborne mover's function range, live-identified from gugi's log in an
earlier step. `ExportFunctionContext.java 0x1435C3290` decompiled that
function and found it calls `FUN_1435be940` **three times** with the exact
`hkLocoStep` argument shape - `(comp, *dt-by-value, &vel-stack-buffer, 0,
ptr, 0, 0)`, matching `hkLocoStep(uintptr_t comp, float dt, float* vel,
uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7)` field-for-field.
`FUN_1435be940`'s own prologue confirms it independently: the old
signature's distinctive byte-parameter store survives byte-identical
(`44 88 48 20` = `MOV [RAX+0x20],R9B`), with exactly one extra qword
home-store inserted before the register pushes - shifting the frame-setup
literals by a consistent `0x10` in both places (`LEA RBP,[RAX-0x798]` ->
`[RAX-0x788]`, `SUB RSP,0x860` -> `SUB RSP,0x850]`), the same
prologue-reshape mechanism behind every other signature this session. It
also calls the **already independently-confirmed** `kSig_MoveUpdate` (RVA
`0x418EFC0`, unchanged on this build - see the "Character/player
resolution" section) and has 12 callers, consistent with a stepper shared
across every movement mode. Verified offline: one match, exactly RVA
`0x35BE940` (`kSig_LocoStepper_TU20100_Candidate`). Candidate `0x2F09060`
from the earlier blind search is NOT among this function's callees or
callers - independent confirmation it was the wrong lead.

A closed loop, not a one-way lookup: `FUN_1435be940`'s three call sites
inside the airborne mover (`0x35C3576`, `0x35C3739`, `0x35C38BA`) all fall
inside `[kAirMover_Lo, kAirMover_Hi)` = `[0x35C3290, 0x35C3C5C)` - the very
range that was copied from gugi's log to build `kAirMover_Lo/Hi` in the
first place. The copied range and the independently re-derived function
corroborate each other.

**Hook-order fix (2026-09-09, same build):** wiring Teleport in surfaced a
real collision, caught before live-testing: `diag_player_accessor.cpp`'s
`PlayerAccessorProbe::Install()` already hooks the exact same
`kSig_MoveUpdate` target as a substitute driver for `Player::RefreshSelf()`
(see the note on that below). It ran BEFORE `Teleport::Install()` in
`mod.cpp`'s original ordering, so `MH_CreateHook` on the same address a
second time would have failed with `MH_ERROR_ALREADY_CREATED` - and
`kSig_MoveUpdate` is `Teleport::Install()`'s only FATAL signature, so
Teleport (including this whole LocoStepper fix) would have silently never
installed, misattributing the failure to the new signature instead of the
call order. Fixed by moving `Teleport::Install()` to run FIRST in that
branch; its `hkMoveUpdate` is a strict superset of the probe's substitute
(also drives `g_playerMoveOwner`, Super Jump, Game Speed), so the probe's
own hook attempt now fails closed and logs, losing only its diagnostic
ticks (CharMgr accessor / SelfChain probing, already exercised and logged
in prior sessions) - not a functional regression.

Wired into `teleport.cpp` (`InstallHookAny(kSig_LocoStepper,
kSig_LocoStepper_TU20100_Candidate)`) and `Teleport::Install()` into
`mod.cpp`'s unverified-build branch (build 2026-09-09, post hook-order fix).
Every other sub-signature was re-checked before wiring: `kSig_MoveUpdate`
(the only FATAL one) is unchanged and confirmed working, so `Install()`
always succeeds; `kSig_DestinationUpdate` and `kSig_TravelToNode`/scene-registry
are still dead and fail closed with a log line each (Teleport to
Destination and the fast-travel menu stay grey/empty, nothing silently
wrong).

**LIVE-TESTED 2026-09-10.** Same-session log confirms a clean install at
exactly the predicted RVA (`teleport: locomotion-stepper hook installed @
0x1435BE940`, matching Ghidra's `ENTRY=1435be940`) and the hook-order fix
working (the probe's competing hook logged `signature NOT FOUND` rather
than crashing or erroring on `MH_CreateHook`). User confirmed Super Run and
Super Jump both work in real play; a coordinate warp completed with
target-world and observed-world coordinates matching exactly. Session ran
19:08-23:55+ with no crash dumps.

**Still open, unchanged:** `kSig_TravelToNode` (BROKEN, fast-travel
trigger), `kSig_DestinationUpdate` status on TU 2.01.00 (unknown - gugi's
log did not show an explicit success/failure line for it), and the
scene-registry table resolver status (also not logged explicitly by gugi's
build). None of these were captured from gugi's log (it did not print
distinct lines for them, unlike the character-manager/inventory captures
from an earlier session, or the airborne-mover range that led to
LocoStepper) - would need either a more targeted read of gugi's live
process or fresh Ghidra work, same as the LocoStepper approach above.

### World / Dye / Parry: same technique, three more signatures re-derived (2026-09-08)

Continuing from gugi97's live log (same session, same running v0.19.0
process): captured RVAs for `world: master frame update` (`0xA541C0`),
`dye: durable upsert` (`0x2353FE0`), and `parry: verdict site` (`0x7FC528`
- the `SETA AL` instruction 4 bytes into the match, matching `parry.cpp`'s
own `kSetaOffset`). All three old TU 2.00.00 patterns are BROKEN (0 matches
each, re-confirmed this session). Ghidra + the same prologue-diffing
technique as the pathing-helper fix produced three new candidates, all
offline-verified unique at exactly the expected RVA
(`tools/TrinitySignatureAudit/candidates-2.01.00.json`):

- `kSig_MasterFrameUpdate_TU20100_Candidate` - prologue reshaped (one extra
  `PUSH R12`, different register-save order/offsets) but the TimeManager
  body (`48 8B F9 48 8B 51 60 8B 42 64 89 42 60`) is byte-identical to what
  the existing doc comment already describes. Wired into `world.cpp` via
  `InstallHookAny`.
- `kSig_DyeUpsert_TU20100_Candidate` - neither old pattern (both mid-
  function, no-prologue anchored) exists on this build at all; this build's
  compiled function needs a full register-save prologue the old ones
  apparently didn't. Anchored at the true function entry instead; the body
  through the documented 16-byte record-stride shift (`49 C1 E0 04`) is
  fully literal. Added as a third option to `dye.cpp`'s existing
  `FindPatternAny` fallback chain.
- `kSig_ParryVerdict_TU20100_Candidate` - trivial: the compiler swapped the
  `VCOMISS` operand order, changing only the ModRM byte (`D3`->`DA`); the
  `SETA`+store that follow (the actual patch site) are byte-identical.
  Added as a second option to `parry.cpp`'s own `FindPatternAny` call.

**Live-wiring decision, per feature (checked each one's OTHER internal
dependencies before deciding, not just the piece fixed this session):**

- **Parry: wired into `mod.cpp`'s unverified branch, build 2026-09-08
  20:35:19. LIVE-TESTED 2026-09-09, confirmed working.** Signature resolved
  cleanly (`parry: verdict site @ ...`), toggle switched on
  (`parry: Easy Parry on.`) and held through a multi-hour real session
  (11:33-15:23+) with zero errors or crashes. User held guard (LB) through
  most fights: blocking itself behaved normally (stamina drain, pushback -
  confirming the guard code path was actively engaged), and the perfect-
  parry green flash + enemy stagger occurred. Confirmation is informal (the
  user accepted "it happened" as sufficient rather than a strict A/B
  before/after-toggle comparison to rule out naturally-good timing), so
  treat as LIKELY-VERIFIED, not lab-grade proof of "every block forced
  perfect" - but combined with the zero-crash multi-hour session, this is
  good enough to consider Easy Parry working for TU 2.01.00.
- **World: wired in, same build.** Only `hkMasterFrameUpdate` (the fixed
  signature above) is confirmed working - it alone provides the Game Speed
  toggle's actual write (`TimeManager.mode`/`timeScale`), no `World::Tick()`
  driver needed for that part. `World::Install()`'s OTHER sub-features
  (`kSig_GameSpeed`'s install-time NOP code patch, `kSig_FieldTimeTick` for
  Freeze Time of Day) were independently re-checked this session and are
  BOTH still `0 matches` on this build - they fail closed exactly as
  designed (no patch applied, hook not installed, `ok=false` but non-
  fatal), so wiring `World::Install()` in does not risk the risky
  install-time patch running against a wrong address. `World::Tick()` is
  deliberately NOT driven from the diagnostic probe, so Freeze/Advance Time
  of Day and the game-speed patch stay fully inert rather than resolve
  partially. NOT yet live-tested.
- **Dye: NOT wired in - checked and it is not actually unlockable yet
  despite the fix above.** `Dye::Install()` checks `kSig_DyeApplyBatch`
  FIRST and returns immediately on failure, before ever reaching the fixed
  `DyeUpsert` fallback chain; `kSig_DyeApplyBatch` and the also-required
  `kSig_EquipBatch` are both independently confirmed `0 matches` on this
  build and were NOT re-derived this session. The `DyeUpsert` fix is real
  and offline-verified, but two more signatures are needed before Dye does
  anything on TU 2.01.00.

**`Teleport::Install()` IS now wired into `mod.cpp`'s unverified-build
branch (2026-09-09, build 16:25)** - see the "LocoStepper" resolution
above. The only FATAL signature (`kSig_MoveUpdate`) is unchanged and
confirmed working, so `Install()` always succeeds; every other
sub-signature fails closed with its own log line rather than doing
something wrong. `kSig_DestinationUpdate`/`kSig_TravelToNode`/scene-registry
remain dead - Teleport to Destination and the fast-travel menu stay
grey/empty until those are re-derived. NOT yet live-tested.

### Inventory re-derived via a user-supplied Cheat Engine table, not the gugi97 log technique (2026-09-10)

8 of `Inventory::Install()`'s 9 signatures were `0 matches` on this build
(only `kSig_InvGetHolder` survived), and `Install()` returns `false`
immediately at the FIRST one it checks (`kSig_InvGetItemQty`) - so the
whole feature was dead, not partially working. gugi97's build was checked
first (same live-log-to-RVA technique that solved five other signatures
this session) and came up empty: it prints three inventory DATA-table
addresses (`ItemGroupInfo`/`stringinfo`/`InventoryInfo`), but those resolve
via a string-based table search independent of the 8 dead byte signatures -
not usable evidence for any of them (consistent with the LocoStepper
finding: gugi's fork does not log everything this project's own code
would).

A static walk from the one surviving signature (`kSig_InvGetHolder`) was
tried next and abandoned: it has 677 callers (vs. LocoStepper's 12), far
too generic to narrow down by hand, and its own callees are two small,
unrelated-looking utility functions.

**Solved instead from a source outside this project's usual toolkit: the
user supplied four Cheat Engine table files from their own collection.**
One of them, `Crimson Desert Ultimate Table.CT` (`by JenAri Dev and
economicalwhale`, v5.0), states in its own header that it targets
**"GAME 2.01.00 (build 1.0.0.2760)"** - the exact file version this whole
port targets - and its Lua source contains detailed, dated commentary from
its own authors independently re-deriving signatures for the exact same
2.01.00 recompile this project has been fighting all session. This is a
new kind of source for this project (not a live process, not another
Trinity fork) but the same underlying legitimacy as the gugi97 technique:
public, already-compiled community tooling targeting the same binary,
consulted read-only, with every claim independently re-verified against
this project's own copy of the executable before being trusted (same
`audit.py`/Ghidra discipline as every other signature this session, not a
blind copy of someone else's AOB).

- **`kSig_InvGetItemQty` (FATAL, blocks all of `Install()`):** the CT
  file's `invCheck` AOB (`48 83 78 10 ?? 7E ?? 80 B8 A0 00 00 00` - a
  bucket-capacity check, "CMP [rax+0x10],0; JLE; CMP byte[rax+0xA0],0")
  matched offline exactly once, at RVA `0x20819AD`, inside a small function
  (Ghidra `ENTRY=0x142081950`). Its prologue register-saves - `MOV
  RDI,R8(keyPtr); MOVZX EBX,DX(typeId); MOV RSI,RCX(container)` - match
  this project's own already-declared `GetItemQty_t` signature
  (`int64_t(void* container, uint16_t typeId, void* keyPtr)`)
  field-for-field; the old dead pattern's visible tail (`49 8B E8 0F B7
  DA`) turns out to be the same `MOVZX EBX,DX` plus a save of the same R8
  argument, just to a different register (`RBP` there vs `RDI` here) - the
  usual prologue-reshape pattern, not a semantic change. Built a fresh
  entry-anchored pattern from Ghidra's own byte dump and confirmed it
  offline-unique at the same RVA.
- **`kSig_InvCoreGlobal` (the durable container-list walk, non-fatal but
  important - without it the list only appears once the HUD happens to
  query a count):** the same CT file's own source comments document this
  EXACT problem already solved by its authors: *"2.01.00 RECOMPILED THE
  HEAD OF THIS SEQUENCE, and that is the whole of 'the container editor was
  not found in this build'... MEASURED against 2.01.00: the old spelling
  has 0 matches, the new one has exactly 1, at exe+5F4C33, and it yields G
  = exe+6C29760 with pm/ent/blk/mo unchanged at 30/50/68/B8."* Their
  `pm/blk` offsets (`0x30`/`0x50`) are exactly this project's own
  `kOff_Global_Mid`/`kOff_Mid_Container` - independent confirmation the
  chain tail never changed, only its head (`mov rax, cs:<global>` becomes
  `mov rcx, cs:<global>`, preceded by an inserted `lea rdx,[rsp+?]`).
  Verified their claimed pattern against this project's own copy of the
  executable (same SHA-256): matches exactly once at RVA `0x5F4C33`, and
  Ghidra independently confirms the RIP load resolves to
  `[0x146C29760]` = RVA `0x6C29760`, exactly matching their claim.

Both offline-verified unique via `audit.py` against the real
`candidates-2.01.00.json` manifest. Wired via `InstallHookAny`
(GetItemQty) and `FindPatternAny` with a per-variant RIP-offset
(InvCoreGlobal, since the old and new patterns' RIP-mov sits at a
different byte offset within each pattern). `Inventory::Install()` is now
wired into `mod.cpp`'s unverified-build branch: both FATAL checks resolve,
so it always succeeds. Checked every remaining dependency before writing
that down as a real result, not a partial-looking one:
`kSig_InvSetExpandSlots` (Slot Size), `kSig_InvCommit`/`kSig_InvHolderInsert`
(server-holder capture - what makes a quantity edit persist instead of
reverting on reconcile) and the Add Item chain
(`kSig_TrItemValueCtor`/`kSig_InvCommitPlacement`/`kSig_InvFreePlacements`)
are all still independently confirmed `0 matches` and all fail closed with
their own log line (none of them are FATAL to `Install()` itself). Net
result: **a browsable, read-only inventory** - the list populates and item
counts can be read, but editing a quantity will not stick, Add Item is
refused, and Slot Size does not apply. Build 2026-09-10 compiles clean.
**NOT yet live-tested.**

Two more leads surfaced in these CT files, not yet acted on:
- `CrimsonDesert.CT`'s own changelog-equivalent notes (different file, not
  the Ultimate Table) document a "Bag Space" fix ("its signature was pinned
  to register choices the recompile changed") that likely corresponds to
  `kSig_InvSetExpandSlots` - not yet located precisely enough to build a
  pattern from.
- The Ultimate Table's changelog also documents, unprompted, a fix for the
  **long-standing, unrelated `damage-apply` mystery** this project has
  never resolved (see "Bug found and fixed: damage-apply consistently
  failed to resolve" above): *"No Fall Damage works again... Its old site
  was deleted by 2.01.00, so it is rebuilt on the damage dispatcher: damage
  aimed at YOU with no attacker behind it is the world, so it is
  cancelled."* This suggests `kSig_DamageApply`'s target function may not
  just be hard to resolve at runtime - it may have been **compiled away
  entirely** on this build, which would explain why a byte-unique offline
  match never once resolves live no matter how long the retry loop runs.
  Worth a dedicated follow-up session against this exact CT file before
  spending more `ReadProcessMemory` time on the old theory.

### Struct-offset drift + InvGetItemQty swap for TU 2.03.00, cross-referenced against gugi97's now-updated repo (2026-09-21)

Steam auto-updated the game past this branch's 2760 target to TU 2.03.00
(file version 1.0.0.2944) before any further live-testing could happen; 2760
itself is gone from this machine, so it can no longer be independently
re-verified. `gameversion.cpp`'s `kKnown` table now also fingerprints 2944 as
unverified (same treatment 2760 got): gameplay hooks still install through
the unverified-build branch, only `isVerified()`-gated code stays off.

`upstream-gugi/fork/tu2.00.00` (previously stalled at v0.17.0/TU 2.00.00) has
six months of real activity since, including two commits covering this exact
transition: `a4db493` (TU 2.01.00/2760 - a full recompile, 30/47 = 64% of
signatures broke - and TU 2.02.00/2850, which broke 0/41) and `e5b3303` (TU
2.03.00/2944, current tip - broke exactly 2/42: `kSig_InvFreePlacements`, a
baked-in jump displacement that broke while the function itself didn't move,
and `kSig_TravelToNode`, recompiled and given a genuine 4th argument - neither
is used by this project). Cross-checked every claim relevant to this port
against this project's own `offsets.h`/`teleport.cpp` before changing
anything:

**Four silent struct-offset drifts, already wrong in this branch's code since
2760 and never caught by any signature scan (a scan only proves the
containing function still exists, not that every offset it reads is still
correct):**

- `kOff_MoveComp_MoveOwner` (teleport.cpp): `0x298` -> `0x2B8`. Free Flight's
  `isPlayer` check compares `comp+off` against the real move-owner; with the
  stale offset this simply never matched, so Free Flight would stay silently
  dead - the hook installs cleanly and logs nothing wrong. Super Run/Super
  Jump are unaffected (they don't use this check).
- `kTls_RealmFlag` (offsets.h): `498` -> `509`. Affects Add Item's realm
  gating (not wired live yet, would have been wrong once it is). A separate
  TLS slot for the field clock also moved (502 -> 492) and still isn't
  represented in this project's code at all - out of scope here, noted for
  later.
- `kOff_EquipComp_Table` (offsets.h): `0x80` -> `0x90`. Feeds Dye/Equipment's
  table-descriptor read; neither feature is wired live on this branch yet
  (Dye blocked on `kSig_DyeApplyBatch`/`kSig_EquipBatch`, both still dead), so
  this had no live effect, only latent risk for when they are picked up.
- `kOff_ItemDef_MaxEndurance` / `kOff_ItemDef_RepairDataList` (offsets.h):
  `0x3F0`/`0x3F8` -> `0x400`/`0x408` (paired fields, same 0x10 shift -
  ItemInfo's row grew above this point). Not currently read by any wired
  feature.

None of the four is independently re-verifiable against 2944 by this
project's own static tooling (they are plain struct offsets, not byte
patterns `audit.py`/Ghidra can scan for) - the correction is trusted on
gugi's cross-build-tested word, same evidentiary tier as every other
third-party-sourced fix this session, until a live diagnostic actually
exercises Free Flight/Add Item/Dye on 2944.

**`kSig_InvGetItemQty`: adopted gugi's independently re-derived pattern in
place of this project's own CT-table-derived candidate.** The two patterns
are structurally different (gugi's spills `typeId` to a stack slot and zeroes
EDI; this project's candidate keeps both arguments in registers) - a
different argument-handling STRATEGY, which argues for genuinely different
functions rather than two compiles of the same one. Re-verified directly
against the real, currently-installed 2944 executable via `audit.py`
(`tools/TrinitySignatureAudit/candidates-2.03.00.json`, baseline SHA-256
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`): **both
patterns match uniquely, but at different RVAs** (gugi's at `0x240F870`, this
project's old candidate at `0x21383E0`) - hard confirmation they are two
distinct functions, not ambiguity in one signature. Deciding factor: gugi's
pattern has been live-tested unmodified across three consecutive builds
(2760/2850/2944) in a working build; this project's own candidate was never
live-tested (its source build, 2760, no longer exists to test against).
`kSig_InvGetItemQty_Gugi` now goes first in `Inventory::Install()`'s
`InstallHookAny` list, with the CT-table candidate kept as a fallback and the
original TU 2.00.00 pattern last. `GetItemQty_t`'s prototype
(`int64_t(__fastcall*)(void* container, uint16_t typeId, void* keyPtr)`) was
confirmed byte-identical between this project's declaration and gugi's own,
so the swap is calling-convention-safe.

**Same offline audit also re-confirmed every other TU 2.01.00-era candidate
already wired live** (`kSig_InvCoreGlobal_TU20100_Candidate`,
`kSig_PathingHelper_TU20100_Candidate`,
`kSig_MasterFrameUpdate_TU20100_Candidate`,
`kSig_ParryVerdict_TU20100_Candidate`,
`kSig_LocoStepper_TU20100_Candidate`) still match uniquely on the real 2944
binary - consistent with gugi's "2/42 broke between 2850 and 2944" finding,
neither of which this project uses. Full manifest and results in
`tools/TrinitySignatureAudit/candidates-2.03.00.json`.

**Other cross-referenced findings, not yet acted on (pure forward
information):**

- `kSig_DestinationUpdate` is gone from gugi's tree entirely - the old
  signature was a false positive matching the road-streamer's terrain
  ground-clamp, not the destination marker, which explains why Teleport to
  Destination never worked on this branch. Replaced there by a direct read
  off `actor+0x168 (NavComp) + 0x1E8 (Dest)`, no hook needed. Not wired here
  (Fast Travel/Teleport-to-Destination are not implemented on this branch at
  all yet).
- `kSig_TravelToNode` gained a genuine 4th argument on 2944 (a mode flag,
  must be `0` for node travel per the game's own call sites). Irrelevant
  until Fast Travel is picked up.
- `kSig_DyeUpsert`'s new TU 2.01.00+ instruction sequence in gugi's tree
  matches this project's own `kSig_DyeUpsert_TU20100_Candidate` - good
  corroboration. Dye is still not wired here pending
  `kSig_DyeApplyBatch`/`kSig_EquipBatch`, both of which gugi has now
  re-derived for 2760 - directly usable once Dye is picked up.
- `kSig_MasterFrameUpdate` is anchored on the function prologue here, which
  offline-verifies unique on this project's own exact binary; gugi anchors
  the same function on a 4-instruction interior sequence instead (5
  functions share the prologue shape on his build, via a new
  `InstallHookInterior` helper) - worth migrating to eventually, since an
  interior anchor is immune to the prologue-reshape pattern that has driven
  most fixes this session.

**New reusable tooling worth adopting regardless of game version, not yet
implemented:** PE-unwind-table-based true function-entry recovery
(`RtlLookupFunctionEntry`, ~25 lines in gugi's `scanner.cpp`) - matches a
short semantic sequence anywhere inside a function body, then asks Windows'
own exception-unwind metadata where the function truly begins, immune to
prologue reshaping by construction; a `sigcheck.py`-style tool that
regex-extracts every `kSig_*` literal straight from source and scans it live
(proposed replacement for the hand-maintained `candidates-*.json` manifest
workflow, which has already cost real friction this session); and
consecutive-identical-line log collapsing (directly relevant to this
project's own `diag/damage` probe log-spam problem).

**Build 2026-09-21 compiles clean. Not yet live-tested** - 2944 is the
currently-installed build, so the next real-play session is the first chance
to confirm any of this, starting with Free Flight (the one drift with an
already-live consumer).

### TU 2.03.01 / 2.03.02 re-audit + air-mover range fix (2026-09-23)

Steam moved past 2944 twice: **2.03.01 = 1.0.0.2949** and **2.03.02 = 1.0.0.2976**
(the installed exe is now 2976, SHA-256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`; 2949 was
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`). Both are
fingerprinted in `gameversion.cpp` as unverified (not live-tested).

`tools/TrinitySignatureAudit/candidates-2.03.01.json` and `candidates-2.03.02.json`
re-check the wired signature set against each exe: **every signature is unique on
both** (`kSig_InvGetItemQty_Gugi`, `kSig_InvCoreGlobal_TU20100_Candidate`,
`kSig_PathingHelper_TU20100_Candidate`, `kSig_MasterFrameUpdate_TU20100_Candidate`,
`kSig_ParryVerdict_TU20100_Candidate`, `kSig_LocoStepper_TU20100_Candidate`,
`kSig_AirMoverStep`). Several moved by 0x10 on 2949 (PathingHelper, LocoStepper,
InvGetItemQty) and shifted again on 2976; nothing needed a new pattern. This
agrees with the third-party Ultimate Table/trainer changelogs (only their give
planner/commit/durability sites moved).

**Second silent bug found by this audit:** `teleport.cpp` carried the airborne
mover as a baked module range (`0x35C3290..0x35C3C5C`, valid for 2760 only). On
2.03.x the mover sits at `0x36A4BB0..0x36A557C` (2949) / `0x36A4C00..0x36A55CC`
(2976), so Free Flight's `inAirMover` test would never have been true - hook
installed, no error, feature dead (same failure class as the stale
`kOff_MoveComp_MoveOwner`). It is now derived at load: `kSig_AirMoverStep`
(gugi97's pattern, b0a87f8) locates one of the mover's calls into the stepper and
`ResolveAirMover()` measures the function via the surrounding `CC CC` int3
padding (capped walks; on failure Free Flight is disabled, Super Run/Jump are
unaffected). Derived size is `0x9CC` on 2760, 2949 and 2976, and the 2949 bounds
equal gugi's live-logged 2944 range + 0x10. Not yet live-tested by us.

Third-party cross-checks (Nexus #3209 Ultimate Table v5.7.5 / trainer v6.9.5,
see memory `reference-crimsoninveditor-ct`): realm byte `0x1FD`, ItemInfo
max-durability `+0x400` and placement record 224 all match our constants; they
date those shifts to 2.0 (2625), not 2760. Their inventory persistence works by
also writing the authoritative parallel array (found by heap scan, identified by
the `slot+0x60` instance-block fingerprint; two of the four parallel arrays are
transient buffers that crash the game if written) - an alternative to our dead
`kSig_InvCommit`/`kSig_InvHolderInsert` capture path.

### Live confirmation after the game update (2026-09-27)

User report from real play on the updated game (TU 2.03.x; exact build not
recorded - installed exe was 2976 at last audit, **confirm before treating as
verified for a specific build**):

- **Working, clearly:** God Mode, One-Hit Kill, Parry, Infinite Stamina.
- **Working, "almost sure":** Infinite Spirit.
- **Not reported / untested:** Teleport, Super Run/Jump, Free Flight (air-mover
  range now derived at load), Inventory browse.

One-Hit Kill working is notable: on 2760 `kSig_DamageApply` never resolved live
(see "Damage application" above). Either the 2.03.x exe made the signature
resolve or the damage path changed - check the log for `kSig_DamageApply` before
updating that table row. Fork-independent leads for the remaining dead
features live in github.com/shin2344234 (flight-freedom, private-storage-master,
stamina-master, bounty-teleportation, master-looter).

#### Third-party oracle: Trinity v1.4.2 "vTweak by Lian" log on PE 2976 (2026-09-27)

The user's live log (pasted, 07:33-07:57) is from a **different binary** than this
repo ("v1.4.2 (vTweak by Lian)", built 2026-09-20; the string does not exist in
this tree). It is therefore NOT verification of our signatures, but it is a
working-build oracle on PE 1.0.0.2976 (TU 2.03.00 per its own detection). Image
base 0x140000000; RVA = address - base:

| Hook (third-party log) | Address | RVA |
|---|---|---|
| damage-apply | `0x1417AE080` | `0x17AE080` (ours on 2760: `0x1718500`) |
| just-window-eval (easy parry/evade) | `0x1408738B0` | `0x8738B0` |
| stat-commit | `0x14C7EFDE0` | `0xC7EFDE0` (unusually high, verify it is inside the image) |
| FrameTimerUpdate | `0x140AD1300` | `0xAD1300` |
| destination pin setter | `0x143836DE8` | `0x3836DE8` |
| inventory core global | `0x146D69190` | `0x6D69190` |

Next step: run `tools/TrinitySignatureAudit` for `kSig_DamageApply` against the
2976 exe and compare its unique match to `0x17AE080`. If equal, the damage row
can be marked resolved on 2976 by cross-check (still not our own live proof, and
our build writes no file log, so live proof needs the overlay/debug output).

**Cross-check result (2026-09-27):** `audit.py` on the installed 2976 exe (SHA-256
`57da440d…`) finds `kSig_DamageApply` exactly once at RVA `0x17AE080`, identical to
the third-party v1.4.2 build's live damage-apply hook. So our existing pattern
targets the right function on 2976 (offline match + working-build oracle). Still
not our own live proof; the 2760 live failure (753 retries) remains unexplained.

### Fast travel port from upstream gugi (2026-09-28)

`upstream-gugi/fork/tu2.00.00` (github.com/gugi97/Trinity) is a fork sharing our
own history (merge-base `7ad5fb1`) that has continued past us. Its tip, commit
`e5b3303` ("feat: support TU 2.03.00..."), independently found and fixed the
exact cause of our dead Fast Travel: TU 2.03.00 (2944) recompiled
`kSig_TravelToNode` a second time AND gave it a new fourth argument (a mode;
node travel needs `0`, read off the game's own three call sites - the other
mode value takes a different branch). A three-argument call left garbage in
`r9`/mode and would have silently taken the wrong branch had the old pattern
still matched; since it does not match at all post-2944 (permanently dead -
the game never rebuilds the three-argument shape), Fast Travel has instead
just failed closed (empty menu, logged) since TU 2.03.00.

Ported: `kSig_TravelToNode` (gugi's re-derived pattern, anchored on the new
arg shuffle + `test r9d,r9d` mode gate, not the frame alone) and the new
`kTravelMode_Node = 0` constant into `offsets.h`; `TravelFn`'s signature and
its one call site in `teleport.cpp` now pass the fourth argument. Gugi
live-tested the pattern in-game on 2944 (one match `@ 0x1406550B0`). We
additionally re-verified it offline against our own installed 2976 exe via
`audit.py` (ad-hoc manifest, not committed): one match at RVA `0x655110`
(`0x60` above gugi's 2944 RVA `0x6550B0` - consistent with the small per-build
drift already seen on our other signatures). Release build compiles clean
(0 warnings/errors). **Not yet live-tested by us** - our build writes no file
log (see memory `live-confirm-2026-09-27`).

**2026-09-29 live session was NOT our build:** the user teleported to several
shops and a bell and everything worked on 2976, but `Trinity.log` in the game's
`bin64` shows the loaded `Trinity.asi` is the third-party vTweak v1.4.2 (built
Sep 20; hooks the destination pin setter @ 0x143836DE8 and has a working
map-marker teleport, hooks=5/5). Our port build was not deployed, so this
neither confirms our `kSig_TravelToNode` fix nor the scene-registry catalog. To
test ours: copy `build\Release\Trinity.asi` over the game's `Trinity.asi`
(back up the vTweak one first) and repeat.

`kSig_DestinationUpdate` ("Teleport to Destination" via the map/quest marker,
a separate capability from node-list Fast Travel) stays at 0 matches on 2976
(confirmed via `audit.py`, 2026-09-28) - inert, not a live bug. Deliberately
left alone this pass: the session budget covered one RE feature, and two
further leads found in the same review (player-identity via
ClientUserActor+0xD0, inventory persistence via the mirror-array write) are
deferred too - see memory `fast-travel-gugi-port` for the reasoning on each.

Note for whoever picks up `kSig_DestinationUpdate`: gugi's tip does not hook
it at all any more. Gugi found the old signature resolved the wrong function
- a terrain/road streamer, RVA in the 2760 build at `0x140675FE0`, whose third
argument is the sector grid centre being streamed around the player, not a
destination; hooking it warped the player to their own position. Gugi's fix
instead reads the marker straight off the nav component (`kOff_NavComp_Dest`,
no hook) via `Player::CharacterOwner()` and a new `kOff_Actor_NavComp` offset
- none of which exist in our tree yet, so porting it is a bigger job than this
one. The third-party vTweak oracle log's "destination pin setter hooked @
0x143836DE8" (RVA `0x3836DE8`) is an independent, unread lead for the same
feature.

### Live teleport check with our own build, v0.18.0 on 2976 (2026-09-29)

First live session with our own `Trinity.asi` (log: "v0.18.0, built Sep 28
23:46"), user report + `Trinity.log`:

- **Saved-location teleport:** works (log: `warp applied ... warp complete`).
- **Area teleport** (named-area catalog, 3665 boxes built): worked - user
  teleported to an Epidot mine. This path logs no warp line after the catalog.
- **Teleport to Destination:** not working, as expected - `kSig_DestinationUpdate`
  NOT FOUND (0 matches on 2976), so the map marker is set but nothing warps.
  Leads: vTweak's pin-setter hook (RVA `0x3836DE8`) or gugi's nav-component read.
- **Fast Travel (node list):** works - user teleported to a bell (~09:48) with
  our own build, confirming the ported 4-arg `kSig_TravelToNode` and that the
  scene-registry catalog resolves on 2976. No log line was checked for it; the
  earlier shop/bell teleports ran on the third-party vTweak build (see above).

### Teleport to Destination: hook removed, nav-component read (2026-09-29)

`kSig_DestinationUpdate` and `hkDestinationUpdate` are deleted. Live on 2976
(Trinity.log) the signature had 0 matches, so setting a map marker never
warped. `LoadDestinationSnapshot` now reads the marker straight off the first
tracked protagonist whose nav component holds one (`actor+0x168` ->
`+0x1E8`, float[3]; three zeroes = cleared), the approach gugi97's fork took.
Offsets are gugi's (live-tested there on 2944), **not yet live-tested by us on
2976**. New `Player::CharacterActor/ActorCount`. Release build compiles clean.
If a live test warps to the wrong place or not at all, the offsets drifted:
re-derive `kOff_Actor_NavComp` / `kOff_NavComp_Dest` first.

### 2976 was stuck in diagnostics-only mode (2026-09-29)

Live log showed `Diagnostics-only mode: gameplay features are unavailable` on
2976: `kKnown` listed it as `verified=false`, so `mod.cpp` never installed the
gameplay hooks (no outgoing damage multiplier / One-Hit Kill; the only
`damage-apply` line was the retry probe). gugi97's fork records 2976 as
verified (42/42 signatures, features run in-game, upstream `d64e72c`). Flipped
to `verified=true` so our own hooks install; **our own live pass is pending**.

**Live-confirmed 2026-09-29 18:18** (our v0.18.0 on 2976, Trinity.log + user report):
Teleport to Destination now warps. Log: `destination world -11034.91, 786.78,
-4065.46` -> `warp complete`, and a second warp to y 757.45 at 18:18:47 (the
observed landing height was consistently ~30 above the target y in both warps,
which the post-hold ground handling accounts for; not investigated further).
So gugi's `kOff_Actor_NavComp 0x168` / `kOff_NavComp_Dest 0x1E8` hold on 2976.
