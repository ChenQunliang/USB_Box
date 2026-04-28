"""
Merge GBS_REG.txt bitfield definitions into tv5725.h.
- Keep existing tv5725.h names for matching entries
- Add new GBS entries with TV5725_RO/RW_ prefix
- Sort by (segment, offset, bit_offset)
- Align TV5725_REG() calls vertically
"""
import re

GBS_REG  = r"c:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\docs\GBS_REG.txt"
TV_H_IN  = r"c:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\headers\tv5725.h"
TV_H_OUT = r"c:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\headers\tv5725.h"

# ── RO/RW ranges (from REG.txt) ─────────────────────────────────
RO_RANGES = [
    (0x00, 0x00, 0x2F, True),
    (0x00, 0x40, 0x5F, False),
    (0x00, 0x90, 0x9F, False),
    (0x01, 0x00, 0x83, False),
    (0x02, 0x00, 0x3C, False),
    (0x03, 0x00, 0x74, False),
    (0x03, 0x80, 0x8F, False),
    (0x04, 0x00, 0x5B, False),
    (0x05, 0x00, 0x69, False),
    (0x05, 0xD0, 0xD0, False),
]

def is_ro(seg, off):
    for s, lo, hi, ro in RO_RANGES:
        if seg == s and lo <= off <= hi:
            return ro
    return True  # default safe

# ── 1. Parse GBS_REG.txt ────────────────────────────────────────
gbs = {}  # (seg,off,bit_off,width) → name
gbs_all = []

