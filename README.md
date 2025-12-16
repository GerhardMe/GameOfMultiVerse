# Game of Life Canonical ID System

## Overview

This project defines a canonical, space-efficient representation for symmetric
Game of Life boards and their evolution across multiple rulesets.

Two deterministic ID systems are provided:

1. **Board ID**  
   A compact binary identifier representing a symmetric Game of Life board.

2. **Ruleset ID**  
   A sequential numeric identifier representing a valid Game of Life ruleset.

These IDs are used consistently across in-memory computation and persistent
storage.

---

## Board ID System

### Purpose

A **Board ID** is a canonical, deterministic binary representation of a Game of
Life board that satisfies **8-fold symmetry**. The ID encodes *only* the board
state and contains no metadata.

---

### Symmetry Model

All boards are assumed to be symmetric under:

- Horizontal reflection
- Vertical reflection
- Reflection across both diagonals

This allows the board to be uniquely represented by storing only the
upper-left diagonal triangular region.

---

### Stored Region Definition

For a board of odd dimension `N = 2k + 1`, only the following cells are stored:

- Row `0`: columns `0 → k`
- Row `1`: columns `1 → k`
- …
- Row `k`: column `k`

This region forms a right triangular matrix including the center cell.

---

### Examples

#### 3×3 Board

```
a b a
b c b
a b a
```

Stored bits: `a b c` (3 bits)

---

#### 5×5 Board

```
a b c b a
b d e d b
c e f e c
b d e d b
a b c b a
```

Stored bits: `a b c d e f` (6 bits)

---

### Bit Count Formula

Let:

```
N = 2k + 1
```

Then the number of stored bits is:

```
bits = (k + 1)(k + 2) / 2
```

| Board Size | Bits |
|-----------|------|
| 1×1 | 1 |
| 3×3 | 3 |
| 5×5 | 6 |
| 7×7 | 10 |
| 101×101 | 2,601 |

---

### Reconstruction

To reconstruct a full board:

1. Place stored bits into the diagonal triangular region
2. Mirror each cell into its 8 symmetric positions

This process is lossless and deterministic.

---

### Board ID API

```cpp
std::uint64_t boardToId(const std::vector<std::vector<int>>& board);
std::vector<std::vector<int>> idToBoard(std::uint64_t id, int size);
```

---

## Ruleset ID System

### Purpose

Each valid Game of Life ruleset is assigned a unique integer identifier.
The mapping is deterministic and reversible.

---

### Ruleset Definition

```cpp
struct Ruleset {
    int underpopEnd;
    int birthStart;
    int birthEnd;
    int overpopStart;
};
```

---

### Validity Constraints

A ruleset is valid if:

- `0 ≤ underpopEnd ≤ 7`
- `underpopEnd < birthStart ≤ 8`
- `birthStart ≤ birthEnd ≤ 8`
- `birthEnd < overpopStart ≤ 9`

---

### ID Assignment

Rulesets are enumerated in lexicographic order:

```
for a = 0..7
  for b = a+1..8
    for c = b..8
      for d = c+1..9
        assign next ID to {a,b,c,d}
```

- Total valid rulesets: **5,005**
- ID range: `0–5004`

---

### Ruleset API

```cpp
int rulesetToId(const Ruleset& ruleset);
Ruleset idToRuleset(int id);
int getTotalRulesets();  // returns 5005
```

---

## Database Schema

### Overview

The database stores boards and all of their evolution results using a
single-table design optimized for space efficiency and lookup speed.

---

### Table Definition

```sql
CREATE TABLE boards (
    board_id   BLOB PRIMARY KEY,
    generation INTEGER NOT NULL,
    size       INTEGER NOT NULL,
    children   MEDIUMBLOB
);

CREATE INDEX idx_generation ON boards(generation);
CREATE INDEX idx_size ON boards(size);
```

---

## Board ID Storage

- Stored as `BLOB`
- Contains only the compressed board bits
- No metadata or size information
- Canonical and deterministic

---

## `children` MEDIUMBLOB Format

### Layout

```
Byte 0:
  Bit 0 → expanded flag
  Bit 1 → shrink-origin flag
  Bits 2–7 → unused (0)

Byte 1+:
  330 evolution Board IDs, stored sequentially
```

---

### Metadata Flags

| Bit | Meaning |
|----|--------|
| 0 | 0 = unexpanded, 1 = expanded |
| 1 | 0 = normal origin, 1 = created by shrinking |

---

## Variable-Length Board IDs in `children`

### Design Principle

Board IDs stored in `children` are **variable-length**. Their length is
**not explicitly stored**. Instead, it is derived deterministically from
the parent board size.

This guarantees unambiguous parsing while minimizing storage.

---

### Growth Rule

Each evolution produces a board that is **one outer cell layer larger**
than its parent.

As a consequence:
- The logical board size increases by 2
- The stored diagonal triangle gains **exactly one additional row**
- The Board ID requires more bits than the parent ID

---

### Bit-Length Derivation

Let the parent board size be:

```
N = 2k + 1
```

Parent ID bit count:

```
(k + 1)(k + 2) / 2
```

Child board size:

```
2(k + 1) + 1
```

Child ID bit count:

```
(k + 2)(k + 3) / 2
```

---

### Bit Increase per Evolution

```
Δbits = (k + 2)
```

This corresponds exactly to the new diagonal row added to the stored region.

---

### Example

| Board | Stored Rows | ID Bits |
|-----|------------|--------|
| 5×5 (`k=2`) | 3 | 6 |
| 7×7 (`k=3`) | 4 | 10 |

The child ID is longer by 4 bits, matching the additional diagonal row.

---

### MEDIUMBLOB Parsing Procedure

After reading the metadata byte:

1. Read the parent board size
2. Compute `k = (size − 1) / 2`
3. Compute child ID bit-length using the formula above
4. Convert bits to bytes (ceil)
5. Read exactly that many bytes
6. Repeat for all 330 evolution entries

No per-entry size markers are required.

---

## Storage Characteristics

| Board Size | ID Size | Expanded Row Size |
|-----------|--------|------------------|
| 5×5 | 1 byte | ~340 bytes |
| 101×101 | 326 bytes | ~108 KB |
| 1,803×1,803 | ~50 KB | ~16.7 MB |

---

## Access Patterns

### Insert New Board

```sql
INSERT INTO boards (board_id, generation, size, children)
VALUES (?, ?, ?, X'00');
```

---

### Check Expansion State

- Read `children[0]`
- Bit 0 indicates whether evolutions are present

---

### Read Evolution Result

- Offset into `children` after metadata byte
- Offset determined solely by computed Board ID length

---

### Mark Board as Expanded

- Replace `children` with metadata byte + 330 evolution IDs
- Set expanded flag (bit 0)

---

## System Properties

- Board IDs contain only board state
- ID length is deterministic and derived
- MEDIUMBLOB parsing is stateless
- No redundant metadata is stored
- Each board has exactly one canonical representation

---

## File Structure

| File | Description |
|----|------------|
| `board_id.h/.cpp` | Board ID encoding and decoding |
| `ruleset_id.h/.cpp` | Ruleset ID mapping |
| `main.cpp` | Tests and examples |

---

## Compilation

```bash
g++ -std=c++17 -O2 main.cpp board_id.cpp ruleset_id.cpp -o multiverse
```
