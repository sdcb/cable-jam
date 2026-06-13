#include "ui/Ui.h"

#include "app/App.h"
#include "game/LevelGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>

namespace cable::ui {
namespace {

using graphics::Color;

const std::array<D2D1_COLOR_F, 7> CableColors{
    Color(0.90f, 0.22f, 0.21f),
    Color(0.96f, 0.49f, 0.00f),
    Color(0.93f, 0.25f, 0.48f),
    Color(0.26f, 0.63f, 0.28f),
    Color(0.15f, 0.78f, 0.85f),
    Color(0.12f, 0.53f, 0.90f),
    Color(0.49f, 0.34f, 0.76f)
};

void DrawBackdrop(graphics::RenderContext& context) {
    context.Clear(Color(0.055f, 0.075f, 0.09f));
    context.FillRect({0.0f, 0.0f, 1280.0f, 720.0f}, Color(0.055f, 0.075f, 0.09f));
    context.FillRect({0.0f, 0.0f, 1280.0f, 86.0f}, Color(0.09f, 0.14f, 0.16f));
    context.FillRect({0.0f, 650.0f, 1280.0f, 70.0f}, Color(0.04f, 0.055f, 0.07f));
}

void DrawMenuDecorations(graphics::RenderContext& context) {
    const std::array<core::Point, 5> leftCable{{{105.0f, 160.0f}, {105.0f, 280.0f}, {180.0f, 280.0f}, {180.0f, 410.0f}, {260.0f, 410.0f}}};
    const std::array<core::Point, 5> rightCable{{{1110.0f, 145.0f}, {1035.0f, 145.0f}, {1035.0f, 265.0f}, {955.0f, 265.0f}, {955.0f, 385.0f}}};
    const std::array<core::Point, 4> bottomCable{{{285.0f, 590.0f}, {430.0f, 590.0f}, {430.0f, 540.0f}, {560.0f, 540.0f}}};
    const std::array<core::Point, 4> lowCable{{{730.0f, 560.0f}, {855.0f, 560.0f}, {855.0f, 612.0f}, {1000.0f, 612.0f}}};
    context.DrawPolyline(leftCable, Color(0.15f, 0.78f, 0.85f, 0.15f), 13.0f);
    context.DrawPolyline(rightCable, Color(0.96f, 0.49f, 0.00f, 0.16f), 12.0f);
    context.DrawPolyline(bottomCable, Color(0.49f, 0.34f, 0.76f, 0.15f), 10.0f);
    context.DrawPolyline(lowCable, Color(0.26f, 0.63f, 0.28f, 0.13f), 9.0f);
    context.StrokeEllipse({244.0f, 394.0f, 32.0f, 32.0f}, Color(0.94f, 0.95f, 0.86f, 0.18f), 4.0f);
    context.StrokeRect({939.0f, 372.0f, 36.0f, 22.0f}, Color(0.94f, 0.95f, 0.86f, 0.18f), 3.0f);
}

float PolylineLength(const std::vector<core::Point>& points) {
    float length = 0.0f;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    return length;
}

core::Point Lerp(core::Point a, core::Point b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

std::optional<core::Point> PointAtDistance(const std::vector<core::Point>& points, float distance) {
    if (points.empty()) {
        return std::nullopt;
    }
    if (distance <= 0.0f) {
        return points.front();
    }
    float walked = 0.0f;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        const float segment = std::sqrt(dx * dx + dy * dy);
        if (walked + segment >= distance) {
            return Lerp(points[i - 1], points[i], segment <= 0.0f ? 0.0f : (distance - walked) / segment);
        }
        walked += segment;
    }
    return points.back();
}

std::vector<core::Point> SlicePolyline(const std::vector<core::Point>& points, float from, float to) {
    std::vector<core::Point> result;
    const float total = PolylineLength(points);
    from = std::clamp(from, 0.0f, total);
    to = std::clamp(to, 0.0f, total);
    if (to <= from || points.size() < 2) {
        return result;
    }
    if (auto start = PointAtDistance(points, from)) {
        result.push_back(*start);
    }

    float walked = 0.0f;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        const float segment = std::sqrt(dx * dx + dy * dy);
        const float segmentStart = walked;
        const float segmentEnd = walked + segment;
        if (segmentEnd > from && segmentEnd < to) {
            result.push_back(points[i]);
        }
        walked += segment;
    }

