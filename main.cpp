#include <iostream>
#include <bitset>
#include <cstdint>
#include <vector>
#include <string>
#include "board_id.h"
#include "ruleset_id.h"

using namespace std;

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

    // Test Conway's Game of Life
    Ruleset conway = {1, 3, 3, 4};
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