#include "game/LevelGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <random>

namespace cable::game {
namespace {

bool InBounds(BoardSize board, GridPos pos) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < board.columns && pos.y < board.rows;
}

GridPos Step(GridPos pos, Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {pos.x, pos.y - 1};
    case Direction::Down:
        return {pos.x, pos.y + 1};
    case Direction::Left:
        return {pos.x - 1, pos.y};
    case Direction::Right:
        return {pos.x + 1, pos.y};
    }
    return pos;
}

Direction Opposite(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    case Direction::Right:
        return Direction::Left;
    }
    return Direction::Down;
}

int MinLengthForLevel(int levelNumber) {
    if (levelNumber <= 18) {
        return 5;
    }
    if (levelNumber <= 45) {
        return 5;
    }
    if (levelNumber <= 63) {
        return 6;
    }
    if (levelNumber <= 72) {
        return 8;
    }
    return 9;
}

int MaxLengthForLevel(int levelNumber) {
    if (levelNumber <= 18) {
        return 9;
    }
    if (levelNumber <= 36) {
        return 10;
    }
    if (levelNumber <= 54) {
        return 12;
    }
    if (levelNumber <= 63) {
        return 14;
    }
    if (levelNumber <= 72) {
        return 16;
    }
    return 18;
}

using CellMap = std::vector<std::vector<unsigned char>>;

bool Assigned(const CellMap& assigned, GridPos pos) {
    return assigned[static_cast<std::size_t>(pos.y)][static_cast<std::size_t>(pos.x)] != 0;
}

void SetAssigned(CellMap& assigned, GridPos pos, bool value) {
    assigned[static_cast<std::size_t>(pos.y)][static_cast<std::size_t>(pos.x)] = value ? 1 : 0;
}

int AssignedCount(const CellMap& assigned) {
    int count = 0;
    for (const auto& row : assigned) {
        for (unsigned char cell : row) {
            count += cell ? 1 : 0;
        }
    }
    return count;
}

bool RayClearToEdge(BoardSize board, const CellMap& assigned, GridPos pos, Direction direction) {
    GridPos scan = Step(pos, direction);
    while (InBounds(board, scan)) {
        if (!Assigned(assigned, scan)) {
            return false;
        }
        scan = Step(scan, direction);
    }
    return true;
}

struct Candidate {
    GridPos plug;
    Direction outward{};
};

