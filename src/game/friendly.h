#pragma once

#include "../mem/hooks.h"

namespace trinity::game
{
    // Trust Multiplier - scales the trust ("Friendly" / 친밀도) you GAIN when
    // gifting NPCs or feeding/taming animals and mounts.
    //
    // How it works, and why it survives game updates (located by byte
    // signature, never a baked address; see the Friendly section of offsets.h
    // and the trinity-friendly-system notes for the full RE):
    //  - "Friendly" is the engine's single trust value behind both mechanics -
    //    gifting an NPC and feeding a wild animal both raise it, and it is the
    //    gauge that fills to tame a mount. It is a 0..100 value; reaching 100
    //    completes the tame.
    //  - Every GAMEPLAY change to it (gift, feed, AI-driven) funnels through one
    //    apply-and-replicate function that the save-loader path deliberately
    //    bypasses. We hook that funnel and, for each change that is a GAIN,
    //    scale the increase over the last value we let through for that target
    //    (a true per-gift delta multiplier), re-clamped to the game's max of
    //    100. Losses and already-max targets pass through untouched, and load-
    //    from-save is never re-scaled. The value is still server-clamped to 100
    //    in-process, so a large multiplier simply reaches max / tames in fewer
    //    gifts rather than overflowing.
    //
    // Normally purely reactive: the hook does all the work. If BOTH setter
    // signatures are dead on the running build (e.g. 1.0.0.3479), Install()
    // falls back to a signature-free poll: a third-party trainer ("Crimson
    // Desert Enhanced Ultimate ASI" build 3209/10.8) still raises trust on
    // that build with no function signature at all - it locates the
    // relationship-record array by walking the player character's own
    // pointers and recognising the array purely by its field value ranges
    // (category/site/trust-value), then reads it every frame. Tick() does
    // the same and feeds the found records through the same ScaleRecord()
    // the hook path uses, so the scaling behaviour is identical either way.
    // Inert either way unless the toggle is on and the multiplier is above
    // 1.0x.
    class Friendly
    {
    public:
        // Installs the friendly-apply hook, or - if neither setter resolves -
        // arms the fallback poll scan (see Tick()). Requires MH_Initialize()
        // first. Non-fatal either way.
        static bool Install();
        static void Remove();

        // Call once per game-thread frame (teleport.cpp's movement-tick
        // driver, alongside World::Tick() etc.). No-op unless Install()
        // armed the fallback scan.
        static void Tick();

        // True once the funnel hook is installed OR the fallback scan has
        // found the relationship array (the feature is available).
        static bool Ready();

        // Health of the backing signatures, for the menu's status dot - see
        // mem::SigStatus.
        static mem::SigStatus Status();
    };
}
