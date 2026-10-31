// Search loaded memory for a hex byte pattern (?? = wildcard byte) and
// decompile the containing function for each hit. Used to re-locate a
// kSig_* AOB directly inside an already-imported Ghidra project when the
// live exe on disk has moved on to a newer build.
//@category Trinity

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.mem.Memory;

public class FindBytePattern extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            throw new IllegalArgumentException("Pass a hex byte pattern, e.g. \"48 89 5C 24 10 ?? 56 57\"");
        }
        String[] tokens = String.join(" ", args).trim().split("\\s+");
        byte[] bytes = new byte[tokens.length];
        byte[] mask = new byte[tokens.length];
        for (int i = 0; i < tokens.length; i++) {
            if (tokens[i].equals("??") || tokens[i].equals("?")) {
                bytes[i] = 0;
                mask[i] = 0;
            } else {
                bytes[i] = (byte) Integer.parseInt(tokens[i], 16);
                mask[i] = (byte) 0xFF;
            }
        }

        Memory memory = currentProgram.getMemory();
        FunctionManager functions = currentProgram.getFunctionManager();
        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.toggleCCode(true);
        decompiler.setSimplificationStyle("decompile");
        if (!decompiler.openProgram(currentProgram)) {
            throw new IllegalStateException(decompiler.getLastMessage());
        }

        try {
            int hitCount = 0;
            Address start = currentProgram.getMinAddress();
            while (start != null) {
                Address hit = memory.findBytes(start, bytes, mask, true, monitor);
                if (hit == null) break;
                ++hitCount;
                println("\n=== HIT " + hitCount + " @ " + hit + " ===");
                Function function = functions.getFunctionContaining(hit);
                if (function == null) {
                    println("NO_FUNCTION_CONTAINING_ADDRESS");
                } else {
                    println("ENTRY=" + function.getEntryPoint());
                    println("NAME=" + function.getName());
                    DecompileResults results = decompiler.decompileFunction(function, 120, monitor);
                    if (results.decompileCompleted() && results.getDecompiledFunction() != null) {
                        println("-- DECOMPILATION --");
                        println(results.getDecompiledFunction().getC());
                    } else {
                        println("DECOMPILE_FAILED=" + results.getErrorMessage());
                    }
                }
                start = hit.add(1);
                if (hitCount >= 10) {
                    println("STOPPED_AFTER=10 hits");
                    break;
                }
            }
            if (hitCount == 0) {
                println("NO_MATCH");
            }
        } finally {
            decompiler.dispose();
        }
    }
}
