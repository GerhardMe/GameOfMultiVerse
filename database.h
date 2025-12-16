#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include "board_id.h"

struct ChildrenData
{
    bool expanded;
    bool fromShrink;
    std::vector<BoardID> evolutions; // 330 evolution results
};

class Database
{
private:
    sqlite3 *db;
    std::string dbPath;

    // Helper: Pack children data into MEDIUMBLOB
    std::vector<uint8_t> packChildren(const ChildrenData &data);

    // Helper: Unpack MEDIUMBLOB into children data
    ChildrenData unpackChildren(const std::vector<uint8_t> &blob);

public:
    Database(const std::string &path);
    ~Database();

    // Initialize database schema
    bool init();

    // Insert a new board (unexpanded)
    bool insertBoard(const BoardID &boardId, int generation, int size, bool fromShrink = false);

    // Check if board exists
    bool boardExists(const BoardID &boardId);

    // Get board metadata
    std::optional<std::pair<int, int>> getBoardInfo(const BoardID &boardId); // returns {generation, size}

    // Check if board is expanded
    bool isExpanded(const BoardID &boardId);

    // Get specific evolution result
    std::optional<BoardID> getEvolution(const BoardID &boardId, int rulesetId);

    // Get all evolutions (returns empty if not expanded)
    std::optional<std::vector<BoardID>> getAllEvolutions(const BoardID &boardId);

    // Mark board as expanded with all evolution results
    bool setEvolutions(const BoardID &boardId, const std::vector<BoardID> &evolutions);

    // Get all boards in a generation
    std::vector<BoardID> getBoardsByGeneration(int generation);

    // Get unexpanded boards count
    int getUnexpandedCount();

    // Statistics
    int getTotalBoards();
    int getMaxGeneration();
};

#endif // DATABASE_H