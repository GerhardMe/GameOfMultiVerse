#include "database.h"
#include "ruleset_id.h"
#include <iostream>
#include <algorithm>
#include <cmath>

Database::Database(const std::string &path) : db(nullptr), dbPath(path) {}

Database::~Database()
{
    if (db)
        sqlite3_close(db);
}

bool Database::init()
{
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS boards (
            board_id BLOB PRIMARY KEY,
            expanded BOOLEAN DEFAULT 0,
            true_parent BOOLEAN DEFAULT 0,
            parent_size INTEGER DEFAULT 0,
            parents BLOB,
            children BLOB
        );
    )";

    char *errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int Database::getBoardSizeFromId(const BoardID &id)
{
    // BoardID has (k+1)(k+2)/2 bits for board size N = 2k+1
    // Bytes = ceil(bits/8)
    // Given bytes, find k, then N = 2k+1
    int bytes = id.size();
    int bits = bytes * 8; // Upper bound

    // Find k where (k+1)(k+2)/2 <= bits*8 and fits in 'bytes' bytes
    // Solve: (k+1)(k+2)/2 = bits => k^2 + 3k + 2 = 2*bits
    // k = (-3 + sqrt(9 + 8*bits - 8)) / 2 = (-3 + sqrt(1 + 8*bits)) / 2

    // We need to find k such that ceil((k+1)(k+2)/2 / 8) = bytes
    for (int k = 0; k < 100; k++)
    {
        int numBits = (k + 1) * (k + 2) / 2;
        int numBytes = (numBits + 7) / 8;
        if (numBytes == bytes)
        {
            return 2 * k + 1; // N = 2k+1
        }
    }
    return -1; // Invalid
}

int Database::getChildIdBytes(const BoardID &parentId)
{
    // Parent: N = 2k+1, Child: N' = 2(k+1)+1 = 2k+3
    // Child bits = (k+2)(k+3)/2
    int parentSize = getBoardSizeFromId(parentId);
    int k = (parentSize - 1) / 2;
    int childBits = (k + 2) * (k + 3) / 2;
    return (childBits + 7) / 8;
}

int Database::getParentIdBytes(const BoardID &childId)
{
    // Child: N = 2k+1, Parent: N' = 2(k-1)+1 = 2k-1
    // Parent bits = k(k+1)/2
    int childSize = getBoardSizeFromId(childId);
    int k = (childSize - 1) / 2;
    if (k <= 0)
        return 0; // No valid parent (1x1 board)
    int parentBits = k * (k + 1) / 2;
    return (parentBits + 7) / 8;
}

bool Database::insertBoard(const BoardID &boardId, bool trueParent)
{
    const char *sql = "INSERT OR IGNORE INTO boards (board_id, expanded, true_parent, parent_size) VALUES (?, 0, ?, 0)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, trueParent ? 1 : 0);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool Database::boardExists(const BoardID &boardId)
{
    const char *sql = "SELECT 1 FROM boards WHERE board_id = ? LIMIT 1";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

bool Database::isExpanded(const BoardID &boardId)
{
    const char *sql = "SELECT expanded FROM boards WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    bool expanded = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        expanded = (sqlite3_column_int(stmt, 0) != 0);
    }

    sqlite3_finalize(stmt);
    return expanded;
}

bool Database::isTrueParent(const BoardID &boardId)
{
    const char *sql = "SELECT true_parent FROM boards WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    bool trueParent = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        trueParent = (sqlite3_column_int(stmt, 0) != 0);
    }

    sqlite3_finalize(stmt);
    return trueParent;
}

