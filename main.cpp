#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <set>
#include <map>
using namespace std;
namespace fs = std::filesystem;

struct UniverseRules
{
    int underpopEnd;
    int birthStart;
    int birthEnd;
    int overpopStart;

    string toSeed() const
    {
        return to_string(underpopEnd) + to_string(birthStart) +
               to_string(birthEnd) + to_string(overpopStart);
    }
};

struct BoardState
{
    vector<vector<int>> board;
    string parentSeed;
    vector<string> seeds;
    string cloneInfo;

    void saveToFile(const string &filepath) const
    {
        ofstream file(filepath);

        if (!cloneInfo.empty())
        {
            file << "CLONE OF " << cloneInfo << "\n";
        }
        else
        {
            file << "VALID\n";
        }

        file << "parent: " << parentSeed << "\n\n";

        for (const auto &row : board)
        {
            for (int cell : row)
            {
                file << cell;
            }
            file << "\n";
        }
        file << "\n";

        for (const auto &seed : seeds)
        {
            file << seed << "\n";
        }
        file.close();
    }
};

bool boardsEqual(const vector<vector<int>> &a, const vector<vector<int>> &b)
{
    if (a.empty() || b.empty())
        return false;
    if (a.size() != b.size() || a[0].size() != b[0].size())
        return false;

    for (size_t i = 0; i < a.size(); i++)
    {
        for (size_t j = 0; j < a[0].size(); j++)
        {
            if (a[i][j] != b[i][j])
                return false;
        }
    }
    return true;
}

struct BoundingBox
{
    int minRow, maxRow, minCol, maxCol;
    bool hasLiveCells;
};

BoundingBox findBoundingBox(const vector<vector<int>> &board)
{
    BoundingBox box = {-1, -1, -1, -1, false};
    if (board.empty() || board[0].empty())
        return box;

    int m = board.size(), n = board[0].size();

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (board[i][j] == 1)
            {
                if (!box.hasLiveCells)
                {
                    box.minRow = box.maxRow = i;
                    box.minCol = box.maxCol = j;
                    box.hasLiveCells = true;
                }
                else
                {
                    box.minRow = min(box.minRow, i);
                    box.maxRow = max(box.maxRow, i);
                    box.minCol = min(box.minCol, j);
                    box.maxCol = max(box.maxCol, j);
                }
            }
        }
    }
    return box;
}

vector<vector<int>> trimBoard(const vector<vector<int>> &board)
{
    BoundingBox box = findBoundingBox(board);

    if (!box.hasLiveCells)
    {
        return {{0}};
    }

    int height = box.maxRow - box.minRow + 1;
    int width = box.maxCol - box.minCol + 1;

    vector<vector<int>> trimmed(height, vector<int>(width));
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            trimmed[i][j] = board[box.minRow + i][box.minCol + j];
        }
    }
    return trimmed;
}

int getBoardSize(const vector<vector<int>> &board)
{
    if (board.empty() || board[0].empty())
        return 0;
    return board.size();
}

int sizeToGeneration(int size)
{
    if (size <= 1)
        return 0;
    return (size - 1) / 2;
}

vector<vector<int>> addMargin(const vector<vector<int>> &board)
{
    if (board.empty() || board[0].empty())
        return board;

    int m = board.size(), n = board[0].size();
    vector<vector<int>> expanded(m + 2, vector<int>(n + 2, 0));

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            expanded[i + 1][j + 1] = board[i][j];
        }
    }
    return expanded;
}

vector<vector<int>> evolveWithRules(const vector<vector<int>> &board, const UniverseRules &rules)
{
    vector<vector<int>> prepared = addMargin(board);
    if (prepared.empty() || prepared[0].empty())
        return prepared;

    int m = prepared.size(), n = prepared[0].size();
    vector<vector<int>> next(m, vector<int>(n));
    vector<vector<int>> dirs = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            int neighbors = 0;
            for (auto &d : dirs)
            {
                int x = i + d[0], y = j + d[1];
                if (x >= 0 && x < m && y >= 0 && y < n)
                {
                    neighbors += prepared[x][y];
                }
            }

            if (neighbors <= rules.underpopEnd || neighbors >= rules.overpopStart)
            {
                next[i][j] = 0;
            }
            else if (neighbors >= rules.birthStart && neighbors <= rules.birthEnd)
            {
                next[i][j] = 1;
            }
            else
            {
                next[i][j] = prepared[i][j];
            }
        }
    }
    return next;
}

