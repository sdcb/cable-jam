#include "game/GameState.h"

#include <algorithm>
#include <cmath>

namespace cable::game {
namespace {

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

bool InBounds(BoardSize board, GridPos pos) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < board.columns && pos.y < board.rows;
}

} // namespace

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
            cable.removeProgress = std::min(1.0f, cable.removeProgress + dt / 0.95f);
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

    GridPos scan = Step(cable->path.front(), cable->plugDirection);
    while (InBounds(level_.board, scan)) {
        for (const Cable& other : level_.cables) {
            if (other.removing) {
                continue;
            }
            if (std::find(other.path.begin(), other.path.end(), scan) != other.path.end()) {
                return false;
            }
        }
        scan = Step(scan, cable->plugDirection);
    }
    return true;
}

std::vector<int> GameState::AvailableCableIds() const {
    std::vector<int> ids;
    for (const Cable& cable : level_.cables) {
        if (!cable.removing && CanRemoveCable(cable.id)) {
            ids.push_back(cable.id);
        }
    }
    return ids;
}

std::optional<int> GameState::HitTestCable(core::Point logicalPoint) const {
    for (auto it = level_.cables.rbegin(); it != level_.cables.rend(); ++it) {
        const Cable& cable = *it;
        if (cable.path.empty() || cable.removing) {
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
