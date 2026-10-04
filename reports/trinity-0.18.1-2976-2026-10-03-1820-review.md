# Runtime diagnostics review: 2026-10-03 18:20

Snapshot: `trinity-0.18.1-2976-2026-10-03-1820.log.txt`.
Easy Parry switched on at 17:11:56. Final off-mode totals are 1838 calls,
189 eligible evaluations, 14 original-perfect evaluations and 145 unreadable
outputs. Final on-mode totals at 18:20:14 are 6661 calls, 870 eligible,
84 original-perfect, 96 successfully forced false-to-true verdicts,
690 unreadable outputs, 690 failed writes and 61 pulse requests.
Thus 690/870 (79.3%) eligible on-mode evaluations could not read or write
the verdict output. Readable eligible outputs total 180, exactly 84 original
perfect plus 96 forced. This establishes some successful verdict writes,
not confirmed counterattacks or effectiveness while holding block.
Output-pointer failures require investigation: possible ABI mismatch or
legitimate non-output call paths; the log cannot distinguish these causes.
The hook is not limited to player-only or unique attack evaluations.
Mode durations and combat exposure differ, so totals cannot establish an
on/off counter rate comparison.

The sole logged warp at 17:05:35 places the player 30 units above the requested
height. No subsequent ground-probe or fall trajectory records are present.
User-reported fall-through remains unresolved. Later no-marker messages at
17:11:52 and 17:23:36 do not negate the successful earlier placement.
No inventory or time-shift actions are logged. Existing dye and legacy
Game Speed initialization errors remain.
