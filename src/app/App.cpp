#include "app/App.h"

#include "ui/Ui.h"

namespace cable::app {

bool App::Initialize(HWND hwnd) {
    progress_ = game::LoadProgress();
    if (!renderContext_.Initialize(hwnd)) {
        return false;
    }
    audio_.Initialize();
    audio_.SetMasterVolume(progress_.masterVolume);
    ChangeScene(SceneId::MainMenu);
    return true;
}

void App::Resize(int width, int height) {
    progress_.windowWidth = width;
    progress_.windowHeight = height;
    renderContext_.Resize(width, height);
}

void App::Update(float dt) {
    if (scene_) {
        scene_->Update(dt);
    }
    for (auto& overlay : overlays_) {
        overlay->Update(dt);
    }
}

void App::Render() {
    renderContext_.BeginFrame(graphics::Color(0.055f, 0.075f, 0.09f));
    if (scene_) {
        scene_->Render(renderContext_);
    }
    for (auto& overlay : overlays_) {
        overlay->Render(renderContext_);
    }
    renderContext_.EndFrame();
}

void App::OnMouseMove(float x, float y) {
    if (!overlays_.empty()) {
        overlays_.back()->OnMouseMove(x, y);
        return;
    }
    if (scene_) {
        scene_->OnMouseMove(x, y);
    }
}

void App::OnMouseDown(float x, float y) {
    if (!overlays_.empty()) {
        overlays_.back()->OnMouseDown(x, y);
        return;
    }
    if (scene_) {
        scene_->OnMouseDown(x, y);
    }
}

void App::ChangeScene(SceneId id, int levelNumber) {
    overlays_.clear();
    switch (id) {
    case SceneId::MainMenu:
        scene_ = ui::MakeMainMenuScene(*this);
        break;
    case SceneId::LevelSelect:
        scene_ = ui::MakeLevelSelectScene(*this);
        break;
    case SceneId::Game:
        scene_ = ui::MakeGameScene(*this, levelNumber);
        break;
    }
}

void App::PushOverlay(std::unique_ptr<core::Overlay> overlay) {
    overlays_.push_back(std::move(overlay));
}

void App::CloseTopOverlay() {
    if (!overlays_.empty()) {
        overlays_.pop_back();
    }
}

void App::RequestClose() {
    PushOverlay(ui::MakeConfirmExitOverlay(*this));
}

void App::ConfirmExit() {
    SaveProgressNow();
    shouldQuit_ = true;
}

void App::SaveProgressNow() {
    if (persistProgress_) {
        game::SaveProgress(progress_);
    }
}

void App::ShowViewerScene(const std::string& scene, const std::string& overlay, const std::string& mock) {
    progress_ = game::Progress{};
    if (mock == "progress") {
        progress_.unlockedLevel = game::MaxLevel;
        progress_.levels[1] = {8, 24.0f, 3};
        progress_.levels[2] = {11, 38.0f, 2};
        progress_.levels[3] = {14, 52.0f, 1};
        progress_.levels[4] = {20, 88.0f, 0};
    }

    if (scene == "levels") {
        ChangeScene(SceneId::LevelSelect);
    } else if (scene == "game-high") {
        ChangeScene(SceneId::Game, game::MaxLevel);
    } else if (scene == "game" || scene == "complete") {
        ChangeScene(SceneId::Game, 12);
    } else {
        ChangeScene(SceneId::MainMenu);
    }

    if (overlay == "help") {
        PushOverlay(ui::MakeHelpOverlay(*this));
    } else if (overlay == "about") {
        PushOverlay(ui::MakeAboutOverlay(*this));
    } else if (overlay == "confirm-exit") {
        PushOverlay(ui::MakeConfirmExitOverlay(*this));
    } else if (overlay == "complete" || scene == "complete") {
        PushOverlay(ui::MakeLevelCompleteOverlay(*this, {12, 18, 2, 74.0f, 2}));
    }
}

} // namespace cable::app
