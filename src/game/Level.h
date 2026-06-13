#pragma once

#include "core/Geometry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cable::game {

constexpr int MaxLevel = 81;

enum class Direction {
    Up,
    Down,
    Left,
    Right
};

struct GridPos {
    int x{};
    int y{};

    friend bool operator==(const GridPos& a, const GridPos& b) {
        return a.x == b.x && a.y == b.y;
    }
};

struct BoardSize {
    int columns{};
    int rows{};

    friend bool operator==(const BoardSize& a, const BoardSize& b) {
        return a.columns == b.columns && a.rows == b.rows;
    }
};

struct BoardLayout {
    BoardSize size{};
    core::Rect playArea{};
    float originX{};
    float originY{};
    float cellSize{};
    float cableWidth{};
    float plugSize{};
    float hitRadius{};
};

struct Cable {
    int id{};
    Direction plugDirection{Direction::Up};
    std::vector<GridPos> path;
    int colorIndex{};
    int plugStyle{};
    bool removing{false};
    float removeProgress{};
    float shakeTime{};
};

struct Level {
    int number{1};
    BoardSize board{8, 6};
    std::vector<Cable> cables;
    std::vector<int> solutionOrder;
};

BoardLayout ComputeBoardLayout(BoardSize size, core::Rect playArea);
core::Point CellCenter(const BoardLayout& layout, GridPos pos);
core::Point DirectionVector(Direction direction);
Direction DirectionFromDelta(GridPos from, GridPos to);
int StarsForFailures(int failures);
std::string FormatSeconds(float seconds);

} // namespace cable::game
