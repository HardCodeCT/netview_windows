#include "MonitorWidget.h"
#include "DataManager.h"
#include "HistoryManager.h"
#include "HistoryWindow.h"
#include "PaymentWindow.h"
#include "LicenseManager.h"
#include "FirebaseManager.h"
#include "UIHelper.h"
#include "resource.h"
#include <dwmapi.h>
#include <shellapi.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

MonitorWidget* MonitorWidget::instance_ = nullptr;

MonitorWidget::MonitorWidget(DataManager* dm, HistoryManager* hm, HistoryWindow* hw) 
    : dataManager_(dm)
    , historyManager_(hm)
    , historyWindow_(hw)
    , paymentWindow_(nullptr)
    , hwnd_(NULL)
    , contactPopupHwnd_(NULL)
    , isHovered_(false)
    , isHistoryHovered_(false)
    , isContactHovered_(false)
    , isContactCloseHovered_(false)
    , contactCardVisible_(true)  // ALWAYS TRUE ON STARTUP
    , isDragging_(false)
    , cumulativeFromDate_(L"")
    , isLicensed_(false)
    , isTrialActive_(true)
    , trialDaysUsed_(0)
    , trialDaysRemaining_(7)
    , hoveredContactButton_(-1) {
    
    instance_ = this;
    historyCardRect_ = {0};
    contactCardRect_ = {0};
    contactCloseRect_ = {0};
    twitterRect_ = {0};
    whatsappRect_ = {0};
    gmailRect_ = {0};
    dragOffset_ = {0, 0};
    
    // Create payment window
    paymentWindow_ = new PaymentWindow();
    
    // Check initial license status
    CheckLicenseStatus();
    
    // Contact card always shows on startup - don't load saved state
}

MonitorWidget::~MonitorWidget() {
    if (paymentWindow_) {
        delete paymentWindow_;
    }
    if (contactPopupHwnd_) {
        DestroyWindow(contactPopupHwnd_);
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
    instance_ = nullptr;
}

void MonitorWidget::CheckLicenseStatus() {
    LicenseManager& licMgr = LicenseManager::GetInstance();
    
    isLicensed_ = licMgr.IsLicensed();
    isTrialActive_ = licMgr.IsTrialActive();
    trialDaysUsed_ = licMgr.GetTrialDaysUsed();
    trialDaysRemaining_ = licMgr.GetTrialDaysRemaining();
    
    // If licensed, check with Firebase for verification
    if (!isLicensed_) {
        FirebaseManager::GetInstance().CheckPaymentStatus(
            licMgr.GetInstallationKey(),
            [this](bool verified) {
                if (verified) {
                    LicenseManager::GetInstance().ActivateLicense();
                    CheckLicenseStatus();
                    InvalidateRect(hwnd_, NULL, TRUE);
                }
            }
        );
    }
}

void MonitorWidget::ShowPaymentWindow() {
    if (paymentWindow_) {
        if (!paymentWindow_->IsVisible()) {
            if (paymentWindow_->GetHandle() == NULL) {
                paymentWindow_->Create(hwnd_);
            }
            paymentWindow_->Show();
        } else {
            paymentWindow_->Hide();
        }
    }
}

void MonitorWidget::SavePosition() {
    RECT windowRect;
    GetWindowRect(hwnd_, &windowRect);
    
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, NULL, 
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD x = windowRect.left;
        DWORD y = windowRect.top;
        
        RegSetValueExW(hKey, L"WidgetX", 0, REG_DWORD, (LPBYTE)&x, sizeof(DWORD));
        RegSetValueExW(hKey, L"WidgetY", 0, REG_DWORD, (LPBYTE)&y, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
}

void MonitorWidget::LoadPosition(int& x, int& y) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD posX = 0, posY = 0;
        DWORD size = sizeof(DWORD);
        
        bool hasX = (RegQueryValueExW(hKey, L"WidgetX", NULL, NULL, (LPBYTE)&posX, &size) == ERROR_SUCCESS);
        size = sizeof(DWORD);
        bool hasY = (RegQueryValueExW(hKey, L"WidgetY", NULL, NULL, (LPBYTE)&posY, &size) == ERROR_SUCCESS);
        
        if (hasX && hasY) {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            
            // Always use full height since contact card always shows on startup
            if (posX >= 0 && posX + MONITOR_WIDTH <= screenWidth &&
                posY >= 0 && posY + MONITOR_HEIGHT <= screenHeight) {
                x = posX;
                y = posY;
            }
        }
        
        RegCloseKey(hKey);
    }
}

void MonitorWidget::SaveCumulativeMode() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, NULL, 
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        // Save the cumulative from date (empty string if not active)
        DWORD dataSize = static_cast<DWORD>((cumulativeFromDate_.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"CumulativeFromDate", 0, REG_SZ, 
                      (LPBYTE)cumulativeFromDate_.c_str(), dataSize);
        
        RegCloseKey(hKey);
        
        wchar_t msg[256];
        swprintf_s(msg, 256, L"[MonitorWidget] Saved cumulative mode: %s\n", 
                  cumulativeFromDate_.empty() ? L"OFF" : cumulativeFromDate_.c_str());
        OutputDebugStringW(msg);
    }
}

