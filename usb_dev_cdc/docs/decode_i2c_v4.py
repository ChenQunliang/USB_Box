import sys, io, zipfile
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

with zipfile.ZipFile('DSLogic U3Pro32-la-260424-160128.dsl', 'r') as zf:
    scl_all = b''
    sda_all = b''
    for i in range(6):
        scl_all += zf.read(f'L-0/{i}')
        sda_all += zf.read(f'L-1/{i}')

block_size = 2097152
byte_start = block_size + 928250
bits_to_scan = 10000

# Extract bitstream
scl = []
sda = []
for i in range(bits_to_scan):
    bi = byte_start + i // 8
    bitp = i % 8
    scl.append((scl_all[bi] >> bitp) & 1 if bi < len(scl_all) else 0)
    sda.append((sda_all[bi] >> bitp) & 1 if bi < len(sda_all) else 0)

# Detailed manual trace of the second transaction
prev_scl = scl[0]
prev_sda = sda[0]
state = 'IDLE'
byte_val = 0
bit_idx = 0
txn_num = 0
data_mode = False
addr_byte = 0
data_bytes = []
starts_found = []

for i in range(1, bits_to_scan):
    sc = scl[i]
    sd = sda[i]
    gs = byte_start * 8 + i

    scl_rise = (prev_scl == 0 and sc == 1)
    sda_fall = (prev_sda == 1 and sd == 0)
    sda_rise = (prev_sda == 0 and sd == 1)

    # Track START conditions
    if state == 'IDLE':
        if sc == 1 and sda_fall:
            txn_num += 1
            state = 'ADDR'
            bit_idx = 0
            byte_val = 0
            data_mode = False
            data_bytes = []
            print(f"\n{'='*60}")
            print(f"TXN {txn_num}: START at sample {gs} ({gs/20e6*1000:.3f}ms)")
            prev_scl, prev_sda = sc, sd
            continue

    # Track STOP conditions
    if state in ('ADDR', 'ACK_ADDR', 'DATA', 'ACK_DATA'):
        if sc == 1 and sda_rise:
            print(f"  STOP at sample {gs} ({gs/20e6*1000:.3f}ms)")
            state = 'IDLE'

    # Sample on SCL rising edge
    if state in ('ADDR', 'DATA'):
        if scl_rise:
            byte_val = (byte_val << 1) | sd
            bit_idx += 1
            if bit_idx == 8:
                if state == 'ADDR':
                    addr = byte_val >> 1
                    rw = 'R' if (byte_val & 1) else 'W'
                    addr8 = byte_val
                    print(f"  ADDR: 7b=0x{addr:02X} 8b=0x{addr8:02X} {rw}")
                    state = 'ACK_ADDR'
                else:
                    print(f"  DATA[{len(data_bytes)}]: 0x{byte_val:02X}")
                    data_bytes.append(byte_val)
                    state = 'ACK_DATA'
                bit_idx = 0

    # Sample ACK on next SCL rising edge
    elif state in ('ACK_ADDR', 'ACK_DATA'):
        if scl_rise:
            ack = (sd == 0)
            ack_str = 'ACK' if ack else 'NACK'
            label = 'ADDR' if state == 'ACK_ADDR' else 'DATA'
            print(f"  ACK({label}): {ack_str}")
            state = 'DATA'
            bit_idx = 0
            byte_val = 0

    prev_scl, prev_sda = sc, sd
