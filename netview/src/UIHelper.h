#pragma once
#include <windows.h>

class UIHelper {
public:
    // Create a modern font (now uses embedded custom font)
    static HFONT CreateModernFont(int size, int weight);
    
    // Set dark theme for window
    static void SetDarkTheme(HWND hwnd);
    
    // Draw a rounded rectangle
    static void DrawRoundedRect(HDC hdc, RECT rect, int radius, 
                               COLORREF fillColor, COLORREF borderColor, int borderWidth);
};
