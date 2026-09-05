// Find instruction windows containing the documented player-tag operands.
// Operand proximity is only a candidate filter; it does not establish data flow.
//@category Trinity

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

public class FindPlayerTagChecks extends GhidraScript {
    private static final int LOOK_BEHIND = 12;
    private static final int LOOK_AHEAD = 6;

    @Override
    public void run() throws Exception {
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        int matches = 0;
        while (instructions.hasNext()) {
            monitor.checkCancelled();
            Instruction instruction = instructions.next();
            if (!instruction.getMnemonicString().equalsIgnoreCase("AND") || !hasScalar(instruction, 0xF7)) {
                continue;
            }

            Function function = getFunctionContaining(instruction.getAddress());
            if (function == null) {
                continue;
            }
            List<Instruction> before = previousInstructions(instruction, function);
            if (!containsScalar(before, 1) || !containsScalar(before, 0x88)) {
                continue;
            }

            ++matches;
            println("\nFUNCTION=" + function.getEntryPoint() + " " + function.getName());
            for (Instruction context : before) {
                println(context.getAddress() + " " + context);
            }
            println(instruction.getAddress() + " " + instruction + "  <MATCH>");

            Instruction next = instruction;
            for (int i = 0; i < LOOK_AHEAD; ++i) {
                next = currentProgram.getListing().getInstructionAfter(next.getAddress());
                if (next == null || !function.getBody().contains(next.getAddress())) {
                    break;
                }
                println(next.getAddress() + " " + next);
            }
        }
        println("\nMATCH_COUNT=" + matches);
    }

    private List<Instruction> previousInstructions(Instruction instruction, Function function) {
        List<Instruction> result = new ArrayList<>();
        Instruction previous = instruction;
        for (int i = 0; i < LOOK_BEHIND; ++i) {
            previous = currentProgram.getListing().getInstructionBefore(previous.getAddress());
            if (previous == null || !function.getBody().contains(previous.getAddress())) {
                break;
            }
            result.add(previous);
        }
        Collections.reverse(result);
        return result;
    }

    private boolean containsScalar(List<Instruction> instructions, long value) {
        for (Instruction instruction : instructions) {
            if (hasScalar(instruction, value)) {
                return true;
            }
        }
        return false;
    }

    private boolean hasScalar(Instruction instruction, long value) {
        for (int operand = 0; operand < instruction.getNumOperands(); ++operand) {
            for (Object object : instruction.getOpObjects(operand)) {
                if (object instanceof Scalar && ((Scalar) object).getUnsignedValue() == value) {
                    return true;
                }
            }
        }
        return false;
    }
}
