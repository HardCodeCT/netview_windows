#include "UIHelper.h"
#include "FontManager.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

HFONT UIHelper::CreateModernFont(int size, int weight) {
    // Use custom font if available, otherwise fall back to Segoe UI
    const wchar_t* fontName = FontManager::IsInitialized() 
        ? FontManager::GetFontName() 
        : L"Segoe UI";
    
    return CreateFontW(
        size,                        // Height
        0,                           // Width
        0,                           // Escapement
        0,                           // Orientation
        weight,                      // Weight
        FALSE,                       // Italic
        FALSE,                       // Underline
        FALSE,                       // StrikeOut
        DEFAULT_CHARSET,             // CharSet
        OUT_DEFAULT_PRECIS,          // OutputPrecision
        CLIP_DEFAULT_PRECIS,         // ClipPrecision
        CLEARTYPE_QUALITY,           // Quality
        DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
        fontName                     // FaceName
    );
}

void UIHelper::SetDarkTheme(HWND hwnd) {
    // Enable dark mode for window title bar (Windows 10 build 17763+)
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode)); // DWMWA_USE_IMMERSIVE_DARK_MODE
}

void UIHelper::DrawRoundedRect(HDC hdc, RECT rect, int radius, 
                               COLORREF fillColor, COLORREF borderColor, int borderWidth) {
    // Create rounded region
    HRGN hRgn = CreateRoundRectRgn(
        rect.left, 
        rect.top, 
        rect.right, 
        rect.bottom, 
        radius, 
        radius
    );
    
    // Fill with background color
    HBRUSH fillBrush = CreateSolidBrush(fillColor);
    FillRgn(hdc, hRgn, fillBrush);
    DeleteObject(fillBrush);
    
    // Draw border
    HPEN borderPen = CreatePen(PS_SOLID, borderWidth, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    DeleteObject(hRgn);
}
