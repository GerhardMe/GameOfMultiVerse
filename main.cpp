#include <iostream>
#include <vector>
#include "board_id.h"
#include "ruleset_id.h"
#include "evolution.h"
#include "database.h"
#include "expand.h"

using namespace std;

void printBoard(const vector<vector<int>> &board)
{
    for (const auto &row : board)
    {
        for (int cell : row)
        {
            cout << (cell ? "#" : ".");
        }
        cout << endl;
    }
}

void printBoardId(const BoardID &id)
{
    for (size_t i = 0; i < id.size(); i++)
    {
        printf("%02x", id[i]);
    }
}

int main()
{
    // Initialize ruleset system
    initRulesets();
    cout << "Initialized " << getTotalRulesets() << " rulesets" << endl;

    // Create database
    Database db("multiverse.db");
    if (!db.init())
    {
        cerr << "Failed to initialize database" << endl;
        return 1;
    }
    cout << "Database initialized" << endl;

    // Create seed board: the 1x1 board with single live cell
    vector<vector<int>> seedBoard = {{1}};
    BoardID seedId = boardToId(seedBoard);

    cout << "\n=== Seed Board ===" << endl;
    printBoard(seedBoard);
    cout << "ID: ";
    printBoardId(seedId);
    cout << endl;

    // Insert seed if not exists
    if (!db.boardExists(seedId))
    {
        db.insertBoard(seedId, true); // true = this is a "true parent" (root)
        cout << "Inserted seed board" << endl;
    }
    else
    {
        cout << "Seed board already exists" << endl;
    }

    // Show initial stats
    cout << "\n=== Initial Stats ===" << endl;
    cout << "Total boards: " << db.getTotalBoards() << endl;
    cout << "Unexpanded: " << db.getUnexpandedCount() << endl;

    // Expand a few generations
    int maxGenerations = 3;
    for (int gen = 0; gen < maxGenerations; gen++)
    {
        cout << "\n=== Generation " << gen << " ===" << endl;

        auto unexpanded = db.getUnexpandedBoards();
        cout << "Boards to expand: " << unexpanded.size() << endl;

        if (unexpanded.empty())
        {
            cout << "No more boards to expand" << endl;
            break;
        }

        int expanded = 0;
        for (const auto &boardId : unexpanded)
        {
            if (expandNode(db, boardId))
            {
                expanded++;
            }
        }

        cout << "Expanded " << expanded << " boards" << endl;
        cout << "Total boards now: " << db.getTotalBoards() << endl;
        cout << "Unexpanded now: " << db.getUnexpandedCount() << endl;
    }

    // Show some results
    cout << "\n=== Final Stats ===" << endl;
    cout << "Total boards: " << db.getTotalBoards() << endl;
    cout << "Unexpanded: " << db.getUnexpandedCount() << endl;

    // Show children of seed board
    cout << "\n=== Children of Seed Board ===" << endl;
    auto children = db.getAllEvolutions(seedId);
    if (children)
    {
        // Count unique children
        vector<BoardID> unique;
        for (const auto &child : *children)
        {
            bool found = false;
            for (const auto &u : unique)
            {
                if (u == child)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                unique.push_back(child);
            }
        }

        cout << "Total children: " << children->size() << endl;
        cout << "Unique children: " << unique.size() << endl;

        cout << "\nFirst 5 unique children:" << endl;
        for (int i = 0; i < min(5, (int)unique.size()); i++)
        {
            cout << "  Child " << i << ": ";
            printBoardId(unique[i]);

            // Check if it's the zero board
            if (unique[i].size() == 1 && unique[i][0] == 0)
            {
                cout << " (zero board)";
            }
            else
            {
                // Derive size and show board
                int bytes = unique[i].size();
                int boardSize = -1;
                for (int k = 0; k < 20; k++)
                {
                    int bits = (k + 1) * (k + 2) / 2;
                    if ((bits + 7) / 8 == bytes)
                    {
                        boardSize = 2 * k + 1;
                        break;
                    }
                }
                if (boardSize > 0)
                {
                    cout << " (" << boardSize << "x" << boardSize << ")";
                }
            }
            cout << endl;
        }
    }
    else
    {
        cout << "Seed board not expanded yet" << endl;
    }

    return 0;
}