// Extracts God of War Ragnarok's smschema field table from GoWR.exe.
//
// Run inside Ghidra (Script Manager, or through the GhidraMCP bridge's
// run_ghidra_script) with GoWR.exe open and analysed. Writes a TSV that
// tools/gen_smschema_header.py turns into
// Source/core/formats/gowr/SmSchemaTable.h.
//
// The table is static data, not code: each record is 32 bytes and is
// recognised by its doubled name pointer.
//
//   +0x00  u64  namePtr
//   +0x08  u64  namePtr        same value -- this is the tell
//   +0x10  u16  fieldOffset
//   +0x12  u16  size
//   +0x14  u16  typeCode
//   +0x16  u16  ownerStructId
//   +0x1A  u16  fieldId
//
// Records live in .rdata around 0x142000000-0x143000000. The scan is a plain
// 8-byte stride over that range; anything whose "name" is not printable ASCII,
// or whose owner/size/offset are implausible, is dropped by the generator
// rather than here, so the TSV stays a faithful record of what was read.
//
//@category GoWR

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import java.io.*;

public class dump_smschema extends GhidraScript {

    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        AddressFactory af = currentProgram.getAddressFactory();

        File out = new File(System.getProperty("user.home"), "smschema_fields.tsv");
        PrintWriter pw = new PrintWriter(new BufferedWriter(new FileWriter(out)));
        pw.println("addr\tname\tfieldOff\tsize\ttype\towner\tunk18\tfieldId\tunk1c\tunk1e");

        long lo = 0x142000000L, hi = 0x143000000L;
        int recs = 0;

        for (long a = lo; a < hi; a += 8) {
            if (monitor.isCancelled()) break;
            try {
                Address addr = af.getAddress(Long.toHexString(a));
                long p1 = mem.getLong(addr);
                if (p1 == 0 || p1 < 0x140000000L || p1 > 0x143000000L) continue;
                if (mem.getLong(addr.add(8)) != p1) continue;

                Address sp = af.getAddress(Long.toHexString(p1));
                StringBuilder sb = new StringBuilder();
                boolean ok = true;
                for (int i = 0; i < 96; i++) {
                    byte b = mem.getByte(sp.add(i));
                    if (b == 0) break;
                    if (b < 32 || b > 126) { ok = false; break; }
                    sb.append((char) b);
                }
                if (!ok || sb.length() < 2) continue;

                pw.printf("%X\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d%n",
                        a, sb,
                        mem.getShort(addr.add(16)) & 0xFFFF,
                        mem.getShort(addr.add(18)) & 0xFFFF,
                        mem.getShort(addr.add(20)) & 0xFFFF,
                        mem.getShort(addr.add(22)) & 0xFFFF,
                        mem.getShort(addr.add(24)) & 0xFFFF,
                        mem.getShort(addr.add(26)) & 0xFFFF,
                        mem.getShort(addr.add(28)) & 0xFFFF,
                        mem.getShort(addr.add(30)) & 0xFFFF);
                recs++;
            } catch (Exception e) {
                // unmapped or misaligned -- expected while striding raw data
            }
        }
        pw.close();
        println("smschema records: " + recs);
        println("wrote: " + out.getAbsolutePath());
    }
}
