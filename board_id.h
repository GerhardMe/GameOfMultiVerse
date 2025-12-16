#ifndef BOARD_ID_H
#define BOARD_ID_H

#include <vector>
#include <cstdint>
#include <stdexcept>

// Convert board to ID by extracting diagonal octant
// Boards must be square, odd-sized, and have 8-fold symmetry
std::uint64_t boardToId(const std::vector<std::vector<int>> &board);

// Convert ID back to full board with 8-fold symmetry
std::vector<std::vector<int>> idToBoard(std::uint64_t id, int size);

#endif // BOARD_ID_H