std::vector<Candidate> ExposedCandidates(BoardSize board, const CellMap& assigned) {
    std::vector<Candidate> candidates;
    constexpr std::array<Direction, 4> directions{Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    for (int y = 0; y < board.rows; ++y) {
        for (int x = 0; x < board.columns; ++x) {
            const GridPos plug{x, y};
            if (Assigned(assigned, plug)) {
                continue;
            }
            for (Direction outward : directions) {
                const GridPos body = Step(plug, Opposite(outward));
                if (!InBounds(board, body) || Assigned(assigned, body)) {
                    continue;
                }
                if (RayClearToEdge(board, assigned, plug, outward)) {
                    candidates.push_back({plug, outward});
                }
            }
        }
    }
    return candidates;
}

int OnwardCount(BoardSize board, const CellMap& assigned, const CellMap& inPath, GridPos pos) {
    int count = 0;
    constexpr std::array<Direction, 4> directions{Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    for (Direction direction : directions) {
        const GridPos next = Step(pos, direction);
        if (InBounds(board, next) && !Assigned(assigned, next) && !Assigned(inPath, next)) {
            ++count;
        }
    }
    return count;
}

std::vector<GridPos> GrowPath(BoardSize board, const CellMap& assigned, Candidate candidate, int targetLength, std::mt19937& rng) {
    std::vector<GridPos> path;
    CellMap inPath(static_cast<std::size_t>(board.rows), std::vector<unsigned char>(static_cast<std::size_t>(board.columns), 0));
    path.push_back(candidate.plug);
    SetAssigned(inPath, candidate.plug, true);

    const GridPos body = Step(candidate.plug, Opposite(candidate.outward));
    path.push_back(body);
    SetAssigned(inPath, body, true);

    GridPos current = body;
    while (static_cast<int>(path.size()) < targetLength) {
        std::vector<GridPos> options;
        constexpr std::array<Direction, 4> directions{Direction::Up, Direction::Down, Direction::Left, Direction::Right};
        for (Direction direction : directions) {
            const GridPos next = Step(current, direction);
            if (InBounds(board, next) && !Assigned(assigned, next) && !Assigned(inPath, next)) {
                options.push_back(next);
            }
        }
        if (options.empty()) {
            break;
        }
        std::shuffle(options.begin(), options.end(), rng);
        std::sort(options.begin(), options.end(), [&](GridPos a, GridPos b) {
            return OnwardCount(board, assigned, inPath, a) < OnwardCount(board, assigned, inPath, b);
        });
        const int top = std::min<int>(3, static_cast<int>(options.size()));
        std::uniform_int_distribution<int> pick(0, top - 1);
        current = options[static_cast<std::size_t>(pick(rng))];
        path.push_back(current);
        SetAssigned(inPath, current, true);
    }
    return path;
}

bool LeavesOnlySolvableComponents(BoardSize board, CellMap assigned) {
    CellMap visited(static_cast<std::size_t>(board.rows), std::vector<unsigned char>(static_cast<std::size_t>(board.columns), 0));
    constexpr std::array<Direction, 4> directions{Direction::Up, Direction::Down, Direction::Left, Direction::Right};

    for (int y = 0; y < board.rows; ++y) {
        for (int x = 0; x < board.columns; ++x) {
            const GridPos start{x, y};
            if (Assigned(assigned, start) || Assigned(visited, start)) {
                continue;
            }
            int componentSize = 0;
            std::queue<GridPos> queue;
            queue.push(start);
            SetAssigned(visited, start, true);
            while (!queue.empty()) {
                const GridPos current = queue.front();
                queue.pop();
                ++componentSize;
                for (Direction direction : directions) {
                    const GridPos next = Step(current, direction);
                    if (InBounds(board, next) && !Assigned(assigned, next) && !Assigned(visited, next)) {
                        SetAssigned(visited, next, true);
                        queue.push(next);
                    }
                }
            }
            if (componentSize == 1) {
                return false;
            }
        }
    }
    return true;
}

Level TryGenerate(int levelNumber, std::uint32_t seed) {
    Level level;
    level.number = levelNumber;
    level.board = LevelGenerator::BoardSizeForLevel(levelNumber);

    std::mt19937 rng(seed);
    CellMap assigned(static_cast<std::size_t>(level.board.rows), std::vector<unsigned char>(static_cast<std::size_t>(level.board.columns), 0));
    const int total = level.board.columns * level.board.rows;
    const int minLength = MinLengthForLevel(levelNumber);
    const int maxLength = MaxLengthForLevel(levelNumber);
    std::uniform_int_distribution<int> lengthDist(minLength, maxLength);

    while (AssignedCount(assigned) < total) {
        const int remaining = total - AssignedCount(assigned);
        if (remaining == 1) {
            return {};
        }

        std::vector<Candidate> candidates = ExposedCandidates(level.board, assigned);
        if (candidates.empty()) {
            return {};
        }
        std::shuffle(candidates.begin(), candidates.end(), rng);

        bool placed = false;
        for (Candidate candidate : candidates) {
            int targetLength = remaining <= maxLength ? remaining : lengthDist(rng);
            if (remaining - targetLength == 1) {
                ++targetLength;
            }
            targetLength = std::clamp(targetLength, 2, remaining);

            for (int shrink = 0; shrink < 4 && !placed; ++shrink) {
                const int wanted = std::max(2, targetLength - shrink);
                std::vector<GridPos> path = GrowPath(level.board, assigned, candidate, wanted, rng);
                if (static_cast<int>(path.size()) < 2) {
                    continue;
                }
                if (remaining - static_cast<int>(path.size()) == 1) {
                    continue;
                }
                CellMap trial = assigned;
                for (GridPos pos : path) {
                    SetAssigned(trial, pos, true);
                }
                if (!LeavesOnlySolvableComponents(level.board, trial)) {
                    continue;
                }

                Cable cable;
                cable.id = static_cast<int>(level.cables.size()) + 1;
                cable.plugDirection = candidate.outward;
                cable.path = std::move(path);
                cable.colorIndex = static_cast<int>((cable.id + rng()) % 7);
                cable.plugStyle = static_cast<int>((cable.id + levelNumber + rng()) % 3);
                level.solutionOrder.push_back(cable.id);
                level.cables.push_back(std::move(cable));
                assigned = std::move(trial);
                placed = true;
            }
            if (placed) {
                break;
            }
        }
        if (!placed) {
            return {};
        }
    }

    return level;
}

std::vector<GridPos> BuildStripPath(BoardSize board, int firstRow, int height, bool fromTop) {
    std::vector<GridPos> path;
    path.reserve(static_cast<std::size_t>(board.columns * height));

    const int lastRow = firstRow + height - 1;
    if (fromTop) {
        for (int x = 0; x < board.columns; ++x) {
            if (x % 2 == 0) {
                for (int y = firstRow; y <= lastRow; ++y) {
                    path.push_back({x, y});
                }
            } else {
                for (int y = lastRow; y >= firstRow; --y) {
                    path.push_back({x, y});
                }
            }
        }
        return path;
    }

    int step = 0;
    for (int x = board.columns - 1; x >= 0; --x, ++step) {
        if (step % 2 == 0) {
            for (int y = lastRow; y >= firstRow; --y) {
                path.push_back({x, y});
            }
        } else {
            for (int y = firstRow; y <= lastRow; ++y) {
                path.push_back({x, y});
            }
        }
    }
    return path;
}

Level BuildFallbackLevel(int levelNumber, std::uint32_t seed) {
    Level level;
    level.number = levelNumber;
    level.board = LevelGenerator::BoardSizeForLevel(levelNumber);

    std::mt19937 rng(seed);
    int topRow = 0;
    int bottomRow = level.board.rows - 1;
    bool takeTop = true;
    bool usedOddTopStrip = false;

    while (topRow <= bottomRow) {
        const int remainingRows = bottomRow - topRow + 1;
        int height = 2;
        if (!usedOddTopStrip && remainingRows % 2 == 1 && remainingRows >= 3) {
            height = 3;
            usedOddTopStrip = true;
        }
        height = std::min(height, remainingRows);

        const bool fromTop = takeTop || remainingRows == height;
        const int firstRow = fromTop ? topRow : bottomRow - height + 1;

        Cable cable;
        cable.id = static_cast<int>(level.cables.size()) + 1;
        cable.plugDirection = fromTop ? Direction::Up : Direction::Down;
        cable.path = BuildStripPath(level.board, firstRow, height, fromTop);
        cable.colorIndex = static_cast<int>((cable.id + rng()) % 7);
        cable.plugStyle = static_cast<int>((cable.id + levelNumber + rng()) % 3);
        level.solutionOrder.push_back(cable.id);
        level.cables.push_back(std::move(cable));

        if (fromTop) {
            topRow += height;
        } else {
            bottomRow -= height;
        }
        takeTop = !takeTop;
    }

    return level;
}

} // namespace

BoardSize LevelGenerator::BoardSizeForLevel(int levelNumber) {
    if (levelNumber <= 9) {
        return {8, 6};
    }
    if (levelNumber <= 18) {
        return {10, 7};
    }
    if (levelNumber <= 27) {
        return {12, 8};
    }
    if (levelNumber <= 36) {
        return {14, 9};
    }
    if (levelNumber <= 45) {
        return {16, 10};
    }
    if (levelNumber <= 54) {
        return {18, 12};
    }
    if (levelNumber <= 63) {
        return {22, 14};
    }
    if (levelNumber <= 72) {
        return {25, 16};
    }
    return {28, 18};
}

Level LevelGenerator::Generate(int levelNumber, std::uint32_t seedBase) {
    levelNumber = std::clamp(levelNumber, 1, MaxLevel);
    for (std::uint32_t attempt = 0; attempt < 96; ++attempt) {
        Level level = TryGenerate(levelNumber, seedBase + static_cast<std::uint32_t>(levelNumber * 977) + attempt * 7919u);
        if (!level.cables.empty()) {
            return level;
        }
    }
    return BuildFallbackLevel(levelNumber, seedBase + static_cast<std::uint32_t>(levelNumber * 13));
}

BoardLayout ComputeBoardLayout(BoardSize size, core::Rect playArea) {
    const float cell = std::floor(std::min(playArea.width / static_cast<float>(size.columns), playArea.height / static_cast<float>(size.rows)));
    const float boardWidth = cell * static_cast<float>(size.columns);
    const float boardHeight = cell * static_cast<float>(size.rows);
    BoardLayout layout;
    layout.size = size;
    layout.playArea = playArea;
    layout.cellSize = cell;
    layout.originX = playArea.x + (playArea.width - boardWidth) * 0.5f;
    layout.originY = playArea.y + (playArea.height - boardHeight) * 0.5f;
    layout.cableWidth = std::clamp(cell * 0.34f, 6.0f, 24.0f);
    layout.plugSize = std::clamp(cell * 0.56f, 11.0f, 40.0f);
    layout.hitRadius = std::max(layout.cableWidth * 1.15f, 10.0f);
    return layout;
}

core::Point CellCenter(const BoardLayout& layout, GridPos pos) {
    return {
        layout.originX + (static_cast<float>(pos.x) + 0.5f) * layout.cellSize,
        layout.originY + (static_cast<float>(pos.y) + 0.5f) * layout.cellSize
    };
}

core::Point DirectionVector(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {0.0f, -1.0f};
    case Direction::Down:
        return {0.0f, 1.0f};
    case Direction::Left:
        return {-1.0f, 0.0f};
    case Direction::Right:
        return {1.0f, 0.0f};
    }
    return {};
}

Direction DirectionFromDelta(GridPos from, GridPos to) {
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    if (dx < 0) {
        return Direction::Left;
    }
    if (dx > 0) {
        return Direction::Right;
    }
    if (dy < 0) {
        return Direction::Up;
    }
    return Direction::Down;
}

int StarsForFailures(int failures) {
    if (failures <= 1) {
        return 3;
    }
    if (failures <= 3) {
        return 2;
    }
    if (failures <= 5) {
        return 1;
    }
    return 0;
}

std::string FormatSeconds(float seconds) {
    const int total = std::max(0, static_cast<int>(std::lround(seconds)));
    const int minutes = total / 60;
    const int sec = total % 60;
    char text[16]{};
    std::snprintf(text, sizeof(text), "%02d:%02d", minutes, sec);
    return text;
}

} // namespace cable::game