    if (auto end = PointAtDistance(points, to)) {
        if (result.empty() || std::abs(result.back().x - end->x) > 0.01f || std::abs(result.back().y - end->y) > 0.01f) {
            result.push_back(*end);
        }
    }
    return result;
}

float ExitDistance(core::Point plug, core::Point direction) {
    if (direction.x < 0.0f) {
        return plug.x + 160.0f;
    }
    if (direction.x > 0.0f) {
        return 1280.0f - plug.x + 160.0f;
    }
    if (direction.y < 0.0f) {
        return plug.y + 160.0f;
    }
    return 720.0f - plug.y + 160.0f;
}

core::Rect CenteredRect(core::Point center, float width, float height) {
    return {center.x - width * 0.5f, center.y - height * 0.5f, width, height};
}

void DrawPlug(graphics::RenderContext& context, core::Point center, game::Direction direction, int style, float size) {
    const core::Point dir = game::DirectionVector(direction);
    const bool horizontal = std::abs(dir.x) > 0.0f;
    const D2D1_COLOR_F shell = Color(0.94f, 0.95f, 0.86f);
    const D2D1_COLOR_F stroke = Color(0.02f, 0.03f, 0.04f);
    const D2D1_COLOR_F dark = Color(0.09f, 0.11f, 0.12f);
    const D2D1_COLOR_F gold = Color(0.96f, 0.74f, 0.26f);

    if (style == 0) {
        const float w = horizontal ? size * 0.82f : size * 0.54f;
        const float h = horizontal ? size * 0.54f : size * 0.82f;
        context.FillRect(CenteredRect(center, w, h), shell);
        context.StrokeRect(CenteredRect(center, w, h), stroke, 2.0f);
        const float slotW = horizontal ? size * 0.38f : size * 0.12f;
        const float slotH = horizontal ? size * 0.12f : size * 0.38f;
        context.FillRect(CenteredRect({center.x + dir.x * size * 0.14f, center.y + dir.y * size * 0.14f}, slotW, slotH), dark);
    } else if (style == 1) {
        const float w = horizontal ? size * 0.92f : size * 0.36f;
        const float h = horizontal ? size * 0.36f : size * 0.92f;
        context.FillRect(CenteredRect(center, w, h), Color(0.88f, 0.89f, 0.82f));
        context.StrokeRect(CenteredRect(center, w, h), stroke, 2.0f);
        core::Point contact = {center.x + dir.x * size * 0.25f, center.y + dir.y * size * 0.25f};
        context.FillRect(CenteredRect(contact, horizontal ? size * 0.10f : size * 0.28f, horizontal ? size * 0.28f : size * 0.10f), gold);
    } else {
        context.FillEllipse(CenteredRect(center, size * 0.78f, size * 0.78f), shell);
        context.StrokeEllipse(CenteredRect(center, size * 0.78f, size * 0.78f), stroke, 2.0f);
        context.FillEllipse(CenteredRect(center, size * 0.36f, size * 0.36f), dark);
        context.StrokeEllipse(CenteredRect(center, size * 0.20f, size * 0.20f), Color(0.78f, 0.82f, 0.76f), 1.5f);
    }

    const core::Point neckA{center.x + dir.x * size * 0.36f, center.y + dir.y * size * 0.36f};
    const core::Point neckB{center.x + dir.x * size * 0.54f, center.y + dir.y * size * 0.54f};
    context.DrawLine(neckA, neckB, dark, std::max(3.0f, size * 0.10f));
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
        DrawMenuDecorations(context);
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
        if (hit == 0) {
            app_.Audio().EnsureProceduralSoundsCreated();
            app_.Audio().Play(audio::SoundId::ButtonClick);
            app_.ChangeScene(app::SceneId::Game, app_.Progress().unlockedLevel);
        } else if (hit == 1) {
            app_.Audio().Play(audio::SoundId::ButtonClick);
            app_.ChangeScene(app::SceneId::LevelSelect);
        } else if (hit == 2) {
            app_.Audio().Play(audio::SoundId::ButtonClick);
            app_.PushOverlay(MakeHelpOverlay(app_));
        } else if (hit == 3) {
            app_.Audio().Play(audio::SoundId::ButtonClick);
            app_.PushOverlay(MakeAboutOverlay(app_));
        } else {
            app_.Audio().Play(audio::SoundId::ButtonClick);
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
        for (int i = 0; i < game::MaxLevel; ++i) {
            const int col = i % 9;
            const int row = i / 9;
            const float x = 138.0f + static_cast<float>(col) * 112.0f;
            const float y = 112.0f + static_cast<float>(row) * 54.0f;
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
                button.text.clear();
            }
            DrawButton(context, button);
            if (button.enabled && found != app_.Progress().levels.end()) {
                context.DrawTextUtf8(std::to_string(levelNumber), {button.rect.x, button.rect.y + 3.0f, button.rect.width, 17.0f}, 15.0f, Color(0.96f, 0.98f, 0.92f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                DrawStars(context, button.rect.x + 24.0f, button.rect.y + 28.0f, 5.5f, found->second.bestStars);
            }
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
        const std::optional<int> hit = state_.HitTestCable({x, y});
        const bool removed = hit && state_.TryClickCableById(*hit);
        if (removed) {
            app_.Audio().PlayCablePull();
        } else if (hit) {
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
        std::vector<core::Point> travelPath;
        const auto& layout = state_.Layout();
        const core::Point direction = game::DirectionVector(cable.plugDirection);
        const float shake = cable.shakeTime > 0.0f ? std::sin(cable.shakeTime * 82.0f) * 5.0f : 0.0f;
        for (auto it = cable.path.rbegin(); it != cable.path.rend(); ++it) {
            game::GridPos pos = *it;
            core::Point p = game::CellCenter(layout, pos);
            p.x += shake;
            travelPath.push_back(p);
        }
        if (travelPath.empty()) {
            return;
        }
        const float originalLength = PolylineLength(travelPath);
        const core::Point plugStart = travelPath.back();
        const float exit = ExitDistance(plugStart, direction);
        travelPath.push_back({plugStart.x + direction.x * exit, plugStart.y + direction.y * exit});
        const float travel = core::Ease(core::Easing::InOutCubic, cable.removeProgress) * (originalLength + exit);
        const std::vector<core::Point> points = cable.removing ? SlicePolyline(travelPath, travel, travel + originalLength) : SlicePolyline(travelPath, 0.0f, originalLength);
        if (points.empty()) {
            return;
        }
        if (points.size() >= 2) {
            context.DrawPolyline(points, Color(0.025f, 0.035f, 0.045f), layout.cableWidth + 8.0f);
            context.DrawPolyline(points, CableColors[static_cast<std::size_t>(cable.colorIndex) % CableColors.size()], layout.cableWidth);
        }
        const core::Point plug = points.back();
        DrawPlug(context, plug, cable.plugDirection, cable.plugStyle, layout.plugSize);
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

void DrawStars(graphics::RenderContext& context, float x, float y, float size, int count) {
    constexpr float Pi = 3.1415926535f;
    for (int star = 0; star < 3; ++star) {
        const core::Point center{x + static_cast<float>(star) * size * 2.35f, y};
        std::array<core::Point, 10> points{};
        for (int i = 0; i < 10; ++i) {
            const float radius = (i % 2 == 0) ? size : size * 0.45f;
            const float angle = -Pi * 0.5f + static_cast<float>(i) * Pi / 5.0f;
            points[static_cast<std::size_t>(i)] = {center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius};
        }
        if (star < count) {
            std::array<core::Point, 10> shadow = points;
            for (core::Point& p : shadow) {
                p.y += 1.4f;
            }
            context.FillPolygon(shadow, Color(0.42f, 0.25f, 0.04f, 0.72f));
            context.FillPolygon(points, Color(1.0f, 0.76f, 0.16f));
            context.StrokePolygon(points, Color(1.0f, 0.93f, 0.45f), 1.1f);
            std::array<core::Point, 10> shine = points;
            for (core::Point& p : shine) {
                p.x = center.x + (p.x - center.x) * 0.55f;
                p.y = center.y + (p.y - center.y) * 0.55f - size * 0.12f;
            }
            context.FillPolygon(shine, Color(1.0f, 0.92f, 0.45f, 0.50f));
        } else {
            context.StrokePolygon(points, Color(0.78f, 0.70f, 0.44f, 0.75f), 1.4f);
        }
    }
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
