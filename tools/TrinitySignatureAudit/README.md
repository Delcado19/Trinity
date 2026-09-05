# Trinity Signature Audit

Read-only offline scanner for comparing Trinity's known TU 2.00.00 byte
patterns with a Crimson Desert executable. It never writes to the executable
and never treats a unique byte match as semantic verification.

## Run

From the repository root:

```powershell
python tools/TrinitySignatureAudit/audit.py `
  "F:\Steam\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe" `
  --game-version 2.01.00 `
  --output-prefix reports/crimson-desert-2.01.00-signatures
```

The command prints a summary and writes JSON and Markdown reports. The JSON
records the executable SHA-256, file version, PE timestamp, image size,
sections scanned, recorded match RVAs, and separate count/semantic statuses.
Match counts are exact; to keep reports reviewable, at most 64 RVAs are stored
per signature and `locationsTruncated` records whether more were found.

The primary pass scans every executable section plus `.link`, where the
baseline documents `kSig_StatCommit`. This intentionally includes executable
`.debug$P`: TU 2.01.00 routes live thunks into that section. When the primary
pass finds nothing, the scanner mirrors Trinity's packed-image fallback by
checking other readable sections; only non-executable `.debug*` data is
excluded.

Policies in `signatures-2.00.00.json` mean:

- `unique`: exactly one raw match was expected by the active baseline path.
- `exact`: a documented raw count other than one is expected.
- `contextual`: raw matches require the source's additional semantic filter.
- `alternative`: one of multiple version-specific alternatives may exist.
- `observe`: historical definition with no active runtime consumer.

Regardless of count, `semanticStatus` remains `UNKNOWN` until the target
function, calling convention, structures, and runtime behavior are verified.

`candidates-2.01.00.json` contains newly rediscovered patterns that are not yet
safe to enable. Pass it with `--manifest` to recheck them against an executable.

## Test

```powershell
python -m unittest discover tools/TrinitySignatureAudit -v
```