with open(GBS_REG, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        m = re.match(
            r'typedef\s+UReg\s*<\s*(0x[0-9A-F]+)\s*,\s*(0x[0-9A-F]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*>\s*(\w+)\s*;',
            line, re.IGNORECASE
        )
        if m:
            seg     = int(m.group(1), 16)
            off     = int(m.group(2), 16)
            bit_off = int(m.group(3))
            width   = int(m.group(4))
            name    = m.group(5)
            key     = (seg, off, bit_off, width)
            if key not in gbs:
                gbs[key] = name
            gbs_all.append((name, seg, off, bit_off, width))

print(f"GBS_REG.txt: {len(gbs)} unique entries")

# ── 2. Parse tv5725.h existing REG entries ───────────────────────
tv = {}  # (seg,off,bit_off,width) → (name_suffix, prefix)

with open(TV_H_IN, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        m = re.match(
            r'#define\s+TV5725_(RO|RW)_(\w+)\s+TV5725_REG\(\s*(0x[0-9A-F]+)\s*,\s*(0x[0-9A-F]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)',
            line
        )
        if m:
            prefix  = m.group(1)
            suffix  = m.group(2)
            seg     = int(m.group(3), 16)
            off     = int(m.group(4), 16)
            bit_off = int(m.group(5))
            width   = int(m.group(6))
            key = (seg, off, bit_off, width)
            if key not in tv:
                tv[key] = (suffix, prefix)

print(f"tv5725.h existing: {len(tv)} entries")

# ── 3. Categorize ───────────────────────────────────────────────
same_keys = set(gbs.keys()) & set(tv.keys())
new_keys  = set(gbs.keys()) - set(tv.keys())

print(f"Same (keep existing): {len(same_keys)}")
print(f"New (to add):         {len(new_keys)}")

# ── 4. Build output entries list ─────────────────────────────────
# Each entry: (seg, off, bit_off, width, name_suffix, prefix, source)
# source: 'tv' = existing tv5725.h, 'gbs' = new from GBS

out = []

# Same entries: keep tv5725.h naming
for key in same_keys:
    seg, off, bit_off, width = key
    suffix, prefix = tv[key]
    out.append((seg, off, bit_off, width, suffix, prefix, 'tv'))

# New entries: use GBS naming with TV5725_RO/RW_ prefix
for key in new_keys:
    seg, off, bit_off, width = key
    name_suffix = gbs[key]
    p = "RO" if is_ro(seg, off) else "RW"
    out.append((seg, off, bit_off, width, name_suffix, p, 'gbs'))

# Sort by (segment, offset, bit_offset)
out.sort(key=lambda x: (x[0], x[1], x[2]))

print(f"Total output entries: {len(out)}")

# ── 5. Read template from tv5725.h (non-register lines) ─────────
with open(TV_H_IN, "r", encoding="utf-8") as f:
    all_lines = f.readlines()

# Find non-register prefix (everything before first TV5725_REG define)
# and suffix (everything after last TV5725_REG define)
reg_start = None
reg_end   = None
for i, line in enumerate(all_lines):
    if re.match(r'#define\s+TV5725_(RO|RW)_\w+\s+TV5725_REG\(', line):
        if reg_start is None:
            reg_start = i
        reg_end = i

prefix_lines = all_lines[:reg_start]
suffix_lines = all_lines[reg_end+1:] if reg_end is not None else []

# ── 6. Generate aligned register definitions ──────────────────────
# Compute max macro name length for alignment
macro_names = [f"TV5725_{p}_{s}" for (_, _, _, _, s, p, _) in out]
max_len = max(len(n) for n in macro_names) if macro_names else 0
align_to = max_len + 2  # pad to this column

reg_lines_out = []
prev_key = None  # for adding blank-line separators

for seg, off, bit_off, width, suffix, prefix, src in out:
    macro = f"TV5725_{prefix}_{suffix}"
    padded = macro.ljust(align_to)
    line = f"#define {padded} TV5725_REG(0x{seg:02X}, 0x{off:02X}, {bit_off}, {width})"
    reg_lines_out.append(line)

print(f"Generated {len(reg_lines_out)} register lines")

# ── 7. Add section comments ─────────────────────────────────────
# Group by segment for comments
seg_comments = {
    0x00: ("Segment 0x00: STATUS / IF / CHIP / GPIO / INT / VDS / MEM / DEINT /\n"
           "                 SYNC_PROC / TEST_BUS / TEST_FIFO / CRC / PLL / DAC /\n"
           "                 RESET / PAD / OSD"),
    0x01: ("Segment 0x01: INPUT_FORMATTER / HD_BYPS / MODE_DET"),
    0x02: ("Segment 0x02: DEINTERLACER"),
    0x03: ("Segment 0x03: VDS_PROC / PIP"),
    0x04: ("Segment 0x04: MEMORY / CAPTURE / PLAY_BACK / FIFO"),
    0x05: ("Segment 0x05: ADC / SYNC_PROC / DEC"),
}

# Insert section comments before each segment boundary
final_lines = []
prev_seg = None
for i, entry in enumerate(out):
    seg = entry[0]
    if seg != prev_seg:
        if prev_seg is not None:
            final_lines.append("")
        comment = seg_comments.get(seg, f"Segment 0x{seg:02X}")
        final_lines.append(f"/* ================================================================")
        for cl in comment.split("\n"):
            final_lines.append(f"   {cl}")
        final_lines.append(f"   ================================================================ */")
        final_lines.append("")
        prev_seg = seg
    final_lines.append(reg_lines_out[i])

# ── 8. Assemble output ──────────────────────────────────────────
output_lines = []
output_lines.append('/* ================================================================')
output_lines.append('   TV5725 register definitions')
output_lines.append('')
output_lines.append('   Convention:')
output_lines.append('     TV5725_RO_* = read-only   (segment 0x00,  0x00-0x2F)')
output_lines.append('     TV5725_RW_* = read-write  (all other registers)')
output_lines.append('')
output_lines.append('   Sorted by segment, offset, bit_offset.')
output_lines.append('   ================================================================ */')
output_lines.append('')

# Find where to insert in prefix_lines
# Insert after the TV5725_REG() macro definition and before the first section comment
insert_after = None
for i, line in enumerate(prefix_lines):
    if "#define TV5725_REG(seg, off, bit, w)" in line:
        insert_after = i + 1
        break

if insert_after is None:
    # fallback
    insert_after = len(prefix_lines)

new_prefix = prefix_lines[:insert_after]
new_prefix.append('\n')
new_prefix.extend([l + '\n' for l in output_lines])

rest = prefix_lines[insert_after:]

# Combine: header prefix + new register section + old suffix
with open(TV_H_OUT, "w", encoding="utf-8") as f:
    # Write prefix (includes, struct, etc.)
    for line in new_prefix:
        f.write(line)

    # Write register definitions
    for line in final_lines:
        f.write(line)
        f.write('\n')
    f.write('\n')

    # Write suffix (function prototypes, enums, etc.)
    for line in suffix_lines:
        f.write(line)

print(f"\nWritten to {TV_H_OUT}")
print(f"Prefix lines: {len(new_prefix)}")
print(f"Register lines: {len(final_lines)}")
print(f"Suffix lines: {len(suffix_lines)}")

# ── 9. Verify ───────────────────────────────────────────────────
# Check for any duplicate names that would cause compilation errors
all_macros = {}
for line in final_lines:
    m = re.match(r'#define\s+(TV5725_(RO|RW)_\w+)\s+', line)
    if m:
        name = m.group(1)
        if name in all_macros:
            print(f"  WARNING: duplicate macro {name}")
        all_macros[name] = True
print(f"Unique macros: {len(all_macros)}")
