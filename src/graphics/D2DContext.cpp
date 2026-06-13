#include "graphics/D2DContext.h"

#include <algorithm>

namespace cable::graphics {

bool RenderContext::Initialize(HWND hwnd) {
    hwnd_ = hwnd;
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.Put()))) {
        return false;
    }
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(dwriteFactory_.Put())))) {
        return false;
    }
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wicFactory_.Put()));

    D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
        D2D1_CAP_STYLE_ROUND,
        D2D1_CAP_STYLE_ROUND,
        D2D1_CAP_STYLE_ROUND,
        D2D1_LINE_JOIN_ROUND);
    d2dFactory_->CreateStrokeStyle(props, nullptr, 0, roundedStroke_.Put());
    return CreateTarget();
}

void RenderContext::Resize(int pixelWidth, int pixelHeight) {
    pixelWidth_ = std::max(1, pixelWidth);
    pixelHeight_ = std::max(1, pixelHeight);
    if (target_) {
        target_->Resize(D2D1::SizeU(static_cast<UINT32>(pixelWidth_), static_cast<UINT32>(pixelHeight_)));
    }
}

void RenderContext::BeginFrame(D2D1_COLOR_F safeColor) {
    EnsureDeviceResources();
    const float sx = static_cast<float>(pixelWidth_) / 1280.0f;
    const float sy = static_cast<float>(pixelHeight_) / 720.0f;
    transform_.scale = std::min(sx, sy);
    transform_.safeWidth = static_cast<float>(pixelWidth_);
    transform_.safeHeight = static_cast<float>(pixelHeight_);
    transform_.offsetX = (static_cast<float>(pixelWidth_) - 1280.0f * transform_.scale) * 0.5f;
    transform_.offsetY = (static_cast<float>(pixelHeight_) - 720.0f * transform_.scale) * 0.5f;

    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(safeColor);
    target_->SetTransform(
        D2D1::Matrix3x2F::Scale(transform_.scale, transform_.scale) *
        D2D1::Matrix3x2F::Translation(transform_.offsetX, transform_.offsetY));
}

bool RenderContext::EndFrame() {
    const HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
        return false;
    }
    return SUCCEEDED(hr);
}

void RenderContext::DiscardDeviceResources() {
    target_.Reset();
}

bool RenderContext::EnsureDeviceResources() {
    return target_ || CreateTarget();
}

void RenderContext::Clear(D2D1_COLOR_F color) {
    target_->Clear(color);
}

void RenderContext::FillRect(const core::Rect& rect, D2D1_COLOR_F color) {
    auto brush = CreateBrush(color);
    target_->FillRectangle(D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height), brush.Get());
}

void RenderContext::StrokeRect(const core::Rect& rect, D2D1_COLOR_F color, float width) {
    auto brush = CreateBrush(color);
    target_->DrawRectangle(D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height), brush.Get(), width);
}

void RenderContext::FillEllipse(const core::Rect& rect, D2D1_COLOR_F color) {
    auto brush = CreateBrush(color);
    target_->FillEllipse(D2D1::Ellipse(
        D2D1::Point2F(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f),
        rect.width * 0.5f,
        rect.height * 0.5f),
        brush.Get());
}

void RenderContext::StrokeEllipse(const core::Rect& rect, D2D1_COLOR_F color, float width) {
    auto brush = CreateBrush(color);
    target_->DrawEllipse(D2D1::Ellipse(
        D2D1::Point2F(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f),
        rect.width * 0.5f,
        rect.height * 0.5f),
        brush.Get(),
        width);
}

void RenderContext::DrawLine(core::Point a, core::Point b, D2D1_COLOR_F color, float width) {
    auto brush = CreateBrush(color);
    target_->DrawLine(D2D1::Point2F(a.x, a.y), D2D1::Point2F(b.x, b.y), brush.Get(), width, roundedStroke_.Get());
}

void RenderContext::DrawPolyline(std::span<const core::Point> points, D2D1_COLOR_F color, float width) {
    if (points.size() < 2) {
        return;
    }
    auto brush = CreateBrush(color);
    ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(d2dFactory_->CreatePathGeometry(geometry.Put()))) {
        return;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(sink.Put()))) {
        return;
    }
    sink->BeginFigure(D2D1::Point2F(points[0].x, points[0].y), D2D1_FIGURE_BEGIN_HOLLOW);
    for (std::size_t i = 1; i < points.size(); ++i) {
        sink->AddLine(D2D1::Point2F(points[i].x, points[i].y));
    }
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();
    target_->DrawGeometry(geometry.Get(), brush.Get(), width, roundedStroke_.Get());
}

void RenderContext::DrawTextUtf8(
    const std::string& text,
    const core::Rect& rect,
    float fontSize,
    D2D1_COLOR_F color,
    DWRITE_TEXT_ALIGNMENT align,
    DWRITE_PARAGRAPH_ALIGNMENT valign) {
    auto brush = CreateBrush(color);
    ComPtr<IDWriteTextFormat> format;
    if (FAILED(dwriteFactory_->CreateTextFormat(
            L"Microsoft YaHei UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"zh-cn",
            format.Put()))) {
        return;
    }
    format->SetTextAlignment(align);
    format->SetParagraphAlignment(valign);
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    const std::wstring wide = Utf8ToWide(text);
    target_->DrawTextW(
        wide.c_str(),
        static_cast<UINT32>(wide.size()),
        format.Get(),
        D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height),
        brush.Get());
}

std::wstring RenderContext::Utf8ToWide(const std::string& text) const {
    if (text.empty()) {
        return {};
    }
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), count);
    return wide;
}

bool RenderContext::CreateTarget() {
    if (!d2dFactory_ || !hwnd_) {
        return false;
    }
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    pixelWidth_ = std::max(1L, rc.right - rc.left);
    pixelHeight_ = std::max(1L, rc.bottom - rc.top);
    const D2D1_RENDER_TARGET_PROPERTIES targetProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN),
        96.0f,
        96.0f);
    const D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        hwnd_,
        D2D1::SizeU(static_cast<UINT32>(pixelWidth_), static_cast<UINT32>(pixelHeight_)),
        D2D1_PRESENT_OPTIONS_NONE);
    return SUCCEEDED(d2dFactory_->CreateHwndRenderTarget(targetProps, hwndProps, target_.Put()));
}

ComPtr<ID2D1SolidColorBrush> RenderContext::CreateBrush(D2D1_COLOR_F color) {
    ComPtr<ID2D1SolidColorBrush> brush;
    target_->CreateSolidColorBrush(color, brush.Put());
    return brush;
}

} // namespace cable::graphics
