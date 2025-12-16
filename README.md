# Game of Life Canonical Board and Ruleset ID System

## Overview

This project defines a canonical, space-efficient representation for symmetric
Game of Life boards and their evolution under multiple rulesets.

The system is built around two deterministic identifier schemes:

1. **Board IDs**  
   Compact binary identifiers that encode symmetric Game of Life boards.

2. **Ruleset IDs**  
   Sequential numeric identifiers that enumerate all valid Game of Life rulesets.

Board IDs are used as primary keys in the database and contain **only board state**.
All metadata is stored separately.

---

## Conceptual Model

It is essential to distinguish between three different representations:

1. **Logical Board**  
   The full `N × N` grid of cells in memory.

2. **Board ID**  
   A compressed binary representation exploiting symmetry.

3. **Database Row**  
   A stored record that may include evolution results for many rulesets.

---

## Board ID System

### Purpose

A **Board ID** is a canonical, deterministic, and minimal binary representation
of a Game of Life board that satisfies **8-fold symmetry**.

The ID:
- Encodes only cell state
- Contains no generation num or metadata
- Is identical for identical boards
- size of board is derived from ID length in bits

---

### Symmetry Assumption

All boards are assumed to be symmetric under:

- Horizontal reflection
- Vertical reflection
- Reflection across both diagonals

Because of this, only a triangular subset of the board must be stored.

---

### Stored Region Definition

Boards always have odd dimensions:

```
N = 2k + 1
```

Only the upper-left diagonal triangle (including the center cell) is stored:

- Row `0`: columns `0 → k`
- Row `1`: columns `1 → k`
- …
- Row `k`: column `k`

This region uniquely determines the full board.

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

### Board Size as ID and raw

A logical board of size `N × N` contains:

```
N² cells (bits)
```
For a board of size `N = 2k + 1`, the number of bits making upp its ID is:

```
bits = (k + 1)(k + 2) / 2
```

| Board Size | ID Bits | Raw Board Bits / Num Cells |
|-----------|---------|----------------|
| 1×1 | 1 | 1 |
| 3×3 | 3 | 9 |
| 5×5 | 6 | 25 |
| 7×7 | 10 | 49 |
| 101×101 | 2,601 | 10,201 |
| 1,803×1,803 | 406,351 | 3,250,809 |

### Reconstruction

To reconstruct a full board:

1. Place stored bits into the diagonal triangle
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

Each valid Game of Life ruleset is assigned a unique integer ID.
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

### Enumeration Order

Rulesets are enumerated lexicographically:

```
for a = 0..7
  for b = a+1..8
    for c = b..8
      for d = c+1..9
        assign next ID to {a,b,c,d}
```

- Total valid rulesets: **330**
- ID range: `0–329`

---

### Ruleset API

```cpp
int rulesetToId(const Ruleset& ruleset);
Ruleset idToRuleset(int id);
int getTotalRulesets();  // returns 330
```

---

## Database Schema

### Overview

All boards and their evolution results are stored in a single table.
Each row corresponds to one canonical board.

---

### Table Definition

```sql
CREATE TABLE boards (
    board_id   BLOB PRIMARY KEY,
    children   MEDIUMBLOB
);

```

---

## `children` MEDIUMBLOB Format

### Purpose

The `children` field stores the evolution results of a board under
all rulesets.

---

### Layout

```
Byte 0:
  Bit 0 → expanded flag
  Bit 1 → shrink-origin flag
  Bits 2–7 → unused (0)

Byte 1+:
  330 child Board IDs stored sequentially
```

---

### Metadata Flags

| Bit | Meaning |
|----|--------|
| 0 | 0 = unexpanded, 1 = expanded |
| 1 | 0 = normal origin, 1 = only created by shrinking larger board |

---

## Variable-Length Child Board IDs

### Fundamental Rule

Each evolution produces a board that is **exactly one outer cell layer larger**
than its parent.

As a result:
- The logical board dimension increases by 2
- The stored diagonal triangle gains one new row
- The Board ID increases in length deterministically

---

### Bit-Length Growth

Let the parent board size be:

```
N = 2k + 1
```

Parent ID bits:

```
(k + 1)(k + 2) / 2
```

Child board size:

```
2(k + 1) + 1
```

Child ID bits:

```
(k + 2)(k + 3) / 2
```

---

### Increase per Evolution

```
Δbits = k + 2
```

This corresponds exactly to the additional diagonal row.

---

### MEDIUMBLOB Parsing Rule

No child ID length is stored explicitly.

To parse the `children` BLOB:

1. Read the metadata byte
2. Read the parent board size
3. Compute `k = (size − 1) / 2`
4. Compute child ID bit-length
5. Convert to byte length (ceil)
6. Read exactly that many bytes
7. Repeat for all 330 children

This process is deterministic and stateless.

---

## Expanded Database Row Size

An expanded row contains:

```
1 metadata byte
+ 330 × child Board ID size
```

Example:

| Parent Board | Parent ID Bits | Child Board | Child ID Bits | All Children Bits | Full Expanded Board Object Size |
|-------------|----------------|-------------|---------------|-------------------|----------------------------------|
| 1×1 | 1 | 3×3 | 3 | 990 | ~124 B |
| 3×3 | 3 | 5×5 | 6 | 1,980 | ~248 B |
| 5×5 | 6 | 7×7 | 10 | 3,300 | ~413 B |
| 7×7 | 10 | 9×9 | 15 | 4,950 | ~620 B |
| 101×101 | 2,601 | 103×103 | 2,703 | 891,990 | ~109 KB |
| 1,803×1,803 | 406,351 | 1,805×1,805 | 407,253 | 134,393,490 | ~16.1 MB |

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
- Offset derived solely from computed Board ID length

---

### Mark Board as Expanded

- Replace `children` with metadata byte + 330 child IDs
- Set expanded flag (bit 0)

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

---

## System Guarantees

- Every board has exactly one canonical Board ID
- Board ID size is minimal and deterministic
- Child ID size is derived, never stored
- Database parsing requires no auxiliary metadata
- Storage overhead scales linearly with board growth
