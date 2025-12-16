# Game of Life ID Systems

## Overview

Two ID systems for compact representation of Game of Life states:
1. **Board ID** - Compresses symmetric boards into minimal binary IDs
2. **Ruleset ID** - Sequential numbering of all valid rulesets (0-5004)

---

## Board ID System

### Symmetry Requirements

All boards have **8-fold symmetry** (4 reflection lines):
- Vertical and horizontal axes
- Both diagonals

Example 5×5 board requires only 6 bits:
```
a b c b a
b d e d b
c e f e c    →  Only store: a,b,c,d,e,f
b d e d b
a b c b a
```

### Extraction Pattern

Extract diagonal triangle from top-left corner to center:
- Row 0: columns 0 → center
- Row 1: columns 1 → center (indent by 1)
- Row 2: columns 2 → center (indent by 2)
- Continue until center cell

**3×3 Example:**
```
a b a
b c b    →  Extract: a, b, c  (3 bits)
a b a
```

**5×5 Example:**
```
a b c b a
b d e d b
c e f e c    →  Extract: a, b, c, d, e, f  (6 bits)
b d e d b
a b c b a
```

### Bit Count Formula

For board size N = 2k+1:
- Bits needed = (k+1)(k+2)/2

Examples:
- 1×1: 1 bit
- 3×3: 3 bits
- 5×5: 6 bits
- 7×7: 10 bits

### Reconstruction

1. Place bits back into diagonal triangle
2. Mirror to all 8 symmetric positions for each bit

---

## Ruleset ID System

### Ruleset Structure

```cpp
struct Ruleset {
    int underpopEnd;   // [0, underpopEnd] → death
    int birthStart;    // [birthStart, birthEnd] → birth/survival
    int birthEnd;
    int overpopStart;  // [overpopStart, 8] → death
}
```

### Constraints

- `0 ≤ underpopEnd ≤ 7`
- `underpopEnd < birthStart ≤ 8`
- `birthStart ≤ birthEnd ≤ 8`
- `birthEnd < overpopStart ≤ 9`

### ID Assignment

Rulesets numbered 0-5004 in lexicographic order:
```
For a=0..7, b=a+1..8, c=b..8, d=c+1..9:
    Assign next ID to {a,b,c,d}
```

**Total: 5,005 valid rulesets**

Example - Conway's Game of Life:
```
{1, 3, 3, 4} → ID varies (lookup required)
```

---

## API

### Board Functions
```cpp
std::uint64_t boardToId(const std::vector<std::vector<int>>& board);
std::vector<std::vector<int>> idToBoard(std::uint64_t id, int size);
```

### Ruleset Functions
```cpp
int rulesetToId(const Ruleset& ruleset);
Ruleset idToRuleset(int id);
int getTotalRulesets();  // Returns 5005
```

---

## File Structure

- `board_id.h/cpp` - Board ID conversion
- `ruleset_id.h/cpp` - Ruleset ID conversion
- `main.cpp` - Tests and examples

**Compile:**
```bash
g++ -std=c++17 -O2 main.cpp board_id.cpp ruleset_id.cpp -o multiverse
```

### Future Extensions

- Support for larger boards (use `__uint128_t` or multiple uint64_t)
- Alternative symmetry groups (4-fold, 2-fold)
- Compressed storage for very large boards