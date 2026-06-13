#pragma once

namespace cable::graphics {
class RenderContext;
}

namespace cable::core {

class Scene {
public:
    virtual ~Scene() = default;
    virtual void Update(float dt) = 0;
    virtual void Render(graphics::RenderContext& context) = 0;
    virtual bool OnMouseMove(float, float) { return false; }
    virtual bool OnMouseDown(float, float) { return false; }
};

} // namespace cable::core
