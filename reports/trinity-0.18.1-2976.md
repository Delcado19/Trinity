# Crimson Desert Trinity Signature Audit

- Game version: `2.03.02`
- Baseline: `Trinity 0.18.1 compatibility port`
- Baseline commit: `working tree based on d0cd168`
- Executable: `CrimsonDesert.exe`
- File version: `1.0.0.2976`
- SHA-256: `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`
- PE timestamp: `0x6AB28F00` (2026-09-22T14:21:52Z)
- SizeOfImage: `0x173AB000`
- Primary PE sections: `.rsrc, .xtls`

> A matching byte count does not verify function semantics, calling conventions, or layouts.

| Signature | Feature | Matches | Count status | Scope | RVAs |
| --- | --- | ---: | --- | --- | --- |
| `InvSetExpandSlots` | InvSetExpandSlots | 1 | OBSERVED | executable-or-link | 0x21358A0 |
| `InvCommit` | InvCommit | 1 | OBSERVED | executable-or-link | 0x2132D30 |
| `TrItemValueCtor` | TrItemValueCtor | 1 | OBSERVED | executable-or-link | 0x2409970 |
| `InvCommitPlacement` | InvCommitPlacement | 1 | OBSERVED | executable-or-link | 0x212DE50 |
| `InvFreePlacements` | InvFreePlacements | 1 | OBSERVED | executable-or-link | 0x885E600 |
| `FieldTimeRealm` | FieldTimeRealm | 1 | OBSERVED | executable-or-link | 0x20E5604 |
| `FieldTimeTick` | FieldTimeTick | 1 | OBSERVED | executable-or-link | 0xA384A0 |
| `InvBumpRevision` | Inventory observers | 1 | OBSERVED | executable-or-link | 0x2B63270 |

All semantic statuses are `UNKNOWN` until manual reverse-engineering and runtime validation.
