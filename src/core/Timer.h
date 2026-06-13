#pragma once

#include <chrono>

namespace cable::core {

class FrameTimer {
public:
    float Tick() {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<float> dt = now - last_;
        last_ = now;
        return dt.count();
    }

private:
    std::chrono::steady_clock::time_point last_{std::chrono::steady_clock::now()};
};

} // namespace cable::core
