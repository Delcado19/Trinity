# Lian vTweak 1.4.2 evening log review (2026-10-02)

- Source: `F:\Steam\steamapps\common\Crimson Desert\bin64\Trinity.log`
- Snapshot: `lian-vtweak-1.4.2-2976-2026-10-02-evening.log.txt`
- Bytes: 28387; lines: 228; distinct lines: 161.
- SHA256: `264478c3a05d9184628af52ca5d4ca641c5cc0e0212dd553245ded2180d3941c`
- Last line: `19:43:14.329 [TID 31852] [INFO] player: client stat source char=0 mount=0 owner=000004AEC895A500 root=000004AEC8952F80 array=000004AE8545CC00 count=20`
- Severity counts (raw, including duplicates): {'INFO': 132, 'OK': 96}.

## Findings

There are two distinct initialization sequences, at 06:58:25.919 and
12:49:05.794, each duplicated verbatim in the file. Four initialization
messages do not imply four independent launches. Both identify Lian vTweak
1.4.2, built September 20, and PE 1.0.0.2976. No 0.18.1 run is present.

The later sequence records character-manager consensus 9/9, installed stat,
damage and parry/evade hooks, marker teleport initialization (5/5 hooks),
inventory constructor 0x142409970, table discovery and frame timer 0x140AD1300.
At 12:51:28 the catalog again contains 6815 named items in 51 groups, from
6816 rows. These establish discovery/initialization, not successful user actions.

Repeated client-stat observations continue through 19:43:14.329. The same
character/mount owners are repeatedly observed from 12:50 through 19:28;
at 19:31 their owner/root/array addresses change. This proves re-resolution
of changed objects, but does not establish whether the cause was a reload,
respawn, or another world transition. Every recorded stat array count is 20.

There are no WARN/ERR/ERROR entries or explicit missing-signature/exception
messages. The disabled DyeApplySlot and equipment refresh signature are
intentional guards, not new failures. No Bounty logs 'reverted' with 0/35 rows;
this is not proof that enabling No Bounty was exercised.

No new quantity-edit, add-item, slot-expansion, time-step, or individual
parry-result records are present. The only direct-silver action is still the
morning 07:18:27 request for 1000000 silver with status=1. As noted in the
previous source review, this status does not establish persistent wallet writes.
The longer evening log therefore does not validate the outstanding inventory,
time, or held-block Easy Parry behavior, nor the local 0.18.1 compatibility build.
