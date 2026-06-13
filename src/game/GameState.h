#pragma once

#include "core/Tween.h"
#include "game/Level.h"

#include <optional>

namespace cable::game {

struct LevelResult {
    int levelNumber{};
    int clicks{};
    int failures{};
    float seconds{};
    int stars{};
};

class GameState {
public:
    explicit GameState(Level level);

    const Level& CurrentLevel() const { return level_; }
    const BoardLayout& Layout() const { return layout_; }
    void SetPlayArea(core::Rect playArea);
    void Update(float dt);

    bool TryClickCable(core::Point logicalPoint);
    bool TryClickCableById(int id);
    bool CanRemoveCable(int id) const;
    std::vector<int> AvailableCableIds() const;
    std::optional<int> HitTestCable(core::Point logicalPoint) const;

    int Clicks() const { return clicks_; }
    int Failures() const { return failures_; }
    float Seconds() const { return elapsedSeconds_; }
    bool Completed() const { return completed_; }
    LevelResult CompleteResult() const;

private:
    Cable* FindCable(int id);
    const Cable* FindCable(int id) const;
    void RemoveFinishedCables();

    Level level_;
    BoardLayout layout_{};
    int clicks_{};
    int failures_{};
    float elapsedSeconds_{};
    bool completed_{false};
};

} // namespace cable::game
