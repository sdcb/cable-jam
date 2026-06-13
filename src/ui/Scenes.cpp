#include "ui/Ui.h"

#include "app/App.h"
#include "game/LevelGenerator.h"

#include <array>
#include <cmath>
#include <cstdio>

namespace cable::ui {
namespace {

using graphics::Color;

const std::array<D2D1_COLOR_F, 8> CableColors{
    Color(0.15f, 0.56f, 0.92f),
    Color(0.92f, 0.30f, 0.53f),
    Color(0.95f, 0.72f, 0.22f),
    Color(0.30f, 0.75f, 0.44f),
    Color(0.58f, 0.42f, 0.95f),
    Color(0.93f, 0.48f, 0.22f),
    Color(0.20f, 0.78f, 0.78f),
    Color(0.82f, 0.84f, 0.38f)
};

void DrawBackdrop(graphics::RenderContext& context) {
    context.Clear(Color(0.055f, 0.075f, 0.09f));
    context.FillRect({0.0f, 0.0f, 1280.0f, 720.0f}, Color(0.055f, 0.075f, 0.09f));
    context.FillRect({0.0f, 0.0f, 1280.0f, 86.0f}, Color(0.09f, 0.14f, 0.16f));
    context.FillRect({0.0f, 650.0f, 1280.0f, 70.0f}, Color(0.04f, 0.055f, 0.07f));
}

std::string Stars(int count) {
    std::string value;
    for (int i = 0; i < 3; ++i) {
        value += i < count ? "*" : "-";
    }
    return value;
}

class MainMenuScene final : public core::Scene {
public:
    explicit MainMenuScene(app::App& app) : app_(app) {
        buttons_ = {
            {{520.0f, 228.0f, 240.0f, 54.0f}, "开始游戏"},
            {{520.0f, 298.0f, 240.0f, 54.0f}, "关卡选择"},
            {{520.0f, 368.0f, 240.0f, 54.0f}, "帮助"},
            {{520.0f, 438.0f, 240.0f, 54.0f}, "关于"},
            {{520.0f, 508.0f, 240.0f, 54.0f}, "退出"}
        };
    }

    void Update(float) override {}

