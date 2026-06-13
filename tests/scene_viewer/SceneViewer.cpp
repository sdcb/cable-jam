#include "app/App.h"
#include "app/Window.h"
#include "core/Timer.h"
#include "graphics/ComPtr.h"

#include <objbase.h>
#include <propidl.h>
#include <wincodec.h>
#include <windows.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

namespace {

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    return wide;
}

struct Args {
    std::string scene{"main"};
    std::string overlay;
    std::string mock;
    std::string screenshot;
    std::optional<float> clickX;
    std::optional<float> clickY;
    float quality{0.75f};
};

Args ParseArgs(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 < argc) {
                return argv[++i];
            }
            return {};
        };
        if (key == "--scene") {
            args.scene = next();
        } else if (key == "--overlay") {
            args.overlay = next();
        } else if (key == "--mock") {
            args.mock = next();
        } else if (key == "--screenshot") {
            args.screenshot = next();
        } else if (key == "--quality") {
            args.quality = std::clamp(std::strtof(next().c_str(), nullptr) / 100.0f, 0.01f, 1.0f);
        } else if (key == "--click") {
            args.clickX = std::strtof(next().c_str(), nullptr);
            args.clickY = std::strtof(next().c_str(), nullptr);
        }
    }
    return args;
}

bool CaptureWindowJpeg(HWND hwnd, const std::wstring& path, float quality) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) {
        return false;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HDC screen = GetDC(hwnd);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!bitmap || !pixels) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        DeleteDC(memory);
        ReleaseDC(hwnd, screen);
        return false;
    }

    HGDIOBJ old = SelectObject(memory, bitmap);
    const BOOL copied = BitBlt(memory, 0, 0, width, height, screen, 0, 0, SRCCOPY);

    bool ok = false;
    if (copied) {
        const std::filesystem::path outPath(path);
        if (outPath.has_parent_path()) {
            std::filesystem::create_directories(outPath.parent_path());
        }

        cable::graphics::ComPtr<IWICImagingFactory> wic;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.Put())))) {
            cable::graphics::ComPtr<IWICBitmap> wicBitmap;
            const UINT stride = static_cast<UINT>(width * 4);
            const UINT bufferSize = static_cast<UINT>(stride * height);
            if (SUCCEEDED(wic->CreateBitmapFromMemory(
                    static_cast<UINT>(width),
                    static_cast<UINT>(height),
                    GUID_WICPixelFormat32bppBGRA,
                    stride,
                    bufferSize,
                    static_cast<BYTE*>(pixels),
                    wicBitmap.Put()))) {
                cable::graphics::ComPtr<IWICFormatConverter> converter;
                cable::graphics::ComPtr<IWICStream> stream;
                cable::graphics::ComPtr<IWICBitmapEncoder> encoder;
                cable::graphics::ComPtr<IWICBitmapFrameEncode> frame;
                IPropertyBag2* rawBag = nullptr;
                if (SUCCEEDED(wic->CreateFormatConverter(converter.Put())) &&
                    SUCCEEDED(converter->Initialize(wicBitmap.Get(), GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut)) &&
                    SUCCEEDED(wic->CreateStream(stream.Put())) &&
                    SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
                    SUCCEEDED(wic->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, encoder.Put())) &&
                    SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) &&
                    SUCCEEDED(encoder->CreateNewFrame(frame.Put(), &rawBag))) {
                    PROPBAG2 option{};
                    option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
                    VARIANT value{};
                    VariantInit(&value);
                    value.vt = VT_R4;
                    value.fltVal = quality;
                    if (rawBag) {
                        rawBag->Write(1, &option, &value);
                    }
                    if (SUCCEEDED(frame->Initialize(rawBag)) &&
                        SUCCEEDED(frame->SetSize(static_cast<UINT>(width), static_cast<UINT>(height)))) {
                        WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
                        if (SUCCEEDED(frame->SetPixelFormat(&format)) &&
                            SUCCEEDED(frame->WriteSource(converter.Get(), nullptr)) &&
                            SUCCEEDED(frame->Commit()) &&
                            SUCCEEDED(encoder->Commit())) {
                            ok = true;
                        }
                    }
                    VariantClear(&value);
                    if (rawBag) {
                        rawBag->Release();
                    }
                } else if (rawBag) {
                    rawBag->Release();
                }
            }
        }
    }

    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(hwnd, screen);
    return ok;
}

} // namespace

int main(int argc, char** argv) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const Args args = ParseArgs(argc, argv);

    cable::app::App app;
    cable::app::Window window;
    if (!window.Create(app, L"scene_viewer", 1280, 720)) {
        CoUninitialize();
        return 1;
    }
    app.SetProgressPersistence(false);
    app.ShowViewerScene(args.scene, args.overlay, args.mock);
    app.Resize(1280, 720);

    if (args.screenshot.empty()) {
        const int result = window.Run();
        CoUninitialize();
        return result;
    }

    MSG msg{};
    for (int frame = 0; frame < 8; ++frame) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        app.Resize(1280, 720);
        app.Update(1.0f / 60.0f);
        app.Render();
        Sleep(16);
    }
    if (args.clickX && args.clickY) {
        app.OnMouseMove(*args.clickX, *args.clickY);
        app.OnMouseDown(*args.clickX, *args.clickY);
        for (int frame = 0; frame < 4; ++frame) {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            app.Update(1.0f / 60.0f);
            app.Render();
            Sleep(16);
        }
    }

    const bool ok = CaptureWindowJpeg(window.Hwnd(), Utf8ToWide(args.screenshot), args.quality);
    app.ConfirmExit();
    CoUninitialize();
    return ok ? 0 : 3;
}
