#pragma once

#include "app/App.h"

#include <windows.h>

namespace cable::app {

class Window {
public:
    bool Create(App& app, const wchar_t* title, int width, int height);
    int Run();
    HWND Hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);
    void ForwardMouse(UINT message, LPARAM lParam);

    HWND hwnd_{};
    App* app_{};
};

} // namespace cable::app
