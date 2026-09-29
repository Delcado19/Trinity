#include "mod.h"
#include <MinHook.h>
#include "logger.h"
#include "settings.h"
#include "state.h"
#include "version.h"
#include "gameversion.h"
#include "../hooks/dx12_hook.h"
#include "../game/player.h"
#include "../game/diag_player_accessor.h"
#include "../game/diag_damage_probe.h"
#include "../game/teleport.h"
#include "../game/inventory.h"
#include "../game/world.h"
#include "../game/dye.h"
#include "../game/equipment.h"
#include "../game/friendly.h"
#include "../game/parry.h"
#include "../game/weather.h"

namespace trinity
{
    void Mod::Initialize(HMODULE module)
    {
        if (m_initialized)
            return;

        m_module = module;
        // Console is created lazily from the render path, so only the process
        // that actually presents the game gets one. Early logs buffer until then.
        LOG("Trinity v%s initializing (built %s %s).", TRINITY_VERSION, __DATE__, __TIME__);
        // Name the game build before anything scans for it, so a log that ends
        // in signature failures already says which build produced them.
        LogGameVersion();
        const bool verifiedGameBuild = CurrentGameVersion().isVerified();

        // Restore last session's feature settings (Trinity.ini) before the
        // feature hooks install, so restored toggles apply from frame one.
        Settings::Load();

        if (MH_Initialize() != MH_OK)
        {
            LOG("MinHook initialization failed.");
            return;
        }

        if (!hooks::InstallDX12Hooks())
        {
            LOG("Failed to install DX12 hooks.");
            MH_Uninitialize();
            return;
        }

        // TU 2.01.00 demonstrates why a surviving unique match is not enough:
        // DamageApply still occurs once while StatCommit and every character-
        // manager anchor are gone. Keep the overlay/logging path available for
        // diagnostics, but never install gameplay hooks on an unverified build.
        if (!verifiedGameBuild)
        {
            // Teleport (see COMPATIBILITY.md "BREAKTHROUGH 2026-09-08/09" -
            // teleport/world/dye/parry signatures re-derived, LocoStepper
            // follow-up). Installed FIRST in this branch specifically so its
            // hkMoveUpdate claims kSig_MoveUpdate before
            // PlayerAccessorProbe's own competing hook on the same address
            // gets a chance to (MinHook refuses a second hook on an
            // already-hooked target) - Teleport's hkMoveUpdate is a strict
            // superset of what the probe's substitute did (it also drives
            // Player::RefreshSelf(), plus g_playerMoveOwner, Super Jump, and
            // Game Speed), so this ordering costs nothing. Install()'s only
            // FATAL signature is kSig_MoveUpdate, which is unchanged and
            // confirmed working on this build - so this always succeeds.
            // What actually works: position tracking, coordinate warps,
            // Saved Locations, Super Jump, and now Super Run
            // (kSig_LocoStepper_TU20100_Candidate, offline-verified unique,
            // NOT yet live-tested through our own hook). Fails closed and
            // logs, not silently: Teleport to Destination
            // (now a hook-free nav-component read, not yet live-tested) and the fast-travel menu
            // (kSig_TravelToNode, scene-registry still dead) stay
            // grey/empty rather than doing something wrong.
            m_teleportInstalled = game::Teleport::Install();

            // Read-only TU 2.01.00 player-accessor probe (see COMPATIBILITY.md
            // "Character/player resolution" and diag_player_accessor.*). Not a
            // gameplay feature: it calls a statically-found candidate function
            // and logs what it returns, never hooks it, never writes anything.
            // Its own move-update-tick hook (a substitute driver for
            // Player::RefreshSelf(), see diag_player_accessor.cpp) now loses
            // the race to Teleport above and fails closed - logged, non-
            // fatal for the probe's other diagnostics (CharMgr accessor,
            // SelfChain), which already ran and logged by this point.
            m_accessorProbeInstalled = game::PlayerAccessorProbe::Install();

            // Read-only battle-damage probe (see diag_damage_probe.h): logs
            // the victim object DamageApply hands to player.cpp's own
            // hkDamageApply directly, sidestepping the character-manager
            // walk above entirely. Independent of it and of every
            // game::* feature below.
            m_damageProbeInstalled = game::DamageProbe::Install();

            // God Mode / Infinite Stamina / Infinite Spirit / damage
            // multipliers (see COMPATIBILITY.md "BREAKTHROUGH 2026-09-06" and
            // the live-verified WalkSelfChain result). Deliberately enabled
            // here, ahead of full version verification: every write it can
            // make is gated behind its own toggles (off by default in
            // State) and behind WalkSelfChain resolving the object first, so
            // there is no path to touching unrelated memory even if a
            // sub-signature turns out wrong - it just does nothing. Driven
            // by Teleport's hkMoveUpdate above, now that Teleport is
            // installed on this build.
            // Inventory/World/Dye/Equipment/Friendly are NOT enabled here -
            // each needs its own single-body identity or engine-call
            // verification this session did not establish.
            m_playerStatsInstalled = game::Player::Install();

            // Game Speed (see COMPATIBILITY.md "BREAKTHROUGH 2026-09-08" -
            // teleport/world/dye/parry signatures re-derived). Only the
            // master-frame-update hook (the toggle's actual write site) is
            // confirmed working on this build; World::Install()'s other
            // sub-features (the game-speed code patch, Freeze/Advance Time
            // of Day) depend on separate signatures not re-derived this
            // session and fail closed (confirmed offline: 0 matches each) -
            // World::Tick() is deliberately NOT driven here, so those stay
            // fully inert rather than resolve partially and do nothing
            // visible. Non-fatal either way, same as Player above.
            m_worldInstalled = game::World::Install();

            // Easy Parry (see COMPATIBILITY.md, same breakthrough) - fully
            // self-contained (one signature, one three-byte patch, no
            // per-tick driver needed), so this delivers the complete
            // feature, not a partial one.
            m_parryInstalled = game::Parry::Install();
            if (State::Get().easyParry)
                game::Parry::SetEnabled(true);

            // Inventory (see COMPATIBILITY.md, 2026-09-10 - found via a
            // user-supplied Cheat Engine table built against this exact
            // build, not the gugi97 live-log technique). Install()'s two
            // FATAL checks - kSig_InvGetItemQty and kSig_InvGetHolder - now
            // both resolve, so this always succeeds. What actually works:
            // the durable container walk (kSig_InvCoreGlobal fixed too), so
            // the item list populates on its own without waiting for the
            // HUD to query a count. What does NOT work yet: quantity edits
            // do not persist (kSig_InvCommit/kSig_InvHolderInsert, the
            // server-holder capture paths, are still dead - see their own
            // comments for why a client-only edit reverts on reconcile),
            // Add Item is refused (kSig_TrItemValueCtor/
            // kSig_InvCommitPlacement/kSig_InvFreePlacements all still
            // dead), and Slot Size does not apply
            // (kSig_InvSetExpandSlots still dead, falls back to its own
            // non-fatal call-only path which also fails). A browsable,
            // read-only inventory, not the full feature - each remaining
            // signature fails closed with its own log line.
            m_inventoryInstalled = game::Inventory::Install();

            // Dye/Equipment/Friendly are NOT enabled here. Dye specifically
            // was checked and is NOT ready despite this session's DyeUpsert
            // fix: Install() fails at the FIRST signature it checks
            // (kSig_DyeApplyBatch, confirmed offline: 0 matches on this
            // build) before ever reaching the fixed DyeUpsert fallback
            // chain - two more signatures (kSig_DyeApplyBatch,
            // kSig_EquipBatch) still need re-deriving first. The others
            // each need their own single-body identity or engine-call
            // verification this session did not establish.
            m_initialized = true;
            LOG_WARN("Diagnostics-only mode: gameplay features are unavailable for this game build.");
            LOG_OK("Overlay ready - INSERT (or LB + DOWN on controller) toggles the menu.");
            return;
        }

        // Weather is not installed. weather.cpp works - it captures every
        // preset the world loads and re-stamps them - but the sky is not drawn
        // from that data, so the feature has no visible effect and its menu rows
        // were removed. Installing the hook anyway would cost a scan, capture
        // 77 structs and write log lines for a feature nobody can use. The code
        // and its research stay; only the call goes.
        // Gameplay features. Non-fatal: if a signature ever fails to resolve
        // the overlay still runs, the feature is just disabled and logged.
        game::Player::Install();    // God Mode / Infinite Stamina
        game::Teleport::Install();  // Live position tracking / Fast Travel
        game::Inventory::Install(); // Item browser / quantity editor
        game::World::Install();     // Game Speed / Time of Day (Freeze, Advance)
        game::Dye::Install();       // Armor dye / material / repair look
        game::Equipment::Install(); // Abyss-gear socket editor
        game::Friendly::Install();  // Trust Multiplier (gift/feed/tame)
        game::Parry::Install();     // Easy Parry (locates the site; patches nothing yet)
        if (State::Get().easyParry)
            game::Parry::SetEnabled(true);
        if (State::Get().noBounty)
            game::Inventory::SetNoBounty(true);

        m_gameplayHooksInstalled = true;
        m_initialized = true;
        LOG_OK("Ready - INSERT (or LB + DOWN on controller) toggles the menu in-game.");
    }

