#include "ContactWindow.h"
#include "UIHelper.h"
#include "resource.h"
#include <shellapi.h>
#include <windowsx.h> 

using namespace Gdiplus;

ContactWindow::ContactWindow()
    : m_hwnd(nullptr)
    , m_parentHwnd(nullptr)
    , m_visible(false)
    , m_hoveredButton(-1)
{
    m_twitterRect = {0};
    m_whatsappRect = {0};
    m_gmailRect = {0};
}

ContactWindow::~ContactWindow() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool ContactWindow::Create(HWND parent) {
    m_parentHwnd = parent;
    
    const wchar_t CLASS_NAME[] = L"NetViewContactWindow";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    
    int x = parentRect.left - WINDOW_WIDTH - 10;
    int y = parentRect.top;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    if (x < 0) x = parentRect.right + 10;
    if (x + WINDOW_WIDTH > screenWidth) x = screenWidth - WINDOW_WIDTH - 10;
    if (y + WINDOW_HEIGHT > screenHeight) y = screenHeight - WINDOW_HEIGHT - 10;
    
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Contact Developer",
        WS_POPUP | WS_BORDER,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
        parent,  // FIXED - was NULL
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (!m_hwnd) return false;
    
    int totalWidth = (ICON_SIZE * 3) + (ICON_SPACING * 2);
    int startX = (WINDOW_WIDTH - totalWidth) / 2;
    int iconY = 60;
    
    m_twitterRect = {startX, iconY, startX + ICON_SIZE, iconY + ICON_SIZE};
    m_whatsappRect = {startX + ICON_SIZE + ICON_SPACING, iconY, 
                      startX + (ICON_SIZE * 2) + ICON_SPACING, iconY + ICON_SIZE};
    m_gmailRect = {startX + (ICON_SIZE * 2) + (ICON_SPACING * 2), iconY,
                   startX + (ICON_SIZE * 3) + (ICON_SPACING * 2), iconY + ICON_SIZE};
    
    return true;
}

void ContactWindow::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        BringWindowToTop(m_hwnd);
        SetForegroundWindow(m_hwnd);
        UpdateWindow(m_hwnd);
        m_visible = true;
    }
}

void ContactWindow::Hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
        m_visible = false;
    }
}

bool ContactWindow::IsVisible() const {
    return m_visible;
}

LRESULT CALLBACK ContactWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ContactWindow* window = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<ContactWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<ContactWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (window) {
        return window->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ContactWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1;
            
        case WM_PAINT:
            Paint();
            return 0;
            
        case WM_LBUTTONDOWN: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            
            if (PtInRect(&m_twitterRect, pt)) {
                OnContactClick(0);
            } else if (PtInRect(&m_whatsappRect, pt)) {
                OnContactClick(1);
            } else if (PtInRect(&m_gmailRect, pt)) {
                OnContactClick(2);
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int oldHovered = m_hoveredButton;
            
            if (PtInRect(&m_twitterRect, pt)) {
                m_hoveredButton = 0;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else if (PtInRect(&m_whatsappRect, pt)) {
                m_hoveredButton = 1;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else if (PtInRect(&m_gmailRect, pt)) {
                m_hoveredButton = 2;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else {
                m_hoveredButton = -1;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            
            if (oldHovered != m_hoveredButton) {
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
            
            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            TrackMouseEvent(&tme);
            
            return 0;
        }
        
        case WM_MOUSELEAVE:
            m_hoveredButton = -1;
            InvalidateRect(m_hwnd, NULL, FALSE);
            return 0;
        
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                Hide();
            }
            return 0;
        
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                Hide();
            }
            return 0;
            
        case WM_CLOSE:
            Hide();
            return 0;
    }
    
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

Image* ContactWindow::LoadPNGFromResource(int resourceID) {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceID), L"PNG");
    if (!hResource) return nullptr;
    
    DWORD imageSize = SizeofResource(hModule, hResource);
    if (imageSize == 0) return nullptr;
    
    HGLOBAL hGlobal = LoadResource(hModule, hResource);
    if (!hGlobal) return nullptr;
    
    void* pResourceData = LockResource(hGlobal);
    if (!pResourceData) return nullptr;
    
    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!hBuffer) return nullptr;
    
    void* pBuffer = GlobalLock(hBuffer);
    if (!pBuffer) {
        GlobalFree(hBuffer);
        return nullptr;
    }
    
    CopyMemory(pBuffer, pResourceData, imageSize);
    GlobalUnlock(hBuffer);
    
    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) != S_OK) {
        GlobalFree(hBuffer);
        return nullptr;
    }
    
    Image* image = new Image(pStream);
    pStream->Release();
    
    if (image->GetLastStatus() != Ok) {
        delete image;
        return nullptr;
    }
    
    return image;
}

