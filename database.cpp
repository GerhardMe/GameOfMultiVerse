#include "database.h"
#include <iostream>
#include <cstring>

Database::Database(const std::string &path) : db(nullptr), dbPath(path) {}

Database::~Database()
{
    if (db)
    {
        sqlite3_close(db);
    }
}

bool Database::init()
{
    // Open database
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Create table
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS boards (
            board_id BLOB PRIMARY KEY,
            generation INTEGER NOT NULL,
            size INTEGER NOT NULL,
            children MEDIUMBLOB
        );
        
        CREATE INDEX IF NOT EXISTS idx_generation ON boards(generation);
        CREATE INDEX IF NOT EXISTS idx_size ON boards(size);
    )";

    char *errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

std::vector<uint8_t> Database::packChildren(const ChildrenData &data)
{
    std::vector<uint8_t> blob;

    // First byte: metadata flags
    uint8_t metadata = 0;
    if (data.expanded)
        metadata |= 0x01;
    if (data.fromShrink)
        metadata |= 0x02;
    blob.push_back(metadata);

    if (!data.expanded || data.evolutions.empty())
    {
        return blob;
    }

    // Calculate child ID size from first evolution (all should be same size)
    int childIdBytes = data.evolutions[0].size();

    // Pack all 330 evolution IDs (all same size, no length prefix needed)
    for (const auto &evo : data.evolutions)
    {
        blob.insert(blob.end(), evo.begin(), evo.end());
    }

    return blob;
}

ChildrenData Database::unpackChildren(const std::vector<uint8_t> &blob)
{
    ChildrenData data;
    data.evolutions.resize(330);

    if (blob.empty())
    {
        data.expanded = false;
        data.fromShrink = false;
        return data;
    }

    // Read metadata byte
    uint8_t metadata = blob[0];
    data.expanded = (metadata & 0x01) != 0;
    data.fromShrink = (metadata & 0x02) != 0;

    if (!data.expanded || blob.size() <= 1)
    {
        return data;
    }

    // Calculate child ID size
    // Total data bytes = blob.size() - 1 (metadata byte)
    // childIdBytes = (blob.size() - 1) / 330
    int totalDataBytes = blob.size() - 1;
    int childIdBytes = totalDataBytes / 330;

    // Read 330 evolution IDs (all same size)
    size_t offset = 1;
    for (int i = 0; i < 330 && offset + childIdBytes <= blob.size(); i++)
    {
        data.evolutions[i].assign(blob.begin() + offset, blob.begin() + offset + childIdBytes);
        offset += childIdBytes;
    }

    return data;
}

bool Database::insertBoard(const BoardID &boardId, int generation, int size, bool fromShrink)
{
    const char *sql = "INSERT OR IGNORE INTO boards (board_id, generation, size, children) VALUES (?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Bind board_id
    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    // Bind generation and size
    sqlite3_bind_int(stmt, 2, generation);
    sqlite3_bind_int(stmt, 3, size);

    // Bind children (just metadata byte, unexpanded)
    uint8_t metadata = fromShrink ? 0x02 : 0x00;
    sqlite3_bind_blob(stmt, 4, &metadata, 1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool Database::boardExists(const BoardID &boardId)
{
    const char *sql = "SELECT 1 FROM boards WHERE board_id = ? LIMIT 1";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_ROW;
}

std::optional<std::pair<int, int>> Database::getBoardInfo(const BoardID &boardId)
{
    const char *sql = "SELECT generation, size FROM boards WHERE board_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int generation = sqlite3_column_int(stmt, 0);
        int size = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);
        return std::make_pair(generation, size);
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool Database::isExpanded(const BoardID &boardId)
{
    const char *sql = "SELECT children FROM boards WHERE board_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const void *data = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);

        if (size > 0)
        {
            uint8_t metadata = *static_cast<const uint8_t *>(data);
            sqlite3_finalize(stmt);
            return (metadata & 0x01) != 0;
        }
    }

    sqlite3_finalize(stmt);
    return false;
}

std::optional<BoardID> Database::getEvolution(const BoardID &boardId, int rulesetId)
{
    if (rulesetId < 0 || rulesetId >= 330)
    {
        return std::nullopt;
    }

    const char *sql = "SELECT children FROM boards WHERE board_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const void *data = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);

        std::vector<uint8_t> blob(static_cast<const uint8_t *>(data),
                                  static_cast<const uint8_t *>(data) + size);

        ChildrenData children = unpackChildren(blob);
        sqlite3_finalize(stmt);

        if (children.expanded && rulesetId < children.evolutions.size())
        {
            return children.evolutions[rulesetId];
        }
    }
    else
    {
        sqlite3_finalize(stmt);
    }

    return std::nullopt;
}

