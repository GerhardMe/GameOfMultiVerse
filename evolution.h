#ifndef EVOLUTION_H
#define EVOLUTION_H

#include <vector>
#include "ruleset_id.h"

// Add a ring of zeros around the board (NxN -> (N+2)x(N+2))
std::vector<std::vector<int>> padBoard(const std::vector<std::vector<int>> &board);

// Evolve a board one generation (size stays the same)
std::vector<std::vector<int>> evolveBoard(
    const std::vector<std::vector<int>> &board,
    const Ruleset &ruleset);

// Check if outer ring is all zeros
bool canTrim(const std::vector<std::vector<int>> &board);

// Remove outer ring (NxN -> (N-2)x(N-2))
// Returns empty vector if board would become empty (the "zero board")
std::vector<std::vector<int>> trimBoard(const std::vector<std::vector<int>> &board);

// Recursively trim until outer ring has live cells or board disappears
// Returns empty vector for the "zero board"
std::vector<std::vector<int>> trimBoardFully(const std::vector<std::vector<int>> &board);

#endif // EVOLUTION_H