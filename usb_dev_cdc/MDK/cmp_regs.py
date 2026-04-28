"""
Compare GBS_REG.txt UReg entries with tv5725.h TV5725_REG defines.
Categorize as "same" (matching params) or "new" (different params).
"""
import re
from collections import defaultdict

GBS_REG  = r"c:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\docs\GBS_REG.txt"
TV_H     = r"c:\Users\Admin\Desktop\All_projects\Un8266_GBSC\USB_Box\usb_dev_cdc\headers\tv5725.h"

# ── RO/RV ranges from REG.txt ─────────────────────────────────────
# (segment, offset_start, offset_end) → True=RO, False=RW
RO_RANGES = [
    (0x00, 0x00, 0x2F, True),   # S0_00-2F: status (RO)
    (0x00, 0x40, 0x5F, False),  # S0_40-5F: control (RW)
    (0x00, 0x90, 0x98, False),  # S0_90-98: OSD (RW)
    (0x01, 0x00, 0x83, False),  # S1_00-83: IF/HD_BYPS/MODE_DET (RW)
    (0x02, 0x00, 0x3C, False),  # S2_00-3C: DEINT (RW)
    (0x03, 0x00, 0x74, False),  # S3_00-74: VDS (RW)
    (0x03, 0x80, 0x8F, False),  # S3_80-8F: PIP (RW)
    (0x04, 0x00, 0x5B, False),  # S4_00-5B: MEM/CAP/PB/FIFO (RW)
    (0x05, 0x00, 0x69, False),  # S5_00-69: ADC/SP (RW)
]

def is_ro(seg, off):
    for s, lo, hi, ro in RO_RANGES:
        if seg == s and lo <= off <= hi:
            return ro
    return False  # default RW

# ── 1. Parse GBS_REG.txt ─────────────────────────────────────────
gbs_entries = []  # (name, seg, off, bit_off, width)

with open(GBS_REG, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        # Pattern: typedef UReg<seg, off, bit_off, width> NAME;
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
            gbs_entries.append((name, seg, off, bit_off, width))

print(f"GBS_REG.txt: {len(gbs_entries)} entries")

# ── 2. Parse tv5725.h ────────────────────────────────────────────
tv_entries = []  # (name_suffix, seg, off, bit_off, width, prefix)

with open(TV_H, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        # Pattern: #define TV5725_RO_NAME TV5725_REG(seg, off, bit_off, width)
        # or:      #define TV5725_RW_NAME TV5725_REG(seg, off, bit_off, width)
        m = re.match(
            r'#define\s+TV5725_(RO|RW)_(\w+)\s+TV5725_REG\(\s*(0x[0-9A-F]+)\s*,\s*(0x[0-9A-F]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)',
            line
        )
        if m:
            prefix  = m.group(1)  # RO or RW
            name    = m.group(2)
            seg     = int(m.group(3), 16)
            off     = int(m.group(4), 16)
            bit_off = int(m.group(5))
            width   = int(m.group(6))
            tv_entries.append((name, seg, off, bit_off, width, prefix))

print(f"tv5725.h:    {len(tv_entries)} REG entries")

# ── 3. Build lookup by (seg, off, bit_off, width) ────────────────
tv_by_params = {}
for name, seg, off, bit_off, width, prefix in tv_entries:
    key = (seg, off, bit_off, width)
    tv_by_params[key] = (name, prefix)

# ── 4. Categorize GBS entries ─────────────────────────────────────
same_entries = []   # exact match found in tv5725.h
new_entries  = []   # no exact match → needs adding

for name, seg, off, bit_off, width in gbs_entries:
    key = (seg, off, bit_off, width)
    if key in tv_by_params:
        tv_name, prefix = tv_by_params[key]
        same_entries.append((name, seg, off, bit_off, width, prefix, tv_name))
    else:
        ro = is_ro(seg, off)
        prefix = "RO" if ro else "RW"
        new_entries.append((name, seg, off, bit_off, width, prefix))

print(f"\n--- Comparison ---")
print(f"Same (match tv5725.h): {len(same_entries)}")
print(f"New  (only in GBS):    {len(new_entries)}")

# ── 5. Show "same" entries (ask user) ─────────────────────────────
if same_entries:
    print(f"\n{'='*70}")
    print(f"※ 以下 {len(same_entries)} 个寄存器与 tv5725.h 的 tv5725_reg_t 完全相同")
    print(f"   请确认是否要用 GBS_REG.txt 的命名替换 tv5725.h 中的定义")
    print(f"{'='*70}")
    same_entries.sort(key=lambda x: (x[1], x[2], x[3]))
    print(f"{'GBS名':<40} {'TV5725名':<40} {'seg':>4} {'off':>4} {'bit':>4} {'wid':>4}")
    print("-"*100)
    for name, seg, off, bit_off, width, prefix, tv_name in same_entries:
        tv_full = f"TV5725_{prefix}_{tv_name}"
        print(f"{name:<40} {tv_full:<40}  0x{seg:02X}  0x{off:02X}  {bit_off:>3}  {width:>3}")

# ── 6. Show "new" entries (will be added) ──────────────────────────
if new_entries:
    print(f"\n{'='*70}")
    print(f"※ 以下 {len(new_entries)} 个寄存器在 GBS_REG.txt 中有定义")
    print(f"   但 tv5725.h 中没有完全相同的 tv5725_reg_t，将被添加")
    print(f"{'='*70}")
    new_entries.sort(key=lambda x: (x[1], x[2], x[3]))
    print(f"{'GBS名':<40} {'seg':>4} {'off':>4} {'bit':>4} {'wid':>4} {'RO/RW':>6}")
    print("-"*70)
    for name, seg, off, bit_off, width, prefix in new_entries:
        print(f"{name:<40}  0x{seg:02X}  0x{off:02X}  {bit_off:>3}  {width:>3}  {prefix}")

# ── 7. Brief summary ──────────────────────────────────────────────
print(f"\n{'='*70}")
print(f"总计: GBS {len(gbs_entries)} 项 → 相同 {len(same_entries)} / 新增 {len(new_entries)}")
