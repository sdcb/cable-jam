#pragma once

#include <map>
#include <string>

namespace cable::game {

struct LevelProgress {
    int bestClicks{};
    float bestSeconds{};
    int bestStars{};
};

struct Progress {
    float masterVolume{0.8f};
    int windowWidth{1280};
    int windowHeight{720};
    int unlockedLevel{1};
    std::map<int, LevelProgress> levels;
};

Progress LoadProgress(const std::string& path = "appsettings.json");
bool SaveProgress(const Progress& progress, const std::string& path = "appsettings.json");
void RecordCompletion(Progress& progress, int levelNumber, int clicks, float seconds, int stars);

} // namespace cable::game
