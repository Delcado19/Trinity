// Scratch probe: report the memory block, byte, and defined data at a VA.
//@category Trinity

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.listing.Data;

public class ProbeAddress extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (String arg : getScriptArgs()) {
            long value = Long.parseUnsignedLong(arg.replaceFirst("(?i)^0x", ""), 16);
            Address addr = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value);
            MemoryBlock block = currentProgram.getMemory().getBlock(addr);
            Data data = currentProgram.getListing().getDataAt(addr);
            println("ADDR=" + arg +
                " BLOCK=" + (block == null ? "<none>" : block.getName() + " " + block.getStart() + "-" + block.getEnd()) +
                " DATA=" + (data == null ? "<undefined>" : data.toString()));
        }
    }
}
