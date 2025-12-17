#include "evolution.h"

namespace
{
    int countNeighbors(const std::vector<std::vector<int>> &board, int row, int col)
    {
        int count = 0;
        int size = board.size();

        for (int dr = -1; dr <= 1; dr++)
        {
            for (int dc = -1; dc <= 1; dc++)
            {
                if (dr == 0 && dc == 0)
                    continue;

                int nr = row + dr;
                int nc = col + dc;

                if (nr >= 0 && nr < size && nc >= 0 && nc < size)
                {
                    count += board[nr][nc];
                }
            }
        }

        return count;
    }

    int nextState(int currentState, int neighbors, const Ruleset &ruleset)
    {
        if (neighbors <= ruleset.underpopEnd)
            return 0;

        if (neighbors >= ruleset.overpopStart)
            return 0;

        if (neighbors >= ruleset.birthStart && neighbors <= ruleset.birthEnd)
            return 1;

        return currentState;
    }
}

std::vector<std::vector<int>> padBoard(const std::vector<std::vector<int>> &board)
{
    int oldSize = board.size();
    int newSize = oldSize + 2;

    std::vector<std::vector<int>> padded(newSize, std::vector<int>(newSize, 0));

    for (int r = 0; r < oldSize; r++)
    {
        for (int c = 0; c < oldSize; c++)
        {
            padded[r + 1][c + 1] = board[r][c];
        }
    }

    return padded;
}

std::vector<std::vector<int>> evolveBoard(
    const std::vector<std::vector<int>> &board,
    const Ruleset &ruleset)
{
    int size = board.size();
    std::vector<std::vector<int>> result(size, std::vector<int>(size, 0));

    for (int r = 0; r < size; r++)
    {
        for (int c = 0; c < size; c++)
        {
            int neighbors = countNeighbors(board, r, c);
            result[r][c] = nextState(board[r][c], neighbors, ruleset);
        }
    }

    return result;
}

bool canTrim(const std::vector<std::vector<int>> &board)
{
    int size = board.size();

    if (size <= 1)
        return false;

    // Check top and bottom rows
    for (int c = 0; c < size; c++)
    {
        if (board[0][c] != 0 || board[size - 1][c] != 0)
            return false;
    }

    // Check left and right columns (excluding corners already checked)
    for (int r = 1; r < size - 1; r++)
    {
        if (board[r][0] != 0 || board[r][size - 1] != 0)
            return false;
    }

    return true;
}

std::vector<std::vector<int>> trimBoard(const std::vector<std::vector<int>> &board)
{
    int oldSize = board.size();

    if (oldSize <= 2)
        return {}; // Board disappears

    int newSize = oldSize - 2;
    std::vector<std::vector<int>> trimmed(newSize, std::vector<int>(newSize, 0));

    for (int r = 0; r < newSize; r++)
    {
        for (int c = 0; c < newSize; c++)
        {
            trimmed[r][c] = board[r + 1][c + 1];
        }
    }

    return trimmed;
}

std::vector<std::vector<int>> trimBoardFully(const std::vector<std::vector<int>> &board)
{
    if (board.empty())
        return {};

    if (!canTrim(board))
        return board;

    auto trimmed = trimBoard(board);

    if (trimmed.empty())
        return {}; // Zero board

    return trimBoardFully(trimmed);
}