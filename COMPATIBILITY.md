# Crimson Desert Compatibility Matrix

Target executable fingerprint:

```text
Game version: 2.01.00
File version: 1.0.0.2760
SHA-256: 4d99c15c58bd20a94d354d10ae395d1fac777d59ef52cba8080dc3fc8dc6f454
PE timestamp: 0x6A998DC4 (2026-09-03T15:09:56Z)
SizeOfImage: 0x16F1F000
```

The current port branch must not be loaded into the game. The table below is
based only on an offline signature count; no gameplay behavior has been tested.
A single match means only that the byte sequence exists once.

| Feature | TU 2.01.00 status | Evidence / next gate |
| --- | --- | --- |
| Unmodified TU 2.00.00 source build | PASS | Release ASI built; this does not establish game compatibility |
| DLL loading/injection | UNTESTED | Wait for unknown-version fail-safe |
| Overlay/UI | UNTESTED | Wait for diagnostic-only runtime |
| Version detection | INCOMPATIBLE | Exact 2.01.00 fingerprint is not registered |
| Character/player resolution | BROKEN | All four baseline CharMgr anchors have zero matches |
| Stat commit | BROKEN | `kSig_StatCommit` has zero matches |
| Damage application | UNKNOWN/HIGH RISK | `kSig_DamageApply` has one match; semantics unverified |
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
