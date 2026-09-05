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
| Character/player resolution | BROKEN | All four baseline CharMgr anchors have zero matches |
| Stat commit | BROKEN | `kSig_StatCommit` has zero matches |
| Damage application | LIKELY/HIGH RISK | Unique match and dispatcher shape confirmed statically; hook safety and state-transition semantics remain unverified |
| Health/God Mode | DISABLED REQUIRED | Depends on broken player/stat paths and unsafe damage semantics |
| Stamina/spirit | DISABLED REQUIRED | Stat hook and player resolver are broken |
| Respawn/revive | REQUIRED TEST | Must wait for isolated health/state validation |
| Boss death/quest completion | REQUIRED TEST | Must wait for isolated damage/death validation |
| Position tracking | UNKNOWN | `kSig_MoveUpdate` has one match; structure semantics unverified |
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
