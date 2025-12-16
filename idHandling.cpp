#include <iostream>
#include <vector>
#include <cstdint>
#include <bitset>

using namespace std;

// ============================================================================
// BOARD ID CONVERSION
// ============================================================================

// Convert board to ID by extracting diagonal octant
// Boards must be square, odd-sized, and have 8-fold symmetry
uint64_t boardToId(const vector<vector<int>> &board)
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

// Convert ID back to full board with 8-fold symmetry
vector<vector<int>> idToBoard(uint64_t id, int size)
{
    vector<vector<int>> board(size, vector<int>(size, 0));
    int center = size / 2;

    // Count total bits
    int totalBits = 0;
    for (int row = 0; row <= center; row++)
    {
        totalBits += (center - row + 1);
    }

    int bitPos = totalBits - 1; // Start from MSB

    // Place bits back into diagonal triangle
    for (int row = 0; row <= center; row++)
    {
        int colStart = row;
        int colEnd = center;

        for (int col = colStart; col <= colEnd; col++)
        {
            int bit = (id >> bitPos) & 1;
            board[row][col] = bit;
            bitPos--;
        }
    }

    // Apply 8-fold symmetry
    // 1. Mirror left-to-right (vertical axis)
    for (int row = 0; row <= center; row++)
    {
        for (int col = 0; col <= center; col++)
        {
            board[row][size - 1 - col] = board[row][col];
        }
    }

    // 2. Mirror top-to-bottom (horizontal axis)
    for (int row = 0; row <= center; row++)
    {
        for (int col = 0; col < size; col++)
        {
            board[size - 1 - row][col] = board[row][col];
        }
    }

    return board;
}

// ============================================================================
// UTILITY
// ============================================================================

void printBoard(const vector<vector<int>> &board)
{
    for (const auto &row : board)
    {
        for (int cell : row)
        {
            cout << cell;
        }
        cout << endl;
    }
}

// ============================================================================
// TESTS
// ============================================================================

void testBoard(const vector<vector<int>> &original, const string &name)
{
    cout << "=== " << name << " ===" << endl;
    cout << "Original:" << endl;
    printBoard(original);

    uint64_t id = boardToId(original);
    int numBits = ((original.size() / 2) + 1) * ((original.size() / 2) + 2) / 2;
    cout << "\nID: " << id;
    cout << " (binary: ";
    for (int i = numBits - 1; i >= 0; i--)
    {
        cout << ((id >> i) & 1);
    }
    cout << ")" << endl;

    vector<vector<int>> restored = idToBoard(id, original.size());
    cout << "\nRestored:" << endl;
    printBoard(restored);
    cout << endl;
}

int main()
{
    // Test 1: 1x1
    vector<vector<int>> board1 = {{1}};
    testBoard(board1, "Test 1: 1x1");

    // Test 2: 3x3
    vector<vector<int>> board2 = {
        {0, 1, 0},
        {1, 0, 1},
        {0, 1, 0}};
    testBoard(board2, "Test 2: 3x3");

    // Test 3: 5x5
    vector<vector<int>> board3 = {
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {1, 1, 0, 1, 1},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0}};
    testBoard(board3, "Test 3: 5x5");

    // Test 4: 7x7
    vector<vector<int>> board4 = {
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {1, 1, 1, 0, 1, 1, 1},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0}};
    testBoard(board4, "Test 4: 7x7");

    return 0;
}