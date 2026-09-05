// Find candidates for the documented owner -> possessor -> owner round trip.
// This matches instruction data flow only; every result still needs review.
//@category Trinity

import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

public class FindPossessorRoundTrips extends GhidraScript {
    private static final int SEARCH_AHEAD = 16;
    private static final int COMPARE_AHEAD = 8;
    private static final Pattern POINTER_LOAD = Pattern.compile(
        "^MOV\\s+([A-Z0-9]+),.*\\[([A-Z0-9]+) \\+ 0XA0\\]$");
    private static final Pattern PAWN_LOAD = Pattern.compile(
        "^MOV\\s+([A-Z0-9]+),.*\\[([A-Z0-9]+) \\+ 0XD0\\]$");

    @Override
    public void run() throws Exception {
        Set<Long> requiredScalars = new LinkedHashSet<>();
        for (String arg : getScriptArgs()) {
            requiredScalars.add(Long.parseUnsignedLong(arg.replaceFirst("(?i)^0x", ""), 16));
        }
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        Set<Address> reported = new HashSet<>();
        int matches = 0;
        while (instructions.hasNext()) {
            monitor.checkCancelled();
            Instruction start = instructions.next();
            Matcher pointerLoad = POINTER_LOAD.matcher(start.toString().toUpperCase());
            if (!pointerLoad.matches()) {
                continue;
            }

            Function function = getFunctionContaining(start.getAddress());
            if (function == null || reported.contains(function.getEntryPoint())) {
                continue;
            }
            String possessorRegister = pointerLoad.group(1);
            String ownerRegister = pointerLoad.group(2);
            // The simple matcher cannot prove a round trip if the load destroys
            // the only copy of the owner register.
            if (possessorRegister.equals(ownerRegister)) {
                continue;
            }
            Instruction pawnRead = start;
            String pawnRegister = null;
            Instruction comparison = null;

            for (int i = 0; i < SEARCH_AHEAD; ++i) {
                pawnRead = nextInFunction(pawnRead, function);
                if (pawnRead == null) {
                    break;
                }
                String text = pawnRead.toString().toUpperCase();
                Matcher pawnLoad = PAWN_LOAD.matcher(text);
                boolean directCompare = text.matches("^CMP\\s+(?:QWORD PTR )?\\[" + possessorRegister +
                    " \\+ 0XD0\\]," + ownerRegister + "$") ||
                    text.matches("^CMP\\s+" + ownerRegister + ",(?:QWORD PTR )?\\[" +
                        possessorRegister + " \\+ 0XD0\\]$");
                if (directCompare) {
                    comparison = pawnRead;
                    break;
                }
                if (!pawnLoad.matches() || !pawnLoad.group(2).equals(possessorRegister)) {
                    continue;
                }

                pawnRegister = pawnLoad.group(1);
                if (pawnRegister.equals(ownerRegister)) {
                    continue;
                }
                Instruction candidate = pawnRead;
                for (int j = 0; j < COMPARE_AHEAD; ++j) {
                    candidate = nextInFunction(candidate, function);
                    if (candidate == null) {
                        break;
                    }
                    String candidateText = candidate.toString().toUpperCase();
                    if (candidate.getMnemonicString().equalsIgnoreCase("CMP") &&
                        (candidateText.equals("CMP " + pawnRegister + "," + ownerRegister) ||
                         candidateText.equals("CMP " + ownerRegister + "," + pawnRegister))) {
                        comparison = candidate;
                        break;
                    }
                }
                if (comparison != null) {
                    break;
                }
            }

            if (comparison == null) {
                continue;
            }
            if (!containsAllScalars(function, requiredScalars)) {
                continue;
            }
            reported.add(function.getEntryPoint());
            ++matches;
            println("\nFUNCTION=" + function.getEntryPoint() + " " + function.getName());
            println("OWNER_TO_POSSESSOR=" + start.getAddress() + " " + start);
            println("POSSESSOR_TO_PAWN=" + pawnRead.getAddress() + " " + pawnRead);
            println("ROUND_TRIP_COMPARE=" + comparison.getAddress() + " " + comparison);
        }
        println("\nMATCH_COUNT=" + matches);
    }

    private Instruction nextInFunction(Instruction instruction, Function function) {
        Instruction next = currentProgram.getListing().getInstructionAfter(instruction.getAddress());
        return next != null && function.getBody().contains(next.getAddress()) ? next : null;
    }

    private boolean containsAllScalars(Function function, Set<Long> wanted) {
        Set<Long> found = new HashSet<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
        while (instructions.hasNext() && !found.containsAll(wanted)) {
            Instruction instruction = instructions.next();
            for (int operand = 0; operand < instruction.getNumOperands(); ++operand) {
                for (Object object : instruction.getOpObjects(operand)) {
                    if (object instanceof Scalar) {
                        long value = ((Scalar) object).getUnsignedValue();
                        if (wanted.contains(value)) {
                            found.add(value);
                        }
                    }
                }
            }
        }
        return found.containsAll(wanted);
    }
}