void MonitorWidget::LoadCumulativeMode() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[256] = {0};
        DWORD bufferSize = sizeof(buffer);
        DWORD type = REG_SZ;
        
        if (RegQueryValueExW(hKey, L"CumulativeFromDate", NULL, &type, 
                            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            cumulativeFromDate_ = buffer;
            
            wchar_t msg[256];
            swprintf_s(msg, 256, L"[MonitorWidget] Loaded cumulative mode: %s\n", 
                      cumulativeFromDate_.empty() ? L"OFF" : cumulativeFromDate_.c_str());
            OutputDebugStringW(msg);
        }
        
        RegCloseKey(hKey);
    }
}

void MonitorWidget::SaveContactCardVisibility() {
    // Not used anymore - contact card always shows on startup
}

void MonitorWidget::LoadContactCardVisibility() {
    // Not used anymore - contact card always shows on startup
}

bool MonitorWidget::Create() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = StaticWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WND_CLASS_MONITOR;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_SURFACE);
    
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    // Register contact popup window class
    WNDCLASSEXW contactWc = {0};
    contactWc.cbSize = sizeof(WNDCLASSEXW);
    contactWc.lpfnWndProc = StaticContactWindowProc;
    contactWc.hInstance = GetModuleHandle(NULL);
    contactWc.lpszClassName = WND_CLASS_CONTACT;
    contactWc.hCursor = LoadCursor(NULL, IDC_ARROW);
    contactWc.hbrBackground = CreateSolidBrush(RGB(18, 18, 18));
    
    if (!RegisterClassExW(&contactWc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Always start with full height since contact card always shows
    int initialHeight = MONITOR_HEIGHT;
    
    int x = screenWidth - MONITOR_WIDTH - 10;
    int y = screenHeight - initialHeight - 60;
    
    LoadPosition(x, y);
    LoadCumulativeMode();  // Load saved cumulative mode setting
    
    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        WND_CLASS_MONITOR,
        L"NetView Monitor",
        WS_POPUP,
        x, y, MONITOR_WIDTH, initialHeight,
        NULL, NULL, GetModuleHandle(NULL), this
    );
    
    if (!hwnd_) return false;
    
    UIHelper::SetDarkTheme(hwnd_);
    
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    
    SetTimer(hwnd_, TIMER_UPDATE, 500, NULL);
    SetTimer(hwnd_, TIMER_LICENSE_CHECK, 3600000, NULL);  // Check license hourly
    
    return true;
}

void MonitorWidget::SetCumulativeMode(const std::wstring& fromDate) {
    cumulativeFromDate_ = fromDate;
    SaveCumulativeMode();  // Persist the setting to registry
    InvalidateRect(hwnd_, NULL, TRUE);
}

void MonitorWidget::Update() {
    if (hwnd_) {
        if (historyManager_ && (isLicensed_ || isTrialActive_)) {
            historyManager_->UpdateTodayUsage(
                dataManager_->GetDataIn(),
                dataManager_->GetDataOut()
            );
        }
        
        InvalidateRect(hwnd_, NULL, TRUE);
    }
}

void MonitorWidget::ShowContactPopup() {
    if (contactPopupHwnd_) {
        HideContactPopup();
        return;
    }
    
    RECT parentRect;
    GetWindowRect(hwnd_, &parentRect);
    
    const int POPUP_WIDTH = 200;
    const int POPUP_HEIGHT = 65;
    
    int x = parentRect.left - POPUP_WIDTH - 10;
    int y = parentRect.top;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    if (x < 0) x = parentRect.right + 10;
    if (x + POPUP_WIDTH > screenWidth) x = screenWidth - POPUP_WIDTH - 10;
    if (y + POPUP_HEIGHT > screenHeight) y = screenHeight - POPUP_HEIGHT - 10;
    
    contactPopupHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WND_CLASS_CONTACT,
        L"Contact Developer",
        WS_POPUP,
        x, y, POPUP_WIDTH, POPUP_HEIGHT,
        hwnd_,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (contactPopupHwnd_) {
        const int ICON_SIZE = 40;
        const int ICON_SPACING = 15;
        const int TOTAL_WIDTH = (ICON_SIZE * 3) + (ICON_SPACING * 2);
        const int START_X = (POPUP_WIDTH - TOTAL_WIDTH) / 2;
        const int ICON_Y = (POPUP_HEIGHT - ICON_SIZE) / 2;
        
        twitterRect_.left = START_X;
        twitterRect_.top = ICON_Y;
        twitterRect_.right = START_X + ICON_SIZE;
        twitterRect_.bottom = ICON_Y + ICON_SIZE;
        
        whatsappRect_.left = START_X + ICON_SIZE + ICON_SPACING;
        whatsappRect_.top = ICON_Y;
        whatsappRect_.right = START_X + (ICON_SIZE * 2) + ICON_SPACING;
        whatsappRect_.bottom = ICON_Y + ICON_SIZE;
        
        gmailRect_.left = START_X + (ICON_SIZE * 2) + (ICON_SPACING * 2);
        gmailRect_.top = ICON_Y;
        gmailRect_.right = START_X + (ICON_SIZE * 3) + (ICON_SPACING * 2);
        gmailRect_.bottom = ICON_Y + ICON_SIZE;
        
        const int CORNER_RADIUS = 8;
        HRGN hRgn = CreateRoundRectRgn(0, 0, POPUP_WIDTH + 1, POPUP_HEIGHT + 1, 
                                       CORNER_RADIUS, CORNER_RADIUS);
        SetWindowRgn(contactPopupHwnd_, hRgn, TRUE);
        
        ShowWindow(contactPopupHwnd_, SW_SHOW);
        UpdateWindow(contactPopupHwnd_);
    }
}

