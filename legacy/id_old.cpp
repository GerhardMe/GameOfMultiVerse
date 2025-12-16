#include <iostream>
#include <vector>
#include <cstdint>
#include <bitset>
#include <stdexcept>

using namespace std;

// ============================================================================
// RULESET STRUCTURE
// ============================================================================

struct Ruleset
{
    int underpopEnd; // [0, underpopEnd] -> death
    int birthStart;  // [birthStart, birthEnd] -> birth/survival
    int birthEnd;
    int overpopStart; // [overpopStart, 8] -> death

    bool operator==(const Ruleset &other) const
    {
        return underpopEnd == other.underpopEnd &&
               birthStart == other.birthStart &&
               birthEnd == other.birthEnd &&
               overpopStart == other.overpopStart;
    }
};

// ============================================================================
// RULESET ID CONVERSION
// ============================================================================

// Generate all valid rulesets in canonical order
vector<Ruleset> generateAllRulesets()
{
    vector<Ruleset> rulesets;

    for (int a = 0; a <= 7; a++)
    {
        for (int b = a + 1; b <= 8; b++)
        {
            for (int c = b; c <= 8; c++)
            {
                for (int d = c + 1; d <= 9; d++)
                {
                    rulesets.push_back({a, b, c, d});
                }
            }
        }
    }

    return rulesets;
}

// Convert ruleset to ID (its index in canonical ordering)
int rulesetToId(const Ruleset &ruleset)
{
    static vector<Ruleset> allRulesets = generateAllRulesets();

    for (size_t i = 0; i < allRulesets.size(); i++)
    {
        if (allRulesets[i] == ruleset)
        {
            return i;
        }
    }

    throw runtime_error("Invalid ruleset");
}

// Convert ID to ruleset
Ruleset idToRuleset(int id)
{
    static vector<Ruleset> allRulesets = generateAllRulesets();

    if (id < 0 || id >= (int)allRulesets.size())
    {
        throw runtime_error("Invalid ruleset ID");
    }

    return allRulesets[id];
}

// Get total number of valid rulesets
int getTotalRulesets()
{
    static vector<Ruleset> allRulesets = generateAllRulesets();
    return allRulesets.size();
}

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

void testRulesets()
{
    cout << "=== Ruleset ID System ===" << endl;

    int total = getTotalRulesets();
    cout << "Total valid rulesets: " << total << endl
         << endl;

    // Test a few specific rulesets
    Ruleset conway = {1, 3, 3, 4}; // Conway's Game of Life
    int conwayId = rulesetToId(conway);
    Ruleset restoredConway = idToRuleset(conwayId);

    cout << "Conway's Game of Life:" << endl;
    cout << "  Ruleset: {" << conway.underpopEnd << ", " << conway.birthStart
         << ", " << conway.birthEnd << ", " << conway.overpopStart << "}" << endl;
    cout << "  ID: " << conwayId << endl;
    cout << "  Restored: {" << restoredConway.underpopEnd << ", " << restoredConway.birthStart
         << ", " << restoredConway.birthEnd << ", " << restoredConway.overpopStart << "}" << endl;
    cout << "  Match: " << (conway == restoredConway ? "YES" : "NO") << endl
         << endl;

    // Show first and last few rulesets
    cout << "First 5 rulesets:" << endl;
    for (int i = 0; i < 5; i++)
    {
        Ruleset r = idToRuleset(i);
        cout << "  ID " << i << ": {" << r.underpopEnd << ", " << r.birthStart
             << ", " << r.birthEnd << ", " << r.overpopStart << "}" << endl;
    }

    cout << "\nLast 5 rulesets:" << endl;
    for (int i = total - 5; i < total; i++)
    {
        Ruleset r = idToRuleset(i);
        cout << "  ID " << i << ": {" << r.underpopEnd << ", " << r.birthStart
             << ", " << r.birthEnd << ", " << r.overpopStart << "}" << endl;
    }
    cout << endl;
}

int main()
{
    // Test boards
    vector<vector<int>> board1 = {{1}};
    testBoard(board1, "Test 1: 1x1");

    vector<vector<int>> board2 = {
        {0, 1, 0},
        {1, 0, 1},
        {0, 1, 0}};
    testBoard(board2, "Test 2: 3x3");

    vector<vector<int>> board3 = {
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {1, 1, 0, 1, 1},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0}};
    testBoard(board3, "Test 3: 5x5");

    vector<vector<int>> board4 = {
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {1, 1, 1, 0, 1, 1, 1},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0}};
    testBoard(board4, "Test 4: 7x7");

    // Test rulesets
    testRulesets();

    return 0;
}