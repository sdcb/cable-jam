#pragma once

#include "core/Scene.h"

namespace cable::core {

class Overlay : public Scene {
public:
    bool BlocksInputBelow() const { return true; }
};

} // namespace cable::core