std::optional<BoardID> Database::getEvolution(const BoardID &boardId, int rulesetId)
{
    int numRulesets = getTotalRulesets();
    if (rulesetId < 0 || rulesetId >= numRulesets)
        return std::nullopt;

    const char *sql = "SELECT expanded, children FROM boards WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    std::optional<BoardID> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        bool expanded = (sqlite3_column_int(stmt, 0) != 0);
        if (expanded)
        {
            const void *data = sqlite3_column_blob(stmt, 1);
            int blobSize = sqlite3_column_bytes(stmt, 1);

            int childBytes = getChildIdBytes(boardId);
            int expectedSize = numRulesets * childBytes;

            if (blobSize == expectedSize)
            {
                const uint8_t *blobData = static_cast<const uint8_t *>(data);
                int offset = rulesetId * childBytes;
                result = BoardID(blobData + offset, blobData + offset + childBytes);
            }
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<std::vector<BoardID>> Database::getAllEvolutions(const BoardID &boardId)
{
    const char *sql = "SELECT expanded, children FROM boards WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    std::optional<std::vector<BoardID>> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        bool expanded = (sqlite3_column_int(stmt, 0) != 0);
        if (expanded)
        {
            const void *data = sqlite3_column_blob(stmt, 1);
            int blobSize = sqlite3_column_bytes(stmt, 1);

            int numRulesets = getTotalRulesets();
            int childBytes = getChildIdBytes(boardId);
            int expectedSize = numRulesets * childBytes;

            if (blobSize == expectedSize)
            {
                const uint8_t *blobData = static_cast<const uint8_t *>(data);
                std::vector<BoardID> evolutions;
                evolutions.reserve(numRulesets);

                for (int i = 0; i < numRulesets; i++)
                {
                    int offset = i * childBytes;
                    evolutions.emplace_back(blobData + offset, blobData + offset + childBytes);
                }
                result = std::move(evolutions);
            }
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

bool Database::setEvolutions(const BoardID &boardId, const std::vector<BoardID> &evolutions)
{
    int numRulesets = getTotalRulesets();

    if (evolutions.size() != numRulesets)
    {
        std::cerr << "Must provide exactly " << numRulesets << " evolutions" << std::endl;
        return false;
    }

    int childBytes = getChildIdBytes(boardId);

    // Pack children into blob
    std::vector<uint8_t> blob;
    blob.reserve(numRulesets * childBytes);

    for (const auto &evo : evolutions)
    {
        if (evo.size() != childBytes)
        {
            std::cerr << "Child ID size mismatch: expected " << childBytes
                      << ", got " << evo.size() << std::endl;
            return false;
        }
        blob.insert(blob.end(), evo.begin(), evo.end());
    }

    // Update database
    const char *sql = "UPDATE boards SET expanded = 1, children = ? WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_blob(stmt, 1, blob.data(), blob.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool Database::addParent(const BoardID &childId, const BoardID &parentId)
{
    int numRulesets = getTotalRulesets();
    int newParentSize = parentId.size();

    // Get current parent data
    const char *selectSql = "SELECT parent_size, parents FROM boards WHERE board_id = ?";
    sqlite3_stmt *selectStmt;

    if (sqlite3_prepare_v2(db, selectSql, -1, &selectStmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_blob(selectStmt, 1, childId.data(), childId.size(), SQLITE_TRANSIENT);

    int currentParentSize = 0;
    std::vector<BoardID> currentParents;

    if (sqlite3_step(selectStmt) == SQLITE_ROW)
    {
        currentParentSize = sqlite3_column_int(selectStmt, 0);

        const void *data = sqlite3_column_blob(selectStmt, 1);
        int blobSize = sqlite3_column_bytes(selectStmt, 1);

        if (currentParentSize > 0 && blobSize > 0)
        {
            const uint8_t *blobData = static_cast<const uint8_t *>(data);
            int parentCount = blobSize / currentParentSize;

            for (int i = 0; i < parentCount; i++)
            {
                int offset = i * currentParentSize;
                // Store actual size, not padded size
                BoardID parent(blobData + offset, blobData + offset + currentParentSize);
                // Trim trailing zeros to get actual ID
                while (!parent.empty() && parent.back() == 0)
                {
                    parent.pop_back();
                }
                if (!parent.empty())
                {
                    currentParents.push_back(parent);
                }
            }
        }
    }
    else
    {
        sqlite3_finalize(selectStmt);
        return false; // Board not found
    }
    sqlite3_finalize(selectStmt);

    // Check for duplicate
    for (const auto &p : currentParents)
    {
        if (p == parentId)
            return true; // Already exists, success
    }

    int currentCount = currentParents.size();

    // Determine action based on current state
    int newMaxSize = currentParentSize;
    bool shouldAdd = false;
    int evictIndex = -1;

    if (currentCount < numRulesets)
    {
        // Room available
        shouldAdd = true;
        if (newParentSize > currentParentSize)
        {
            newMaxSize = newParentSize;
        }
    }
    else
    {
        // Full - check if we should evict
        // Parents are sorted largest-first, so first one is largest
        if (!currentParents.empty() && newParentSize < currentParents[0].size())
        {
            shouldAdd = true;
            evictIndex = 0; // Evict the largest
        }
    }

    if (!shouldAdd)
        return true; // Can't add, but not an error

    // Build new parent list
    std::vector<BoardID> newParents;
    if (evictIndex >= 0)
    {
        // Copy all except evicted
        for (int i = 0; i < currentParents.size(); i++)
        {
            if (i != evictIndex)
            {
                newParents.push_back(currentParents[i]);
            }
        }
    }
    else
    {
        newParents = currentParents;
    }
    newParents.push_back(parentId);

    // Sort by size descending (largest first)
    std::sort(newParents.begin(), newParents.end(),
              [](const BoardID &a, const BoardID &b)
              {
                  return a.size() > b.size();
              });

    // Determine new max size (largest in list)
    newMaxSize = newParents.empty() ? 0 : newParents[0].size();

    // Pack into blob with padding
    std::vector<uint8_t> blob;
    blob.reserve(newParents.size() * newMaxSize);

    for (const auto &p : newParents)
    {
        // Add parent with padding to newMaxSize
        blob.insert(blob.end(), p.begin(), p.end());
        // Pad with zeros
        for (int i = p.size(); i < newMaxSize; i++)
        {
            blob.push_back(0);
        }
    }

    // Update database
    const char *updateSql = "UPDATE boards SET parent_size = ?, parents = ? WHERE board_id = ?";
    sqlite3_stmt *updateStmt;

    if (sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(updateStmt, 1, newMaxSize);
    sqlite3_bind_blob(updateStmt, 2, blob.data(), blob.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(updateStmt, 3, childId.data(), childId.size(), SQLITE_TRANSIENT);

    bool success = (sqlite3_step(updateStmt) == SQLITE_DONE);
    sqlite3_finalize(updateStmt);
    return success;
}

std::vector<BoardID> Database::getParents(const BoardID &boardId)
{
    std::vector<BoardID> parents;

    const char *sql = "SELECT parent_size, parents FROM boards WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return parents;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int parentSize = sqlite3_column_int(stmt, 0);
        const void *data = sqlite3_column_blob(stmt, 1);
        int blobSize = sqlite3_column_bytes(stmt, 1);

        if (parentSize > 0 && blobSize > 0)
        {
            const uint8_t *blobData = static_cast<const uint8_t *>(data);
            int parentCount = blobSize / parentSize;

            for (int i = 0; i < parentCount; i++)
            {
                int offset = i * parentSize;
                BoardID parent(blobData + offset, blobData + offset + parentSize);
                // Trim trailing zeros
                while (!parent.empty() && parent.back() == 0)
                {
                    parent.pop_back();
                }
                if (!parent.empty())
                {
                    parents.push_back(parent);
                }
            }
        }
    }

    sqlite3_finalize(stmt);
    return parents;
}

int Database::getParentCount(const BoardID &boardId)
{
    const char *sql = "SELECT parent_size, LENGTH(parents) FROM boards WHERE board_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return 0;

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int parentSize = sqlite3_column_int(stmt, 0);
        int blobSize = sqlite3_column_int(stmt, 1);
        if (parentSize > 0)
        {
            count = blobSize / parentSize;
        }
    }

    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalBoards()
{
    const char *sql = "SELECT COUNT(*) FROM boards";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;

    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int Database::getUnexpandedCount()
{
    const char *sql = "SELECT COUNT(*) FROM boards WHERE expanded = 0";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;

    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}