void MonitorWidget::HideContactPopup() {
    if (contactPopupHwnd_) {
        DestroyWindow(contactPopupHwnd_);
        contactPopupHwnd_ = NULL;
        hoveredContactButton_ = -1;
    }
}

void MonitorWidget::OpenURL(const std::wstring& url) {
    ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void MonitorWidget::OnContactPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    
    const int POPUP_WIDTH = 200;
    const int POPUP_HEIGHT = 65;
    const int CORNER_RADIUS = 8;
    
    GraphicsPath path;
    path.AddArc(0, 0, CORNER_RADIUS * 2, CORNER_RADIUS * 2, 180, 90);
    path.AddArc(POPUP_WIDTH - CORNER_RADIUS * 2, 0, CORNER_RADIUS * 2, CORNER_RADIUS * 2, 270, 90);
    path.AddArc(POPUP_WIDTH - CORNER_RADIUS * 2, POPUP_HEIGHT - CORNER_RADIUS * 2, 
                CORNER_RADIUS * 2, CORNER_RADIUS * 2, 0, 90);
    path.AddArc(0, POPUP_HEIGHT - CORNER_RADIUS * 2, CORNER_RADIUS * 2, CORNER_RADIUS * 2, 90, 90);
    path.CloseFigure();
    
    SolidBrush bgBrush(Color(255, 18, 18, 18));
    graphics.FillPath(&bgBrush, &path);
    
    Pen silverBorderPen(Color(255, 192, 192, 192), 2.0f);
    graphics.DrawPath(&silverBorderPen, &path);
    
    auto drawIcon = [&](const RECT& rect, int resourceID, bool isHovered) {
        Color bgColor = isHovered ? Color(255, 45, 45, 45) : Color(255, 30, 30, 30);
        SolidBrush circleBrush(bgColor);
        graphics.FillEllipse(&circleBrush, rect.left, rect.top, 
                            rect.right - rect.left, rect.bottom - rect.top);
        
        Image* image = LoadPNGFromResource(resourceID);
        if (image) {
            int padding = 8;
            Rect iconRect(rect.left + padding, rect.top + padding,
                         (rect.right - rect.left) - (padding * 2),
                         (rect.bottom - rect.top) - (padding * 2));
            
            graphics.DrawImage(image, iconRect,
                             0, 0, image->GetWidth(), image->GetHeight(),
                             UnitPixel);
            delete image;
        }
        
        if (isHovered) {
            Pen hoverPen(Color(100, 255, 255, 255), 2);
            graphics.DrawEllipse(&hoverPen, rect.left - 2, rect.top - 2,
                               (rect.right - rect.left) + 4,
                               (rect.bottom - rect.top) + 4);
        }
    };
    
    drawIcon(twitterRect_, IDR_TWITTER_ICON, hoveredContactButton_ == 0);
    drawIcon(whatsappRect_, IDR_WHATSAPP_ICON, hoveredContactButton_ == 1);
    drawIcon(gmailRect_, IDR_GMAIL_ICON, hoveredContactButton_ == 2);
    
    EndPaint(hwnd, &ps);
}

