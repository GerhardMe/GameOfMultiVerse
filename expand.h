#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include "board_id.h"

class Database
{
private:
    sqlite3 *db;
    std::string dbPath;

    // Helper: Get board size from BoardID length
    int getBoardSizeFromId(const BoardID &id);

    // Helper: Get child ID byte count from parent BoardID
    int getChildIdBytes(const BoardID &parentId);

    // Helper: Get parent ID byte count from child BoardID
    int getParentIdBytes(const BoardID &childId);

public:
    Database(const std::string &path);
    ~Database();

    // Initialize database schema
    bool init();

    // Insert a new board (unexpanded, no parents)
    bool insertBoard(const BoardID &boardId, bool trueParent = false);

    // Check if board exists
    bool boardExists(const BoardID &boardId);

    // Get board flags
    bool isExpanded(const BoardID &boardId);
    bool isTrueParent(const BoardID &boardId);

    // === Children operations ===

    // Get specific evolution result (returns nullopt if not expanded)
    std::optional<BoardID> getEvolution(const BoardID &boardId, int rulesetId);

    // Get all evolutions (returns nullopt if not expanded)
    std::optional<std::vector<BoardID>> getAllEvolutions(const BoardID &boardId);

    // Mark board as expanded with all evolution results
    bool setEvolutions(const BoardID &boardId, const std::vector<BoardID> &evolutions);

    // === Parent operations ===

    // Add a parent to a board (handles size comparison and eviction)
    bool addParent(const BoardID &childId, const BoardID &parentId);

    // Get all parents of a board
    std::vector<BoardID> getParents(const BoardID &boardId);

    // Get parent count
    int getParentCount(const BoardID &boardId);

    // === Statistics ===

    int getTotalBoards();
    int getUnexpandedCount();

    // Get all unexpanded boards
    std::vector<BoardID> getUnexpandedBoards();
};

#endif // DATABASE_H