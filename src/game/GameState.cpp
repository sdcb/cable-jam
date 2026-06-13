#include "game/GameState.h"

#include <algorithm>
#include <cmath>

namespace cable::game {

GameState::GameState(Level level) : level_(std::move(level)) {
    SetPlayArea({190.0f, 112.0f, 900.0f, 500.0f});
}

void GameState::SetPlayArea(core::Rect playArea) {
    layout_ = ComputeBoardLayout(level_.board, playArea);
}

void GameState::Update(float dt) {
    if (!completed_) {
        elapsedSeconds_ += dt;
    }

    for (Cable& cable : level_.cables) {
        if (cable.removing) {
            cable.removeProgress = std::min(1.0f, cable.removeProgress + dt / 0.42f);
        }
        if (cable.shakeTime > 0.0f) {
            cable.shakeTime = std::max(0.0f, cable.shakeTime - dt);
        }
    }

    RemoveFinishedCables();
    if (!completed_ && level_.cables.empty()) {
        completed_ = true;
    }
}

bool GameState::TryClickCable(core::Point logicalPoint) {
    const std::optional<int> hit = HitTestCable(logicalPoint);
    if (!hit) {
        return false;
    }
    return TryClickCableById(*hit);
}

bool GameState::TryClickCableById(int id) {
    Cable* cable = FindCable(id);
    if (!cable || cable->removing || completed_) {
        return false;
    }

    ++clicks_;
    if (CanRemoveCable(id)) {
        cable->removing = true;
        cable->removeProgress = 0.0f;
        return true;
    }

    ++failures_;
    cable->shakeTime = 0.28f;
    return false;
}

bool GameState::CanRemoveCable(int id) const {
    const Cable* cable = FindCable(id);
    if (!cable || cable->path.empty()) {
        return false;
    }

    const GridPos plug = cable->path.front();
    switch (cable->plugDirection) {
    case Direction::Up:
        return plug.y == 0;
    case Direction::Down:
        return plug.y == level_.board.rows - 1;
    case Direction::Left:
        return plug.x == 0;
    case Direction::Right:
        return plug.x == level_.board.columns - 1;
    }
    return false;
}

std::optional<int> GameState::HitTestCable(core::Point logicalPoint) const {
    for (auto it = level_.cables.rbegin(); it != level_.cables.rend(); ++it) {
        const Cable& cable = *it;
        if (cable.path.empty()) {
            continue;
        }
        if (cable.path.size() == 1) {
            const core::Point center = CellCenter(layout_, cable.path.front());
            if (core::DistanceToSegment(logicalPoint, center, center) <= layout_.hitRadius) {
                return cable.id;
            }
            continue;
        }
        for (std::size_t i = 1; i < cable.path.size(); ++i) {
            const core::Point a = CellCenter(layout_, cable.path[i - 1]);
            const core::Point b = CellCenter(layout_, cable.path[i]);
            if (core::DistanceToSegment(logicalPoint, a, b) <= layout_.hitRadius) {
                return cable.id;
            }
        }
    }
    return std::nullopt;
}

LevelResult GameState::CompleteResult() const {
    return {level_.number, clicks_, failures_, elapsedSeconds_, StarsForFailures(failures_)};
}

Cable* GameState::FindCable(int id) {
    auto it = std::find_if(level_.cables.begin(), level_.cables.end(), [id](const Cable& cable) { return cable.id == id; });
    return it == level_.cables.end() ? nullptr : &*it;
}

const Cable* GameState::FindCable(int id) const {
    auto it = std::find_if(level_.cables.begin(), level_.cables.end(), [id](const Cable& cable) { return cable.id == id; });
    return it == level_.cables.end() ? nullptr : &*it;
}

void GameState::RemoveFinishedCables() {
    level_.cables.erase(
        std::remove_if(level_.cables.begin(), level_.cables.end(), [](const Cable& cable) {
            return cable.removing && cable.removeProgress >= 1.0f;
        }),
        level_.cables.end());
}

} // namespace cable::game
