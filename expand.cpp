#include "expand.h"
#include "evolution.h"
#include "ruleset_id.h"
#include <iostream>

bool expandNode(Database &db, const BoardID &boardId)
{
    // Check if node exists and is not already expanded
    if (!db.boardExists(boardId))
    {
        std::cerr << "Board does not exist" << std::endl;
        return false;
    }

    if (db.isExpanded(boardId))
    {
        return false; // Already expanded
    }

    // Get board size from ID
    int idBytes = boardId.size();
    int boardSize = -1;
    for (int k = 0; k < 100; k++)
    {
        int bits = (k + 1) * (k + 2) / 2;
        int bytes = (bits + 7) / 8;
        if (bytes == idBytes)
        {
            boardSize = 2 * k + 1;
            break;
        }
    }

    if (boardSize < 0)
    {
        std::cerr << "Invalid board ID size" << std::endl;
        return false;
    }

    // Convert to board
    auto board = idToBoard(boardId, boardSize);

    // Pad once for evolution
    auto padded = padBoard(board);

    // Compute all 330 children
    int numRulesets = getTotalRulesets();
    std::vector<BoardID> children;
    children.reserve(numRulesets);

    for (int r = 0; r < numRulesets; r++)
    {
        Ruleset ruleset = idToRuleset(r);

        // Evolve
        auto evolved = evolveBoard(padded, ruleset);

        // Trim fully
        auto trimmed = trimBoardFully(evolved);

        // Convert to ID
        BoardID childId;
        if (trimmed.empty())
        {
            // Zero board - use single zero byte
            childId = {0};
        }
        else
        {
            childId = boardToId(trimmed);
        }

        children.push_back(childId);

        // Insert child into database if new
        if (!childId.empty() && childId[0] != 0)
        {
            if (!db.boardExists(childId))
            {
                db.insertBoard(childId, false);
            }

            // Register parent relationship
            db.addParent(childId, boardId);
        }
    }

    // Store all children for this node
    db.setEvolutions(boardId, children);

    return true;
}

int expandAllNodes(Database &db)
{
    int expanded = 0;

    while (true)
    {
        auto unexpandedBoards = db.getUnexpandedBoards();

        if (unexpandedBoards.empty())
            break;

        for (const auto &boardId : unexpandedBoards)
        {
            if (expandNode(db, boardId))
            {
                expanded++;
            }
        }
    }

    return expanded;
}