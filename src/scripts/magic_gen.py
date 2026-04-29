import random
import itertools

from tqdm import tqdm


squares_bb = [1 << i for i in range(64)]

bb_fileA = 0x0101010101010101
bb_fileB = 0x0202020202020202
bb_fileC = 0x0404040404040404
bb_fileD = 0x0808080808080808
bb_fileE = 0x1010101010101010
bb_fileF = 0x2020202020202020
bb_fileG = 0x4040404040404040
bb_fileH = 0x8080808080808080

bb_files = [bb_fileA, bb_fileB, bb_fileC, bb_fileD, bb_fileE, bb_fileF, bb_fileG, bb_fileH]

bb_rank1 = 0x00000000000000FF
bb_rank2 = 0x000000000000FF00
bb_rank3 = 0x0000000000FF0000
bb_rank4 = 0x00000000FF000000
bb_rank5 = 0x000000FF00000000
bb_rank6 = 0x0000FF0000000000
bb_rank7 = 0x00FF000000000000
bb_rank8 = 0xFF00000000000000

bb_ranks = [bb_rank1, bb_rank2, bb_rank3, bb_rank4, bb_rank5, bb_rank6, bb_rank7, bb_rank8]

squares = [
    'a1', 'b1', 'c1', 'd1', 'e1', 'f1', 'g1', 'h1',
    'a2', 'b2', 'c2', 'd2', 'e2', 'f2', 'g2', 'h2',
    'a3', 'b3', 'c3', 'd3', 'e3', 'f3', 'g3', 'h3',
    'a4', 'b4', 'c4', 'd4', 'e4', 'f4', 'g4', 'h4',
    'a5', 'b5', 'c5', 'd5', 'e5', 'f5', 'g5', 'h5',
    'a6', 'b6', 'c6', 'd6', 'e6', 'f6', 'g6', 'h6',
    'a7', 'b7', 'c7', 'd7', 'e7', 'f7', 'g7', 'h7',
    'a8', 'b8', 'c8', 'd8', 'e8', 'f8', 'g8', 'h8'
]


def magic_candidate():
    return random.getrandbits(64) & random.getrandbits(64) & random.getrandbits(64)

def rook_occupancy_masks(square):
    rank = square // 8
    file = square % 8

    rook_mask = bb_files[file] | bb_ranks[rank]

    if rank != 0:
        rook_mask &= ~bb_ranks[0]
    if rank != 7:
        rook_mask &= ~bb_ranks[7]
    if file != 0:
        rook_mask &= ~bb_files[0]
    if file != 7:
        rook_mask &= ~bb_files[7]
        
    rook_mask &= ~(1 << square)

    bit_positions = [i for i in range(64) if (rook_mask >> i) & 1]

    occupancies = []
    for r in range(len(bit_positions) + 1):
        occupancies.extend(itertools.combinations(bit_positions, r))

    available_squares = []
    for occupancy in occupancies:
        reachable = []
        
        for r in range(rank + 1, 8):
            sq = r * 8 + file
            reachable.append(sq)
            if sq in occupancy:
                break
                
        for r in range(rank - 1, -1, -1):
            sq = r * 8 + file
            reachable.append(sq)
            if sq in occupancy:
                break
                
        for f in range(file + 1, 8):
            sq = rank * 8 + f
            reachable.append(sq)
            if sq in occupancy:
                break
                
        for f in range(file - 1, -1, -1):
            sq = rank * 8 + f
            reachable.append(sq)
            if sq in occupancy:
                break

        available_squares.append(reachable)

    occupancies = [sum(1 << sq for sq in occupancy) for occupancy in occupancies]

    return rook_mask, occupancies, available_squares

def bishop_occupancy_masks(square):
    rank = square // 8
    file = square % 8

    bishop_mask = 0
    for r, f in zip(range(rank + 1, 8), range(file + 1, 8)):
        bishop_mask |= 1 << (r * 8 + f)
    for r, f in zip(range(rank + 1, 8), range(file - 1, -1, -1)):
        bishop_mask |= 1 << (r * 8 + f)
    for r, f in zip(range(rank - 1, -1, -1), range(file + 1, 8)):
        bishop_mask |= 1 << (r * 8 + f)
    for r, f in zip(range(rank - 1, -1, -1), range(file - 1, -1, -1)):
        bishop_mask |= 1 << (r * 8 + f)

    if rank != 0 and file != 0:
        bishop_mask &= ~(bb_ranks[0] | bb_files[0])
    if rank != 0 and file != 7:
        bishop_mask &= ~(bb_ranks[0] | bb_files[7])
    if rank != 7 and file != 0:
        bishop_mask &= ~(bb_ranks[7] | bb_files[0])
    if rank != 7 and file != 7:
        bishop_mask &= ~(bb_ranks[7] | bb_files[7])

    bishop_mask &= ~(1 << square)

    bit_positions = [i for i in range(64) if (bishop_mask >> i) & 1]

    occupancies = []
    for r in range(len(bit_positions) + 1):
        occupancies.extend(itertools.combinations(bit_positions, r))

    available_squares = []
    for occupancy in occupancies:
        reachable = []
        
        for r, f in zip(range(rank + 1, 8), range(file + 1, 8)):
            sq = r * 8 + f
            reachable.append(sq)
            if sq in occupancy:
                break
                
        for r, f in zip(range(rank + 1, 8), range(file - 1, -1, -1)):
            sq = r * 8 + f
            reachable.append(sq)
            if sq in occupancy:
                break
                
        for r, f in zip(range(rank - 1, -1, -1), range(file + 1, 8)):
            sq = r * 8 + f
            reachable.append(sq)
            if sq in occupancy:
                break
                
        for r, f in zip(range(rank - 1, -1, -1), range(file - 1, -1, -1)):
            sq = r * 8 + f
            reachable.append(sq)
            if sq in occupancy:
                break

        available_squares.append(reachable)

    occupancies = [sum(1 << sq for sq in occupancy) for occupancy in occupancies]

    return bishop_mask, occupancies, available_squares