    void Render(graphics::RenderContext& context) override {
        DrawBackdrop(context);
        context.DrawTextUtf8("Cable Jam", {0.0f, 108.0f, 1280.0f, 64.0f}, 50.0f, Color(0.94f, 0.95f, 0.88f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        context.DrawTextUtf8("充电请排队", {0.0f, 166.0f, 1280.0f, 40.0f}, 30.0f, Color(0.40f, 0.80f, 0.95f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        for (const Button& button : buttons_) {
            DrawButton(context, button);
        }
    }

    bool OnMouseMove(float x, float y) override {
        UpdateButtonHover(buttons_, x, y);
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        const int hit = HitButton(buttons_, x, y);
        if (hit < 0) {
            return false;
        }
        app_.Audio().Play(audio::SoundId::ButtonClick);
        if (hit == 0) {
            app_.ChangeScene(app::SceneId::Game, app_.Progress().unlockedLevel);
        } else if (hit == 1) {
            app_.ChangeScene(app::SceneId::LevelSelect);
        } else if (hit == 2) {
            app_.PushOverlay(MakeHelpOverlay(app_));
        } else if (hit == 3) {
            app_.PushOverlay(MakeAboutOverlay(app_));
        } else {
            app_.RequestClose();
        }
        return true;
    }

private:
    app::App& app_;
    std::vector<Button> buttons_;
};

class LevelSelectScene final : public core::Scene {
public:
    explicit LevelSelectScene(app::App& app) : app_(app) {
        buttons_.push_back({{46.0f, 660.0f, 132.0f, 42.0f}, "返回"});
        for (int i = 0; i < 100; ++i) {
            const int col = i % 10;
            const int row = i / 10;
            const float x = 110.0f + static_cast<float>(col) * 106.0f;
            const float y = 118.0f + static_cast<float>(row) * 50.0f;
            Button button{{x, y, 88.0f, 38.0f}, std::to_string(i + 1), i + 1 <= app_.Progress().unlockedLevel, false, 18.0f};
            buttons_.push_back(button);
        }
    }

    void Update(float) override {}

    void Render(graphics::RenderContext& context) override {
        DrawBackdrop(context);
        context.DrawTextUtf8("关卡选择", {0.0f, 20.0f, 1280.0f, 50.0f}, 34.0f, Color(0.94f, 0.95f, 0.88f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        for (int i = 1; i < static_cast<int>(buttons_.size()); ++i) {
            Button button = buttons_[static_cast<std::size_t>(i)];
            const int levelNumber = i;
            auto found = app_.Progress().levels.find(levelNumber);
            if (!button.enabled) {
                button.text = "锁";
            } else if (found != app_.Progress().levels.end()) {
                button.text = std::to_string(levelNumber) + " " + Stars(found->second.bestStars);
                button.fontSize = 14.0f;
            }
            DrawButton(context, button);
        }
        DrawButton(context, buttons_[0]);
    }

    bool OnMouseMove(float x, float y) override {
        UpdateButtonHover(buttons_, x, y);
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        const int hit = HitButton(buttons_, x, y);
        if (hit < 0) {
            return false;
        }
        app_.Audio().Play(audio::SoundId::ButtonClick);
        if (hit == 0) {
            app_.ChangeScene(app::SceneId::MainMenu);
        } else {
            app_.ChangeScene(app::SceneId::Game, hit);
        }
        return true;
    }

private:
    app::App& app_;
    std::vector<Button> buttons_;
};

class GameScene final : public core::Scene {
public:
    GameScene(app::App& app, int levelNumber)
        : app_(app), state_(game::LevelGenerator::Generate(levelNumber)) {
        buttons_ = {{{46.0f, 660.0f, 132.0f, 42.0f}, "返回"}};
    }

    void Update(float dt) override {
        state_.Update(dt);
        if (state_.Completed() && !completionShown_) {
            completionShown_ = true;
            const game::LevelResult result = state_.CompleteResult();
            game::RecordCompletion(app_.Progress(), result.levelNumber, result.clicks, result.seconds, result.stars);
            app_.SaveProgressNow();
            app_.Audio().Play(audio::SoundId::LevelComplete);
            app_.PushOverlay(MakeLevelCompleteOverlay(app_, result));
        }
    }

    void Render(graphics::RenderContext& context) override {
        DrawBackdrop(context);
        char left[64]{};
        std::snprintf(left, sizeof(left), "第 %d 关   点击 %d", state_.CurrentLevel().number, state_.Clicks());
        context.DrawTextUtf8(left, {42.0f, 22.0f, 360.0f, 44.0f}, 24.0f, Color(0.92f, 0.95f, 0.88f));
        context.DrawTextUtf8(game::FormatSeconds(state_.Seconds()), {1040.0f, 22.0f, 190.0f, 44.0f}, 25.0f, Color(0.92f, 0.95f, 0.88f), DWRITE_TEXT_ALIGNMENT_TRAILING);
        DrawBoard(context);
        DrawButton(context, buttons_[0]);
    }

    bool OnMouseMove(float x, float y) override {
        UpdateButtonHover(buttons_, x, y);
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        if (HitButton(buttons_, x, y) == 0) {
            app_.Audio().Play(audio::SoundId::Cancel);
            app_.ChangeScene(app::SceneId::MainMenu);
            return true;
        }
        const bool removed = state_.TryClickCable({x, y});
        if (removed) {
            app_.Audio().Play(audio::SoundId::CablePull);
        } else if (state_.HitTestCable({x, y})) {
            app_.Audio().Play(audio::SoundId::CableBlocked);
        }
        return true;
    }

private:
    void DrawBoard(graphics::RenderContext& context) {
        const auto& layout = state_.Layout();
        const float boardWidth = layout.cellSize * layout.size.columns;
        const float boardHeight = layout.cellSize * layout.size.rows;
        context.FillRect({layout.originX - 18.0f, layout.originY - 18.0f, boardWidth + 36.0f, boardHeight + 36.0f}, Color(0.09f, 0.115f, 0.13f));
        context.StrokeRect({layout.originX - 18.0f, layout.originY - 18.0f, boardWidth + 36.0f, boardHeight + 36.0f}, Color(0.29f, 0.43f, 0.47f), 2.0f);
        for (int c = 0; c <= layout.size.columns; ++c) {
            const float x = layout.originX + c * layout.cellSize;
            context.DrawLine({x, layout.originY}, {x, layout.originY + boardHeight}, Color(0.18f, 0.24f, 0.26f), 1.0f);
        }
        for (int r = 0; r <= layout.size.rows; ++r) {
            const float y = layout.originY + r * layout.cellSize;
            context.DrawLine({layout.originX, y}, {layout.originX + boardWidth, y}, Color(0.18f, 0.24f, 0.26f), 1.0f);
        }
        for (const game::Cable& cable : state_.CurrentLevel().cables) {
            DrawCable(context, cable);
        }
    }

    void DrawCable(graphics::RenderContext& context, const game::Cable& cable) {
        std::vector<core::Point> points;
        const auto& layout = state_.Layout();
        const core::Point direction = game::DirectionVector(cable.plugDirection);
        const float remove = core::Ease(core::Easing::InOutCubic, cable.removeProgress) * layout.cellSize * 3.2f;
        const float shake = cable.shakeTime > 0.0f ? std::sin(cable.shakeTime * 82.0f) * 5.0f : 0.0f;
        for (game::GridPos pos : cable.path) {
            core::Point p = game::CellCenter(layout, pos);
            p.x += direction.x * remove + shake;
            p.y += direction.y * remove;
            points.push_back(p);
        }
        if (points.size() >= 2) {
            context.DrawPolyline(points, Color(0.025f, 0.035f, 0.045f), layout.cableWidth + 8.0f);
            context.DrawPolyline(points, CableColors[static_cast<std::size_t>(cable.colorIndex) % CableColors.size()], layout.cableWidth);
        }
        const core::Point plug = points.empty() ? core::Point{} : points.front();
        context.FillEllipse({plug.x - layout.plugSize * 0.5f, plug.y - layout.plugSize * 0.5f, layout.plugSize, layout.plugSize}, Color(0.94f, 0.95f, 0.88f));
        context.StrokeEllipse({plug.x - layout.plugSize * 0.5f, plug.y - layout.plugSize * 0.5f, layout.plugSize, layout.plugSize}, Color(0.02f, 0.03f, 0.04f), 2.0f);
        const core::Point tip{plug.x + direction.x * layout.plugSize * 0.42f, plug.y + direction.y * layout.plugSize * 0.42f};
        context.DrawLine(plug, tip, Color(0.10f, 0.14f, 0.16f), 4.0f);
        if (!points.empty()) {
            const core::Point tail = points.back();
            context.FillEllipse({tail.x - layout.cableWidth * 0.38f, tail.y - layout.cableWidth * 0.38f, layout.cableWidth * 0.76f, layout.cableWidth * 0.76f}, Color(0.97f, 0.92f, 0.76f));
        }
    }

    app::App& app_;
    game::GameState state_;
    std::vector<Button> buttons_;
    bool completionShown_{false};
};

} // namespace

void DrawButton(graphics::RenderContext& context, const Button& button) {
    const D2D1_COLOR_F fill = button.enabled
        ? (button.hover ? Color(0.20f, 0.58f, 0.72f) : Color(0.13f, 0.34f, 0.42f))
        : Color(0.13f, 0.15f, 0.16f);
    const D2D1_COLOR_F stroke = button.enabled ? Color(0.45f, 0.78f, 0.86f) : Color(0.30f, 0.33f, 0.34f);
    const D2D1_COLOR_F text = button.enabled ? Color(0.96f, 0.98f, 0.92f) : Color(0.52f, 0.56f, 0.57f);
    context.FillRect(button.rect, fill);
    context.StrokeRect(button.rect, stroke, 1.5f);
    context.DrawTextUtf8(button.text, button.rect, button.fontSize, text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

int HitButton(const std::vector<Button>& buttons, float x, float y) {
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        if (buttons[i].Hit(x, y)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void UpdateButtonHover(std::vector<Button>& buttons, float x, float y) {
    for (Button& button : buttons) {
        button.hover = button.Hit(x, y);
    }
}

std::unique_ptr<core::Scene> MakeMainMenuScene(app::App& app) {
    return std::make_unique<MainMenuScene>(app);
}

std::unique_ptr<core::Scene> MakeLevelSelectScene(app::App& app) {
    return std::make_unique<LevelSelectScene>(app);
}

std::unique_ptr<core::Scene> MakeGameScene(app::App& app, int levelNumber) {
    return std::make_unique<GameScene>(app, levelNumber);
}

} // namespace cable::ui
