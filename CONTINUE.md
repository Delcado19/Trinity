# Continuation notes (2026-11-05)

Handoff for whichever agent picks this up next (Codex CLI or otherwise).
Read this, then `AGENTS.md` and `COMPATIBILITY.md` (the long-running,
dated project journal - search it for the matching date before assuming
something is undocumented).

## Branch / build

- Branch: `port/tu-2.03.00`. Remotes: `origin` (this fork), `upstream-gugi`
  (the upstream fork this project tracks).
- Game is currently patched to **Crimson Desert 1.0.0.3549 (TU 2.04.07)**.
  Registered in `src/core/gameversion.cpp`'s `kKnown` table.
- Build: `cmake --build build` fails here because the configured SDK
  (`10.0.26100.0`) is not installed (only `10.0.28000.0` is). Build directly
  with MSBuild instead, and pass the override:
  ```
  & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    build\Trinity.vcxproj /p:Configuration=Release /p:WindowsTargetPlatformVersion=10.0.28000.0 /nologo /v:minimal
  ```
  **Must run via PowerShell, not Bash** - the Bash tool's MSYS path
  translation strips the leading `/` off `/p:` flags and the build fails
  with `MSB1008`.
- Current state: builds clean (0 warnings/errors) as of this commit.
- `Trinity.log` lives in `F:\Steam\steamapps\common\Crimson Desert\bin64\`.
  Always check its mtime + init line against `build\Release\Trinity.asi`'s
  mtime before trusting it - DMM deletes the log on install, and a stale
  log from a different build/binary looks superficially similar.

## Policy: no third-party trainer names or URLs in this repo

The user does not want this project associated with other Crimson Desert
trainers/cheat tools for liability reasons. Every reference to a third-party
mod, Cheat Engine table, or author in code comments, `COMPATIBILITY.md`, and
`reports/` must stay generic ("a third-party inventory-editor trainer", "a
foreign fork of this project", "a third-party loot mod") - no proper names,
Nexus mod numbers/URLs, GitHub URLs, or author handles. This was retroactively
applied across the whole repo + the Claude memory store on 2026-11-05 (see
the git history around this commit for the scrub). Keep doing this going
forward: analyzing other ASI mods for RE leads is fine and encouraged
(see `feedback_dmm_folder_offlimits` equivalent policy - reading/analyzing
mod files in the DMM Mods folder is allowed, only *mounting* them is the
user's own manual job), but strip identifying names before writing anything
down. The one exception is `gugi97`/`upstream-gugi` - that is this project's
own upstream fork lineage (same codebase, not a competing trainer), and its
name/URL stays as normal project history.

## What's done this session (2026-11-05)

1. **Build 3549/TU 2.04.07 registered** in `gameversion.cpp`, confirmed
   recognized live via `Trinity.log`.
2. **Bag Slot Size / Max Stack Size**: already fully implemented end-to-end
   (state, settings, UI, backend, Tick-driven apply) from an earlier
   session - no code change was needed, just confirmed. Slot Size's live
   effect is separately blocked by a dead signature (`inventory: slot-
   expansion setter signature NOT FOUND`), not by missing implementation.
3. **Inventory holder resolver (`InvGetHolder`) - FIXED, root cause
   understood.** The live AOB scan for this function was intermittently
   returning 0 matches despite matching uniquely against the on-disk exe.
   Root cause, confirmed by a controlled A/B test (deploying Trinity ALONE,
   no other ASI mods, and diffing `Trinity.log` against a run with other
   mods loaded): **another, earlier-loading third-party mod hooks this
   exact function first**, overwriting its prologue with a JMP before
   Trinity's own scan runs - so the live bytes genuinely don't match the
   pristine pattern anymore, even though the function and address are both
   correct. Two independent third-party mods' own logs confirm the exact
   same RVA and describe exactly this ("under a jump an unknown module put
   over its entry; hooking on top of it").
   - Fix, in `src/game/inventory.cpp`'s `ResolveHolderPinned()`: added a
     `LooksLikeForeignHookStub()` check (near JMP rel32 / `FF 25` indirect
     JMP / MinHook-style `mov rax,imm64; jmp rax` stub) as a second
     acceptance path alongside the existing byte-identity check. If the
     pinned RVA's live bytes look like a foreign hook stub rather than the
     pristine prologue, we still trust the address and call through it -
     safe for a pure accessor, since a well-behaved foreign hook (confirmed
     by the same third-party logs) reads through to the real function and
     returns its result unchanged.
   - Solo-mount test (`Trinity.log`, 2026-11-05 11:34) confirms the live AOB
     scan itself now succeeds (`holder resolver matched fallback pattern
     #1`) once the collision is removed - i.e. the underlying byte pattern
     was never actually dead, it was always a collision artifact.