vector<UniverseRules> generateAllRulesets()
{
    vector<UniverseRules> rulesets;

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

bool shouldSkipFile(const string &filepath)
{
    ifstream file(filepath);
    if (!file.is_open())
        return true;

    string firstLine;
    getline(file, firstLine);
    file.close();

    return (firstLine.substr(0, 8) == "CLONE OF");
}

vector<vector<int>> readBoardFromFile(const string &filepath)
{
    ifstream file(filepath);
    if (!file.is_open())
        return {};

    vector<vector<int>> board;
    string line;

    getline(file, line);
    getline(file, line);
    getline(file, line);

    while (getline(file, line))
    {
        if (line.empty())
            break;
        vector<int> row;
        for (char c : line)
        {
            if (c == '0' || c == '1')
            {
                row.push_back(c - '0');
            }
        }
        if (!row.empty())
        {
            board.push_back(row);
        }
    }

    file.close();
    return board;
}

string getFilenameWithoutExtension(const string &filepath)
{
    fs::path p(filepath);
    return p.stem().string();
}

// Global map to track new files created in all generations
map<int, int> newFilesPerGeneration;

void createMockFile(const vector<vector<int>> &trimmedBoard, int targetGen, int sourceGen, const string &sourceSeed)
{
    string genFolder = "data/gen" + to_string(targetGen);
    fs::create_directories(genFolder);

    string filename = sourceSeed + "-gen" + to_string(sourceGen) + ".txt";
    string filepath = genFolder + "/" + filename;

    // Check if file already exists
    bool isNew = !fs::exists(filepath);

    ofstream file(filepath);
    file << "CLONE OF gen" << sourceGen << " " << sourceSeed << "\n";
    file << "parent: none\n\n";

    for (const auto &row : trimmedBoard)
    {
        for (int cell : row)
        {
            file << cell;
        }
        file << "\n";
    }
    file << "\n";

    file.close();

    if (isNew)
    {
        newFilesPerGeneration[targetGen]++;
    }
}

string parseMockFilename(const string &filename)
{
    size_t dashPos = filename.find("-gen");
    if (dashPos == string::npos)
    {
        return "";
    }

    string seed = filename.substr(0, dashPos);
    string genPart = filename.substr(dashPos + 1);

    return genPart + " " + seed;
}

string checkForClone(const vector<vector<int>> &board, int currentGen, const string &currentSeed)
{
    if (board.empty() || board[0].empty())
        return "";

    vector<vector<int>> trimmed = trimBoard(board);
    int trueSize = getBoardSize(trimmed);
    int targetGen = sizeToGeneration(trueSize);

    if (targetGen < currentGen)
    {
        string targetGenFolder = "data/gen" + to_string(targetGen);

        if (fs::exists(targetGenFolder))
        {
            for (const auto &entry : fs::directory_iterator(targetGenFolder))
            {
                if (entry.path().extension() == ".txt")
                {
                    vector<vector<int>> otherBoard = readBoardFromFile(entry.path().string());
                    if (otherBoard.empty())
                        continue;

                    vector<vector<int>> otherTrimmed = trimBoard(otherBoard);

                    if (boardsEqual(trimmed, otherTrimmed))
                    {
                        string filename = getFilenameWithoutExtension(entry.path().string());

                        string mockInfo = parseMockFilename(filename);
                        if (!mockInfo.empty())
                        {
                            return mockInfo;
                        }
                        else
                        {
                            return "gen" + to_string(targetGen) + " " + filename;
                        }
                    }
                }
            }

            createMockFile(trimmed, targetGen, currentGen, currentSeed);
        }
        else
        {
            createMockFile(trimmed, targetGen, currentGen, currentSeed);
        }
    }

    return "";
}

void saveGeneration(vector<BoardState> &states, int generation)
{
    string dirName = "data/gen" + to_string(generation);
    fs::create_directories(dirName);

    // Get existing files before we start
    set<string> existingFiles;
    if (fs::exists(dirName))
    {
        for (const auto &entry : fs::directory_iterator(dirName))
        {
            if (entry.path().extension() == ".txt")
            {
                existingFiles.insert(entry.path().filename().string());
            }
        }
    }

    int totalStates = states.size();
    int processedStates = 0;

    cout << "\rChecking for clones: 0%  " << flush;

    for (auto &state : states)
    {
        if (!state.board.empty())
        {
            string cloneInfo = checkForClone(state.board, generation, state.seeds[0]);
            if (!cloneInfo.empty())
            {
                state.cloneInfo = cloneInfo;
            }
        }
        processedStates++;
        int percentage = (processedStates * 100) / totalStates;
        cout << "\rChecking for clones: " << percentage << "%  " << flush;
    }

    cout << "\r                              \r" << flush;
    processedStates = 0;

    int newFiles = 0;
    for (size_t i = 0; i < states.size(); i++)
    {
        string filename = states[i].seeds[0] + ".txt";
        string filepath = dirName + "/" + filename;

        // Check if this file already existed
        bool wasNew = existingFiles.find(filename) == existingFiles.end();

        states[i].saveToFile(filepath);

        if (wasNew)
        {
            newFiles++;
        }

        processedStates++;
        int percentage = (processedStates * 100) / totalStates;
        cout << "\rSaving files: " << percentage << "%  " << flush;
    }

    newFilesPerGeneration[generation] = newFiles;

    cout << "\r                              \r" << flush;
}

vector<BoardState> evolveFromParent(const vector<vector<int>> &parentBoard, const string &parentSeed)
{
    vector<UniverseRules> allRules = generateAllRulesets();
    vector<BoardState> uniqueBoards;

    for (const auto &rules : allRules)
    {
        vector<vector<int>> newBoard = evolveWithRules(parentBoard, rules);
        if (newBoard.empty())
            continue;

        string seed = rules.toSeed();

        bool found = false;
        for (auto &existing : uniqueBoards)
        {
            if (boardsEqual(existing.board, newBoard))
            {
                existing.seeds.push_back(seed);
                found = true;
                break;
            }
        }

        if (!found)
        {
            uniqueBoards.push_back({newBoard, parentSeed, {seed}, ""});
        }
    }
    return uniqueBoards;
}

vector<BoardState> evolveGenerationFromFolder(const string &genFolder)
{
    vector<BoardState> allChildren;

    // First count total parents
    int totalParents = 0;
    for (const auto &entry : fs::directory_iterator(genFolder))
    {
        if (entry.path().extension() == ".txt" && !shouldSkipFile(entry.path().string()))
        {
            totalParents++;
        }
    }

    int processedParents = 0;

    for (const auto &entry : fs::directory_iterator(genFolder))
    {
        if (entry.path().extension() == ".txt")
        {
            if (shouldSkipFile(entry.path().string()))
            {
                continue;
            }

            string parentSeed = getFilenameWithoutExtension(entry.path().string());
            vector<vector<int>> board = readBoardFromFile(entry.path().string());

            if (board.empty())
                continue;

            vector<BoardState> children = evolveFromParent(board, parentSeed);

            for (auto &child : children)
            {
                bool found = false;
                for (auto &existing : allChildren)
                {
                    if (boardsEqual(existing.board, child.board))
                    {
                        for (const auto &seed : child.seeds)
                        {
                            existing.seeds.push_back(seed);
                        }
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    allChildren.push_back(child);
                }
            }

            processedParents++;
            int percentage = (processedParents * 100) / totalParents;
            cout << "\rEvolving boards: " << percentage << "%  " << flush;
        }
    }

    cout << "\r                              \r" << flush;

    return allChildren;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        vector<vector<int>> seed = {{1}};

        BoardState gen0 = {seed, "", {"0"}, ""};
        vector<BoardState> gen0States = {gen0};
        saveGeneration(gen0States, 0);

        vector<BoardState> gen1States = evolveFromParent(seed, "0");
        saveGeneration(gen1States, 1);

        // Print summary
        for (auto &[gen, count] : newFilesPerGeneration)
        {
            cout << "gen" << gen << ": " << count << " new files" << endl;
        }
    }
    else
    {
        string inputFolder = argv[1];

        // If user didn't include "data/" prefix, add it
        if (inputFolder.find("data/") != 0)
        {
            inputFolder = "data/" + inputFolder;
        }

        vector<BoardState> nextGenStates = evolveGenerationFromFolder(inputFolder);

        // Extract generation number from path (data/gen3 -> 3 or gen3 -> 3)
        size_t genPos = inputFolder.find("gen");
        if (genPos != string::npos)
        {
            string genStr = inputFolder.substr(genPos + 3);
            int currentGen = stoi(genStr);
            int nextGen = currentGen + 1;

            saveGeneration(nextGenStates, nextGen);
        }

        // Print summary for all generations that had new files
        for (auto &[gen, count] : newFilesPerGeneration)
        {
            cout << "gen" << gen << ": " << count << " new files" << endl;
        }
    }
    return 0;
}