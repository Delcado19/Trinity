# Crimson Desert Trinity Signature Audit

- Game version: `2.04.07`
- Baseline: `gugi97 Trinity TU 2.00.00`
- Baseline commit: `7ad5fb1895021d7132083b4194ca1b1ab208610b`
- Executable: `CrimsonDesert.exe`
- File version: `1.0.0.3549`
- SHA-256: `25f24e06a1f886e9310fd290f9462f66b480a5e7763d1fe156bfddaa8d1992b9`
- PE timestamp: `0x6AEB7D59` (2026-11-04T18:45:45Z)
- SizeOfImage: `0x13CFF000`
- Primary PE sections: `.xtext, .xpdata, .link`

> A matching byte count does not verify function semantics, calling conventions, or layouts.

| Signature | Feature | Matches | Count status | Scope | RVAs |
| --- | --- | ---: | --- | --- | --- |
| `kSig_StatCommit` | Health, stamina, and spirit stat commit | 0 | MISSING | none | - |
| `kSig_DamageApply` | Damage application and multipliers | 1 | EXPECTED_COUNT | executable-or-link | 0x18E6D80 |
| `kSig_MountStaminaTick` | Historical mount stamina tick | 0 | NOT_FOUND | none | - |
| `kCharMgrAnchor_1` | Character manager consensus anchor | 0 | MISSING | none | - |
| `kCharMgrAnchor_2` | Character manager consensus anchor | 0 | MISSING | none | - |
| `kCharMgrAnchor_3` | Character manager consensus anchor | 0 | MISSING | none | - |
| `kCharMgrAnchor_4` | Character manager consensus fallback anchor | 0 | MISSING | none | - |
| `kSig_MoveUpdate` | Player position tracking | 1 | EXPECTED_COUNT | executable-or-link | 0x4585370 |
| `kSig_LocoStepper` | Locomotion and super run | 0 | MISSING | none | - |
| `kSig_TravelToNode` | Fast travel | 0 | MISSING | none | - |
| `kSig_DestinationUpdate` | Current destination tracking | 0 | MISSING | none | - |
| `kSig_PathingHelper` | Teleport pathing helper | 0 | MISSING | none | - |
| `kSig_MarkerOriginPrefix` | Marker origin candidate prefix | 26 | CONTEXT_REQUIRED | executable-or-link | 0x7695E4, 0x769955, 0x3975937, 0x397CF16, 0x39E1815, 0x3A02449, 0x3A08B53, 0x3A08FE7, 0x3A152AD, 0x3A1BADE, 0x3A2DFBC, 0x3A306E9, 0x3CC373C, 0x3CE0427, 0x3CE... |
| `kSig_LeaR8Rip` | RIP-relative table/string candidate | 123105 | CONTEXT_REQUIRED | executable-or-link | 0x93BE, 0x9C01, 0x75E3B, 0x767D5, 0x77229, 0x779E6, 0x77A4D, 0x7E3EC, 0x7E3FD, 0x7E411, 0x7E425, 0x7E439, 0x7E44D, 0x7E4CC, 0x7E4DD, 0x7E4F1, 0x7E56C, 0x7E57... |
| `kSig_TableResolverPrologue` | 32-bit-key table resolver prologue | 0 | MISSING | none | - |
| `kResolverPrologue16` | 16-bit-key table resolver prologue | 0 | MISSING | none | - |
| `kSig_InvGetItemQty` | Inventory item quantity accessor | 0 | MISSING | none | - |
| `kSig_InvGetHolder` | Inventory holder resolution | 0 | MISSING | none | - |
| `kSig_InvSetExpandSlots` | Inventory slot expansion | 0 | MISSING | none | - |
| `kSig_InvHolderInsert` | Inventory insertion planner | 0 | MISSING | none | - |
| `kSig_InvCommit` | Inventory transaction commit | 0 | MISSING | none | - |
| `kSig_InvCoreGlobal` | Inventory core global | 0 | MISSING | none | - |
| `kSig_TrItemValueCtor` | Item-value constructor | 0 | MISSING | none | - |
| `kSig_InvCommitPlacement` | Inventory placement commit | 0 | MISSING | none | - |
| `kSig_InvFreePlacements` | Inventory placement cleanup | 0 | MISSING | none | - |
| `kSig_MovR8Rip` | Indirect RIP-relative table/string candidate | 18493 | CONTEXT_REQUIRED | executable-or-link | 0x160786, 0x1FE151, 0x1FEA01, 0x2489F6, 0x248A96, 0x248B36, 0x248BD6, 0x248C76, 0x248D16, 0x248DB6, 0x248E56, 0x248EF6, 0x248F96, 0x249036, 0x2490D6, 0x24917... |
| `kSig_LocStringGet` | Localized string lookup | 1 | EXPECTED_COUNT | executable-or-link | 0x14A0570 |
| `kSig_ReportStealthExecute` | Historical stealth crime path | 0 | NOT_FOUND | none | - |
| `kSig_TheftCrimeReport` | Historical theft crime path | 0 | NOT_FOUND | none | - |
| `kSig_ClientStealItemExecute` | Historical client steal execution | 1 | OBSERVED | executable-or-link | 0x2D369B0 |
| `kSig_ClientStealItemProcess` | Historical client steal processing | 2 | OBSERVED | executable-or-link | 0x22EDCC0, 0xD2AD5C0 |
| `kSig_AIFuncRegistCrime` | Historical AI crime registration | 2 | OBSERVED | executable-or-link | 0x24D5640, 0x24D64C0 |
| `kSig_AIFuncWitnessCriminal` | Historical AI crime witness | 1 | OBSERVED | executable-or-link | 0x24D12A0 |
| `kSig_WantedAddCrimeRecord` | Historical wanted-state crime record | 0 | NOT_FOUND | none | - |
| `kSig_SetCrimeTarget` | Historical crime target setter | 3 | OBSERVED | executable-or-link | 0x2ED4850, 0x2EE79F0, 0x42339C0 |
| `kSig_MasterFrameUpdate` | Master frame update | 0 | MISSING | none | - |
| `kSig_GameSpeed` | Game speed state | 0 | MISSING | none | - |
| `kSig_FieldTimeRealm` | Client/server field-time realm | 0 | MISSING | none | - |
| `kSig_FieldTimeTick` | Field-time tick | 0 | MISSING | none | - |
| `kSig_TodEngineGlobal` | Time-of-day engine global | 1 | EXPECTED_COUNT | executable-or-link | 0x2F9137A |
| `kSig_EquipBatch` | Equipment batch update | 0 | MISSING | none | - |
| `kSig_DyeApplyBatch` | Dye batch application | 0 | MISSING | none | - |
| `kSig_DyeUpsert` | Current dye-record upsert alternative | 0 | NOT_FOUND | none | - |
| `kSig_DyeUpsert_1180` | Legacy dye-record upsert alternative | 0 | NOT_FOUND | none | - |
| `kSig_EquipEffectRefresh` | Equipment-effect refresh | 1 | EXPECTED_COUNT | executable-or-link | 0xA80880 |
| `kSig_FriendlySetNpc` | Current NPC trust setter alternative | 0 | NOT_FOUND | none | - |
| `kSig_FriendlySetNpc_1180` | Legacy NPC trust setter alternative | 0 | NOT_FOUND | none | - |
| `kSig_FriendlySetPet` | Pet trust setter | 0 | MISSING | none | - |
| `kSig_ParryVerdict` | Easy Parry executable patch site | 0 | MISSING | none | - |
| `kSig_WeatherDeserialize` | Weather preset deserializer | 0 | MISSING | none | - |

All semantic statuses are `UNKNOWN` until manual reverse-engineering and runtime validation.
