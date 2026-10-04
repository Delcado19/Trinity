# Crimson Desert Trinity Signature Audit

- Game version: `2.03.02`
- Baseline: `Inventory/world source comparison against installed PE 2976`
- Baseline commit: `d0cd168`
- Executable: `CrimsonDesert.exe`
- File version: `1.0.0.2976`
- SHA-256: `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`
- PE timestamp: `0x6AB28F00` (2026-09-22T14:21:52Z)
- SizeOfImage: `0x173AB000`
- Primary PE sections: `.rsrc, .xtls`

> A matching byte count does not verify function semantics, calling conventions, or layouts.

| Signature | Feature | Matches | Count status | Scope | RVAs |
| --- | --- | ---: | --- | --- | --- |
| `ours_kSig_InvGetItemQty` | kSig_InvGetItemQty | 0 | NOT_FOUND | none | - |
| `ours_kSig_InvGetItemQty_Gugi` | kSig_InvGetItemQty_Gugi | 1 | OBSERVED | executable-or-link | 0x240F8D0 |
| `ours_kSig_InvGetHolder` | kSig_InvGetHolder | 1 | OBSERVED | executable-or-link | 0x212A150 |
| `ours_kSig_InvSetExpandSlots` | kSig_InvSetExpandSlots | 0 | NOT_FOUND | none | - |
| `ours_kSig_InvHolderInsert` | kSig_InvHolderInsert | 1 | OBSERVED | executable-or-link | 0x24077D0 |
| `ours_kSig_InvCommit` | kSig_InvCommit | 0 | NOT_FOUND | none | - |
| `ours_kSig_TrItemValueCtor` | kSig_TrItemValueCtor | 0 | NOT_FOUND | none | - |
| `ours_kSig_InvCommitPlacement` | kSig_InvCommitPlacement | 0 | NOT_FOUND | none | - |
| `ours_kSig_InvFreePlacements` | kSig_InvFreePlacements | 0 | NOT_FOUND | none | - |
| `ours_kSig_GameSpeed` | kSig_GameSpeed | 0 | NOT_FOUND | none | - |
| `ours_kSig_FieldTimeRealm` | kSig_FieldTimeRealm | 0 | NOT_FOUND | none | - |
| `ours_kSig_FieldTimeTick` | kSig_FieldTimeTick | 0 | NOT_FOUND | none | - |
| `ours_kSig_TodEngineGlobal` | kSig_TodEngineGlobal | 1 | OBSERVED | executable-or-link | 0x2CE53DA |
| `gugi_kSig_InvGetItemQty` | kSig_InvGetItemQty | 1 | OBSERVED | executable-or-link | 0x240F8D0 |
| `gugi_kSig_InvGetHolder` | kSig_InvGetHolder | 1 | OBSERVED | executable-or-link | 0x212A150 |
| `gugi_kSig_InvSetExpandSlots` | kSig_InvSetExpandSlots | 1 | OBSERVED | executable-or-link | 0x21358A0 |
| `gugi_kSig_InvHolderInsert` | kSig_InvHolderInsert | 1 | OBSERVED | executable-or-link | 0x24077D0 |
| `gugi_kSig_InvCommit` | kSig_InvCommit | 1 | OBSERVED | executable-or-link | 0x2132D30 |
| `gugi_kSig_TrItemValueCtor` | kSig_TrItemValueCtor | 1 | OBSERVED | executable-or-link | 0x2409970 |
| `gugi_kSig_InvCommitPlacement` | kSig_InvCommitPlacement | 1 | OBSERVED | executable-or-link | 0x212DE50 |
| `gugi_kSig_InvFreePlacements` | kSig_InvFreePlacements | 1 | OBSERVED | executable-or-link | 0x885E600 |
| `gugi_kSig_FieldTimeRealm` | kSig_FieldTimeRealm | 1 | OBSERVED | executable-or-link | 0x20E5604 |
| `gugi_kSig_FieldTimeTick` | kSig_FieldTimeTick | 1 | OBSERVED | executable-or-link | 0xA384A0 |
| `gugi_kSig_TodEngineGlobal` | kSig_TodEngineGlobal | 1 | OBSERVED | executable-or-link | 0x2CE53DA |
| `lian_kSig_InvGetItemQty` | kSig_InvGetItemQty | 1 | OBSERVED | executable-or-link | 0x17F8DF0 |
| `lian_kSig_InvGetHolder` | kSig_InvGetHolder | 1 | OBSERVED | executable-or-link | 0x212A150 |
| `lian_kSig_InvSetExpandSlots` | kSig_InvSetExpandSlots | 1 | OBSERVED | executable-or-link | 0x21358A0 |
| `lian_kSig_InvHolderInsert` | kSig_InvHolderInsert | 1 | OBSERVED | executable-or-link | 0x2AE7400 |
| `lian_kSig_InvCommit` | kSig_InvCommit | 1 | OBSERVED | executable-or-link | 0x2B699D0 |
| `lian_kSig_TrItemValueCtor` | kSig_TrItemValueCtor | 1 | OBSERVED | executable-or-link | 0x2409970 |
| `lian_kSig_InvCommitPlacement` | kSig_InvCommitPlacement | 1 | OBSERVED | executable-or-link | 0x2130D70 |
| `lian_kSig_InvFreePlacements` | kSig_InvFreePlacements | 1 | OBSERVED | executable-or-link | 0x4880C0 |
| `lian_kSig_FrameTimerBody` | kSig_FrameTimerBody | 1 | OBSERVED | executable-or-link | 0xAD1300 |
| `lian_kSig_FieldTimeRealm` | kSig_FieldTimeRealm | 1 | OBSERVED | executable-or-link | 0x20E5604 |
| `lian_kSig_FieldTimeTick` | kSig_FieldTimeTick | 1 | OBSERVED | executable-or-link | 0xA384A0 |
| `lian_kSig_TodEngineGlobal` | kSig_TodEngineGlobal | 1 | OBSERVED | executable-or-link | 0x2CE53DA |
| `lian_ctor_fallback_1` | TrItemValue constructor fallback | 46 | OBSERVED | executable-or-link | 0x3C4780, 0x493C30, 0x7944E0, 0x8A9090, 0x8BE850, 0x994A60, 0xCCA1D0, 0xCCA720, 0xCCAC70, 0xCCB1C0, 0xCCB710, 0xCCBC60, 0xCCDCA0, 0xCCE1E0, 0xCD2230, 0xCD295... |
| `lian_ctor_fallback_2` | TrItemValue constructor fallback | 117 | OBSERVED | executable-or-link | 0x3C4780, 0x3F79C0, 0x457160, 0x493C30, 0x496300, 0x5587A0, 0x665460, 0x66A7B0, 0x741310, 0x758F90, 0x75D5F0, 0x7734A0, 0x7944E0, 0x8A9090, 0x8BE850, 0x8D746... |
| `lian_ctor_fallback_3` | TrItemValue constructor fallback | 1241 | OBSERVED | executable-or-link | 0x4AE580, 0x5286B0, 0x558480, 0x5FFF40, 0x65C150, 0x66A340, 0x785D50, 0x7C00F0, 0x85CB00, 0x93BF20, 0x9E3E70, 0xA4F370, 0xA8B2D0, 0xAFA940, 0xC87810, 0xCB639... |
| `lian_ctor_fallback_4` | TrItemValue constructor fallback | 1 | OBSERVED | executable-or-link | 0x3FDE960 |
| `lian_ctor_fallback_5` | TrItemValue constructor fallback | 763 | OBSERVED | executable-or-link | 0x393F50, 0x3B0B70, 0x3CBEE0, 0x3D6B30, 0x3DBA30, 0x400AF0, 0x40E630, 0x41A740, 0x435ED0, 0x438390, 0x463780, 0x475300, 0x47B460, 0x47B800, 0x4938B0, 0x4D7BF... |

All semantic statuses are `UNKNOWN` until manual reverse-engineering and runtime validation.
