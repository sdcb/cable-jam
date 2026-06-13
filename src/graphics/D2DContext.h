#pragma once

#include "core/Geometry.h"
#include "graphics/ComPtr.h"

#include <span>
#include <string>

#include <d2d1.h>
#include <dwrite.h>
#include <windows.h>
#include <wincodec.h>

namespace cable::graphics {

class RenderContext {
public:
    bool Initialize(HWND hwnd);
    void Resize(int pixelWidth, int pixelHeight);
    void BeginFrame(D2D1_COLOR_F safeColor = D2D1::ColorF(0.055f, 0.075f, 0.09f));
    bool EndFrame();
    void DiscardDeviceResources();
    bool EnsureDeviceResources();

    core::ViewTransform ViewTransform() const { return transform_; }
    ID2D1HwndRenderTarget* Target() const { return target_.Get(); }

    void Clear(D2D1_COLOR_F color);
    void FillRect(const core::Rect& rect, D2D1_COLOR_F color);
    void StrokeRect(const core::Rect& rect, D2D1_COLOR_F color, float width = 1.0f);
    void FillEllipse(const core::Rect& rect, D2D1_COLOR_F color);
    void StrokeEllipse(const core::Rect& rect, D2D1_COLOR_F color, float width = 1.0f);
    void DrawLine(core::Point a, core::Point b, D2D1_COLOR_F color, float width);
    void DrawPolyline(std::span<const core::Point> points, D2D1_COLOR_F color, float width);
    void DrawTextUtf8(
        const std::string& text,
        const core::Rect& rect,
        float fontSize,
        D2D1_COLOR_F color,
        DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT valign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    std::wstring Utf8ToWide(const std::string& text) const;

private:
    bool CreateTarget();
    ComPtr<ID2D1SolidColorBrush> CreateBrush(D2D1_COLOR_F color);

    HWND hwnd_{};
    int pixelWidth_{1280};
    int pixelHeight_{720};
    core::ViewTransform transform_{};
    ComPtr<ID2D1Factory> d2dFactory_;
    ComPtr<IDWriteFactory> dwriteFactory_;
    ComPtr<IWICImagingFactory> wicFactory_;
    ComPtr<ID2D1HwndRenderTarget> target_;
    ComPtr<ID2D1StrokeStyle> roundedStroke_;
};

inline D2D1_COLOR_F Color(float r, float g, float b, float a = 1.0f) {
    return D2D1::ColorF(r, g, b, a);
}

} // namespace cable::graphics
