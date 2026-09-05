# Crimson Desert Trinity Signature Audit

- Game version: `2.01.00`
- Baseline: `gugi97 Trinity TU 2.00.00`
- Baseline commit: `7ad5fb1895021d7132083b4194ca1b1ab208610b`
- Executable: `CrimsonDesert.exe`
- File version: `1.0.0.2760`
- SHA-256: `4d99c15c58bd20a94d354d10ae395d1fac777d59ef52cba8080dc3fc8dc6f454`
- PE timestamp: `0x6A998DC4` (2026-09-03T15:09:56Z)
- SizeOfImage: `0x16F1F000`
- Primary PE sections: `.data2, .debug$P`

> A matching byte count does not verify function semantics, calling conventions, or layouts.

| Signature | Feature | Matches | Count status | Scope | RVAs |
| --- | --- | ---: | --- | --- | --- |
| `kSig_StatCommit` | Health, stamina, and spirit stat commit | 0 | MISSING | none | - |
| `kSig_DamageApply` | Damage application and multipliers | 1 | EXPECTED_COUNT | executable-or-link | 0x1718500 |
| `kSig_MountStaminaTick` | Historical mount stamina tick | 0 | NOT_FOUND | none | - |
| `kCharMgrAnchor_1` | Character manager consensus anchor | 0 | MISSING | none | - |
| `kCharMgrAnchor_2` | Character manager consensus anchor | 0 | MISSING | none | - |
| `kCharMgrAnchor_3` | Character manager consensus anchor | 0 | MISSING | none | - |
| `kCharMgrAnchor_4` | Character manager consensus fallback anchor | 0 | MISSING | none | - |
| `kSig_MoveUpdate` | Player position tracking | 1 | EXPECTED_COUNT | executable-or-link | 0x418EFC0 |
| `kSig_LocoStepper` | Locomotion and super run | 0 | MISSING | none | - |
| `kSig_TravelToNode` | Fast travel | 0 | MISSING | none | - |
| `kSig_DestinationUpdate` | Current destination tracking | 0 | MISSING | none | - |
| `kSig_PathingHelper` | Teleport pathing helper | 0 | MISSING | none | - |
| `kSig_MarkerOriginPrefix` | Marker origin candidate prefix | 26 | CONTEXT_REQUIRED | executable-or-link | 0x626864, 0x626BD5, 0x35BB575, 0x35C26D6, 0x362540A, 0x3642A29, 0x36490E3, 0x3649587, 0x36555FD, 0x365BE50, 0x366E13C, 0x3670821, 0x38FACCC, 0x3914817, 0x391... |
| `kSig_LeaR8Rip` | RIP-relative table/string candidate | 142942 | CONTEXT_REQUIRED | executable-or-link | 0x93BE, 0x9C01, 0x7597B, 0x76315, 0x76D69, 0x77526, 0x7758D, 0x7C9D6, 0x7C9E7, 0x7C9FB, 0x7CA0F, 0x7CA23, 0x7CA37, 0x7CAA6, 0x7CAB7, 0x7CACB, 0x7CB46, 0x7CB5... |
| `kSig_TableResolverPrologue` | 32-bit-key table resolver prologue | 24 | CONTEXT_REQUIRED | executable-or-link | 0x382BC0, 0x3BE130, 0x404170, 0x432D50, 0x5115C0, 0x54E870, 0x561860, 0x597B50, 0x59A3A0, 0x74B2B0, 0x8B04F0, 0x1456670, 0x1495F20, 0x14985F0, 0x14CEE60, 0x1... |
| `kResolverPrologue16` | 16-bit-key table resolver prologue | 121 | CONTEXT_REQUIRED | executable-or-link | 0x381D30, 0x382060, 0x382240, 0x382A90, 0x3830D0, 0x383200, 0x383330, 0x3847D0, 0x39A210, 0x3A27B0, 0x3AD6B0, 0x3BDDA0, 0x3BDED0, 0x3BE000, 0x3BE310, 0x3BE75... |
| `kSig_InvGetItemQty` | Inventory item quantity accessor | 0 | MISSING | none | - |
| `kSig_InvGetHolder` | Inventory holder resolution | 1 | EXPECTED_COUNT | executable-or-link | 0x2073710 |
| `kSig_InvSetExpandSlots` | Inventory slot expansion | 0 | MISSING | none | - |
| `kSig_InvHolderInsert` | Inventory insertion planner | 0 | MISSING | none | - |
| `kSig_InvCommit` | Inventory transaction commit | 0 | MISSING | none | - |
| `kSig_InvCoreGlobal` | Inventory core global | 0 | MISSING | none | - |
| `kSig_TrItemValueCtor` | Item-value constructor | 0 | MISSING | none | - |
| `kSig_InvCommitPlacement` | Inventory placement commit | 0 | MISSING | none | - |
| `kSig_InvFreePlacements` | Inventory placement cleanup | 0 | MISSING | none | - |
| `kSig_MovR8Rip` | Indirect RIP-relative table/string candidate | 23823 | CONTEXT_REQUIRED | executable-or-link | 0x1432B6, 0x212936, 0x2129D6, 0x212A76, 0x212B16, 0x212BB6, 0x212C56, 0x212CF6, 0x212D96, 0x212E36, 0x212ED6, 0x212F76, 0x213016, 0x2130B6, 0x213156, 0x2131F... |
| `kSig_LocStringGet` | Localized string lookup | 1 | EXPECTED_COUNT | executable-or-link | 0x1231450 |
| `kSig_ReportStealthExecute` | Historical stealth crime path | 0 | NOT_FOUND | none | - |
| `kSig_TheftCrimeReport` | Historical theft crime path | 0 | NOT_FOUND | none | - |
| `kSig_ClientStealItemExecute` | Historical client steal execution | 1 | OBSERVED | executable-or-link | 0x20703D0 |
| `kSig_ClientStealItemProcess` | Historical client steal processing | 3 | OBSERVED | executable-or-link | 0x10C7530, 0x2070530, 0xEA3A880 |
| `kSig_AIFuncRegistCrime` | Historical AI crime registration | 2 | OBSERVED | executable-or-link | 0x2238CB0, 0x22399F0 |
| `kSig_AIFuncWitnessCriminal` | Historical AI crime witness | 1 | OBSERVED | executable-or-link | 0x22344F0 |
| `kSig_WantedAddCrimeRecord` | Historical wanted-state crime record | 0 | NOT_FOUND | none | - |
| `kSig_SetCrimeTarget` | Historical crime target setter | 2 | OBSERVED | executable-or-link | 0x2B61100, 0x3E3D8D0 |
| `kSig_MasterFrameUpdate` | Master frame update | 0 | MISSING | none | - |
| `kSig_GameSpeed` | Game speed state | 0 | MISSING | none | - |
| `kSig_FieldTimeRealm` | Client/server field-time realm | 0 | MISSING | none | - |
| `kSig_FieldTimeTick` | Field-time tick | 0 | MISSING | none | - |
| `kSig_TodEngineGlobal` | Time-of-day engine global | 1 | EXPECTED_COUNT | executable-or-link | 0x2C1394A |
| `kSig_EquipBatch` | Equipment batch update | 0 | MISSING | none | - |
| `kSig_DyeApplyBatch` | Dye batch application | 0 | MISSING | none | - |
| `kSig_DyeUpsert` | Current dye-record upsert alternative | 0 | NOT_FOUND | none | - |
| `kSig_DyeUpsert_1180` | Legacy dye-record upsert alternative | 0 | NOT_FOUND | none | - |
| `kSig_EquipEffectRefresh` | Equipment-effect refresh | 1 | EXPECTED_COUNT | executable-or-link | 0x902770 |
| `kSig_FriendlySetNpc` | Current NPC trust setter alternative | 0 | NOT_FOUND | none | - |
| `kSig_FriendlySetNpc_1180` | Legacy NPC trust setter alternative | 0 | NOT_FOUND | none | - |
| `kSig_FriendlySetPet` | Pet trust setter | 0 | MISSING | none | - |
| `kSig_ParryVerdict` | Easy Parry executable patch site | 0 | MISSING | none | - |
| `kSig_WeatherDeserialize` | Weather preset deserializer | 0 | MISSING | none | - |

All semantic statuses are `UNKNOWN` until manual reverse-engineering and runtime validation.
