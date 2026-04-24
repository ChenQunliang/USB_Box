import sys, io, zipfile
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

with zipfile.ZipFile('DSLogic U3Pro32-la-260424-160128.dsl', 'r') as zf:
    scl_all = b''
    sda_all = b''
    for i in range(6):
        scl_all += zf.read(f'L-0/{i}')
        sda_all += zf.read(f'L-1/{i}')

block_size = 2097152
# Start looking from block 1, byte 928250
byte_start = block_size + 928250

def get_byte_bits(data, offset):
    """Get 8 bits from a byte, LSB first"""
    b = data[offset] if offset < len(data) else 0
    return [(b >> i) & 1 for i in range(8)]

# Dump raw bytes with SCL/SDA bits for a window
print("Raw byte dump around I2C activity:")
print(f"{'Offset':>10s} {'Byte':>10s} {'SCL[7:0]':>16s} {'SDA[7:0]':>16s} {'SCL_hex':>8s} {'SDA_hex':>8s}")
print("-" * 80)

for off in range(byte_start, byte_start + 120):
    scl_bits = get_byte_bits(scl_all, off)
    sda_bits = get_byte_bits(sda_all, off)
    # Show LSB on the LEFT (earliest sample first)
    scl_str = ''.join(str(b) for b in scl_bits)  # bit0..bit7
    sda_str = ''.join(str(b) for b in sda_bits)
    if scl_all[off] != 0xFF or sda_all[off] != 0xFF:
        print(f"{off:10d} {off-block_size:10d} {scl_str:>16s} {sda_str:>16s}  0x{scl_all[off]:02x}    0x{sda_all[off]:02x}")

# Now do focused edge detection and manual decode
print("\n\nEdge detection in first transaction:")
bits_to_scan = 5000
scl_bits = []
sda_bits = []
for i in range(bits_to_scan):
    bi = byte_start + i // 8
    bitp = i % 8
    scl_bits.append((scl_all[bi] >> bitp) & 1 if bi < len(scl_all) else 0)
    sda_bits.append((sda_all[bi] >> bitp) & 1 if bi < len(sda_all) else 0)

prev_scl, prev_sda = scl_bits[0], sda_bits[0]
print(f"Initial: SCL={prev_scl}, SDA={prev_sda}")

for i in range(1, bits_to_scan):
    sc, sd = scl_bits[i], sda_bits[i]

    scl_rise = prev_scl == 0 and sc == 1
    scl_fall = prev_scl == 1 and sc == 0
    sda_rise = prev_sda == 0 and sd == 1
    sda_fall = prev_sda == 1 and sd == 0

    if scl_rise or scl_fall or sda_rise or sda_fall:
        gs = byte_start * 8 + i
        events = []
        if scl_rise: events.append("SCL^")
        if scl_fall: events.append("SCLv")
        if sda_rise: events.append("SDA^")
        if sda_fall: events.append("SDAv")

        # Mark I2C protocol events
        extra = ""
        if sc == 1 and prev_sda == 1 and sd == 0:
            extra = " << START"
        if sc == 1 and prev_sda == 0 and sd == 1:
            extra = " << STOP"

        print(f"  sample={gs:10d}  SCL={sc} SDA={sd}  {', '.join(events)}{extra}")

    prev_scl, prev_sda = sc, sd

    # Limit output
    if i > 0 and gs > byte_start * 8 + 4000:
        break
