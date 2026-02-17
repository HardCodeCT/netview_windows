#pragma once
#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <string>

class ContactWindow {
public:
    ContactWindow();
    ~ContactWindow();
    
    bool Create(HWND parent);
    void Show();
    void Hide();
    bool IsVisible() const;
    HWND GetHandle() const { return m_hwnd; }
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void Paint();
    void DrawContactIcon(HDC hdc, int x, int y, int width, int height, int resourceID);
    void OnContactClick(int contactType);
    void OpenURL(const std::wstring& url);
    Gdiplus::Image* LoadPNGFromResource(int resourceID);
    
    HWND m_hwnd;
    HWND m_parentHwnd;
    bool m_visible;
    
    // Contact button areas
    RECT m_twitterRect;
    RECT m_whatsappRect;
    RECT m_gmailRect;
    
    int m_hoveredButton;
    
    static const int WINDOW_WIDTH = 280;
    static const int WINDOW_HEIGHT = 140;
    static const int ICON_SIZE = 40;
    static const int ICON_SPACING = 20;
};
