#include "ui/Ui.h"

#include "app/App.h"

#include <cstdio>

namespace cable::ui {
namespace {

using graphics::Color;

void DrawModal(graphics::RenderContext& context, core::Rect rect) {
    context.FillRect({0.0f, 0.0f, 1280.0f, 720.0f}, Color(0.0f, 0.0f, 0.0f, 0.55f));
    context.FillRect(rect, Color(0.075f, 0.105f, 0.12f, 0.98f));
    context.StrokeRect(rect, Color(0.42f, 0.70f, 0.76f), 2.0f);
}

class SimpleOverlay final : public core::Overlay {
public:
    enum class Kind {
        ConfirmExit,
        Help,
        About,
        Complete
    };

    SimpleOverlay(app::App& app, Kind kind, game::LevelResult result = {})
        : app_(app), kind_(kind), result_(result) {
        if (kind == Kind::ConfirmExit) {
            buttons_ = {
                {{470.0f, 424.0f, 150.0f, 48.0f}, "确认退出"},
                {{660.0f, 424.0f, 150.0f, 48.0f}, "取消"}
            };
        } else if (kind == Kind::Complete) {
            buttons_ = {
                {{440.0f, 474.0f, 180.0f, 48.0f}, "继续下一关", result.levelNumber < game::MaxLevel},
                {{660.0f, 474.0f, 180.0f, 48.0f}, "返回主菜单"}
            };
        } else {
            buttons_ = {{{560.0f, 512.0f, 160.0f, 48.0f}, "关闭"}};
        }
    }

    void Update(float) override {}

    void Render(graphics::RenderContext& context) override {
        if (kind_ == Kind::ConfirmExit) {
            DrawModal(context, {380.0f, 250.0f, 520.0f, 260.0f});
            context.DrawTextUtf8("确认关闭游戏？", {380.0f, 304.0f, 520.0f, 64.0f}, 30.0f, Color(0.96f, 0.98f, 0.92f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        } else if (kind_ == Kind::Help) {
            DrawModal(context, {300.0f, 110.0f, 680.0f, 520.0f});
            context.DrawTextUtf8("帮助", {300.0f, 140.0f, 680.0f, 46.0f}, 32.0f, Color(0.96f, 0.98f, 0.92f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            context.DrawTextUtf8("点击数据线任意位置。插头在棋盘边缘并且朝向外侧时，数据线会被抽走。不能抽走的数据线会轻微抖动。清空所有数据线即可过关。", {360.0f, 218.0f, 560.0f, 190.0f}, 23.0f, Color(0.82f, 0.88f, 0.86f), DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            context.DrawTextUtf8("低关卡棋盘更小，高关卡棋盘更大，数据线也更细更密。", {360.0f, 420.0f, 560.0f, 70.0f}, 21.0f, Color(0.55f, 0.84f, 0.92f), DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        } else if (kind_ == Kind::About) {
            DrawModal(context, {360.0f, 150.0f, 560.0f, 450.0f});
            context.DrawTextUtf8("关于", {360.0f, 194.0f, 560.0f, 46.0f}, 32.0f, Color(0.96f, 0.98f, 0.92f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            context.DrawTextUtf8("开发者：sdcb\n版本：v1.0\n版权：MIT\n依赖：cJSON", {420.0f, 280.0f, 440.0f, 150.0f}, 24.0f, Color(0.82f, 0.88f, 0.86f), DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        } else {
            DrawModal(context, {340.0f, 150.0f, 600.0f, 420.0f});
            char summary[160]{};
            std::snprintf(summary, sizeof(summary), "点击次数：%d\n用时：%s\n失败尝试：%d\n评分：", result_.clicks, game::FormatSeconds(result_.seconds).c_str(), result_.failures);
            context.DrawTextUtf8("过关", {340.0f, 190.0f, 600.0f, 56.0f}, 38.0f, Color(0.96f, 0.98f, 0.92f), DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            context.DrawTextUtf8(summary, {460.0f, 278.0f, 360.0f, 150.0f}, 25.0f, Color(0.82f, 0.88f, 0.86f), DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            DrawStars(context, 536.0f, 390.0f, 15.0f, result_.stars);
        }
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
            return true;
        }
        if (kind_ == Kind::ConfirmExit) {
            if (hit == 0) {
                app_.Audio().Play(audio::SoundId::Confirm);
                app_.ConfirmExit();
            } else {
                app_.Audio().Play(audio::SoundId::Cancel);
                app_.CloseTopOverlay();
            }
        } else if (kind_ == Kind::Complete) {
            app_.Audio().Play(audio::SoundId::ButtonClick);
            if (hit == 0 && result_.levelNumber < game::MaxLevel) {
                app_.ChangeScene(app::SceneId::Game, result_.levelNumber + 1);
            } else {
                app_.ChangeScene(app::SceneId::MainMenu);
            }
        } else {
            app_.Audio().Play(audio::SoundId::Cancel);
            app_.CloseTopOverlay();
        }
        return true;
    }

private:
    app::App& app_;
    Kind kind_{};
    game::LevelResult result_{};
    std::vector<Button> buttons_;
};

} // namespace

std::unique_ptr<core::Overlay> MakeConfirmExitOverlay(app::App& app) {
    return std::make_unique<SimpleOverlay>(app, SimpleOverlay::Kind::ConfirmExit);
}

std::unique_ptr<core::Overlay> MakeHelpOverlay(app::App& app) {
    return std::make_unique<SimpleOverlay>(app, SimpleOverlay::Kind::Help);
}

std::unique_ptr<core::Overlay> MakeAboutOverlay(app::App& app) {
    return std::make_unique<SimpleOverlay>(app, SimpleOverlay::Kind::About);
}

std::unique_ptr<core::Overlay> MakeLevelCompleteOverlay(app::App& app, const game::LevelResult& result) {
    return std::make_unique<SimpleOverlay>(app, SimpleOverlay::Kind::Complete, result);
}

} // namespace cable::ui
