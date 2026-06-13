#include "game/LevelGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
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

bool Occupied(const std::vector<std::vector<bool>>& occupied, GridPos pos) {
    return occupied[static_cast<std::size_t>(pos.y)][static_cast<std::size_t>(pos.x)];
}

void SetOccupied(std::vector<std::vector<bool>>& occupied, const std::vector<GridPos>& path) {
    for (GridPos pos : path) {
        occupied[static_cast<std::size_t>(pos.y)][static_cast<std::size_t>(pos.x)] = true;
    }
}

std::pair<GridPos, Direction> RandomEdgePlug(BoardSize board, std::mt19937& rng) {
    std::uniform_int_distribution<int> edgeDist(0, 3);
    const int edge = edgeDist(rng);
    if (edge == 0) {
        std::uniform_int_distribution<int> col(0, board.columns - 1);
        return {{col(rng), 0}, Direction::Up};
    }
    if (edge == 1) {
        std::uniform_int_distribution<int> col(0, board.columns - 1);
        return {{col(rng), board.rows - 1}, Direction::Down};
    }
    if (edge == 2) {
        std::uniform_int_distribution<int> row(0, board.rows - 1);
        return {{0, row(rng)}, Direction::Left};
    }
    std::uniform_int_distribution<int> row(0, board.rows - 1);
    return {{board.columns - 1, row(rng)}, Direction::Right};
}

std::vector<GridPos> BuildPath(
    BoardSize board,
    const std::vector<std::vector<bool>>& occupied,
    GridPos plug,
    Direction outward,
    int targetLength,
    float bendChance,
    std::mt19937& rng) {
    if (Occupied(occupied, plug)) {
        return {};
    }

    std::vector<GridPos> path{plug};
    Direction direction = Opposite(outward);
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);

    while (static_cast<int>(path.size()) < targetLength) {
        std::array<Direction, 4> candidates{Direction::Up, Direction::Down, Direction::Left, Direction::Right};
        std::shuffle(candidates.begin(), candidates.end(), rng);
        if (chance(rng) > bendChance) {
            std::swap(candidates[0], *std::find(candidates.begin(), candidates.end(), direction));
        }

        bool moved = false;
        for (Direction candidate : candidates) {
            GridPos next = Step(path.back(), candidate);
            if (!InBounds(board, next) || Occupied(occupied, next) ||
                std::find(path.begin(), path.end(), next) != path.end()) {
                continue;
            }
            direction = candidate;
            path.push_back(next);
            moved = true;
            break;
        }
        if (!moved) {
            break;
        }
    }

    return path.size() >= 2 ? path : std::vector<GridPos>{};
}

int CableCountForLevel(int levelNumber, BoardSize board) {
    const float t = std::clamp((levelNumber - 1) / 99.0f, 0.0f, 1.0f);
    const int raw = static_cast<int>(std::lround(5.0f + 23.0f * t));
    return std::min(raw, (board.columns * board.rows) / 3);
}

int MinLengthForLevel(int levelNumber) {
    return levelNumber < 20 ? 3 : (levelNumber < 60 ? 4 : 5);
}

int MaxLengthForLevel(int levelNumber) {
    if (levelNumber < 20) {
        return 5;
    }
    if (levelNumber < 55) {
        return 7;
    }
    if (levelNumber < 80) {
        return 8;
    }
    return 10;
}

} // namespace

BoardSize LevelGenerator::BoardSizeForLevel(int levelNumber) {
    if (levelNumber <= 20) {
        return {8, 6};
    }
    if (levelNumber <= 45) {
        return {10, 7};
    }
    if (levelNumber <= 75) {
        return {12, 8};
    }
    return {14, 9};
}

Level LevelGenerator::Generate(int levelNumber, std::uint32_t seedBase) {
    levelNumber = std::clamp(levelNumber, 1, 100);
    Level level;
    level.number = levelNumber;
    level.board = BoardSizeForLevel(levelNumber);

    std::mt19937 rng(seedBase + static_cast<std::uint32_t>(levelNumber * 977));
    std::vector<std::vector<bool>> occupied(
        static_cast<std::size_t>(level.board.rows),
        std::vector<bool>(static_cast<std::size_t>(level.board.columns), false));

    const int targetCount = CableCountForLevel(levelNumber, level.board);
    const int minLength = MinLengthForLevel(levelNumber);
    const int maxLength = MaxLengthForLevel(levelNumber);
    const float bendChance = 0.18f + 0.32f * ((levelNumber - 1) / 99.0f);
    std::uniform_int_distribution<int> lengthDist(minLength, maxLength);

    int attempts = 0;
    while (static_cast<int>(level.cables.size()) < targetCount && attempts < targetCount * 180) {
        ++attempts;
        auto [plug, direction] = RandomEdgePlug(level.board, rng);
        std::vector<GridPos> path = BuildPath(level.board, occupied, plug, direction, lengthDist(rng), bendChance, rng);
        if (path.empty()) {
            continue;
        }

        Cable cable;
        cable.id = static_cast<int>(level.cables.size()) + 1;
        cable.plugDirection = direction;
        cable.path = std::move(path);
        cable.colorIndex = (cable.id - 1) % 8;
        cable.plugStyle = (cable.id + levelNumber) % 3;
        SetOccupied(occupied, cable.path);
        level.solutionOrder.push_back(cable.id);
        level.cables.push_back(std::move(cable));
    }

    return level;
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
    layout.cableWidth = std::clamp(cell * 0.30f, 12.0f, 26.0f);
    layout.plugSize = std::clamp(cell * 0.48f, 22.0f, 42.0f);
    layout.hitRadius = std::max(layout.cableWidth * 0.90f, 18.0f);
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
