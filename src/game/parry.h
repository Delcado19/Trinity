#pragma once

namespace trinity::game
{
    // Easy Parry.
    //
    // The engine decides a parry in one place. Having read it:
    //
    //     vsubss  xmm0, xmm2, xmm0      ; duration  = t1 - t0
    //     vmulss  xmm1, xmm0, [k]       ; margin    = duration * k
    //     vsubss  xmm3, xmm2, xmm1      ; threshold = t1 - margin
    //     vcomiss xmm2, xmm3
    //     seta    al                    ; success   = windowEnd > threshold
    //     mov     byte ptr [rsi], al    ; the parry result
    //
    // Everything before that point is the overlap test - whether the incoming
    // attack and your guard occupy the same moment at all. Only the final
    // comparison is the timing MARGIN, and that is the part a player misses by
    // a few frames. Replacing `seta al` with `mov al, 1` drops the margin test
    // and keeps the overlap requirement, so a parry still has to be attempted
    // against a real attack - it just no longer has to be frame-perfect.
    //
    // Prefer the evaluator detour, which also requests fresh input edges for
    // held block. The three-byte verdict patch is a compatibility fallback.
    // Evaluator diagnostics include original timing verdicts in both modes;
    // neither an eligible window nor a perfect verdict confirms a counter hit.
    class Parry
    {
    public:
        static bool Install();   // install evaluator hook or locate fallback patch site
        static void Remove();    // restore the original bytes if patched

        static bool Available(); // was the site found?
        static bool Enabled();
        static void SetEnabled(bool on);
    };
}
