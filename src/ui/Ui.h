#pragma once

#include "core/Geometry.h"
#include "core/Overlay.h"
#include "core/Scene.h"
#include "game/GameState.h"
#include "graphics/D2DContext.h"

#include <memory>
#include <string>
#include <vector>

namespace cable::app {
class App;
}

namespace cable::ui {

struct Button {
    core::Rect rect;
    std::string text;
    bool enabled{true};
    bool hover{false};
    float fontSize{22.0f};

    bool Hit(float x, float y) const {
        return enabled && rect.Contains(x, y);
    }
};

void DrawButton(graphics::RenderContext& context, const Button& button);
void DrawStars(graphics::RenderContext& context, float x, float y, float size, int count);
int HitButton(const std::vector<Button>& buttons, float x, float y);
void UpdateButtonHover(std::vector<Button>& buttons, float x, float y);

std::unique_ptr<core::Scene> MakeMainMenuScene(app::App& app);
std::unique_ptr<core::Scene> MakeLevelSelectScene(app::App& app);
std::unique_ptr<core::Scene> MakeGameScene(app::App& app, int levelNumber);

std::unique_ptr<core::Overlay> MakeConfirmExitOverlay(app::App& app);
std::unique_ptr<core::Overlay> MakeHelpOverlay(app::App& app);
std::unique_ptr<core::Overlay> MakeAboutOverlay(app::App& app);
std::unique_ptr<core::Overlay> MakeLevelCompleteOverlay(app::App& app, const game::LevelResult& result);

} // namespace cable::ui
