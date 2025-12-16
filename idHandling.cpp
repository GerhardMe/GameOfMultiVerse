#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include <sstream>
#include <cstdint>
#include <bitset>

using namespace std;

// ============================================================================
// BOARD ID SYSTEM
// ============================================================================

// Convert board to ID by extracting one octant (diagonal triangle from center)
// Board must be square and odd-sized with 8-fold symmetry
uint64_t boardToId(const vector<vector<int>> &board)
{
    if (board.empty() || board.size() != board[0].size())
    {
        throw runtime_error("Board must be square");
    }

    int size = board.size();
    if (size % 2 == 0)
    {
        throw runtime_error("Board size must be odd");
    }

    int center = size / 2;
    uint64_t id = 0;
    int bitPosition = 0;

    // Special case: 1x1 board (just the center)
    if (size == 1)
    {
        return board[0][0];
    }

    // Extract diagonal triangle from center to top-left corner
    // For each diagonal distance d from center (0 to center),
    // extract cells from (center-d, center-d) to (center-d, center)
    for (int d = 0; d <= center; d++)
    {
        int row = center - d;
        for (int col = center - d; col <= center; col++)
        {
            if (board[row][col] == 1)
            {
                id |= (1ULL << bitPosition);
            }
            bitPosition++;
        }
    }

    return id;
}

// Convert ID back to full board using 8-fold symmetry
vector<vector<int>> idToBoard(uint64_t id, int size)
{
    if (size % 2 == 0)
    {
        throw runtime_error("Board size must be odd");
    }

    vector<vector<int>> board(size, vector<int>(size, 0));
    int center = size / 2;
    int bitPosition = 0;

    // Special case: 1x1 board
    if (size == 1)
    {
        board[0][0] = id & 1;
        return board;
    }

    // Read diagonal triangle
    for (int d = 0; d <= center; d++)
    {
        int row = center - d;
        for (int col = center - d; col <= center; col++)
        {
            int bit = (id >> bitPosition) & 1;
            board[row][col] = bit;
            bitPosition++;
        }
    }

    // Apply 8-fold symmetry
    // 1. Mirror horizontally (left-right across vertical center line)
    for (int i = 0; i <= center; i++)
    {
        for (int j = 0; j <= center; j++)
        {
            board[i][size - 1 - j] = board[i][j];
        }
    }

    // 2. Mirror vertically (top-bottom across horizontal center line)
    for (int i = 0; i <= center; i++)
    {
        for (int j = 0; j < size; j++)
        {
            board[size - 1 - i][j] = board[i][j];
        }
    }

    return board;
}

// ============================================================================
// RULESET ID SYSTEM
// ============================================================================

