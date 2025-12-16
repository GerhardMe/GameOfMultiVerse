#ifndef BOARD_ID_H
#define BOARD_ID_H

#include <vector>
#include <cstdint>
#include <stdexcept>

// Board ID is now a vector of bytes (dynamic size)
using BoardID = std::vector<std::uint8_t>;

// Convert board to ID by extracting diagonal octant
// Boards must be square, odd-sized, and have 8-fold symmetry
BoardID boardToId(const std::vector<std::vector<int>> &board);

// Convert ID back to full board with 8-fold symmetry
std::vector<std::vector<int>> idToBoard(const BoardID &id, int size);

#endif // BOARD_ID_H