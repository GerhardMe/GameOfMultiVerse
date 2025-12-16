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


# Database Schema Documentation

## Overview

The database stores Game of Life boards and their evolutions using a single-table design optimized for space efficiency and lookup speed.

---

## Data Type Limits

### Board ID Storage (BLOB)
- **Type:** BLOB
- **Max size:** 65,535 bytes = 524,280 bits
- **Supports:** Boards up to √(524,280 × 8) ≈ 2,045 × 2,045 cells
- **Purpose:** Pure board identifier, no metadata

### Children Storage (MEDIUMBLOB)
- **Type:** MEDIUMBLOB  
- **Max size:** 16,777,215 bytes
- **Per evolution allocation:** 16,777,215 / 330 ≈ 50,840 bytes = 406,720 bits
- **Supports:** Boards up to √(406,720 × 8) ≈ 1,803 × 1,803 cells

**Hard limit: 1,803 × 1,803 = 3,249,609 cells maximum**

---

## Table Schema

```sql
CREATE TABLE boards (
    board_id BLOB PRIMARY KEY,        -- Pure board ID (no metadata)
    generation INTEGER NOT NULL,      -- Generation number
    size INTEGER NOT NULL,            -- Board dimension (always odd)
    children MEDIUMBLOB               -- Packed evolution data
);

CREATE INDEX idx_generation ON boards(generation);
CREATE INDEX idx_size ON boards(size);
```

---

## Children MEDIUMBLOB Format

The `children` field packs metadata and 330 evolution results:

### Bit Layout

```
Bit 0:        Expanded flag (0 = unexpanded, 1 = expanded)
Bit 1:        Shrink flag (0 = normal, 1 = created by smaller board)
Bits 2-7:     Reserved (unused, set to 0)
Bits 8+:      Evolution IDs (330 consecutive board IDs)
```

### Metadata Flags

**Expanded Flag (Bit 0):**
- `0` = Board has not been expanded (evolution IDs not computed)
- `1` = Board has been expanded (all 330 evolution IDs present)

**Shrink Flag (Bit 1):**
- `0` = Board created normally (same or larger size parent)
- `1` = Board is result of shrinking from larger board

### Evolution IDs Layout

After the first byte of metadata, 330 board IDs are stored sequentially:

```
Bytes 1 to N:     Evolution for ruleset 0
Bytes N+1 to M:   Evolution for ruleset 1
...
Last bytes:       Evolution for ruleset 329
```

Each evolution ID is a variable-length BLOB (matching the board_id size).

**Special values:**
- Evolution ID = `0` (all bits zero) → evolves to empty board
- Evolution ID = NULL concept handled by expanded flag

---

## Board ID Structure

Board IDs are pure compressed board representations with **no metadata**.

### Properties
- **8-fold symmetric:** Only diagonal triangle stored
- **Binary representation:** Each bit = one cell value
- **Deterministic:** Same pattern always produces same ID
- **Canonical:** Each unique pattern has exactly one ID

### Size Examples

| Board Size | Cells   | ID Bits | ID Bytes |
|------------|---------|---------|----------|
| 3×3        | 9       | 3       | 1        |
| 5×5        | 25      | 6       | 1        |
| 7×7        | 49      | 10      | 2        |
| 101×101    | 10,201  | 2,601   | 326      |
| 1,001×1,001| 1,002,001| 250,501| 31,313   |
| 1,803×1,803| 3,250,809| 406,351| 50,794   |

---

## Access Patterns

### Insert New Board
```sql
INSERT INTO boards (board_id, generation, size, children)
VALUES (?, ?, ?, X'00');  -- children starts as single 0x00 byte (unexpanded)
```

### Check if Expanded
```sql
SELECT children FROM boards WHERE board_id = ?;
-- Read bit 0: 0 = unexpanded, 1 = expanded
```

