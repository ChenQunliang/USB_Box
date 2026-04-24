"""Full I2C decoder for DSLogic capture - find all TV5725 transactions."""
import sys, io, zipfile
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

with zipfile.ZipFile('DSLogic U3Pro32-la-260424-160128.dsl', 'r') as zf:
    scl_all = b''
    sda_all = b''
    for i in range(6):
        scl_all += zf.read(f'L-0/{i}')
        sda_all += zf.read(f'L-1/{i}')

total_bytes = len(scl_all)
total_samples = total_bytes * 8
print(f"Total samples: {total_samples} ({total_samples/20e6:.3f}s)")

# Decode ALL I2C transactions using a robust state machine
# We process the entire capture byte by byte for efficiency

def get_bit(data, sample):
    bi = sample // 8
    bitp = sample % 8
    return (data[bi] >> bitp) & 1 if bi < len(data) else 0

def decode_range(byte_start, num_samples):
    """Decode I2C in a range of samples. Returns list of transactions."""
    prev_scl = get_bit(scl_all, byte_start * 8)
    prev_sda = get_bit(sda_all, byte_start * 8)

    state = 'IDLE'  # IDLE, ADDR, ACK, DATA
    bit_count = 0
    byte_val = 0
    transactions = []
    current = None

    for i in range(1, num_samples):
        gs = byte_start * 8 + i
        if gs >= total_samples:
            break

        sc = get_bit(scl_all, gs)
        sd = get_bit(sda_all, gs)

        scl_rise = (prev_scl == 0 and sc == 1)
        sda_fall = (prev_sda == 1 and sd == 0)
        sda_rise = (prev_sda == 0 and sd == 1)

        if state == 'IDLE':
            if sc == 1 and sda_fall:
                state = 'ADDR'
                bit_count = 0
                byte_val = 0
                current = {
                    'start': gs,
                    'addr': 0, 'rw': '', 'data': [], 'acks': [], 'stop': None
                }
        elif state == 'ADDR':
            if scl_rise:
                byte_val = (byte_val << 1) | sd
                bit_count += 1
                if bit_count == 8:
                    current['addr'] = byte_val >> 1
                    current['rw'] = 'R' if (byte_val & 1) else 'W'
                    state = 'ACK'
                    bit_count = 0
            if sc == 1 and sda_rise and bit_count == 0:
                # STOP right after START (empty transaction)
                current['stop'] = gs
                transactions.append(current)
                current = None
                state = 'IDLE'
        elif state == 'ACK':
            if scl_rise:
                current['acks'].append(sd == 0)
                if sd == 0:
                    state = 'DATA'
                else:
                    # NACK - expect STOP next
                    state = 'WAIT_STOP'
                bit_count = 0
                byte_val = 0
            if sc == 1 and sda_rise:
                current['stop'] = gs
                transactions.append(current)
                current = None
                state = 'IDLE'
        elif state == 'DATA':
            if scl_rise:
                byte_val = (byte_val << 1) | sd
                bit_count += 1
                if bit_count == 8:
                    current['data'].append(byte_val)
                    state = 'ACK'
                    bit_count = 0
            if sc == 1 and sda_rise:
                current['stop'] = gs
                transactions.append(current)
                current = None
                state = 'IDLE'
        elif state == 'WAIT_STOP':
            if sc == 1 and sda_rise:
                current['stop'] = gs
                transactions.append(current)
                current = None
                state = 'IDLE'
            # Could also be repeated START
            if sc == 1 and sda_fall:
                # Repeated START
                current['data'].append('RS')  # marker
                state = 'ADDR'
                bit_count = 0
                byte_val = 0

        prev_scl, prev_sda = sc, sd

    if current and len(current['data']) > 0:
        transactions.append(current)

    return transactions

# Decode the first I2C region (block 1)
print("\n" + "="*60)
print("REGION 1: Block 1 I2C activity")
print("="*60)
byte_start1 = 2097152 + 928000  # Start a bit before the first non-trivial data
txns1 = decode_range(byte_start1, 300000)
count = 0
for t in txns1:
    if t['addr'] == 0x17 or t['addr'] == 0x78:  # TV5725 or OLED
        duration_us = (t['stop'] - t['start']) / 20.0 if t['stop'] else 0
        addr8 = (t['addr'] << 1) | (1 if t['rw'] == 'R' else 0)
        chip = 'TV5725' if t['addr'] == 0x17 else 'OLED'
        print(f"\n[{chip}] START={t['start']/20e6*1000:.3f}ms  Addr=0x{addr8:02X} {t['rw']}  dur={duration_us:.1f}us")
        for j, b in enumerate(t['data']):
            if b == 'RS':
                print(f"  --- Repeated START ---")
            else:
                ack = t['acks'][j+1] if j+1 < len(t['acks']) else '?'
                ack_str = 'ACK' if ack else ('NACK' if ack is False else '?')
                print(f"  [{j}]: 0x{b:02X} ({ack_str})")

# Decode the second I2C region (block 5)
print("\n" + "="*60)
print("REGION 2: Block 5 I2C activity")
print("="*60)
byte_start2 = 5 * 2097152 + 66200
txns2 = decode_range(byte_start2, 100000)
for t in txns2:
    if t['addr'] == 0x17 or t['addr'] == 0x78:
        duration_us = (t['stop'] - t['start']) / 20.0 if t['stop'] else 0
        addr8 = (t['addr'] << 1) | (1 if t['rw'] == 'R' else 0)
        chip = 'TV5725' if t['addr'] == 0x17 else 'OLED'
        print(f"\n[{chip}] START={t['start']/20e6*1000:.3f}ms  Addr=0x{addr8:02X} {t['rw']}  dur={duration_us:.1f}us")
        for j, b in enumerate(t['data']):
            if b == 'RS':
                print(f"  --- Repeated START ---")
            else:
                ack = t['acks'][j+1] if j+1 < len(t['acks']) else '?'
                ack_str = 'ACK' if ack else ('NACK' if ack is False else '?')
                print(f"  [{j}]: 0x{b:02X} ({ack_str})")

# Also quick scan: find ALL TV5725 transactions anywhere in the capture
print("\n" + "="*60)
print("FULL SCAN: All TV5725 (0x17) transactions")
print("="*60)
# Scan in chunks
chunk_size = 2000000  # 2M samples per chunk
for chunk_start in range(0, total_samples, chunk_size):
    chunk_end = min(chunk_start + chunk_size + 50000, total_samples)  # overlap
    byte_start = max(0, chunk_start // 8 - 100)
    num_samps = chunk_end - chunk_start
    txns = decode_range(byte_start, num_samps)
    for t in txns:
        if t['addr'] == 0x17:
            duration_us = (t['stop'] - t['start']) / 20.0 if t['stop'] else 0
            addr8 = (t['addr'] << 1) | (1 if t['rw'] == 'R' else 0)
            print(f"\nSTART={t['start']/20e6*1000:.3f}ms  Addr=0x{addr8:02X} {t['rw']}  dur={duration_us:.1f}us")
            for j, b in enumerate(t['data']):
                if b == 'RS':
                    print(f"  --- Repeated START ---")
                else:
                    ack = t['acks'][j+1] if j+1 < len(t['acks']) else '?'
                    ack_str = 'ACK' if ack else ('NACK' if ack is False else '?')
                    print(f"  [{j}]: 0x{b:02X} ({ack_str})")