struct Ruleset
{
    int underpopEnd; // [0, underpopEnd] -> death
    int birthStart;  // [birthStart, birthEnd] -> life
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

// Generate all valid rulesets in deterministic order
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

// Convert ruleset to ID (its index in the canonical ordering)
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

// Get total number of rulesets
int getTotalRulesets()
{
    static vector<Ruleset> allRulesets = generateAllRulesets();
    return allRulesets.size();
}

// ============================================================================
// UTILITY FUNCTIONS
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

string boardToString(const vector<vector<int>> &board)
{
    stringstream ss;
    for (const auto &row : board)
    {
        for (int cell : row)
        {
            ss << cell;
        }
        ss << "\n";
    }
    return ss.str();
}

// ============================================================================
// TESTING
// ============================================================================

void testBoardConversion()
{
    cout << "Testing board ID conversion..." << endl;

    // Test 1: Simple 1x1 board
    cout << "\n=== Test 1: 1x1 board ===" << endl;
    vector<vector<int>> board1 = {{1}};
    cout << "Original:" << endl;
    printBoard(board1);
    uint64_t id1 = boardToId(board1);
    cout << "ID: " << id1 << " (binary: " << bitset<4>(id1) << ")" << endl;
    vector<vector<int>> restored1 = idToBoard(id1, 1);
    cout << "Restored:" << endl;
    printBoard(restored1);
    assert(board1 == restored1);
    cout << "✓ 1x1 board test passed" << endl;

    // Test 2: 3x3 board
    cout << "\n=== Test 2: 3x3 board ===" << endl;
    vector<vector<int>> board2 = {
        {0, 1, 0},
        {1, 0, 1},
        {0, 1, 0}};
    cout << "Original:" << endl;
    printBoard(board2);
    uint64_t id2 = boardToId(board2);
    cout << "ID: " << id2 << " (binary: " << bitset<8>(id2) << ")" << endl;
    vector<vector<int>> restored2 = idToBoard(id2, 3);
    cout << "Restored:" << endl;
    printBoard(restored2);
    if (board2 != restored2)
    {
        cout << "ERROR: Boards don't match!" << endl;
    }
    assert(board2 == restored2);
    cout << "✓ 3x3 board test passed" << endl;

    // Test 3: 5x5 board from your example
    cout << "\n=== Test 3: 5x5 board ===" << endl;
    vector<vector<int>> board3 = {
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {1, 1, 0, 1, 1},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0}};
    cout << "Original:" << endl;
    printBoard(board3);
    uint64_t id3 = boardToId(board3);
    cout << "ID: " << id3 << " (binary: " << bitset<16>(id3) << ")" << endl;
    vector<vector<int>> restored3 = idToBoard(id3, 5);
    cout << "Restored:" << endl;
    printBoard(restored3);
    if (board3 != restored3)
    {
        cout << "ERROR: Boards don't match!" << endl;
    }
    assert(board3 == restored3);
    cout << "✓ 5x5 board test passed" << endl;

    // Test 4: All zeros
    cout << "\n=== Test 4: 5x5 all zeros ===" << endl;
    vector<vector<int>> board4 = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0}};
    cout << "Original:" << endl;
    printBoard(board4);
    uint64_t id4 = boardToId(board4);
    cout << "ID: " << id4 << " (binary: " << bitset<16>(id4) << ")" << endl;
    vector<vector<int>> restored4 = idToBoard(id4, 5);
    cout << "Restored:" << endl;
    printBoard(restored4);
    assert(board4 == restored4);
    assert(id4 == 0);
    cout << "✓ All-zeros board test passed" << endl;
}

void testRulesetConversion()
{
    cout << "\nTesting ruleset ID conversion..." << endl;

    int totalRulesets = getTotalRulesets();
    cout << "Total rulesets: " << totalRulesets << endl;

    // Test all rulesets can be converted back and forth
    for (int id = 0; id < totalRulesets; id++)
    {
        Ruleset ruleset = idToRuleset(id);
        int convertedId = rulesetToId(ruleset);
        assert(id == convertedId);
    }
    cout << "✓ All " << totalRulesets << " rulesets convert correctly" << endl;

    // Test specific rulesets
    Ruleset conway = {1, 3, 3, 4}; // Conway's rules: die if <2 or >3, birth at 3
    int conwayId = rulesetToId(conway);
    Ruleset restoredConway = idToRuleset(conwayId);
    assert(conway == restoredConway);
    cout << "✓ Conway's Game of Life ruleset test passed (ID: " << conwayId << ")" << endl;

    // Print first few rulesets
    cout << "\nFirst 10 rulesets:" << endl;
    for (int i = 0; i < min(10, totalRulesets); i++)
    {
        Ruleset r = idToRuleset(i);
        cout << "ID " << i << ": underpop≤" << r.underpopEnd
             << ", birth=" << r.birthStart << "-" << r.birthEnd
             << ", overpop≥" << r.overpopStart << endl;
    }
}

int main()
{
    try
    {
        testBoardConversion();
        testRulesetConversion();

        cout << "\n✅ All tests passed!" << endl;
    }
    catch (const exception &e)
    {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}