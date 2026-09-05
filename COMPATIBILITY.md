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
| Gameplay hook gate | PASS | Runtime entered diagnostics-only mode before any gameplay installer ran |
| Character/player resolution | LIKELY | Old anchors are broken; a local-player accessor and manager global candidate were identified statically but remain disabled |
| Stat commit | LIKELY/HIGH RISK | Old AOB is broken; equivalent implementation and a unique candidate AOB were identified statically, but remain disabled |
| Damage application | LIKELY/HIGH RISK | Unique match and dispatcher shape confirmed statically; hook safety and state-transition semantics remain unverified |
| Health/God Mode | DISABLED REQUIRED | Depends on broken player/stat paths and unsafe damage semantics |
| Stamina/spirit | DISABLED REQUIRED | Stat hook and player resolver are broken |
| Respawn/revive | REQUIRED TEST | Must wait for isolated health/state validation |
| Boss death/quest completion | REQUIRED TEST | Must wait for isolated damage/death validation |
| Position tracking | LIKELY | Unique match retains the seven-argument integrator and `+0x90`/`+0xC0`/`+0xD0` roles; player exclusivity is unverified |
| Locomotion/Super Run | BROKEN | `kSig_LocoStepper` has zero matches |
| Fast travel | BROKEN | Travel, destination, and pathing signatures have zero matches |
| Inventory read/write | BROKEN | Holder accessor survives; most inventory primitives have zero matches |
| Localization lookup | UNKNOWN | `kSig_LocStringGet` has one match; semantics unverified |
| Time of day | BROKEN | ToD global survives; master/tick/realm paths do not |
| Equipment/dye | BROKEN | Refresh survives; batch/dye operations do not |
| Trust/friendly | BROKEN | Setter signatures have zero matches |
| Easy Parry | BROKEN | Executable patch signature has zero matches |
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

Character/player resolution is now `LIKELY`, not `VERIFIED`. The accessor has
no recovered direct callers, Ghidra labels its calling convention unknown, its
game-thread requirements are untested, and the old direct manager
`+0xB8/+0xC0` container layout has not been established for TU 2.01.00. The
candidate is recorded only in the offline candidate manifest; runtime source
and all dependent hooks remain disabled.

Classification at this stage: accessor/global, type descriptor `+0x88`, tag
byte `+1`, and possessor/pawn `+0xA0/+0xD0` are `LIKELY`; the temporary result
stride `0x20` and owner field `+0x08` are `VERIFIED` instruction behavior for
this accessor only; direct manager fields `+0xB8/+0xC0` remain `UNKNOWN`.

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

Position tracking is therefore `LIKELY`, not `VERIFIED`, and the hook remains
disabled. A read-only live test must first prove call frequency, object
stability, finite coordinate values, and correlation with local-player motion;
no teleport, velocity, jump, or game-thread write may be enabled during that
test.
