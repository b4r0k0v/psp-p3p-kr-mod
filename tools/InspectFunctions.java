// Decompile functions at the hexadecimal entry addresses passed to the script.
//@category P3P

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.script.GhidraScript;

public class InspectFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] entries = getScriptArgs();
        if (entries.length == 0) {
            throw new IllegalArgumentException("Pass one or more hexadecimal function entry addresses");
        }

        var decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(decompiler.getLastMessage());
            }
            for (String entry : entries) {
                monitor.checkCancelled();
                var address = toAddr(Long.parseUnsignedLong(entry.replaceFirst("^0[xX]", ""), 16));
                var function = getFunctionAt(address);
                if (function == null) {
                    printerr("No function starts at " + address);
                    continue;
                }

                var result = decompiler.decompileFunction(function, 25, monitor);
                if (result.decompileCompleted()) {
                    println(address + "\n" + result.getDecompiledFunction().getC());
                } else {
                    printerr(address + ": " + result.getErrorMessage());
                }
            }
        } finally {
            decompiler.dispose();
        }
    }
}
