#include "FontManager.h"
#include "resource.h"

HANDLE FontManager::fontHandle_ = NULL;
DWORD FontManager::fontCount_ = 0;

bool FontManager::Initialize() {
    if (fontHandle_) {
        return true;
    }

    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) {
        return false;
    }

    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_FONT_AFACAD), RT_RCDATA);
    if (!hResource) {
        return false;
    }

    DWORD fontSize = SizeofResource(hModule, hResource);
    if (fontSize == 0) {
        return false;
    }

    HGLOBAL hGlobal = LoadResource(hModule, hResource);
    if (!hGlobal) {
        return false;
    }

    void* pFontData = LockResource(hGlobal);
    if (!pFontData) {
        return false;
    }

    fontHandle_ = AddFontMemResourceEx(
        pFontData,
        fontSize,
        NULL,
        &fontCount_
    );

    return (fontHandle_ != NULL);
}

void FontManager::Cleanup() {
    if (fontHandle_) {
        RemoveFontMemResourceEx(fontHandle_);
        fontHandle_ = NULL;
        fontCount_ = 0;
    }
}