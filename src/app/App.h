#pragma once

#include "audio/AudioEngine.h"
#include "core/Overlay.h"
#include "core/Scene.h"
#include "game/ProgressStore.h"
#include "graphics/D2DContext.h"

#include <memory>
#include <string>
#include <vector>

namespace cable::app {

enum class SceneId {
    MainMenu,
    LevelSelect,
    Game
};

class App {
public:
    bool Initialize(HWND hwnd);
    void Resize(int width, int height);
    void Update(float dt);
    void Render();

    void OnMouseMove(float x, float y);
    void OnMouseDown(float x, float y);

    void ChangeScene(SceneId id, int levelNumber = 1);
    void PushOverlay(std::unique_ptr<core::Overlay> overlay);
    void CloseTopOverlay();
    void RequestClose();
    void ConfirmExit();
    bool ShouldQuit() const { return shouldQuit_; }

    graphics::RenderContext& RenderContext() { return renderContext_; }
    audio::AudioEngine& Audio() { return audio_; }
    game::Progress& Progress() { return progress_; }
    void SaveProgressNow();
    void SetProgressPersistence(bool enabled) { persistProgress_ = enabled; }
    void ShowViewerScene(const std::string& scene, const std::string& overlay, const std::string& mock);

private:
    graphics::RenderContext renderContext_;
    audio::AudioEngine audio_;
    game::Progress progress_;
    std::unique_ptr<core::Scene> scene_;
    std::vector<std::unique_ptr<core::Overlay>> overlays_;
    bool persistProgress_{true};
    bool shouldQuit_{false};
};

} // namespace cable::app
