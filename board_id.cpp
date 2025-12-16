#include "board_id.h"

std::uint64_t boardToId(const std::vector<std::vector<int>> &board)
{
    int size = board.size();
    int center = size / 2;

    // First, count total bits needed
    int totalBits = 0;
    for (int row = 0; row <= center; row++)
    {
        totalBits += (center - row + 1);
    }

    uint64_t id = 0;
    int bitPos = totalBits - 1; // Start from MSB

    // Start at top-left corner, work down row by row
    // Each row starts at the diagonal and goes to center column
    for (int row = 0; row <= center; row++)
    {
        int colStart = row;  // diagonal position (indent by row number)
        int colEnd = center; // always go to center column

        for (int col = colStart; col <= colEnd; col++)
        {
            if (board[row][col] == 1)
            {
                id |= (1ULL << bitPos);
            }
            bitPos--; // Move to next lower bit
        }
    }

    return id;
}

std::vector<std::vector<int>> idToBoard(std::uint64_t id, int size)
{
    std::vector<std::vector<int>> board(size, std::vector<int>(size, 0));
    int center = size / 2;

    // Count total bits needed
    int totalBits = 0;
    for (int row = 0; row <= center; row++)
    {
        totalBits += (center - row + 1);
    }

    int bitPos = totalBits - 1; // Start from MSB

    // Place bits and apply 8-fold symmetry for each bit
    for (int row = 0; row <= center; row++)
    {
        int colStart = row;
        int colEnd = center;

        for (int col = colStart; col <= colEnd; col++)
        {
            int bit = (id >> bitPos) & 1;

            if (bit == 1)
            {
                // Place in all 8 symmetric positions
                board[row][col] = 1;                       // Original
                board[row][size - 1 - col] = 1;            // Mirror across vertical axis
                board[size - 1 - row][col] = 1;            // Mirror across horizontal axis
                board[size - 1 - row][size - 1 - col] = 1; // Mirror across both axes
                board[col][row] = 1;                       // Mirror across main diagonal
                board[col][size - 1 - row] = 1;            // Diagonal + vertical
                board[size - 1 - col][row] = 1;            // Diagonal + horizontal
                board[size - 1 - col][size - 1 - row] = 1; // Diagonal + both
            }

            bitPos--;
        }
    }

    return board;
}