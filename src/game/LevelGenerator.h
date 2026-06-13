#pragma once

#include "game/Level.h"

#include <cstdint>

namespace cable::game {

class LevelGenerator {
public:
    static BoardSize BoardSizeForLevel(int levelNumber);
    static Level Generate(int levelNumber, std::uint32_t seedBase = 0x434A2026u);
};

} // namespace cable::game
