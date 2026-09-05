# Ghidra function context export

`ExportFunctionContext.java` produces read-only evidence for virtual addresses
that were already found by the signature auditor. It reports the containing
function, call and non-call references, callees, instructions, and Ghidra decompilation.
It does not discover or validate addresses.

Run it against an existing analyzed Ghidra project:

```powershell
$ghidra = "$env:LOCALAPPDATA\Programs\Ghidra\ghidra_12.1.3_PUBLIC"
& "$ghidra\support\analyzeHeadless.bat" .\ghidra-project CrimsonDesert-2.01.00 `
  -process CrimsonDesert.exe -readOnly -noanalysis `
  -scriptPath .\tools\ghidra `
  -postScript ExportFunctionContext.java 0x141718500
```

Pass more virtual addresses after the script name to export them in the same
run. Keep project files and raw analysis logs under ignored `ghidra-project/`;
record only reviewed conclusions and their confidence in `COMPATIBILITY.md`.

`FindFunctionsByScalars.java` narrows semantic searches without inventing byte
patterns. Pass hexadecimal instruction operands; it reports only functions
containing every requested value. Each result still requires manual review.

`FindPlayerTagChecks.java` searches for instruction windows containing the
three operands from the old player-class expression (`+0x88`, tag byte `+1`,
mask `0xF7`) and prints their context. It deliberately does not infer data
flow; its results are candidates, not validated accessors.

`FindPossessorRoundTrips.java` looks for the documented `owner+0xA0` to
`possessor+0xD0` pointer round trip while preserving register data flow through
the comparison. It is a candidate search, not proof that either offset is still
valid in the target build. Optional hexadecimal arguments require those scalar
operands to occur in the same function.
