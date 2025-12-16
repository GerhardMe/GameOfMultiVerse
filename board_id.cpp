#include "board_id.h"

// Helper: Set a bit in a byte vector
void setBit(BoardID &id, int bitPos)
{
    int byteIndex = bitPos / 8;
    int bitIndex = bitPos % 8;

    // Ensure vector is large enough
    while (id.size() <= byteIndex)
    {
        id.push_back(0);
    }

    id[byteIndex] |= (1 << bitIndex);
}

// Helper: Get a bit from a byte vector
int getBit(const BoardID &id, int bitPos)
{
    int byteIndex = bitPos / 8;
    int bitIndex = bitPos % 8;

    if (byteIndex >= id.size())
    {
        return 0;
    }

    return (id[byteIndex] >> bitIndex) & 1;
}

BoardID boardToId(const std::vector<std::vector<int>> &board)
{
    int size = board.size();
    int center = size / 2;

    // Count total bits needed
    int totalBits = 0;
    for (int row = 0; row <= center; row++)
    {
        totalBits += (center - row + 1);
    }

    // Calculate required bytes
    int numBytes = (totalBits + 7) / 8;
    BoardID id(numBytes, 0);

    int bitPos = totalBits - 1; // Start from MSB

    // Extract bits from board
    for (int row = 0; row <= center; row++)
    {
        int colStart = row;
        int colEnd = center;

        for (int col = colStart; col <= colEnd; col++)
        {
            if (board[row][col] == 1)
            {
                setBit(id, bitPos);
            }
            bitPos--;
        }
    }

    return id;
}

std::vector<std::vector<int>> idToBoard(const BoardID &id, int size)
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

    // Place bits and apply 8-fold symmetry
    for (int row = 0; row <= center; row++)
    {
        int colStart = row;
        int colEnd = center;

        for (int col = colStart; col <= colEnd; col++)
        {
            int bit = getBit(id, bitPos);

            if (bit == 1)
            {
                // Place in all 8 symmetric positions
                board[row][col] = 1;
                board[row][size - 1 - col] = 1;
                board[size - 1 - row][col] = 1;
                board[size - 1 - row][size - 1 - col] = 1;
                board[col][row] = 1;
                board[col][size - 1 - row] = 1;
                board[size - 1 - col][row] = 1;
                board[size - 1 - col][size - 1 - row] = 1;
            }

            bitPos--;
        }
    }

    return board;
}