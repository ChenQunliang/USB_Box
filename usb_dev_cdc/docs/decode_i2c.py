import sys, io, zipfile
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def get_bits(data, byte_start, num_bits):
    bits = []
    for i in range(num_bits):
        bi = byte_start + i // 8
        bitp = i % 8
        if bi < len(data):
            bits.append((data[bi] >> bitp) & 1)
        else:
            bits.append(0)
    return bits

def decode_i2c(scl, sda, byte_start_global, num_bits):
    """Decode I2C transactions from bitstreams. Returns list of transactions."""
    prev_scl, prev_sda = scl[0], sda[0]
    txn_state = 'IDLE'
    bit_count = 0
    current_byte = 0
    current_txn = None
    transactions = []

    for i in range(num_bits):
        sc = scl[i]
        sd = sda[i]
        gs = byte_start_global * 8 + i

        scl_rise = (prev_scl == 0 and sc == 1)
        sda_fall = (prev_sda == 1 and sd == 0)
        sda_rise = (prev_sda == 0 and sd == 1)

        # Check for START (SDA fall while SCL high)
        if txn_state == 'IDLE':
            if sc == 1 and sda_fall:
                txn_state = 'ADDR'
                bit_count = 0
                current_byte = 0
                current_txn = {'start': gs, 'addr': 0, 'rw': '', 'data': [], 'acks': []}
        elif txn_state == 'ADDR':
            if scl_rise:
                current_byte = (current_byte << 1) | sd
                bit_count += 1
                if bit_count == 8:
                    current_txn['addr'] = current_byte >> 1
                    current_txn['rw'] = 'R' if (current_byte & 1) else 'W'
                    txn_state = 'ACK_ADDR'
                    bit_count = 0
            if sc == 1 and sda_rise:
                # STOP during addr (unlikely but handle)
                current_txn['stop'] = gs
                transactions.append(current_txn)
                current_txn = None
                txn_state = 'IDLE'
        elif txn_state in ('ACK_ADDR', 'ACK_DATA'):
            if scl_rise:
                current_txn['acks'].append(sd == 0)
                txn_state = 'DATA'
                bit_count = 0
                current_byte = 0
            if sc == 1 and sda_rise:
                current_txn['stop'] = gs
                transactions.append(current_txn)
                current_txn = None
                txn_state = 'IDLE'
        elif txn_state == 'DATA':
            if scl_rise:
                current_byte = (current_byte << 1) | sd
                bit_count += 1
                if bit_count == 8:
                    current_txn['data'].append(current_byte)
                    txn_state = 'ACK_DATA'
                    bit_count = 0
            if sc == 1 and sda_rise:
                current_txn['stop'] = gs
                transactions.append(current_txn)
                current_txn = None
                txn_state = 'IDLE'

        prev_scl, prev_sda = sc, sd

    # Capture any pending transaction
    if current_txn and len(current_txn.get('data', [])) > 0:
        transactions.append(current_txn)

    return transactions

def print_transactions(transactions, label=""):
    print(f"\n{'='*60}")
    print(f"  {label}")
    print(f"{'='*60}")
    for idx, txn in enumerate(transactions):
        t_ms = txn['start'] / 20e6 * 1000
        print(f"\n  --- Txn {idx+1}: START @ sample {txn['start']} ({t_ms:.3f}ms) ---")
        addr8 = (txn['addr'] << 1) | (1 if txn['rw'] == 'R' else 0)
        print(f"  Addr: 7-bit=0x{txn['addr']:02X}, 8-bit=0x{addr8:02X}, {txn['rw']}")
        if txn['data']:
            for j, b in enumerate(txn['data']):
                ack = txn['acks'][j+1] if j+1 < len(txn['acks']) else '?'
                ack_str = 'ACK' if ack else 'NACK'
                print(f"  Data[{j}]: 0x{b:02X} ({ack_str})")
        if 'stop' in txn:
            t_ms_stop = txn['stop'] / 20e6 * 1000
            print(f"  STOP @ sample {txn['stop']} ({t_ms_stop:.3f}ms)")

# Main
with zipfile.ZipFile('DSLogic U3Pro32-la-260424-160128.dsl', 'r') as zf:
    scl_all = b''
    sda_all = b''
    for i in range(6):
        scl_all += zf.read(f'L-0/{i}')
        sda_all += zf.read(f'L-1/{i}')

block_size = 2097152

# Region 1: Block 1, starting from byte ~928258
print("Decoding Region 1 (Block 1 I2C activity)...")
byte_start1 = block_size + 928250
num_bits1 = 200000

scl1 = get_bits(scl_all, byte_start1, num_bits1)
sda1 = get_bits(sda_all, byte_start1, num_bits1)
txns1 = decode_i2c(scl1, sda1, byte_start1, num_bits1)
print_transactions(txns1, "Region 1: TV5725 Init I2C")

# Region 2: Block 5, starting from byte ~66300
print("\nDecoding Region 2 (Block 5 I2C activity)...")
byte_start2 = 5 * block_size + 66300
num_bits2 = 20000

scl2 = get_bits(scl_all, byte_start2, num_bits2)
sda2 = get_bits(sda_all, byte_start2, num_bits2)
txns2 = decode_i2c(scl2, sda2, byte_start2, num_bits2)
print_transactions(txns2, "Region 2: Block 5 I2C")
