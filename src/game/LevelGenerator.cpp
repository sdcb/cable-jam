#include "game/LevelGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace cable::game {
namespace {

bool InBounds(BoardSize board, GridPos pos) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < board.columns && pos.y < board.rows;
}

int MinLengthForLevel(int levelNumber) {
    return levelNumber < 18 ? 6 : (levelNumber < 50 ? 5 : (levelNumber < 64 ? 7 : 8));
}

int MaxLengthForLevel(int levelNumber) {
    if (levelNumber < 18) {
        return 10;
    }
    if (levelNumber < 50) {
        return 9;
    }
    return levelNumber < 64 ? 11 : 12;
}

std::vector<GridPos> SnakeCells(BoardSize board) {
    std::vector<GridPos> cells;
    cells.reserve(static_cast<std::size_t>(board.columns * board.rows));
    for (int y = 0; y < board.rows; ++y) {
        if (y % 2 == 0) {
            for (int x = 0; x < board.columns; ++x) {
                cells.push_back({x, y});
            }
        } else {
            for (int x = board.columns - 1; x >= 0; --x) {
                cells.push_back({x, y});
            }
        }
    }
    return cells;
}

bool IsRowStartIndex(int index, BoardSize board) {
    return index > 0 && index < board.columns * board.rows && (index % board.columns) == 0;
}

std::vector<int> PartitionSnake(BoardSize board, int levelNumber, std::mt19937& rng) {
    const int total = board.columns * board.rows;
    const int minLength = MinLengthForLevel(levelNumber);
    const int maxLength = MaxLengthForLevel(levelNumber);
    std::uniform_int_distribution<int> lengthDist(minLength, maxLength);

    std::vector<int> lengths;
    int index = 0;
    while (total - index > maxLength) {
        int length = lengthDist(rng);
        for (int attempt = 0; attempt < 32; ++attempt) {
            const int next = index + length;
            const int remaining = total - next;
            if (remaining != 1 && !IsRowStartIndex(next, board)) {
                break;
            }
            length = lengthDist(rng);
        }
        if (total - (index + length) == 1) {
            ++length;
        }
        if (IsRowStartIndex(index + length, board)) {
            if (length + 1 <= maxLength && total - (index + length + 1) != 1) {
                ++length;
            } else if (length - 1 >= minLength && total - (index + length - 1) != 1) {
                --length;
            }
        }
        lengths.push_back(length);
        index += length;
    }

    int remaining = total - index;
    if (remaining == 1 && !lengths.empty()) {
        --lengths.back();
        remaining = 2;
    }
    if (remaining > 0) {
        lengths.push_back(remaining);
    }

    for (int pass = 0; pass < 128; ++pass) {
        bool changed = false;
        int start = 0;
        for (std::size_t i = 1; i < lengths.size(); ++i) {
            start += lengths[i - 1];
            if (!IsRowStartIndex(start, board)) {
                continue;
            }
            if (lengths[i - 1] > 2 && !IsRowStartIndex(start - 1, board)) {
                --lengths[i - 1];
                ++lengths[i];
                changed = true;
                break;
            }
            if (lengths[i] > 2 && !IsRowStartIndex(start + 1, board)) {
                ++lengths[i - 1];
                --lengths[i];
                changed = true;
                break;
            }
        }
        if (!changed) {
            break;
        }
    }

    return lengths;
}

} // namespace

BoardSize LevelGenerator::BoardSizeForLevel(int levelNumber) {
    if (levelNumber <= 18) {
        return {8, 6};
    }
    if (levelNumber <= 42) {
        return {10, 7};
    }
    if (levelNumber <= 63) {
        return {12, 8};
    }
    return {14, 9};
}

Level LevelGenerator::Generate(int levelNumber, std::uint32_t seedBase) {
    levelNumber = std::clamp(levelNumber, 1, MaxLevel);
    Level level;
    level.number = levelNumber;
    level.board = BoardSizeForLevel(levelNumber);

    std::mt19937 rng(seedBase + static_cast<std::uint32_t>(levelNumber * 977));
    const std::vector<GridPos> cells = SnakeCells(level.board);
    const std::vector<int> lengths = PartitionSnake(level.board, levelNumber, rng);

    int index = 0;
    for (int length : lengths) {
        if (length < 2 || index + length > static_cast<int>(cells.size())) {
            break;
        }
        Cable cable;
        cable.id = static_cast<int>(level.cables.size()) + 1;
        cable.path.assign(cells.begin() + index, cells.begin() + index + length);
        cable.plugDirection = DirectionFromDelta(cable.path[1], cable.path[0]);
        cable.colorIndex = (cable.id - 1) % 8;
        cable.plugStyle = (cable.id + levelNumber) % 3;
        level.solutionOrder.push_back(cable.id);
        level.cables.push_back(std::move(cable));
        index += length;
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
