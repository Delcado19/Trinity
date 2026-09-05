// Find functions containing every requested instruction scalar operand.
//@category Trinity

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

public class FindFunctionsByScalars extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            throw new IllegalArgumentException("Pass hexadecimal scalar values, for example B8 C0 88 A0 D0");
        }

        Set<Long> wanted = new LinkedHashSet<>();
        for (String arg : args) {
            wanted.add(Long.parseUnsignedLong(arg.replaceFirst("(?i)^0x", ""), 16));
        }

        Map<Function, Map<Long, List<String>>> found = new LinkedHashMap<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        while (instructions.hasNext()) {
            monitor.checkCancelled();
            Instruction instruction = instructions.next();
            for (int operand = 0; operand < instruction.getNumOperands(); ++operand) {
                for (Object object : instruction.getOpObjects(operand)) {
                    if (!(object instanceof Scalar)) {
                        continue;
                    }
                    long value = ((Scalar) object).getUnsignedValue();
                    if (!wanted.contains(value)) {
                        continue;
                    }
                    Function function = getFunctionContaining(instruction.getAddress());
                    if (function == null) {
                        continue;
                    }
                    found.computeIfAbsent(function, unused -> new LinkedHashMap<>())
                        .computeIfAbsent(value, unused -> new ArrayList<>())
                        .add(instruction.getAddress() + " " + instruction);
                }
            }
        }

        List<Function> matches = new ArrayList<>();
        for (Map.Entry<Function, Map<Long, List<String>>> entry : found.entrySet()) {
            if (entry.getValue().keySet().containsAll(wanted)) {
                matches.add(entry.getKey());
            }
        }
        matches.sort(Comparator.comparing(Function::getEntryPoint));

        println("MATCH_COUNT=" + matches.size());
        for (Function function : matches) {
            println("\nFUNCTION=" + function.getEntryPoint() + " " + function.getName());
            Map<Long, List<String>> values = found.get(function);
            for (long value : wanted) {
                println(String.format("  0x%X: %s", value, values.get(value)));
            }
        }
    }
}
