#pragma once
#include <windows.h>

class FontManager {
public:
    static bool Initialize();
    static void Cleanup();
    static const wchar_t* GetFontName() { return L"Afacad Flux SemiBold"; }
    static bool IsInitialized() { return fontHandle_ != NULL; }

private:
    static HANDLE fontHandle_;
    static DWORD fontCount_;
};