    void Mod::Shutdown()
    {
        if (!m_initialized)
            return;

        // Menu changes already save as they happen; this catches anything
        // mutated outside the menu since the last write. In the launcher this
        // is inert - Save() only writes for the process that owns the file.
        if (State::Get().autoSave)
            Settings::Save();

        if (m_accessorProbeInstalled)
        {
            game::PlayerAccessorProbe::Remove();
            m_accessorProbeInstalled = false;
        }

        if (m_damageProbeInstalled)
        {
            game::DamageProbe::Remove();
            m_damageProbeInstalled = false;
        }

        if (m_playerStatsInstalled)
        {
            game::Player::Remove();
            m_playerStatsInstalled = false;
        }

        if (m_worldInstalled)
        {
            game::World::Remove();
            m_worldInstalled = false;
        }

        if (m_parryInstalled)
        {
            game::Parry::Remove();
            m_parryInstalled = false;
        }

        if (m_teleportInstalled)
        {
            game::Teleport::Remove();
            m_teleportInstalled = false;
        }

        if (m_inventoryInstalled)
        {
            game::Inventory::Remove();
            m_inventoryInstalled = false;
        }

        if (m_gameplayHooksInstalled)
        {
            game::Player::Remove();
            game::Teleport::Remove();
            game::Inventory::Remove();
            game::World::Remove();
            game::Dye::Remove();
            game::Equipment::Remove();
            game::Parry::Remove();      // restore the game's own bytes first
            game::Friendly::Remove();
            m_gameplayHooksInstalled = false;
        }
        hooks::RemoveDX12Hooks();
        MH_Uninitialize();
        Logger::Shutdown();
        m_initialized = false;
    }
}