std::optional<std::vector<BoardID>> Database::getAllEvolutions(const BoardID &boardId)
{
    const char *sql = "SELECT children FROM boards WHERE board_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_blob(stmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const void *data = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);

        std::vector<uint8_t> blob(static_cast<const uint8_t *>(data),
                                  static_cast<const uint8_t *>(data) + size);

        ChildrenData children = unpackChildren(blob);
        sqlite3_finalize(stmt);

        if (children.expanded)
        {
            return children.evolutions;
        }
    }
    else
    {
        sqlite3_finalize(stmt);
    }

    return std::nullopt;
}

bool Database::setEvolutions(const BoardID &boardId, const std::vector<BoardID> &evolutions)
{
    if (evolutions.size() != 330)
    {
        std::cerr << "Must provide exactly 330 evolutions" << std::endl;
        return false;
    }

    // Get existing shrink flag
    const char *selectSql = "SELECT children FROM boards WHERE board_id = ?";
    sqlite3_stmt *selectStmt;
    bool fromShrink = false;

    if (sqlite3_prepare_v2(db, selectSql, -1, &selectStmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_blob(selectStmt, 1, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

        if (sqlite3_step(selectStmt) == SQLITE_ROW)
        {
            const void *data = sqlite3_column_blob(selectStmt, 0);
            int size = sqlite3_column_bytes(selectStmt, 0);

            if (size > 0)
            {
                uint8_t metadata = *static_cast<const uint8_t *>(data);
                fromShrink = (metadata & 0x02) != 0;
            }
        }
        sqlite3_finalize(selectStmt);
    }

    // Pack children with expanded flag
    ChildrenData data;
    data.expanded = true;
    data.fromShrink = fromShrink;
    data.evolutions = evolutions;

    std::vector<uint8_t> blob = packChildren(data);

    // Update database
    const char *updateSql = "UPDATE boards SET children = ? WHERE board_id = ?";
    sqlite3_stmt *updateStmt;
    int rc = sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_blob(updateStmt, 1, blob.data(), blob.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(updateStmt, 2, boardId.data(), boardId.size(), SQLITE_TRANSIENT);

    rc = sqlite3_step(updateStmt);
    sqlite3_finalize(updateStmt);

    return rc == SQLITE_DONE;
}

std::vector<BoardID> Database::getBoardsByGeneration(int generation)
{
    std::vector<BoardID> boards;

    const char *sql = "SELECT board_id FROM boards WHERE generation = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return boards;
    }

    sqlite3_bind_int(stmt, 1, generation);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const void *data = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);

        BoardID id(static_cast<const uint8_t *>(data),
                   static_cast<const uint8_t *>(data) + size);
        boards.push_back(id);
    }

    sqlite3_finalize(stmt);
    return boards;
}

int Database::getUnexpandedCount()
{
    const char *sql = "SELECT COUNT(*) FROM boards WHERE (children IS NULL OR LENGTH(children) = 1)";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalBoards()
{
    const char *sql = "SELECT COUNT(*) FROM boards";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int Database::getMaxGeneration()
{
    const char *sql = "SELECT MAX(generation) FROM boards";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return -1;
    }

    int maxGen = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        maxGen = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return maxGen;
}