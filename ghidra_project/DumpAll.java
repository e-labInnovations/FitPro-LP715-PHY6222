// Decompile every function to <out>/decomp.c and list functions with entry/size.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class DumpAll extends GhidraScript {
    public void run() throws Exception {
        String out = getScriptArgs()[0];
        new File(out).mkdirs();
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter c = new PrintWriter(new FileWriter(out + "/decomp.c"));
        PrintWriter l = new PrintWriter(new FileWriter(out + "/functions.txt"));
        c.printf("// MD5 %s\n", currentProgram.getExecutableMD5());
        int n = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.getBody().getNumAddresses() <= 1) continue;   // ROM stubs
            l.printf("%s %6d %s\n", f.getEntryPoint(), f.getBody().getNumAddresses(), f.getName());
            DecompileResults r = di.decompileFunction(f, 60, monitor);
            c.printf("\n// ==== %s @ %s\n", f.getName(), f.getEntryPoint());
            c.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// FAILED " + r.getErrorMessage());
            n++;
        }
        c.close(); l.close();
        printf("decompiled %d functions\n", n);
    }
}
