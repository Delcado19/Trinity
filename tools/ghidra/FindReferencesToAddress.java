// Find every reference (RIP-relative load, LEA, etc.) TO a given address, so
// a byte-level signature can be built for whichever instruction reaches it.
// Read-only: reports references only, does not infer data flow or validate
// that a hit is the "right" one - every result still needs manual review.
//@category Trinity

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class FindReferencesToAddress extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            throw new IllegalArgumentException("Pass a target virtual address, e.g. 0x146C29C68");
        }

        for (String arg : args) {
            long value = Long.parseUnsignedLong(arg.replaceFirst("(?i)^0x", ""), 16);
            Address target = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value);
            FunctionManager functions = currentProgram.getFunctionManager();

            println("\n=== TARGET " + arg + " ===");
            ReferenceIterator references = currentProgram.getReferenceManager().getReferencesTo(target);
            int count = 0;
            while (references.hasNext()) {
                monitor.checkCancelled();
                Reference reference = references.next();
                Address from = reference.getFromAddress();
                Function function = functions.getFunctionContaining(from);
                Instruction instruction = currentProgram.getListing().getInstructionAt(from);
                println("REF_FROM=" + from +
                    " TYPE=" + reference.getReferenceType() +
                    " FUNCTION=" + (function == null ? "<none>" : (function.getEntryPoint() + " " + function.getName())) +
                    " INSN=" + (instruction == null ? "<none>" : instruction.toString()));
                ++count;
            }
            println("MATCH_COUNT=" + count);
        }
    }
}