4. **Item-table resolver (`iteminfo`/`ItemGroupInfo`/`stringinfo`/
   `InventoryInfo`) - STILL DEAD, root cause narrowed, fix NOT started.**
   This is what drives the "Catalog unavailable" / generic item labels /
   no categories / no icons / no storage names symptoms, independent of the
   holder-collision issue above (confirmed via the same solo-mount log:
   `item-info table not found`, `ItemGroupInfo table not found`, `stringinfo
   table not found`, `InventoryInfo table not found`, all four, with NO
   other ASI mods loaded - this is a genuine signature regression on 3549,
   not a collision artifact).
   - The root cause is narrower than it looks: `inventory.cpp`'s
     `FindTableGlobal(kStr_ItemInfoTable)` (the "iteminfo" string-anchor +
     `kItemPrologue[]` prologue-scan technique documented in `offsets.h`
     around line 1297) returns 0 on this build, and **every other table's
     fallback cascades off that same failure** (`ItemGroupInfo`/`stringinfo`/
     `InventoryInfo` all fall back to `g_itemTableGlobal + 0x20/0x30/0x18`,
     guarded by `&& g_itemTableGlobal`, so none of those fallbacks even run
     if the base lookup fails). **Fixing just the `iteminfo` global
     resolution should restore all four at once.**
   - New leads, not yet applied: a background RE pass against the currently
     active third-party mods (analyzed via their own runtime logs, not
     their binaries) surfaced concrete RVAs on this exact build/session:
     an "ItemInfo reader" function RVA, an "InventoryInfo reader" function
     RVA, specific field-read instruction sites for ItemInfo's max-stack
     field and InventoryInfo's fields, and an "InventoryInfoManager global"
     RVA. These were found moments before this session ended and have NOT
     been cross-checked against Trinity's own resolver, nor verified
     offline. Caution: the exact relationship between "InventoryInfoManager
     global" and Trinity's own `g_itemTableGlobal` (is it the same array
     base, or a sibling slot at a known offset like `+0x18`?) is an
     unverified hypothesis, not a confirmed fact - do not hardcode it as a
     pinned RVA without the same byte/structural verification discipline
     used for the holder-resolver pin above (see `ResolveHolderPinned()` for
     the pattern to follow: verify before trusting, fail closed to
     NOT_FOUND if verification fails).
   - In-progress investigation approach (abandoned mid-way when this
     session ended): write an offline Python script, reusing
     `tools/TrinitySignatureAudit/audit.py`'s own helpers (`parse_pe`,
     `scan_sections`, `resolve_rip`, `parse_pattern`) against the installed
     `CrimsonDesert.exe`, to re-run the SAME string-anchor + prologue-scan
     logic `FindTableGlobal`/`FindItemPrologueAbove` do live, but offline
     and fast-iterating (no rebuild/redeploy per attempt) - the goal being
     to find out whether the `"iteminfo"` string moved, whether
     `kItemPrologue[]`'s byte shape changed, or whether the RIP-relative
     `mov r8,[rip+disp]`/`lea r8,[rip+disp]` signature itself needs
     updating for this build. This is the recommended next step before
     touching any code for this fix - do NOT guess-hardcode an RVA from the
     third-party leads without this kind of independent, structural
     verification; an item mis-key is "worse than a missing one" (a lesson
     independently learned by one of the trainers this project analyzed -
     see the Claude memory store's inventory-editor-trainer notes).
5. **Repo hygiene**: deleted stray scratch/debug files that had
   trainer-coded names (`fun_180001060.c`, `scratch_audit_3527_ciev113*`).
   Renamed `reports/lian-vtweak-*` to `reports/foreign-fork-*` and scrubbed
   `COMPATIBILITY.md` + two code comments (`src/game/offsets.h`,
   `src/game/inventory.cpp`) of third-party trainer names per the policy
   above.

## Explicitly requested but NOT YET DONE

- **Diagnostic logging extension**: user asked to "build the diagnostic
  logging into Trinity itself, and extend this logging to other features
  too" (not just inventory). No code written for this yet. Follow the
  existing `src/game/diag_player_accessor.cpp`/`diag_damage_probe.cpp`
  pattern (a dedicated diagnostic probe file, wired in from `mod.cpp`,
  one-shot or rate-limited log lines) as the established convention.
- **Status()-LED semantics redesign**: user wants green to mean "confirmed
  working for the player", not just "hook installed" - explicitly triggered
  by Inventory showing green while Catalog is "unavailable". Deferred twice
  already (once for the two live regressions above, most recently re-raised
  scoped to Inventory specifically: "ich will keine grüne LED mehr sehen,
  die mich auf eine kaputte Funktion leitet"). Concrete design material is
  in the Claude memory store (`reference_trainer_inventory_editor_v115`):
  plausibility checks on resolved pointers, two-consecutive-poll
  confirmation before trusting an offset, read-back-after-write, and a
  3-way status split (ready / waiting-for-something-in-game / off) kept
  distinct from "hook installed". The narrowest version of this ask, not
  yet done: make `Inventory::Status()` degrade (not stay `Ok`/green) when
  the item-info/category/string/inventory-info table resolution fails, even
  though the FATAL item-count-accessor/holder checks both still pass.
- **New feature leads researched but not implemented** (from the same
  background RE pass against third-party mods, names stripped per policy):
  - **Flight/Glide regression** (broke on the 3549 patch, per user report):
    no drop-in fix found, but a reusable technique pattern: resolve
    movement/camera hook targets via RTTI type-descriptor string matching
    (survives rebases) instead of raw signatures, and add a periodic
    hook-ownership self-check (re-read your own hook bytes; if they no
    longer point at your trampoline, disable the feature cleanly instead
    of corrupting state). One mod's camera-preset system is driven by an
    XML file (`ActionChart/Xml/Description/PlayerCameraPreset.xml`) with
    named glide-speed presets (normal/fast/zoom/fall/rush) - worth checking
    if Trinity's own "glide" issue is camera-side rather than
    movement-side.
  - **Fishing ("Angel Trainer")**: concrete state machine found across two
    third-party mods - `HOOK_SET` -> `CATCH_ING` -> `GET_START`, driven by
    forcing a per-actor `CATCH_INPUT` success flag false->true only after
    the player manually casts/hooks, held for a minimum settle time
    (~350ms) and gated on at least two real catch-stage ticks having
    occurred first (avoids desyncing game logic). Nine native hook sites
    per mod, not yet re-derived for 3549. Concrete enough to prototype a new
    Trinity feature once those RVAs are re-derived.
  - **Teleport with a carried/shouldered NPC** (bounty-capture carry):
    best-documented of the three leads. Algorithm: on a map-teleport-start
    hook, if the player is currently carrying another actor, temporarily
    **arm 4 known "release" call-sites** (the ones that normally end a
    carry) by skipping them for the duration of that one teleport, restore
    them the instant the carry ends or after a ~0.5s safety timeout if no
    teleport follows; separately force the despawn/departure-sweep's
    "still held" check to answer true for the carried actor so it survives
    a zone crossing. Byte offsets only cover builds up to 3527 (not 3549),
    but the algorithm itself is a complete, portable design for Trinity's
    currently-dead bounty-carry-teleport feature.
- **Undiagnosed crash**: a third-party mod's own fault handler observed a
  first-chance access violation *inside Trinity.asi itself* (offset
  `+0xA2CC` relative to its own module base, reading an obviously invalid
  pointer) during a session where multiple ASI mods were loaded together.
  Not investigated at all - unknown whether it's related to the hook-
  collision issue above or something unrelated. Worth a dedicated look
  before assuming Trinity is crash-free under multi-mod conditions.

## Where to look for more detail

- `COMPATIBILITY.md` - the long-running, dated project journal. Search by
  date or feature name; it has far more detail than this file on everything
  historical.
- `src/game/inventory.cpp` around `ResolveHolder()`/`ResolveHolderPinned()`/
  `FindTableGlobal()`/`FindItemPrologueAbove()` for the two inventory fixes
  above.
- `tools/TrinitySignatureAudit/audit.py` + its `README.md` for how to run an
  offline signature sweep against a given exe (reused heavily this session
  via ad-hoc Python scripts importing its helpers directly).
