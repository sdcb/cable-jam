#include "app/App.h"
#include "app/Window.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    cable::app::App app;
    cable::app::Window window;
    if (!window.Create(app, L"Cable Jam - 充电请排队", 1280, 720)) {
        CoUninitialize();
        return 1;
    }
    const int result = window.Run();
    CoUninitialize();
    return result;
}
