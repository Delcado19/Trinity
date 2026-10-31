// Scratch probe: list every memory block's start/end so a VA can be checked by hand.
//@category Trinity

import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.MemoryBlock;

public class ListMemoryRanges extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            println("BLOCK=" + block.getName() + " START=" + block.getStart() + " END=" + block.getEnd()
                + " SIZE=0x" + Long.toHexString(block.getSize()));
        }
    }
}