void ContactWindow::Paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
    
    SolidBrush bgBrush(Color(255, 18, 18, 18));
    graphics.FillRectangle(&bgBrush, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    Pen borderPen(Color(255, 50, 50, 50), 1);
    graphics.DrawRectangle(&borderPen, 0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1);
    
    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 11, FontStyleRegular, UnitPoint);
    SolidBrush titleBrush(Color(180, 255, 255, 255));
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    
    RectF titleRect(0, 15, WINDOW_WIDTH, 30);
    graphics.DrawString(L"Contact Developer", -1, &titleFont, titleRect, &format, &titleBrush);
    
    DrawContactIcon(hdc, m_twitterRect.left, m_twitterRect.top, ICON_SIZE, ICON_SIZE, IDR_TWITTER_ICON);
    DrawContactIcon(hdc, m_whatsappRect.left, m_whatsappRect.top, ICON_SIZE, ICON_SIZE, IDR_WHATSAPP_ICON);
    DrawContactIcon(hdc, m_gmailRect.left, m_gmailRect.top, ICON_SIZE, ICON_SIZE, IDR_GMAIL_ICON);
    
    EndPaint(m_hwnd, &ps);
}

void ContactWindow::DrawContactIcon(HDC hdc, int x, int y, int width, int height, int resourceID) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    
    bool isHovered = false;
    if (resourceID == IDR_TWITTER_ICON && m_hoveredButton == 0) isHovered = true;
    if (resourceID == IDR_WHATSAPP_ICON && m_hoveredButton == 1) isHovered = true;
    if (resourceID == IDR_GMAIL_ICON && m_hoveredButton == 2) isHovered = true;
    
    Color bgColor = isHovered ? Color(255, 45, 45, 45) : Color(255, 30, 30, 30);
    SolidBrush bgBrush(bgColor);
    graphics.FillEllipse(&bgBrush, x, y, width, height);
    
    Image* image = LoadPNGFromResource(resourceID);
    if (image) {
        int iconPadding = 8;
        Rect iconRect(x + iconPadding, y + iconPadding, 
                     width - (iconPadding * 2), height - (iconPadding * 2));
        
        graphics.DrawImage(image, iconRect, 
                          0, 0, image->GetWidth(), image->GetHeight(), 
                          UnitPixel);
        
        delete image;
    }
    
    if (isHovered) {
        Pen hoverPen(Color(100, 255, 255, 255), 2);
        graphics.DrawEllipse(&hoverPen, x - 2, y - 2, width + 4, height + 4);
    }
}

void ContactWindow::OnContactClick(int contactType) {
    switch (contactType) {
        case 0:
            OpenURL(L"https://x.com/Hard_Code_T");
            break;
        case 1:
            OpenURL(L"https://wa.me/2348165713623");
            break;
        case 2:
            OpenURL(L"mailto:firmino3535@gmail.com?subject=Contact from NetView");
            break;
    }
}

void ContactWindow::OpenURL(const std::wstring& url) {
    ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}