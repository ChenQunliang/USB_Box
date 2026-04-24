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
bits_to_scan = 8000

# Extract bitstream (LSB first within each byte)
scl = []
sda = []
for i in range(bits_to_scan):
    bi = byte_start + i // 8
    bitp = i % 8
    scl.append((scl_all[bi] >> bitp) & 1 if bi < len(scl_all) else 0)
    sda.append((sda_all[bi] >> bitp) & 1 if bi < len(sda_all) else 0)

# State machine for I2C decoding
# States: IDLE, ADDR, ACK_ADDR, DATA, ACK_DATA
state = 'IDLE'
bit_count = 0
current_byte = 0
current_txn = None
transactions = []

prev_scl = scl[0]
prev_sda = sda[0]

for i in range(1, bits_to_scan):
    sc = scl[i]
    sd = sda[i]
    gs = byte_start * 8 + i

    scl_rise = (prev_scl == 0 and sc == 1)
    sda_fall = (prev_sda == 1 and sd == 0)
    sda_rise = (prev_sda == 0 and sd == 1)

    # START detection (SDA falls while SCL is HIGH)
    if state == 'IDLE':
        if sc == 1 and sda_fall:
            state = 'ADDR'
            bit_count = 0
            current_byte = 0
            current_txn = {
                'start_sample': gs,
                'start_ms': gs / 20e6 * 1000,
                'addr': 0,
                'rw': '',
                'data': [],
                'acks': []
            }
            # Don't process this sample further
            prev_scl, prev_sda = sc, sd
            continue

    # In ADDR or DATA state, sample SDA on SCL rising edge
    if state in ('ADDR', 'DATA'):
        if scl_rise:
            current_byte = (current_byte << 1) | sd
            bit_count += 1
            if bit_count == 8:
                if state == 'ADDR':
                    current_txn['addr'] = current_byte >> 1
                    current_txn['rw'] = 'R' if (current_byte & 1) else 'W'
                else:
                    current_txn['data'].append(current_byte)
                state = 'ACK_ADDR' if state == 'ADDR' else 'ACK_DATA'
                bit_count = 0
        # STOP detection during address/data
        if sc == 1 and sda_rise:
            if current_txn:
                current_txn['stop_sample'] = gs
                current_txn['stop_ms'] = gs / 20e6 * 1000
                transactions.append(current_txn)
                current_txn = None
            state = 'IDLE'

    # In ACK state, sample SDA on SCL rising edge for ACK bit
    elif state in ('ACK_ADDR', 'ACK_DATA'):
        if scl_rise:
            ack = (sd == 0)
            current_txn['acks'].append(ack)
            state = 'DATA'
            bit_count = 0
            current_byte = 0
        # STOP detection during ACK
        if sc == 1 and sda_rise:
            if current_txn:
                current_txn['stop_sample'] = gs
                current_txn['stop_ms'] = gs / 20e6 * 1000
                transactions.append(current_txn)
                current_txn = None
            state = 'IDLE'

    prev_scl, prev_sda = sc, sd

# Add pending transaction
if current_txn and len(current_txn['data']) > 0:
    transactions.append(current_txn)

# Filter out transactions with no data (probe noise)
valid_txns = [t for t in transactions if len(t['data']) > 0 or t['rw'] != '']
# Also keep transactions with valid addresses
valid_txns = [t for t in transactions if t['addr'] != 0x78]  # Filter OLED ghost detections

print(f"Found {len(transactions)} raw transactions, {len(valid_txns)} after filtering")
print()

for idx, txn in enumerate(transactions):
    print(f"{'='*60}")
    print(f"Transaction {idx+1}: START @ {txn['start_ms']:.3f}ms (sample {txn['start_sample']})")
    print(f"  Address: 7-bit=0x{txn['addr']:02X}, 8-bit=0x{(txn['addr']<<1)|(1 if txn['rw']=='R' else 0):02X}, {txn['rw']}")
    if txn['data']:
        for j, b in enumerate(txn['data']):
            ack_idx = j + 1  # +1 for address ACK
            ack = txn['acks'][ack_idx] if ack_idx < len(txn['acks']) else None
            ack_str = 'ACK' if ack else ('NACK' if ack is False else '?')
            print(f"  Data[{j}]: 0x{b:02X} ({ack_str})")
    elif txn['acks']:
        ack = txn['acks'][0]
        ack_str = 'ACK' if ack else 'NACK'
        print(f"  (no data, addr ACK={ack_str})")
    if 'stop_sample' in txn:
        print(f"  STOP @ {txn['stop_ms']:.3f}ms (sample {txn['stop_sample']})")
        duration_us = (txn['stop_sample'] - txn['start_sample']) / 20.0
        print(f"  Duration: {duration_us:.1f}us")
