#include "app/Dpi.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace cable::app {

void EnableSystemDpiAwareness() {
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (!user32) {
        return;
    }
    using SetProcessDpiAwarenessContextProc = BOOL(WINAPI*)(HANDLE);
    auto setContext = reinterpret_cast<SetProcessDpiAwarenessContextProc>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (setContext) {
        setContext(reinterpret_cast<HANDLE>(-2)); // DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
        FreeLibrary(user32);
        return;
    }
    FreeLibrary(user32);
    SetProcessDPIAware();
}

} // namespace cable::app