def generate_magic(square, piece_mask, occupancies, available_squares, piece_type, tries = 1_000_000):

    best_shift = 64 - len(occupancies).bit_length()
    best_magic = None

    pbar = tqdm(range(tries), desc=f'Finding {piece_type} magic for square {square}')
    for _ in pbar:
        magic = magic_candidate()
        lookup_table = {}

        for occupancy, reachable in zip(occupancies, available_squares):
            idx = (((occupancy & piece_mask) * magic) & 0xFFFFFFFFFFFFFFFF) >> (best_shift+1)

            if idx in lookup_table and lookup_table[idx] != reachable:
                break
            lookup_table[idx] = reachable
        else:
            best_magic = magic
            best_shift += 1

            pbar.set_description(f'Finding {piece_type} magic for square {square} (best shift so far: {best_shift})')

    return best_magic, best_shift, piece_mask

def generate_magics():
    rook_magics = {}
    for square in range(64):
        rook_mask, occupancies, available_squares = rook_occupancy_masks(square)
        magic, shift, rook_mask = generate_magic(square, rook_mask, occupancies, available_squares, 'rook')
        rook_magics[square] = {'magic': magic, 'shift': shift, 'mask': rook_mask}

    lookup_tables_size = [64 - rook_magics[square]['shift'] for square in range(64)] # bits needed for each table
    lookup_tables_size = [1 << size for size in lookup_tables_size] # size of each table in entries, each entry is 8 bytes
    lookup_tables_size = [size * 8 for size in lookup_tables_size] # size of each table in bytes
    lookup_tables_size = [size / 1024 for size in lookup_tables_size] # size of each table in kilobytes

    total_size_kb = sum(lookup_tables_size)
    print(f'Total size of rook lookup tables: {total_size_kb:.2f} KB')

    bishop_magics = {}
    for square in range(64):
        bishop_mask, occupancies, available_squares = bishop_occupancy_masks(square)
        magic, shift, bishop_mask = generate_magic(square, bishop_mask, occupancies, available_squares, 'bishop')
        bishop_magics[square] = {'magic': magic, 'shift': shift, 'mask': bishop_mask}

    lookup_tables_size = [64 - bishop_magics[square]['shift'] for square in range(64)] # bits needed for each table
    lookup_tables_size = [1 << size for size in lookup_tables_size] # size of each table in entries, each entry is 8 bytes
    lookup_tables_size = [size * 8 for size in lookup_tables_size] # size of each table in bytes
    lookup_tables_size = [size / 1024 for size in lookup_tables_size] # size of each table in kilobytes

    total_size_kb = sum(lookup_tables_size)
    print(f'Total size of bishop lookup tables: {total_size_kb:.2f} KB')

    with open('rook_magics.txt', 'w') as f:
        for square in range(64):
            magic = rook_magics[square]['magic']
            shift = rook_magics[square]['shift']
            mask = rook_magics[square]['mask']
            f.write(f'{square} {magic:#018x} {shift} {mask:#018x}\n')

    with open('bishop_magics.txt', 'w') as f:
        f.write(f'square magic shift mask\n')
        for square in range(64):
            magic = bishop_magics[square]['magic']
            shift = bishop_magics[square]['shift']
            mask = bishop_magics[square]['mask']
            f.write(f'{square} {magic:#018x} {shift} {mask:#018x}\n')

    with open('rook_magics.cpp', 'w') as f:
        for square in range(64):
            f.write(f'Magic({rook_magics[square]["mask"]:#018x}, {rook_magics[square]["magic"]:#018x}, {rook_magics[square]["shift"]}){"," if square != 63 else ""} // {squares[square]}\n')

    with open('bishop_magics.cpp', 'w') as f:
        for square in range(64):
            f.write(f'Magic({bishop_magics[square]["mask"]:#018x}, {bishop_magics[square]["magic"]:#018x}, {bishop_magics[square]["shift"]}){"," if square != 63 else ""} // {squares[square]}\n')

if __name__ == '__main__':
    generate_magics()
        