void MonitorWidget::DrawCloseX(HDC hdc, int centerX, int centerY, int size, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    
    int halfSize = size / 2;
    
    // Draw X as two diagonal lines
    MoveToEx(hdc, centerX - halfSize, centerY - halfSize, NULL);
    LineTo(hdc, centerX + halfSize, centerY + halfSize);
    
    MoveToEx(hdc, centerX + halfSize, centerY - halfSize, NULL);
    LineTo(hdc, centerX - halfSize, centerY + halfSize);
    
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void MonitorWidget::DrawDownArrow(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    
    int halfSize = size / 2;
    int arrowWidth = size / 3;
    
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y + halfSize);
    
    POINT head[3] = {
        {x, y + halfSize},
        {x - arrowWidth, y + halfSize - arrowWidth},
        {x + arrowWidth, y + halfSize - arrowWidth}
    };
    Polygon(hdc, head, 3);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void MonitorWidget::DrawUpArrow(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    
    int halfSize = size / 2;
    int arrowWidth = size / 3;
    
    MoveToEx(hdc, x, y + halfSize, NULL);
    LineTo(hdc, x, y);
    
    POINT head[3] = {
        {x, y},
        {x - arrowWidth, y + arrowWidth},
        {x + arrowWidth, y + arrowWidth}
    };
    Polygon(hdc, head, 3);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

Image* MonitorWidget::LoadPNGFromResource(int resourceID) {
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

void MonitorWidget::DrawIconFromResource(HDC hdc, int resourceID, int x, int y, int size) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    
    Image* image = LoadPNGFromResource(resourceID);
    if (!image) return;
    
    graphics.DrawImage(
        image,
        Rect(x - size/2, y, size, size),
        0, 0, image->GetWidth(), image->GetHeight(),
        UnitPixel
    );
    
    delete image;
}

LRESULT CALLBACK MonitorWidget::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MonitorWidget* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MonitorWidget*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (MonitorWidget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->WindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MonitorWidget::StaticContactWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MonitorWidget* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MonitorWidget*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (MonitorWidget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->ContactWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MonitorWidget::ContactWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1;
            
        case WM_PAINT:
            OnContactPaint(hwnd);
            return 0;
            
        case WM_LBUTTONDOWN: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            
            if (PtInRect(&twitterRect_, pt)) {
                OpenURL(L"https://x.com/Hard_Code_T");
                HideContactPopup();
            } else if (PtInRect(&whatsappRect_, pt)) {
                OpenURL(L"https://wa.me/2348165713623");
                HideContactPopup();
            } else if (PtInRect(&gmailRect_, pt)) {
                OpenURL(L"mailto:firmino3535@gmail.com?subject=Contact from NetView");
                HideContactPopup();
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int oldHovered = hoveredContactButton_;
            
            if (PtInRect(&twitterRect_, pt)) {
                hoveredContactButton_ = 0;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else if (PtInRect(&whatsappRect_, pt)) {
                hoveredContactButton_ = 1;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else if (PtInRect(&gmailRect_, pt)) {
                hoveredContactButton_ = 2;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else {
                hoveredContactButton_ = -1;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            
            if (oldHovered != hoveredContactButton_) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            
            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            
            return 0;
        }
        
        case WM_MOUSELEAVE:
            hoveredContactButton_ = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                HideContactPopup();
            }
            return 0;
        
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                HideContactPopup();
            }
            return 0;
            
        case WM_CLOSE:
            HideContactPopup();
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MonitorWidget::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_TIMER:
            if (wParam == TIMER_UPDATE) {
                Update();
            } else if (wParam == TIMER_LICENSE_CHECK) {
                CheckLicenseStatus();
            }
            return 0;
            
        case WM_MOUSEMOVE: {
            if (isDragging_) {
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                
                int newX = cursorPos.x - dragOffset_.x;
                int newY = cursorPos.y - dragOffset_.y;
                
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                
                if (newX < 0) newX = 0;
                if (newY < 0) newY = 0;
                if (newX + MONITOR_WIDTH > screenWidth) newX = screenWidth - MONITOR_WIDTH;
                if (newY + MONITOR_HEIGHT > screenHeight) newY = screenHeight - MONITOR_HEIGHT;
                
                SetWindowPos(hwnd_, NULL, newX, newY, 0, 0, 
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            } else {
                if (!isHovered_) {
                    isHovered_ = true;
                    TRACKMOUSEEVENT tme = {0};
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    TrackMouseEvent(&tme);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);
                
                bool wasHistoryHovered = isHistoryHovered_;
                bool wasContactHovered = isContactHovered_;
                bool wasContactCloseHovered = isContactCloseHovered_;
                
                isHistoryHovered_ = PtInRect(&historyCardRect_, pt);
                isContactHovered_ = contactCardVisible_ && PtInRect(&contactCardRect_, pt) && !PtInRect(&contactCloseRect_, pt);
                isContactCloseHovered_ = contactCardVisible_ && PtInRect(&contactCloseRect_, pt);
                
                if (wasHistoryHovered != isHistoryHovered_ || 
                    wasContactHovered != isContactHovered_ ||
                    wasContactCloseHovered != isContactCloseHovered_) {
                    SetCursor(LoadCursor(NULL, (isHistoryHovered_ || isContactHovered_ || isContactCloseHovered_) ? IDC_HAND : IDC_ARROW));
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            
            return 0;
        }
        
        case WM_SETCURSOR: {
            if (isDragging_) {
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                return TRUE;
            }
            if (isHistoryHovered_ || isContactHovered_ || isContactCloseHovered_) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
            
        case WM_MOUSELEAVE:
            if (!isDragging_) {
                isHovered_ = false;
                isHistoryHovered_ = false;
                isContactHovered_ = false;
                isContactCloseHovered_ = false;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
            
        case WM_LBUTTONDOWN: {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            
            // Check close button first
            if (contactCardVisible_ && PtInRect(&contactCloseRect_, pt)) {
                contactCardVisible_ = false;
                // DON'T save the state - it always shows on startup
                
                // Resize window to 110px height
                RECT windowRect;
                GetWindowRect(hwnd_, &windowRect);
                SetWindowPos(hwnd_, NULL, windowRect.left, windowRect.top, 
                           MONITOR_WIDTH, 110, SWP_NOZORDER | SWP_NOACTIVATE);
                
                InvalidateRect(hwnd_, NULL, TRUE);
            } else if (PtInRect(&contactCardRect_, pt)) {
                ShowContactPopup();
            } else if (PtInRect(&historyCardRect_, pt)) {
                if (!isLicensed_ && !isTrialActive_) {
                    ShowPaymentWindow();
                } else {
                    if (historyWindow_) {
                        if (historyWindow_->IsVisible()) {
                            historyWindow_->Hide();
                        } else {
                            // Restore toggle state to match current cumulative mode
                            historyWindow_->RestoreToggleState(cumulativeFromDate_);
                            historyWindow_->Show();
                        }
                    }
                }
            } else {
                isDragging_ = true;
                
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                
                RECT windowRect;
                GetWindowRect(hwnd_, &windowRect);
                
                dragOffset_.x = cursorPos.x - windowRect.left;
                dragOffset_.y = cursorPos.y - windowRect.top;
                
                SetCapture(hwnd_);
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            if (isDragging_) {
                isDragging_ = false;
                ReleaseCapture();
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                SavePosition();
            }
            return 0;
        }
        
        case WM_QUERYENDSESSION:
            OutputDebugStringW(L"[MonitorWidget] WM_QUERYENDSESSION - Emergency save triggered\n");
            if (historyManager_ && (isLicensed_ || isTrialActive_)) {
                historyManager_->SaveTodayUsage();
            }
            return TRUE;
            
        case WM_ENDSESSION:
            if (wParam) {
                OutputDebugStringW(L"[MonitorWidget] WM_ENDSESSION - Final emergency save\n");
                if (historyManager_ && (isLicensed_ || isTrialActive_)) {
                    historyManager_->SaveTodayUsage();
                }
            }
            return 0;
            
        case WM_ERASEBKGND:
            return 1;
            
        case WM_DESTROY:
            KillTimer(hwnd, TIMER_UPDATE);
            KillTimer(hwnd, TIMER_LICENSE_CHECK);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MonitorWidget::OnPaint() {
    HDC hdcScreen = GetDC(NULL);
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    int width = clientRect.right;
    int height = clientRect.bottom;
    
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pBits = nullptr;
    HDC memDC = CreateCompatibleDC(hdcScreen);
    HBITMAP memBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    memset(pBits, 0, width * height * 4);
    
    // SET TRANSPARENT MODE GLOBALLY - THIS IS CRITICAL!
    SetBkMode(memDC, TRANSPARENT);
    
    int radius = 2;
    HRGN cardRegion = CreateRoundRectRgn(0, 0, width, height, radius * 2, radius * 2);
    
    COLORREF bgColor = isHovered_ ? RGB(20, 20, 20) : RGB(2, 2, 2);
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRgn(memDC, cardRegion, bgBrush);
    DeleteObject(bgBrush);
    
    HPEN borderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
    RoundRect(memDC, 0, 0, width, height, radius * 2, radius * 2);
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBrush);
    DeleteObject(borderPen);
    DeleteObject(cardRegion);
    
    UINT64 dataIn = 0, dataOut = 0;
    std::wstring labelText;
    
    if (isLicensed_ || isTrialActive_) {
        if (!cumulativeFromDate_.empty()) {
            auto cumulativeTotals = historyManager_->GetCumulativeTotalsFrom(cumulativeFromDate_);
            dataIn = cumulativeTotals.first;
            dataOut = cumulativeTotals.second;
            labelText = L"From " + cumulativeFromDate_;
        } else {
            dataIn = dataManager_->GetDataIn();
            dataOut = dataManager_->GetDataOut();
        }
    } else {
        dataIn = 0;
        dataOut = 0;
    }
    
    std::wstring dataInText = dataManager_->FormatBytes(dataIn);
    std::wstring dataOutText = dataManager_->FormatBytes(dataOut);
    
    int currentY = 8;
    int glassMargin = 8;
    int glassTopPadding = 6;
    int glassBottomPadding = 6;
    
    Graphics graphics(memDC);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    
    // Draw contact card only if visible
    if (contactCardVisible_) {
        int contactHeight = 26;
        RectF contactGlassRect(glassMargin, currentY, width - glassMargin * 2, contactHeight);
        
        LinearGradientBrush contactGradientBrush(
            PointF(contactGlassRect.X, contactGlassRect.Y),
            PointF(contactGlassRect.X, contactGlassRect.Y + contactGlassRect.Height),
            isContactHovered_ ? Color(45, 255, 255, 255) : Color(35, 255, 255, 255),
            isContactHovered_ ? Color(25, 255, 255, 255) : Color(15, 255, 255, 255)
        );
        graphics.FillRectangle(&contactGradientBrush, contactGlassRect);
        
        Pen contactGlassPen(isContactHovered_ ? Color(80, 255, 255, 255) : Color(60, 255, 255, 255), 1.0f);
        graphics.DrawRectangle(&contactGlassPen, contactGlassRect);
        
        LinearGradientBrush contactHighlightBrush(
            PointF(contactGlassRect.X, contactGlassRect.Y),
            PointF(contactGlassRect.X, contactGlassRect.Y + 12),
            Color(45, 255, 255, 255),
            Color(0, 255, 255, 255)
        );
        RectF contactHighlightRect(contactGlassRect.X + 1, contactGlassRect.Y + 1, contactGlassRect.Width - 2, 12);
        graphics.FillRectangle(&contactHighlightBrush, contactHighlightRect);
        
        contactCardRect_.left = glassMargin;
        contactCardRect_.top = currentY;
        contactCardRect_.right = width - glassMargin;
        contactCardRect_.bottom = currentY + contactHeight;
        
        int appIconSize = 20;
        int iconTextSpacing = 6;
        
        HFONT hContactFont = UIHelper::CreateModernFont(15, FW_BOLD);
        HFONT hOldFont = (HFONT)SelectObject(memDC, hContactFont);
        SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Ensure transparent after font change
        
        SIZE textSize;
        GetTextExtentPoint32W(memDC, L"Contact Developer", 17, &textSize);
        
        int totalContentWidth = appIconSize + iconTextSpacing + textSize.cx;
        int centerX = width / 2;
        int startX = centerX - (totalContentWidth / 2);
        
        HICON hAppIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON));
        if (hAppIcon) {
            DrawIconEx(memDC, startX, currentY + 3, hAppIcon, appIconSize, appIconSize, 0, NULL, DI_NORMAL);
            DestroyIcon(hAppIcon);
        }
        
        SetTextColor(memDC, COLOR_TEXT);
        RECT contactTextRect = {
            startX + appIconSize + iconTextSpacing, 
            currentY, 
            startX + totalContentWidth, 
            currentY + contactHeight
        };
        DrawTextW(memDC, L"Contact Developer", -1, &contactTextRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        // Draw close button (X)
        int closeButtonSize = 12;
        int closeButtonX = width - glassMargin - closeButtonSize - 8;
        int closeButtonY = currentY + (contactHeight / 2);
        
        contactCloseRect_.left = closeButtonX - 4;
        contactCloseRect_.top = closeButtonY - closeButtonSize/2 - 4;
        contactCloseRect_.right = closeButtonX + closeButtonSize + 4;
        contactCloseRect_.bottom = closeButtonY + closeButtonSize/2 + 4;
        
        COLORREF xColor = isContactCloseHovered_ ? RGB(255, 100, 100) : RGB(200, 200, 200);
        DrawCloseX(memDC, closeButtonX + closeButtonSize/2, closeButtonY, closeButtonSize, xColor);
        
        DeleteObject(hContactFont);
        SelectObject(memDC, hOldFont);
        
        currentY += contactHeight + 12;
    } else {
        // Card is hidden, add some top padding
        contactCardRect_ = {0, 0, 0, 0};
        contactCloseRect_ = {0, 0, 0, 0};
        currentY += 5;  // ADD TOP PADDING WHEN HIDDEN
    }
    
    int midPoint = width / 2;
    int topMargin = currentY;
    int iconSize = 16;
    int glassHeight = 58;
    
    {
        RectF glassRect(glassMargin, topMargin - glassTopPadding, midPoint - glassMargin * 2, glassHeight + glassTopPadding + glassBottomPadding);
        
        LinearGradientBrush gradientBrush(
            PointF(glassRect.X, glassRect.Y),
            PointF(glassRect.X, glassRect.Y + glassRect.Height),
            Color(35, 255, 255, 255),
            Color(15, 255, 255, 255)
        );
        graphics.FillRectangle(&gradientBrush, glassRect);
        
        Pen glassPen(Color(60, 255, 255, 255), 1.0f);
        graphics.DrawRectangle(&glassPen, glassRect);
        
        LinearGradientBrush highlightBrush(
            PointF(glassRect.X, glassRect.Y),
            PointF(glassRect.X, glassRect.Y + 15),
            Color(45, 255, 255, 255),
            Color(0, 255, 255, 255)
        );
        RectF highlightRect(glassRect.X + 1, glassRect.Y + 1, glassRect.Width - 2, 15);
        graphics.FillRectangle(&highlightBrush, highlightRect);
    }
    
    {
        RectF glassRect(midPoint + glassMargin, topMargin - glassTopPadding, midPoint - glassMargin * 2, glassHeight + glassTopPadding + glassBottomPadding);
        
        LinearGradientBrush gradientBrush(
            PointF(glassRect.X, glassRect.Y),
            PointF(glassRect.X, glassRect.Y + glassRect.Height),
            Color(35, 255, 255, 255),
            Color(15, 255, 255, 255)
        );
        graphics.FillRectangle(&gradientBrush, glassRect);
        
        Pen glassPen(Color(60, 255, 255, 255), 1.0f);
        graphics.DrawRectangle(&glassPen, glassRect);
        
        LinearGradientBrush highlightBrush(
            PointF(glassRect.X, glassRect.Y),
            PointF(glassRect.X, glassRect.Y + 15),
            Color(45, 255, 255, 255),
            Color(0, 255, 255, 255)
        );
        RectF highlightRect(glassRect.X + 1, glassRect.Y + 1, glassRect.Width - 2, 15);
        graphics.FillRectangle(&highlightBrush, highlightRect);
    }
    
    HFONT hLabelFont = UIHelper::CreateModernFont(12, FW_BOLD);
    HFONT hDataFont = UIHelper::CreateModernFont(16, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(memDC, hLabelFont);
    SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Set after font selection
    
    {
        int leftPanelCenter = midPoint / 2;
        int iconLabelSpacing = 6;
        
        SelectObject(memDC, hLabelFont);
        SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
        SIZE labelSize;
        GetTextExtentPoint32W(memDC, L"Download", 8, &labelSize);
        
        int totalWidth = iconSize + iconLabelSpacing + labelSize.cx;
        int startX = leftPanelCenter - (totalWidth / 2);
        
        int arrowX = startX + iconSize / 2;
        int arrowY = topMargin + 2;
        
        DrawIconFromResource(memDC, IDR_DOWNLOAD_ICON, arrowX, arrowY, iconSize);
        
        SetTextColor(memDC, COLOR_TEXT);
        RECT labelRect = {startX + iconSize + iconLabelSpacing, topMargin + 2, midPoint - glassMargin, topMargin + 20};
        DrawTextW(memDC, L"Download", -1, &labelRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        
        SelectObject(memDC, hDataFont);
        SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
        SetTextColor(memDC, COLOR_TEXT);
        RECT dataRect = {glassMargin, topMargin + 26, midPoint - glassMargin, topMargin + 46};
        DrawTextW(memDC, dataInText.c_str(), -1, &dataRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
        
        if (isLicensed_ || isTrialActive_) {
            UINT64 rateIn = dataManager_->GetRateIn();
            if (rateIn > 0) {
                std::wstring rateText = dataManager_->FormatBytes(rateIn) + L"/s";
                HFONT hRateFont = UIHelper::CreateModernFont(13, FW_SEMIBOLD);
                SelectObject(memDC, hRateFont);
                SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
                SetTextColor(memDC, COLOR_TEXT);
                RECT rateRect = {glassMargin, topMargin + 48, midPoint - glassMargin, topMargin + 62};
                DrawTextW(memDC, rateText.c_str(), -1, &rateRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
                DeleteObject(hRateFont);
            }
        }
    }
    
    {
        int rightPanelCenter = midPoint + (midPoint / 2);
        int iconLabelSpacing = 6;
        
        SelectObject(memDC, hLabelFont);
        SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
        SIZE labelSize;
        GetTextExtentPoint32W(memDC, L"Upload", 6, &labelSize);
        
        int totalWidth = iconSize + iconLabelSpacing + labelSize.cx;
        int startX = rightPanelCenter - (totalWidth / 2);
        
        int arrowX = startX + iconSize / 2;
        int arrowY = topMargin + 2;
        
        DrawIconFromResource(memDC, IDR_UPLOAD_ICON, arrowX, arrowY, iconSize);
        
        SetTextColor(memDC, COLOR_TEXT);
        RECT labelRect = {startX + iconSize + iconLabelSpacing, topMargin + 2, width - glassMargin, topMargin + 20};
        DrawTextW(memDC, L"Upload", -1, &labelRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        
        SelectObject(memDC, hDataFont);
        SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
        SetTextColor(memDC, COLOR_TEXT);
        RECT dataRect = {midPoint + glassMargin, topMargin + 26, width - glassMargin, topMargin + 46};
        DrawTextW(memDC, dataOutText.c_str(), -1, &dataRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
        
        if (isLicensed_ || isTrialActive_) {
            UINT64 rateOut = dataManager_->GetRateOut();
            if (rateOut > 0) {
                std::wstring rateText = dataManager_->FormatBytes(rateOut) + L"/s";
                HFONT hRateFont = UIHelper::CreateModernFont(13, FW_SEMIBOLD);
                SelectObject(memDC, hRateFont);
                SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
                SetTextColor(memDC, COLOR_TEXT);
                RECT rateRect = {midPoint + glassMargin, topMargin + 48, width - glassMargin, topMargin + 62};
                DrawTextW(memDC, rateText.c_str(), -1, &rateRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
                DeleteObject(hRateFont);
            }
        }
    }
    
    HFONT hStatusFont = UIHelper::CreateModernFont(8, FW_NORMAL);
    SelectObject(memDC, hStatusFont);
    SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
    
    if (!isLicensed_) {
        if (isTrialActive_) {
            wchar_t trialText[64];
            swprintf_s(trialText, L"● TRIAL: %d of 7 days used", trialDaysUsed_);
            SetTextColor(memDC, COLOR_TRIAL_WARNING);
            
            RECT trialRect = {10, height - 28, width / 2 + 30, height - 10};
            DrawTextW(memDC, trialText, -1, &trialRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        } else {
            SetTextColor(memDC, RGB(255, 69, 0));
            RECT expiredRect = {10, height - 28, width / 2 + 30, height - 10};
            DrawTextW(memDC, L"● TRIAL EXPIRED", -1, &expiredRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    } else {
        bool isMonitoring = dataManager_->IsMonitoring();
        const wchar_t* statusText = isMonitoring ? L"● MONITORING" : L"○ PAUSED";
        COLORREF statusColor = isMonitoring ? COLOR_PRIMARY_IN : COLOR_TEXT_DIM;
        SetTextColor(memDC, statusColor);
        
        RECT statusRect = {10, height - 28, width / 2, height - 10};
        DrawTextW(memDC, statusText, -1, &statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    
    DeleteObject(hStatusFont);
    
    if (!labelText.empty() && (isLicensed_ || isTrialActive_)) {
        HFONT hCumulativeFont = UIHelper::CreateModernFont(8, FW_BOLD);
        SelectObject(memDC, hCumulativeFont);
        SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
        SetTextColor(memDC, RGB(255, 200, 0));
        
        RECT cumulativeRect = {10, height - 16, width - 10, height - 4};
        DrawTextW(memDC, labelText.c_str(), -1, &cumulativeRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        DeleteObject(hCumulativeFont);
    }
    
    int cardWidth = 60;
    int cardHeight = 20;
    int cardMargin = 6;
    historyCardRect_.left = width - cardWidth - cardMargin;
    historyCardRect_.top = height - cardHeight - cardMargin;
    historyCardRect_.right = width - cardMargin;
    historyCardRect_.bottom = height - cardMargin;
    
    int cardRadius = 2;
    HRGN historyRegion = CreateRoundRectRgn(
        historyCardRect_.left, historyCardRect_.top,
        historyCardRect_.right, historyCardRect_.bottom,
        cardRadius * 2, cardRadius * 2
    );
    
    COLORREF cardBgColor;
    if (!isLicensed_ && !isTrialActive_) {
        cardBgColor = isHistoryHovered_ ? RGB(255, 100, 50) : COLOR_PAY_CARD_BG;
    } else {
        cardBgColor = isHistoryHovered_ ? RGB(240, 240, 240) : RGB(255, 255, 255);
    }
    
    HBRUSH historyBrush = CreateSolidBrush(cardBgColor);
    FillRgn(memDC, historyRegion, historyBrush);
    DeleteObject(historyBrush);
    DeleteObject(historyRegion);
    
    HPEN historyPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen2 = (HPEN)SelectObject(memDC, historyPen);
    HBRUSH oldBrush2 = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
    RoundRect(memDC, historyCardRect_.left, historyCardRect_.top, 
              historyCardRect_.right, historyCardRect_.bottom, cardRadius * 2, cardRadius * 2);
    SelectObject(memDC, oldPen2);
    SelectObject(memDC, oldBrush2);
    DeleteObject(historyPen);
    
    HFONT hHistoryFont = UIHelper::CreateModernFont(15, FW_BOLD);
    SelectObject(memDC, hHistoryFont);
    SetBkMode(memDC, TRANSPARENT);  // CRITICAL: Re-set after font change
    SetTextColor(memDC, RGB(0, 0, 0));
    
    const wchar_t* buttonText = (!isLicensed_ && !isTrialActive_) ? L"PAY" : L"History";
    
    DrawTextW(memDC, buttonText, -1, &historyCardRect_, 
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(hHistoryFont);
    SelectObject(memDC, hOldFont);
    DeleteObject(hLabelFont);
    DeleteObject(hDataFont);
    
    BYTE* pixels = (BYTE*)pBits;
    BYTE desiredAlpha = 240;
    
    for (int i = 0; i < width * height; i++) {
        BYTE blue = pixels[i * 4 + 0];
        BYTE green = pixels[i * 4 + 1];
        BYTE red = pixels[i * 4 + 2];
        
        if (red || green || blue) {
            pixels[i * 4 + 0] = (blue * desiredAlpha) / 255;
            pixels[i * 4 + 1] = (green * desiredAlpha) / 255;
            pixels[i * 4 + 2] = (red * desiredAlpha) / 255;
            pixels[i * 4 + 3] = desiredAlpha;
        }
    }
    
    RECT windowRect;
    GetWindowRect(hwnd_, &windowRect);
    POINT ptSrc = {0, 0};
    POINT ptDst = {windowRect.left, windowRect.top};
    SIZE sizeWnd = {width, height};
    
    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    
    UpdateLayeredWindow(hwnd_, hdcScreen, &ptDst, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdcScreen);
    
    ValidateRect(hwnd_, NULL);
}