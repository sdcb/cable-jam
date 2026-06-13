#include "game/ProgressStore.h"

#include "core/WinFile.h"

#include <algorithm>
#include <cJSON.h>

namespace cable::game {
namespace {

int ClampLevel(int value) {
    return std::clamp(value, 1, 100);
}

} // namespace

Progress LoadProgress(const std::string& path) {
    Progress progress;
    const std::string content = core::ReadTextFile(path);
    if (content.empty()) {
        return progress;
    }

    cJSON* root = cJSON_Parse(content.c_str());
    if (!root) {
        return progress;
    }

    if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "masterVolume"); cJSON_IsNumber(value)) {
        progress.masterVolume = std::clamp(static_cast<float>(value->valuedouble), 0.0f, 1.0f);
    }
    if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "windowWidth"); cJSON_IsNumber(value)) {
        progress.windowWidth = std::max(1280, value->valueint);
    }
    if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "windowHeight"); cJSON_IsNumber(value)) {
        progress.windowHeight = std::max(720, value->valueint);
    }
    if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "unlockedLevel"); cJSON_IsNumber(value)) {
        progress.unlockedLevel = ClampLevel(value->valueint);
    }

    const cJSON* levels = cJSON_GetObjectItemCaseSensitive(root, "levels");
    if (cJSON_IsObject(levels)) {
        const cJSON* item = nullptr;
        cJSON_ArrayForEach(item, levels) {
            if (!cJSON_IsObject(item) || !item->string) {
                continue;
            }
            const int levelNumber = ClampLevel(std::atoi(item->string));
            LevelProgress level;
            if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(item, "bestClicks"); cJSON_IsNumber(value)) {
                level.bestClicks = std::max(0, value->valueint);
            }
            if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(item, "bestSeconds"); cJSON_IsNumber(value)) {
                level.bestSeconds = std::max(0.0f, static_cast<float>(value->valuedouble));
            }
            if (const cJSON* value = cJSON_GetObjectItemCaseSensitive(item, "bestStars"); cJSON_IsNumber(value)) {
                level.bestStars = std::clamp(value->valueint, 0, 3);
            }
            progress.levels[levelNumber] = level;
        }
    }

    cJSON_Delete(root);
    return progress;
}

bool SaveProgress(const Progress& progress, const std::string& path) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "masterVolume", progress.masterVolume);
    cJSON_AddNumberToObject(root, "windowWidth", progress.windowWidth);
    cJSON_AddNumberToObject(root, "windowHeight", progress.windowHeight);
    cJSON_AddNumberToObject(root, "unlockedLevel", progress.unlockedLevel);

    cJSON* levels = cJSON_CreateObject();
    for (const auto& [levelNumber, level] : progress.levels) {
        char key[8]{};
        std::snprintf(key, sizeof(key), "%d", levelNumber);
        cJSON* item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "bestClicks", level.bestClicks);
        cJSON_AddNumberToObject(item, "bestSeconds", level.bestSeconds);
        cJSON_AddNumberToObject(item, "bestStars", level.bestStars);
        cJSON_AddItemToObject(levels, key, item);
    }
    cJSON_AddItemToObject(root, "levels", levels);

    char* text = cJSON_Print(root);
    cJSON_Delete(root);
    if (!text) {
        return false;
    }
    const bool ok = core::WriteTextFile(path, text);
    cJSON_free(text);
    return ok;
}

void RecordCompletion(Progress& progress, int levelNumber, int clicks, float seconds, int stars) {
    levelNumber = ClampLevel(levelNumber);
    LevelProgress& current = progress.levels[levelNumber];
    const bool first = current.bestClicks == 0 && current.bestSeconds == 0.0f && current.bestStars == 0;
    const bool better =
        first ||
        stars > current.bestStars ||
        (stars == current.bestStars && clicks < current.bestClicks) ||
        (stars == current.bestStars && clicks == current.bestClicks && seconds < current.bestSeconds);
    if (better) {
        current.bestClicks = clicks;
        current.bestSeconds = seconds;
        current.bestStars = stars;
    }
    if (levelNumber < 100) {
        progress.unlockedLevel = std::max(progress.unlockedLevel, levelNumber + 1);
    }
}

} // namespace cable::game