### Get Evolution Result
```sql
SELECT children FROM boards WHERE board_id = ?;
-- Extract ruleset_id evolution from bytes 1+
```

### Mark as Expanded
```sql
UPDATE boards SET children = ? WHERE board_id = ?;
-- Set bit 0 to 1, write all 330 evolution IDs
```

---

## Packing/Unpacking Children BLOB

### Pseudocode for Packing

```cpp
MEDIUMBLOB packChildren(bool expanded, bool fromShrink, vector<BoardID> evolutions) {
    BLOB result;
    
    // First byte: metadata
    uint8_t metadata = 0;
    if (expanded) metadata |= 0x01;
    if (fromShrink) metadata |= 0x02;
    result.append(metadata);
    
    // Append 330 evolution IDs
    for (int i = 0; i < 330; i++) {
        result.append(evolutions[i]);
    }
    
    return result;
}
```

### Pseudocode for Unpacking

```cpp
struct ChildrenData {
    bool expanded;
    bool fromShrink;
    vector<BoardID> evolutions;
};

ChildrenData unpackChildren(MEDIUMBLOB blob) {
    ChildrenData data;
    
    // Read metadata byte
    uint8_t metadata = blob[0];
    data.expanded = (metadata & 0x01) != 0;
    data.fromShrink = (metadata & 0x02) != 0;
    
    // Read 330 evolution IDs
    int offset = 1;
    for (int i = 0; i < 330; i++) {
        data.evolutions[i] = readBoardID(blob, offset);
        offset += getBoardIDSize(data.evolutions[i]);
    }
    
    return data;
}
```

---

## Storage Efficiency

### Per Board Overhead
- `board_id`: Variable (1-50,794 bytes depending on size)
- `generation`: 4 bytes (INTEGER)
- `size`: 4 bytes (INTEGER)
- `children` (unexpanded): 1 byte (just metadata)
- `children` (expanded): Variable (1 + 330×board_id_size)

### Example: 5×5 Board
- Board ID: 1 byte (6 bits padded)
- Metadata: 8 bytes (generation + size)
- Children unexpanded: 1 byte
- Children expanded: 1 + 330×1 = 331 bytes
- **Total unexpanded:** 10 bytes
- **Total expanded:** 340 bytes

### Example: 101×101 Board
- Board ID: 326 bytes
- Metadata: 8 bytes
- Children unexpanded: 1 byte
- Children expanded: 1 + 330×326 = 107,581 bytes
- **Total unexpanded:** 335 bytes
- **Total expanded:** 107,915 bytes (~105 KB)

---

## Design Rationale

### Why BLOB for Board IDs?
- Variable size boards require variable storage
- BLOBs handle arbitrary bit patterns efficiently
- Primary key lookups are O(1) with proper indexing

### Why MEDIUMBLOB for Children?
- 330 evolutions per board
- Each evolution ID can be large (up to 50KB)
- Total: 330 × 50KB = 16.5MB (within MEDIUMBLOB limit)

### Why Single Table?
- Avoids 330-way JOIN for fetching evolutions
- Single row read = all data
- Cache friendly: related data stored together
- Simpler schema: no foreign key cascades

### Why Metadata in Children BLOB?
- Board ID stays pure (used for lookups/hashing)
- Expanded flag avoids separate column
- Shrink flag enables bidirectional graph traversal
- Keeps board_id globally unique across all contexts

---

## Limitations

1. **Maximum board size:** 1,803 × 1,803 cells
2. **No partial expansion:** All 330 evolutions computed at once
3. **Fixed ruleset count:** Schema assumes exactly 330 rulesets
4. **No compression:** BLOBs store raw bit patterns

---

## Future Considerations

- **Compression:** gzip/zstd on children BLOB could save ~50%
- **Incremental expansion:** Split children into chunks
- **Larger boards:** Use LONGBLOB (4GB) for boards beyond 1,803×1,803
- **Additional metadata:** Bits 2-7 reserved for future flags