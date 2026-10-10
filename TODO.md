# TODO

Backlog of known gaps and improvement leads. See COMPATIBILITY.md for the
live-test history behind each status claim below.

## A. Currently broken or fallback-reliant (robustness)

Most of these resolve today only via a fallback signature (`matched fallback
pattern #1/#2` in Trinity.log), not the primary one - they work, but are one
recompile away from breaking again.

- [ ] **Char-manager anchors**: only 2/6 anchors resolve on 2976; the other 4
      need re-deriving. Lead: CrimsonDesertTelemetry's RTTI-anchored
      player-object chain, World Builder's RTTI/unwind anchoring - both find
      objects via compiler metadata (type names, vtables, exception
      directory) instead of literal bytes, so they survive more recompiles.
      Related unfinished thread: COMPATIBILITY.md "Vtable histogram live
      result" - a singleton (1-member) vtable class was found and never
      inspected; `diag/mgrwalk3` was built to watch it across a Kliff/Damiane
      switch but never live-tested.
- [ ] **Stat-commit, locomotion-stepper, pathing-helper, master-frame-update**:
      all currently on fallback pattern #1. Same RTTI/unwind-anchoring
      approach as above is the planned fix, not a one-off pattern patch.
- [ ] **Dye**: `apply-batch` signature NOT FOUND, feature fully disabled. No
      external repo lead currently covers this - needs its own signature hunt.
- [ ] **Destination-teleport landing bug**: player falls through terrain after
      warping until the game resets; no ground-probe exists yet. Lead:
      CD_Companion's map-marker-store hook as a third technique; Orcax's
      pointer-chain methodology (sourceCtx->actor->marker->target) is a
      transferable pattern for building a ground-height validation pass.
- [ ] **Friendly (NPC/pet trust setters)**: fallback #2 / #1 respectively. No
      external lead yet; needs its own re-derivation.
- [ ] **Localization lookup**: `kSig_LocStringGet` matches once, but its
      semantics are unverified. Low priority.

### Planned approach for the RTTI/vtable+unwind resolver

Build one reusable resolver module (anchor-by-type-name / vtable-identity /
exception-directory function lookup) instead of patching features one at a
time. Start with char-manager (biggest fallback share, 4/6 anchors dead),
then reuse the same infra for stat-commit/locomotion-stepper/pathing-helper/
master-frame-update. Does not remove the need to re-verify field offsets and
calling conventions per build - it only makes finding the right object/
function survive more recompiles. Not yet started.

## B. New features (not fixes - genuinely missing capability)

- [ ] **Auto-loot**: master-looter repo uses the game's own event queue to
      auto-collect nearby items. Trinity has nothing like this today.
- [ ] **EngineAdd-style real-pickup-path for Add Item**: call the game's own
      item-pickup code instead of our manual transaction replication (ctor +
      placement + commit + cleanup + id-allocation). Add Item already works
      and is live-confirmed (2026-10-10), so this is a simplification/
      robustness refactor, not a fix. Needs its own signature hunt for the
      real pickup function. Source: Nexus #3209 CrimsonInvEditor ASI.
- [ ] **Easy Parry window-widen alternative** (widen the real timing window
      instead of forcing the verdict byte): documented in COMPATIBILITY.md
      as a hypothesis (candidate global at RVA 0x6CB4F28), not implemented.
      Low priority - the current verdict-patch + held-block pulse approach is
      now live-confirmed at 100% eligible-window coverage (2026-10-10).
