import re
import sys

# ====== Step 1: Parse the PDF text ======
with open(r'C:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\docs\registers_text.txt', 'r', encoding='utf-8', errors='replace') as f:
    text = f.read()

# Remove null bytes
text = text.replace('\x00', '')

print("=" * 80)
print("PARSING PDF REGISTER DEFINITIONS")
print("=" * 80)

# Extract register entries - pattern: REG S{seg}_{offset}
regs_from_pdf = {}  # key: (seg, offset) -> {"names": set}

# Match register lines: name + REG S{seg}_{offset}
reg_matches = re.finditer(r'([A-Z][A-Z0-9_ /()\-,\.\*\'"]+?)\s+REG\s+S(\d+)_([0-9A-F]+)', text)
for m in reg_matches:
    name = m.group(1).strip()
    seg = int(m.group(2))
    offset_str = m.group(3)
    offset = int(offset_str, 16)

    key = (seg, offset)
    if key not in regs_from_pdf:
        regs_from_pdf[key] = {"names": set()}
    regs_from_pdf[key]["names"].add(name)

# Also try to extract from table of contents-like lines
toc_matches = re.finditer(r'REG\s+S(\d+)_([0-9A-F]+)', text)
for m in toc_matches:
    seg = int(m.group(1))
    offset = int(m.group(2), 16)
    key = (seg, offset)
    if key not in regs_from_pdf:
        regs_from_pdf[key] = {"names": set()}

print(f"\nFound {len(regs_from_pdf)} register addresses in PDF")
for key in sorted(regs_from_pdf.keys()):
    info = regs_from_pdf[key]
    names = ', '.join(sorted(info['names'])[:3])
    print(f"  S{key[0]}_{key[1]:02X} ({key[0]}:0x{key[1]:02X}) -> {names}")

# ====== Step 2: Parse tv5725.h ======
with open(r'C:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\headers\tv5725.h', 'r', encoding='utf-8', errors='replace') as f:
    header = f.read()

# Extract all TV5725_REG definitions
header_regs = {}  # key: (seg, offset) -> {bit_offset: {bit_width, name}}
header_all_defs = {}  # name -> {seg, offset, bit_offset, bit_width}

# Find all REG definitions
reg_defs = re.finditer(
    r'#define\s+(\w+)\s+TV5725_REG\(0x([0-9A-F]+),\s*0x([0-9A-F]+),\s*(\d+),\s*(\d+)\)',
    header)
for m in reg_defs:
    name = m.group(1)
    seg = int(m.group(2), 16)
    offset = int(m.group(3), 16)
    bit_off = int(m.group(4))
    bit_w = int(m.group(5))

    key = (seg, offset)
    if key not in header_regs:
        header_regs[key] = {}
    header_regs[key][bit_off] = {"width": bit_w, "name": name}
    header_all_defs[name] = {"seg": seg, "offset": offset, "bit_off": bit_off,
                             "bit_w": bit_w}

print(f"\n{'=' * 80}")
print(f"PARSING tv5725.h REGISTER DEFINITIONS")
print(f"{'=' * 80}")
print(f"\nFound {len(header_all_defs)} register definitions in tv5725.h")

# ====== Step 3: Compare ======
print(f"\n{'=' * 80}")
print(f"COMPARISON: Registers in PDF but MISSING from tv5725.h")
print(f"{'=' * 80}")

missing_count = 0
for key in sorted(regs_from_pdf.keys()):
    if key not in header_regs:
        info = regs_from_pdf[key]
        names = ', '.join(sorted(info['names'])[:3])
        print(f"  S{key[0]}_{key[1]:02X} (seg={key[0]}, offset=0x{key[1]:02X}) - {names}")
        missing_count += 1

if missing_count == 0:
    print("  None! All register addresses from PDF are defined in tv5725.h")
else:
    print(f"  Total missing: {missing_count}")

print(f"\n{'=' * 80}")
print(f"COMPARISON: Registers in tv5725.h but NOT in PDF")
print(f"{'=' * 80}")

extra_count = 0
for key in sorted(header_regs.keys()):
    if key not in regs_from_pdf:
        names = [v['name'] for v in header_regs[key].values()]
        print(f"  S{key[0]}_{key[1]:02X} (seg={key[0]}, offset=0x{key[1]:02X}) - {', '.join(names[:3])}")
        extra_count += 1

if extra_count == 0:
    print("  None!")
else:
    print(f"  Total extra: {extra_count}")

print(f"\n{'=' * 80}")
print(f"DETAILED AUDIT BY SEGMENT")
print(f"{'=' * 80}")

# Group by segment
for seg in range(6):
    print(f"\n--- Segment {seg} ---")
    pdf_keys = {k for k in regs_from_pdf.keys() if k[0] == seg}
    header_keys = {k for k in header_regs.keys() if k[0] == seg}

    all_keys = sorted(pdf_keys | header_keys)
    for key in all_keys:
        in_pdf = key in pdf_keys
        in_hdr = key in header_regs

        if in_pdf and in_hdr:
            pdf_name = ', '.join(sorted(regs_from_pdf[key]['names'])[:2])
            hdr_names = [v['name'] for v in header_regs[key].values()]
            print(f"  OK  S{key[0]}_{key[1]:02X} PDF:{pdf_name} | HDR:{', '.join(hdr_names[:3])}")
        elif in_pdf and not in_hdr:
            pdf_name = ', '.join(sorted(regs_from_pdf[key]['names'])[:2])
            print(f"  MISS S{key[0]}_{key[1]:02X} PDF:{pdf_name} | HDR:--- MISSING ---")
        else:
            hdr_names = [v['name'] for v in header_regs[key].values()]
            print(f"  EXTRA S{key[0]}_{key[1]:02X} PDF:--- NOT IN PDF --- | HDR:{', '.join(hdr_names[:3])}")

print(f"\n{'=' * 80}")
print(f"DONE")
print(f"{'=' * 80}")
