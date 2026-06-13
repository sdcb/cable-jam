#include "game/GameState.h"
#include "game/LevelGenerator.h"
#include "game/ProgressStore.h"

#include <doctest/doctest.h>
#include <algorithm>
#include <cmath>
#include <set>

using namespace cable::game;

TEST_CASE("board size follows difficulty bands") {
    CHECK(LevelGenerator::BoardSizeForLevel(1) == BoardSize{8, 6});
    CHECK(LevelGenerator::BoardSizeForLevel(9) == BoardSize{8, 6});
    CHECK(LevelGenerator::BoardSizeForLevel(10) == BoardSize{10, 7});
    CHECK(LevelGenerator::BoardSizeForLevel(27) == BoardSize{12, 8});
    CHECK(LevelGenerator::BoardSizeForLevel(36) == BoardSize{14, 9});
    CHECK(LevelGenerator::BoardSizeForLevel(45) == BoardSize{16, 10});
    CHECK(LevelGenerator::BoardSizeForLevel(54) == BoardSize{18, 12});
    CHECK(LevelGenerator::BoardSizeForLevel(63) == BoardSize{22, 14});
    CHECK(LevelGenerator::BoardSizeForLevel(72) == BoardSize{25, 16});
    CHECK(LevelGenerator::BoardSizeForLevel(81) == BoardSize{28, 18});
}

TEST_CASE("star rating depends only on failed attempts") {
    CHECK(StarsForFailures(0) == 3);
    CHECK(StarsForFailures(1) == 3);
    CHECK(StarsForFailures(2) == 2);
    CHECK(StarsForFailures(3) == 2);
    CHECK(StarsForFailures(4) == 1);
    CHECK(StarsForFailures(5) == 1);
    CHECK(StarsForFailures(6) == 0);
}

TEST_CASE("edge outward plug is removable") {
    Level level;
    level.number = 1;
    level.board = {8, 6};
    level.cables.push_back({1, Direction::Up, {{3, 0}, {3, 1}, {4, 1}}, 0, 0});
    GameState state(level);
    CHECK(state.CanRemoveCable(1));

    Level blocked = level;
    blocked.cables[0].plugDirection = Direction::Down;
    GameState blockedState(blocked);
    CHECK_FALSE(blockedState.CanRemoveCable(1));
}

TEST_CASE("generated levels are bounded, full, non-overlapping, and solvable") {
    for (int levelNumber = 1; levelNumber <= MaxLevel; ++levelNumber) {
        Level level = LevelGenerator::Generate(levelNumber);
        std::set<std::pair<int, int>> occupied;
        CHECK(level.board == LevelGenerator::BoardSizeForLevel(levelNumber));
        CHECK_FALSE(level.cables.empty());
        GameState state(level);
        CHECK(state.AvailableCableIds().size() < level.cables.size());
        int maxAvailable = static_cast<int>(state.AvailableCableIds().size());
        for (const Cable& cable : level.cables) {
            CHECK(cable.path.size() >= 2);
            CHECK(cable.colorIndex >= 0);
            CHECK(cable.colorIndex < 7);
            CHECK(cable.plugStyle >= 0);
            CHECK(cable.plugStyle < 3);
            CHECK(cable.plugDirection == DirectionFromDelta(cable.path[1], cable.path[0]));
            for (std::size_t i = 0; i < cable.path.size(); ++i) {
                const GridPos pos = cable.path[i];
                CHECK(pos.x >= 0);
                CHECK(pos.y >= 0);
                CHECK(pos.x < level.board.columns);
                CHECK(pos.y < level.board.rows);
                CHECK(occupied.insert({pos.x, pos.y}).second);
                if (i > 0) {
                    const GridPos prev = cable.path[i - 1];
                    const int manhattan = std::abs(prev.x - pos.x) + std::abs(prev.y - pos.y);
                    CHECK(manhattan == 1);
                }
            }
        }
        CHECK(occupied.size() == static_cast<std::size_t>(level.board.columns * level.board.rows));
        for (int id : level.solutionOrder) {
            const std::vector<int> available = state.AvailableCableIds();
            maxAvailable = std::max(maxAvailable, static_cast<int>(available.size()));
            CHECK(std::find(available.begin(), available.end(), id) != available.end());
            CHECK(state.TryClickCableById(id));
            state.Update(1.0f);
        }
        CHECK(state.Completed());
        CHECK(maxAvailable >= 2);
    }
}

TEST_CASE("generated levels are deterministic per seed and varied by seed") {
    const Level a = LevelGenerator::Generate(81, 1234);
    const Level b = LevelGenerator::Generate(81, 1234);
    const Level c = LevelGenerator::Generate(81, 5678);
    REQUIRE(a.cables.size() == b.cables.size());
    CHECK(a.board == BoardSize{28, 18});
    CHECK(a.cables.front().path == b.cables.front().path);
    CHECK(a.cables.front().path != c.cables.front().path);

    int rowMajorDrops = 0;
    int previous = -1;
    for (const Cable& cable : a.cables) {
        const int current = cable.path.front().y * a.board.columns + cable.path.front().x;
        if (previous >= 0 && current < previous) {
            ++rowMajorDrops;
        }
        previous = current;
    }
    CHECK(rowMajorDrops >= 4);
}

TEST_CASE("hit testing works on a cable body") {
    Level level;
    level.number = 1;
    level.board = {8, 6};
    level.cables.push_back({1, Direction::Left, {{0, 2}, {1, 2}, {2, 2}}, 0, 0});
    GameState state(level);
    const BoardLayout layout = state.Layout();
    const auto hitPoint = CellCenter(layout, {1, 2});
    CHECK(state.HitTestCable(hitPoint).value_or(-1) == 1);
    CHECK(state.TryClickCable(hitPoint));
}

TEST_CASE("record completion keeps best result and unlocks next level") {
    Progress progress;
    RecordCompletion(progress, 1, 12, 40.0f, 2);
    CHECK(progress.unlockedLevel == 2);
    CHECK(progress.levels[1].bestClicks == 12);
    CHECK(progress.levels[1].bestStars == 2);

    RecordCompletion(progress, 1, 20, 80.0f, 1);
    CHECK(progress.levels[1].bestClicks == 12);
    CHECK(progress.levels[1].bestStars == 2);

    RecordCompletion(progress, 1, 18, 70.0f, 3);
    CHECK(progress.levels[1].bestClicks == 18);
    CHECK(progress.levels[1].bestStars == 3);
}
