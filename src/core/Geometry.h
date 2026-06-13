#pragma once

#include <algorithm>
#include <cmath>

namespace cable::core {

struct Point {
    float x{};
    float y{};
};

struct Rect {
    float x{};
    float y{};
    float width{};
    float height{};

    bool Contains(float px, float py) const {
        return px >= x && py >= y && px <= x + width && py <= y + height;
    }
};

struct ViewTransform {
    float scale{1.0f};
    float offsetX{};
    float offsetY{};
    float safeWidth{1280.0f};
    float safeHeight{720.0f};
};

inline Point ToLogical(Point p, const ViewTransform& transform) {
    return {
        (p.x - transform.offsetX) / std::max(0.001f, transform.scale),
        (p.y - transform.offsetY) / std::max(0.001f, transform.scale)
    };
}

inline float DistanceToSegment(Point p, Point a, Point b) {
    const float vx = b.x - a.x;
    const float vy = b.y - a.y;
    const float wx = p.x - a.x;
    const float wy = p.y - a.y;
    const float len2 = vx * vx + vy * vy;
    const float t = len2 <= 0.0001f ? 0.0f : std::clamp((wx * vx + wy * vy) / len2, 0.0f, 1.0f);
    const float cx = a.x + vx * t;
    const float cy = a.y + vy * t;
    const float dx = p.x - cx;
    const float dy = p.y - cy;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace cable::core
