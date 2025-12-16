# Game of Life ID Systems

## Overview

This document describes the two ID systems used in the Game of Life multiverse simulator:
1. **Board ID System** - Compact binary representation of symmetric board states
2. **Ruleset ID System** - Sequential numbering of all valid rulesets

---

## Board ID System

### Symmetry Constraints

All boards must have **8-fold symmetry** with reflection across:
- Vertical axis (left-right mirror)
- Horizontal axis (top-bottom mirror)
- Main diagonal (top-left to bottom-right)
- Anti-diagonal (top-right to bottom-left)

This means for a board like:
```
a b c b a
b d e d b
c e f e c
b d e d b
a b c b a
```

Only the values `a, b, c, d, e, f` (6 values) are needed to reconstruct the entire 5×5 board (25 cells).

### Extraction Pattern

For an odd-sized square board (size = 2k+1), we extract a **diagonal triangle** from the center to one corner:

**3×3 Board Example:**
```
a b a       Extract: a, b, c
b c b       (3 bits)
a b a
```

**5×5 Board Example:**
```
a b c b a   Extract: a, b, c, d, e, f
b d e d b   (6 bits)
c e f e c
b d e d b
a b c b a
```

**7×7 Board Example:**
```
a b c d c b a   Extract: a, b, c, d, e, f, g, h, i, j
b e f g f e b   (10 bits)
c f h i h f c
d g i j i g d
c f h i h f c
b e f g f e b
a b c d c b a
```

### Bit Count Formula

For a board of size N = 2k+1:
- Number of bits needed = (k+1)(k+2)/2
- This is the (k+1)th triangular number

Examples:
- 1×1: k=0 → 1 bit
- 3×3: k=1 → 3 bits
- 5×5: k=2 → 6 bits
- 7×7: k=3 → 10 bits
- 9×9: k=4 → 15 bits

### Extraction Algorithm

Starting from the center, extract diagonally outward:

```
For distance d = 0 to k:
    row = center - d
    For col = center - d to center:
        Extract bit from (row, col)
```

**3×3 Example (center = 1):**
```
Distance 0: (1,1) → c
Distance 1: (0,0), (0,1) → a, b
Result: bits = [c, a, b] → ID = cab (binary)
```

**5×5 Example (center = 2):**
```
Distance 0: (2,2) → f
Distance 1: (1,1), (1,2) → e, d
Distance 2: (0,0), (0,1), (0,2) → a, b, c
Result: bits = [f, e, d, a, b, c] → ID = fedabc (binary)
```

### Reconstruction Algorithm

1. Place extracted bits back into the diagonal triangle (top-left quadrant including center)
2. Mirror horizontally across vertical center line
3. Mirror vertically across horizontal center line

This creates the full 8-fold symmetric board.

### Storage

Board IDs are stored as `uint64_t`, which supports boards up to:
- 64 bits → k=10 → 23×23 board maximum

---

## Ruleset ID System

### Ruleset Structure

Each ruleset defines cell behavior based on neighbor count (0-8):

```cpp
struct Ruleset {
    int underpopEnd;   // [0, underpopEnd] → cell dies
    int birthStart;    // [birthStart, birthEnd] → cell becomes alive
    int birthEnd;
    int overpopStart;  // [overpopStart, 8] → cell dies
}
```

### Validity Constraints

A valid ruleset must satisfy:
1. `0 ≤ underpopEnd ≤ 7`
2. `underpopEnd < birthStart ≤ 8`
3. `birthStart ≤ birthEnd ≤ 8`
4. `birthEnd < overpopStart ≤ 9`

The gaps between zones define neutral regions where cells maintain their current state.

### Example Rulesets

**Conway's Game of Life:**
```
underpopEnd = 1    (die with ≤1 neighbors)
birthStart = 3     (birth/survive with 3 neighbors)
birthEnd = 3
overpopStart = 4   (die with ≥4 neighbors)
```

**HighLife:**
```
underpopEnd = 1
birthStart = 3
birthEnd = 3
overpopStart = 4
(with additional birth at 6 - requires different encoding)
```

### ID Assignment

Rulesets are numbered sequentially (0, 1, 2, ...) based on lexicographic ordering:

```
For a = 0 to 7:
    For b = a+1 to 8:
        For c = b to 8:
            For d = c+1 to 9:
                Assign next ID to ruleset {a, b, c, d}
```

This generates **exactly 5,005 valid rulesets**.

### Total Count Calculation

```
Sum over a=0..7, b=a+1..8, c=b..8, d=c+1..9 of 1

= Sum_{a=0}^{7} Sum_{b=a+1}^{8} Sum_{c=b}^{8} (9-c)

This evaluates to 5,005 rulesets
```

### Lookup Tables

Two static lookup tables enable O(1) conversions:

```cpp
int rulesetToId(Ruleset r) → returns 0..5004
Ruleset idToRuleset(int id) → returns ruleset
```

---

## Database Schema

### Boards Table
```sql
CREATE TABLE boards (
    board_id INTEGER PRIMARY KEY,  -- Board ID (uint64_t)
    generation INTEGER,
    size INTEGER,                   -- Board dimension (always odd)
    is_initial BOOLEAN              -- Is this gen0?
);
```

### Evolutions Table
```sql
CREATE TABLE evolutions (
    from_board_id INTEGER,
    ruleset_id INTEGER,
    to_board_id INTEGER,
    PRIMARY KEY (from_board_id, ruleset_id),
    FOREIGN KEY (from_board_id) REFERENCES boards(board_id),
    FOREIGN KEY (to_board_id) REFERENCES boards(board_id)
);
```

### Benefits

- **Compact storage**: 5×5 board = 6 bits + metadata
- **Fast lookup**: Both conversions are O(1)
- **Canonical representation**: Each unique symmetric pattern has exactly one ID
- **Deduplication**: Same pattern in different generations gets same board_id

---

## Implementation Notes

### Debugging

To verify board conversion:
```cpp
vector<vector<int>> original = /* your board */;
uint64_t id = boardToId(original);
vector<vector<int>> restored = idToBoard(id, original.size());
assert(original == restored);
```

### Common Pitfalls

1. **Non-symmetric boards**: The ID system assumes 8-fold symmetry. Asymmetric boards will be "corrected" to their symmetric form.

2. **Even-sized boards**: Only odd-sized boards (1×1, 3×3, 5×5, ...) are supported.

3. **Bit ordering**: LSB corresponds to center cell (distance 0), then radiates outward.

### Future Extensions

- Support for larger boards (use `__uint128_t` or multiple uint64_t)
- Alternative symmetry groups (4-fold, 2-fold)
- Compressed storage for very large boards