# Ghidra function context export

`ExportFunctionContext.java` produces read-only evidence for virtual addresses
that were already found by the signature auditor. It reports the containing
function, direct callers and callees, instructions, and Ghidra decompilation.
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
