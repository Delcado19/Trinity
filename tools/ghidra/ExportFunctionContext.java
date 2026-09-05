// Export function evidence for one or more already-audited virtual addresses.
//@category Trinity

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class ExportFunctionContext extends GhidraScript {
    private static final int MAX_INSTRUCTIONS = 500;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            throw new IllegalArgumentException("Pass at least one virtual address, for example 0x141718500");
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.toggleCCode(true);
        decompiler.setSimplificationStyle("decompile");
        if (!decompiler.openProgram(currentProgram)) {
            throw new IllegalStateException(decompiler.getLastMessage());
        }

        try {
            for (String arg : args) {
                exportAddress(arg, decompiler);
            }
        }
        finally {
            decompiler.dispose();
        }
    }

    private void exportAddress(String text, DecompInterface decompiler) throws Exception {
        long value = Long.parseUnsignedLong(text.replaceFirst("(?i)^0x", ""), 16);
        Address address = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value);
        FunctionManager functions = currentProgram.getFunctionManager();
        Function function = functions.getFunctionContaining(address);

        println("\n=== TARGET " + text + " ===");
        if (function == null) {
            println("NO_FUNCTION_CONTAINING_ADDRESS");
            return;
        }

        println("ENTRY=" + function.getEntryPoint());
        println("NAME=" + function.getName());
        println("BODY_ADDRESSES=" + function.getBody().getNumAddresses());
        println("CALLING_CONVENTION=" + function.getCallingConventionName());
        println("SIGNATURE=" + function.getSignature().getPrototypeString());

        println("\n-- DIRECT CALLERS --");
        ReferenceIterator references = currentProgram.getReferenceManager().getReferencesTo(function.getEntryPoint());
        int callerCount = 0;
        while (references.hasNext()) {
            Reference reference = references.next();
            if (!reference.getReferenceType().isCall()) {
                continue;
            }
            Function caller = functions.getFunctionContaining(reference.getFromAddress());
            println(reference.getFromAddress() + " " + (caller == null ? "<no function>" : caller.getName()));
            ++callerCount;
        }
        println("CALLER_COUNT=" + callerCount);

        println("\n-- DIRECT CALLEES --");
        for (Function callee : function.getCalledFunctions(monitor)) {
            println(callee.getEntryPoint() + " " + callee.getName());
        }

        println("\n-- INSTRUCTIONS --");
        InstructionIterator instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
        int instructionCount = 0;
        while (instructions.hasNext() && instructionCount < MAX_INSTRUCTIONS) {
            Instruction instruction = instructions.next();
            println(instruction.getAddress() + "  " + formatBytes(instruction.getBytes()) + "  " + instruction);
            ++instructionCount;
        }
        if (instructions.hasNext()) {
            println("TRUNCATED_AFTER=" + MAX_INSTRUCTIONS);
        }

        println("\n-- DECOMPILATION --");
        DecompileResults results = decompiler.decompileFunction(function, 120, monitor);
        if (results.decompileCompleted() && results.getDecompiledFunction() != null) {
            println(results.getDecompiledFunction().getC());
        }
        else {
            println("DECOMPILE_FAILED=" + results.getErrorMessage());
        }
    }

    private String formatBytes(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 3);
        for (byte value : bytes) {
            if (result.length() > 0) {
                result.append(' ');
            }
            result.append(String.format("%02X", value & 0xff));
        }
        return result.toString();
    }
}
