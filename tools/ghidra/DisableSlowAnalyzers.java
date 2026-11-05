// Disables the auto-analyzers that dominate import time on CrimsonDesert.exe
// (332MB image) without being needed for scalar/operand or function-boundary
// searches: Scalar Operand References, x86 Constant Reference Analyzer,
// Reference and Stack together ate >11000s of a 19921s full import on
// build 3460 (see ghidra-project/import-3460.console.log). Disassembly and
// Function ID/Start Search still run and are what FindFunctionsByScalars
// actually needs.
//@category Trinity

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Program;
import ghidra.framework.options.Options;

public class DisableSlowAnalyzers extends GhidraScript {
    private static final String[] SLOW_ANALYZERS = {
        "Scalar Operand References",
        "x86 Constant Reference Analyzer",
        "Reference",
        "Stack",
    };

    @Override
    public void run() throws Exception {
        Options analysisOptions = currentProgram.getOptions(Program.ANALYSIS_PROPERTIES);
        for (String name : SLOW_ANALYZERS) {
            if (analysisOptions.contains(name)) {
                analysisOptions.setBoolean(name, false);
                println("Disabled analyzer: " + name);
            } else {
                println("Analyzer not found (name mismatch?): " + name);
            }
        }
    }
}
