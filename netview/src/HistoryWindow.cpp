#include "HistoryWindow.h"
#include "MonitorWidget.h"
#include "UIHelper.h"
#include "FontManager.h"
#include "resource.h"
#include <windows.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <algorithm>
#include <shellapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Color scheme
#define HISTORY_BG RGB(15, 15, 15)
#define HISTORY_HEADER_BG RGB(35, 35, 35)
#define HISTORY_ROW_BG RGB(25, 25, 25)
#define HISTORY_ROW_ALT_BG RGB(20, 20, 20)
#define HISTORY_TOTAL_BG RGB(45, 35, 60)
#define HISTORY_BORDER RGB(160, 120, 200)
#define HISTORY_TEXT RGB(255, 255, 255)
#define HISTORY_TEXT_DIM RGB(180, 180, 180)
#define HISTORY_ACCENT RGB(160, 120, 200)
#define HISTORY_MONTHLY_TEXT RGB(255, 200, 100)
#define HISTORY_YEARLY_TEXT RGB(100, 255, 200)
#define TOGGLE_ACTIVE_TRACK RGB(160, 120, 200)
#define TOGGLE_INACTIVE_TRACK RGB(70, 70, 70)
#define TOGGLE_THUMB_ACTIVE RGB(220, 190, 255)
#define TOGGLE_THUMB_INACTIVE RGB(120, 120, 120)
#define CARD_BG RGB(25, 25, 25)
#define CARD_BORDER RGB(160, 120, 200)

HistoryWindow* HistoryWindow::instance_ = nullptr;

HistoryWindow::HistoryWindow(HistoryManager* historyMgr)
    : historyManager_(historyMgr)
    , monitorWidget_(nullptr)
    , hwnd_(NULL)
    , scrollOffset_(0)
    , maxScrollOffset_(0)
    , toggledDate_(L"")
    , hoveredSocialIcon_(-1) {
    instance_ = this;
    twitterIconRect_ = {0};
    whatsappIconRect_ = {0};
    gmailIconRect_ = {0};
}

HistoryWindow::~HistoryWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
    instance_ = nullptr;
}

bool HistoryWindow::Create() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = StaticWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WND_CLASS_HISTORY;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(HISTORY_BG);
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - HISTORY_WIDTH) / 2;
    int y = (screenHeight - HISTORY_HEIGHT) / 2;
    
    hwnd_ = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
        WND_CLASS_HISTORY,
        L"NetView - Usage History",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, HISTORY_WIDTH, HISTORY_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), this
    );
    
    if (!hwnd_) return false;
    
    UIHelper::SetDarkTheme(hwnd_);
    
    return true;
}

void HistoryWindow::Show() {
    if (hwnd_) {
        scrollOffset_ = 0;
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        SetTimer(hwnd_, TIMER_HISTORY_REFRESH, 1000, NULL);
    }
}

void HistoryWindow::Hide() {
    if (hwnd_) {
        KillTimer(hwnd_, TIMER_HISTORY_REFRESH);
        ShowWindow(hwnd_, SW_HIDE);
    }
}

bool HistoryWindow::IsVisible() const {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void HistoryWindow::OpenURL(const std::wstring& url) {
    ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

Image* HistoryWindow::LoadPNGFromResource(int resourceID) {
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

LRESULT CALLBACK HistoryWindow::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HistoryWindow* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (HistoryWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (HistoryWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->WindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT HistoryWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_TIMER:
            if (wParam == TIMER_HISTORY_REFRESH) {
                InvalidateRect(hwnd_, NULL, TRUE);
            }
            return 0;
            
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            // Check social icons first
            POINT pt = {x, y};
            if (PtInRect(&twitterIconRect_, pt)) {
                OpenURL(L"https://x.com/Hard_Code_T");
                return 0;
            } else if (PtInRect(&whatsappIconRect_, pt)) {
                OpenURL(L"https://wa.me/2348165713623");
                return 0;
            } else if (PtInRect(&gmailIconRect_, pt)) {
                OpenURL(L"mailto:firmino3535@gmail.com?subject=Contact from NetView");
                return 0;
            }
            
            // Handle toggle clicks
            HandleToggleClick(x, y);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            POINT pt = {x, y};
            int oldHovered = hoveredSocialIcon_;
            
            if (PtInRect(&twitterIconRect_, pt)) {
                hoveredSocialIcon_ = 0;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else if (PtInRect(&whatsappIconRect_, pt)) {
                hoveredSocialIcon_ = 1;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else if (PtInRect(&gmailIconRect_, pt)) {
                hoveredSocialIcon_ = 2;
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else {
                hoveredSocialIcon_ = -1;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            
            if (oldHovered != hoveredSocialIcon_) {
                InvalidateRect(hwnd_, NULL, FALSE);
            }
            
            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd_;
            TrackMouseEvent(&tme);
            
            return 0;
        }
        
        case WM_MOUSELEAVE:
            hoveredSocialIcon_ = -1;
            InvalidateRect(hwnd_, NULL, FALSE);
            return 0;
            
        case WM_CLOSE:
            Hide();
            return 0;
            
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            scrollOffset_ -= delta / 4;
            
            if (scrollOffset_ < 0) scrollOffset_ = 0;
            if (scrollOffset_ > maxScrollOffset_) scrollOffset_ = maxScrollOffset_;
            
            InvalidateRect(hwnd_, NULL, TRUE);
            return 0;
        }
            
        case WM_ERASEBKGND:
            return 1;
            
        case WM_DESTROY:
            KillTimer(hwnd_, TIMER_HISTORY_REFRESH);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

RECT HistoryWindow::GetToggleRect(int rowIndex, int rowY) {
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    int width = clientRect.right;
    int tableRight = width - 20;
    
    int switchWidth = 36;
    int switchHeight = 18;
    RECT toggleRect;
    toggleRect.left = tableRight - switchWidth - 10;
    toggleRect.top = rowY + (28 - switchHeight) / 2;
    toggleRect.right = toggleRect.left + switchWidth;
    toggleRect.bottom = toggleRect.top + switchHeight;
    
    return toggleRect;
}

void HistoryWindow::HandleToggleClick(int x, int y) {
    std::vector<DisplayRow> rows = GenerateDisplayRows();
    if (rows.empty()) return;
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    int yPos = 20 + 35 + 30;
    int rowHeight = 28;
    int startY = yPos - scrollOffset_;
    
    for (size_t i = 0; i < rows.size(); i++) {
        const DisplayRow& row = rows[i];
        
        if (row.type != RowType::DAILY) continue;
        
        int rowY = startY + static_cast<int>(i) * rowHeight;
        
        if (rowY + rowHeight < yPos || rowY > clientRect.bottom) {
            continue;
        }
        
        RECT toggleRect = GetToggleRect(static_cast<int>(i), rowY);
        
        POINT pt = {x, y};
        if (PtInRect(&toggleRect, pt)) {
            if (toggledDate_ == row.date) {
                toggledDate_ = L"";
                if (monitorWidget_) {
                    monitorWidget_->SetCumulativeMode(L"");
                }
            } else {
                toggledDate_ = row.date;
                if (monitorWidget_) {
                    monitorWidget_->SetCumulativeMode(row.date);
                }
            }
            
            InvalidateRect(hwnd_, NULL, TRUE);
            return;
        }
    }
}

void HistoryWindow::CalculateTotals(const std::vector<DailyUsage>& daily,
                                   std::map<std::wstring, std::pair<UINT64, UINT64>>& monthlyTotals,
                                   std::map<std::wstring, std::pair<UINT64, UINT64>>& yearlyTotals) {
    for (const auto& day : daily) {
        if (day.date.length() >= 7) {
            std::wstring month = day.date.substr(0, 7);
            monthlyTotals[month].first += day.downloadBytes;
            monthlyTotals[month].second += day.uploadBytes;
        }
        
        if (day.date.length() >= 4) {
            std::wstring year = day.date.substr(0, 4);
            yearlyTotals[year].first += day.downloadBytes;
            yearlyTotals[year].second += day.uploadBytes;
        }
    }
}

void HistoryWindow::CalculateSummaryStats(SummaryStats& stats) {
    std::vector<DailyUsage> daily = historyManager_->GetAllDailyRecords();
    
    stats.todayTotal = 0;
    stats.last7DaysTotal = 0;
    stats.thisMonthTotal = 0;
    stats.lastMonthTotal = 0;
    
    if (daily.empty()) return;
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t todayStr[11];
    swprintf_s(todayStr, 11, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    std::wstring today(todayStr);
    
    wchar_t thisMonthStr[8];
    swprintf_s(thisMonthStr, 8, L"%04d-%02d", st.wYear, st.wMonth);
    std::wstring thisMonth(thisMonthStr);
    
    int lastMonthYear = st.wYear;
    int lastMonthMonth = st.wMonth - 1;
    if (lastMonthMonth == 0) {
        lastMonthMonth = 12;
        lastMonthYear--;
    }
    wchar_t lastMonthStr[8];
    swprintf_s(lastMonthStr, 8, L"%04d-%02d", lastMonthYear, lastMonthMonth);
    std::wstring lastMonth(lastMonthStr);
    
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    
    ULONGLONG sevenDaysInNano = 7ULL * 24ULL * 60ULL * 60ULL * 10000000ULL;
    uli.QuadPart -= sevenDaysInNano;
    
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;
    
    SYSTEMTIME st7DaysAgo;
    FileTimeToSystemTime(&ft, &st7DaysAgo);
    wchar_t sevenDaysAgoStr[11];
    swprintf_s(sevenDaysAgoStr, 11, L"%04d-%02d-%02d", st7DaysAgo.wYear, st7DaysAgo.wMonth, st7DaysAgo.wDay);
    std::wstring sevenDaysAgo(sevenDaysAgoStr);
    
    for (const auto& day : daily) {
        if (day.date == today) {
            stats.todayTotal = day.totalBytes;
        }
        
        if (day.date >= sevenDaysAgo && day.date <= today) {
            stats.last7DaysTotal += day.totalBytes;
        }
        
        if (day.date.length() >= 7 && day.date.substr(0, 7) == thisMonth) {
            stats.thisMonthTotal += day.totalBytes;
        }
        
        if (day.date.length() >= 7 && day.date.substr(0, 7) == lastMonth) {
            stats.lastMonthTotal += day.totalBytes;
        }
    }
}

std::vector<DisplayRow> HistoryWindow::GenerateDisplayRows() {
    std::vector<DisplayRow> rows;
    
    std::vector<DailyUsage> daily = historyManager_->GetAllDailyRecords();
    
    if (daily.empty()) {
        return rows;
    }
    
    std::map<std::wstring, std::pair<UINT64, UINT64>> monthlyTotals;
    std::map<std::wstring, std::pair<UINT64, UINT64>> yearlyTotals;
    CalculateTotals(daily, monthlyTotals, yearlyTotals);
    
    std::wstring previousMonth;
    std::wstring previousYear;
    
    for (size_t i = 0; i < daily.size(); i++) {
        const DailyUsage& day = daily[i];
        
        std::wstring currentMonth = day.date.substr(0, 7);
        std::wstring currentYear = day.date.substr(0, 4);
        
        if (!previousYear.empty() && currentYear != previousYear) {
            if (!previousMonth.empty() && monthlyTotals.count(previousMonth)) {
                auto& mt = monthlyTotals[previousMonth];
                std::wstring monthLabel = L"Total Monthly Usage: " + previousMonth;
                rows.push_back(DisplayRow(
                    RowType::MONTHLY_TOTAL,
                    monthLabel,
                    historyManager_->FormatBytes(mt.first),
                    historyManager_->FormatBytes(mt.second),
                    historyManager_->FormatBytes(mt.first + mt.second)
                ));
            }
            
            if (yearlyTotals.count(previousYear)) {
                auto& yt = yearlyTotals[previousYear];
                std::wstring yearLabel = L"Total Yearly Usage: " + previousYear;
                rows.push_back(DisplayRow(
                    RowType::YEARLY_TOTAL,
                    yearLabel,
                    historyManager_->FormatBytes(yt.first),
                    historyManager_->FormatBytes(yt.second),
                    historyManager_->FormatBytes(yt.first + yt.second)
                ));
            }
        }
        else if (!previousMonth.empty() && currentMonth != previousMonth) {
            if (monthlyTotals.count(previousMonth)) {
                auto& mt = monthlyTotals[previousMonth];
                std::wstring monthLabel = L"Total Monthly Usage: " + previousMonth;
                rows.push_back(DisplayRow(
                    RowType::MONTHLY_TOTAL,
                    monthLabel,
                    historyManager_->FormatBytes(mt.first),
                    historyManager_->FormatBytes(mt.second),
                    historyManager_->FormatBytes(mt.first + mt.second)
                ));
            }
        }
        
        rows.push_back(DisplayRow(
            RowType::DAILY,
            day.date,
            historyManager_->FormatBytes(day.downloadBytes),
            historyManager_->FormatBytes(day.uploadBytes),
            historyManager_->FormatBytes(day.totalBytes),
            day.date
        ));
        
        previousMonth = currentMonth;
        previousYear = currentYear;
    }
    
    if (!previousMonth.empty() && monthlyTotals.count(previousMonth)) {
        auto& mt = monthlyTotals[previousMonth];
        std::wstring monthLabel = L"Total Monthly Usage: " + previousMonth;
        rows.push_back(DisplayRow(
            RowType::MONTHLY_TOTAL,
            monthLabel,
            historyManager_->FormatBytes(mt.first),
            historyManager_->FormatBytes(mt.second),
            historyManager_->FormatBytes(mt.first + mt.second)
        ));
    }
    
    if (!previousYear.empty() && yearlyTotals.count(previousYear)) {
        auto& yt = yearlyTotals[previousYear];
        std::wstring yearLabel = L"Total Yearly Usage: " + previousYear;
        rows.push_back(DisplayRow(
            RowType::YEARLY_TOTAL,
            yearLabel,
            historyManager_->FormatBytes(yt.first),
            historyManager_->FormatBytes(yt.second),
            historyManager_->FormatBytes(yt.first + yt.second)
        ));
    }
    
    return rows;
}

void HistoryWindow::DrawSummaryCards(HDC hdc, const RECT& clientRect) {
    SummaryStats stats;
    CalculateSummaryStats(stats);
    
    int cardHeight = 70;
    int cardSpacing = 15;
    int sideMargin = 20;
    int bottomMargin = 20;
    
    int totalWidth = clientRect.right - (2 * sideMargin);
    int cardWidth = (totalWidth - (3 * cardSpacing)) / 4;
    
    int yPos = clientRect.bottom - bottomMargin - cardHeight;
    
    struct CardData {
        std::wstring title;
        UINT64 value;
    };
    
    CardData cards[4] = {
        {L"Today", stats.todayTotal},
        {L"Last 7 Days", stats.last7DaysTotal},
        {L"This Month", stats.thisMonthTotal},
        {L"Last Month", stats.lastMonthTotal}
    };
    
    HFONT hTitleFont = UIHelper::CreateModernFont(14, FW_SEMIBOLD);
    HFONT hValueFont = UIHelper::CreateModernFont(16, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
    
    SetBkMode(hdc, TRANSPARENT);
    
    for (int i = 0; i < 4; i++) {
        int xPos = sideMargin + (i * (cardWidth + cardSpacing));
        
        RECT cardRect = {xPos, yPos, xPos + cardWidth, yPos + cardHeight};
        
        HBRUSH cardBrush = CreateSolidBrush(CARD_BG);
        FillRect(hdc, &cardRect, cardBrush);
        DeleteObject(cardBrush);
        
        HPEN borderPen = CreatePen(PS_SOLID, 2, CARD_BORDER);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        
        RoundRect(hdc, cardRect.left, cardRect.top, cardRect.right, cardRect.bottom, 10, 10);
        
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(borderPen);
        
        SelectObject(hdc, hTitleFont);
        SetTextColor(hdc, RGB(180, 180, 180));
        RECT titleRect = {cardRect.left + 10, cardRect.top + 10, cardRect.right - 10, cardRect.top + 30};
        DrawTextW(hdc, cards[i].title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, hValueFont);
        SetTextColor(hdc, HISTORY_TEXT);
        std::wstring valueStr = historyManager_->FormatBytes(cards[i].value);
        RECT valueRect = {cardRect.left + 10, cardRect.top + 35, cardRect.right - 10, cardRect.bottom - 10};
        DrawTextW(hdc, valueStr.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hTitleFont);
    DeleteObject(hValueFont);
}

void HistoryWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    int width = clientRect.right;
    int height = clientRect.bottom;
    
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    HBRUSH bgBrush = CreateSolidBrush(HISTORY_BG);
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw title bar with social icons
    DrawTitleBar(memDC, clientRect);
    DrawContent(memDC, clientRect);
    DrawSummaryCards(memDC, clientRect);
    
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    
    EndPaint(hwnd_, &ps);
}

void HistoryWindow::DrawTitleBar(HDC hdc, const RECT& clientRect) {
    int width = clientRect.right;
    int yPos = 20;
    
    SetBkMode(hdc, TRANSPARENT);
    
    // Draw title text
    HFONT hTitleFont = UIHelper::CreateModernFont(18, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
    SetTextColor(hdc, HISTORY_TEXT);
    
    SIZE textSize;
    GetTextExtentPoint32W(hdc, L"Usage History", 13, &textSize);
    
    int textX = 20;
    RECT titleRect = {textX, yPos, textX + textSize.cx, yPos + 25};
    DrawTextW(hdc, L"Usage History", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(hTitleFont);
    SelectObject(hdc, hOldFont);
    
    // Calculate social icons position (center of title bar)
    const int ICON_SIZE = 24;
    const int ICON_SPACING = 15;
    const int TOTAL_ICON_WIDTH = (ICON_SIZE * 3) + (ICON_SPACING * 2);
    
    int centerX = width / 2;
    int iconsStartX = centerX - (TOTAL_ICON_WIDTH / 2);
    int iconsY = yPos + (25 - ICON_SIZE) / 2;
    
    // Setup icon rectangles
    twitterIconRect_.left = iconsStartX;
    twitterIconRect_.top = iconsY;
    twitterIconRect_.right = iconsStartX + ICON_SIZE;
    twitterIconRect_.bottom = iconsY + ICON_SIZE;
    
    whatsappIconRect_.left = iconsStartX + ICON_SIZE + ICON_SPACING;
    whatsappIconRect_.top = iconsY;
    whatsappIconRect_.right = iconsStartX + (ICON_SIZE * 2) + ICON_SPACING;
    whatsappIconRect_.bottom = iconsY + ICON_SIZE;
    
    gmailIconRect_.left = iconsStartX + (ICON_SIZE * 2) + (ICON_SPACING * 2);
    gmailIconRect_.top = iconsY;
    gmailIconRect_.right = iconsStartX + (ICON_SIZE * 3) + (ICON_SPACING * 2);
    gmailIconRect_.bottom = iconsY + ICON_SIZE;
    
    // Draw icons using GDI+
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    
    auto drawSocialIcon = [&](const RECT& rect, int resourceID, bool isHovered) {
        // Draw background circle if hovered
        if (isHovered) {
            SolidBrush hoverBrush(Color(40, 255, 255, 255));
            graphics.FillEllipse(&hoverBrush, rect.left - 2, rect.top - 2, 
                               (rect.right - rect.left) + 4, (rect.bottom - rect.top) + 4);
        }
        
        Image* image = LoadPNGFromResource(resourceID);
        if (image) {
            Rect iconRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
            graphics.DrawImage(image, iconRect, 0, 0, image->GetWidth(), image->GetHeight(), UnitPixel);
            delete image;
        }
    };
    
    drawSocialIcon(twitterIconRect_, IDR_TWITTER_ICON, hoveredSocialIcon_ == 0);
    drawSocialIcon(whatsappIconRect_, IDR_WHATSAPP_ICON, hoveredSocialIcon_ == 1);
    drawSocialIcon(gmailIconRect_, IDR_GMAIL_ICON, hoveredSocialIcon_ == 2);
}

void HistoryWindow::DrawToggleSwitch(HDC hdc, const RECT& toggleRect, bool isToggled) {
    int trackHeight = toggleRect.bottom - toggleRect.top;
    int trackWidth = toggleRect.right - toggleRect.left;
    int thumbRadius = (trackHeight - 4) / 2;
    
    COLORREF trackColor = isToggled ? TOGGLE_ACTIVE_TRACK : TOGGLE_INACTIVE_TRACK;
    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    HPEN trackPen = CreatePen(PS_SOLID, 1, trackColor);
    
    HPEN oldPen = (HPEN)SelectObject(hdc, trackPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, trackBrush);
    
    RoundRect(hdc, toggleRect.left, toggleRect.top, toggleRect.right, toggleRect.bottom, 
              trackHeight, trackHeight);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(trackPen);
    DeleteObject(trackBrush);
    
    COLORREF thumbColor = isToggled ? TOGGLE_THUMB_ACTIVE : TOGGLE_THUMB_INACTIVE;
    HBRUSH thumbBrush = CreateSolidBrush(thumbColor);
    HPEN thumbPen = CreatePen(PS_SOLID, 1, thumbColor);
    
    oldPen = (HPEN)SelectObject(hdc, thumbPen);
    oldBrush = (HBRUSH)SelectObject(hdc, thumbBrush);
    
    int thumbCenterY = toggleRect.top + trackHeight / 2;
    int thumbCenterX;
    
    if (isToggled) {
        thumbCenterX = toggleRect.right - thumbRadius - 3;
    } else {
        thumbCenterX = toggleRect.left + thumbRadius + 3;
    }
    
    Ellipse(hdc, 
            thumbCenterX - thumbRadius, 
            thumbCenterY - thumbRadius,
            thumbCenterX + thumbRadius, 
            thumbCenterY + thumbRadius);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(thumbPen);
    DeleteObject(thumbBrush);
}

void HistoryWindow::DrawContent(HDC hdc, const RECT& clientRect) {
    int width = clientRect.right;
    int yPos = 20 + 35;  // After title bar
    
    SetBkMode(hdc, TRANSPARENT);
    
    int tableLeft = 20;
    int tableRight = width - 20;
    int tableWidth = tableRight - tableLeft;
    
    int col1Width = 150;
    int col2Width = 120;
    int col3Width = 120;
    int col4Width = tableWidth - col1Width - col2Width - col3Width - 60;
    
    RECT headerRect = {tableLeft, yPos, tableRight, yPos + 30};
    HBRUSH headerBrush = CreateSolidBrush(HISTORY_HEADER_BG);
    FillRect(hdc, &headerRect, headerBrush);
    DeleteObject(headerBrush);
    
    HPEN headerPen = CreatePen(PS_SOLID, 1, HISTORY_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, headerPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, headerRect.left, headerRect.top, headerRect.right, headerRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(headerPen);
    
    HFONT hHeaderFont = UIHelper::CreateModernFont(18, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hHeaderFont);
    SetTextColor(hdc, HISTORY_ACCENT);
    
    int headerY = yPos + 7;
    RECT dateHeaderRect = {tableLeft + 10, headerY, tableLeft + col1Width, headerY + 16};
    DrawTextW(hdc, L"Date", -1, &dateHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    RECT downloadHeaderRect = {tableLeft + col1Width, headerY, tableLeft + col1Width + col2Width, headerY + 16};
    DrawTextW(hdc, L"Download", -1, &downloadHeaderRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    RECT uploadHeaderRect = {tableLeft + col1Width + col2Width, headerY, tableLeft + col1Width + col2Width + col3Width, headerY + 16};
    DrawTextW(hdc, L"Upload", -1, &uploadHeaderRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    RECT totalHeaderRect = {tableLeft + col1Width + col2Width + col3Width, headerY, tableRight - 60, headerY + 16};
    DrawTextW(hdc, L"Total", -1, &totalHeaderRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    RECT toggleHeaderRect = {tableRight - 55, headerY, tableRight - 10, headerY + 16};
    DrawTextW(hdc, L"View", -1, &toggleHeaderRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(hHeaderFont);
    
    yPos += 30;
    
    std::vector<DisplayRow> rows = GenerateDisplayRows();
    
    if (rows.empty()) {
        HFONT hFont = UIHelper::CreateModernFont(16, FW_NORMAL);
        SelectObject(hdc, hFont);
        SetTextColor(hdc, HISTORY_TEXT_DIM);
        RECT noDataRect = {tableLeft, yPos + 20, tableRight, yPos + 50};
        DrawTextW(hdc, L"No usage data available", -1, &noDataRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(hFont);
        SelectObject(hdc, hOldFont);
        return;
    }
    
    int rowHeight = 28;
    int contentHeight = static_cast<int>(rows.size()) * rowHeight;
    
    int reservedBottomSpace = 70 + 20 + 15;
    int viewportHeight = clientRect.bottom - yPos - reservedBottomSpace;
    maxScrollOffset_ = max(0, contentHeight - viewportHeight);
    
    if (scrollOffset_ > maxScrollOffset_) scrollOffset_ = maxScrollOffset_;
    
    int startY = yPos - scrollOffset_;
    
    HFONT hRowFont = UIHelper::CreateModernFont(15, FW_NORMAL);
    HFONT hRowBoldFont = UIHelper::CreateModernFont(15, FW_SEMIBOLD);
    
    for (size_t i = 0; i < rows.size(); i++) {
        const DisplayRow& row = rows[i];
        int rowY = startY + static_cast<int>(i) * rowHeight;
        
        if (rowY + rowHeight < yPos || rowY > clientRect.bottom - reservedBottomSpace) {
            continue;
        }
        
        RECT rowRect = {tableLeft, rowY, tableRight, rowY + rowHeight};
        
        COLORREF rowBgColor;
        if (row.type == RowType::YEARLY_TOTAL) {
            rowBgColor = RGB(30, 50, 50);
        } else if (row.type == RowType::MONTHLY_TOTAL) {
            rowBgColor = RGB(50, 40, 30);
        } else {
            rowBgColor = (i % 2 == 0) ? HISTORY_ROW_BG : HISTORY_ROW_ALT_BG;
        }
        
        HBRUSH rowBrush = CreateSolidBrush(rowBgColor);
        FillRect(hdc, &rowRect, rowBrush);
        DeleteObject(rowBrush);
        
        HPEN rowPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
        oldPen = (HPEN)SelectObject(hdc, rowPen);
        MoveToEx(hdc, rowRect.left, rowRect.bottom - 1, NULL);
        LineTo(hdc, rowRect.right, rowRect.bottom - 1);
        SelectObject(hdc, oldPen);
        DeleteObject(rowPen);
        
        COLORREF textColor;
        if (row.type == RowType::YEARLY_TOTAL) {
            textColor = HISTORY_YEARLY_TEXT;
            SelectObject(hdc, hRowBoldFont);
        } else if (row.type == RowType::MONTHLY_TOTAL) {
            textColor = HISTORY_MONTHLY_TEXT;
            SelectObject(hdc, hRowBoldFont);
        } else {
            textColor = HISTORY_TEXT;
            SelectObject(hdc, hRowFont);
        }
        SetTextColor(hdc, textColor);
        
        int textY = rowY + 6;
        
        RECT col1Rect = {tableLeft + 10, textY, tableLeft + col1Width - 5, textY + 16};
        DrawTextW(hdc, row.text1.c_str(), -1, &col1Rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        RECT col2Rect = {tableLeft + col1Width, textY, tableLeft + col1Width + col2Width, textY + 16};
        DrawTextW(hdc, row.text2.c_str(), -1, &col2Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        RECT col3Rect = {tableLeft + col1Width + col2Width, textY, tableLeft + col1Width + col2Width + col3Width, textY + 16};
        DrawTextW(hdc, row.text3.c_str(), -1, &col3Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        RECT col4Rect = {tableLeft + col1Width + col2Width + col3Width + 5, textY, tableRight - 60, textY + 16};
        DrawTextW(hdc, row.text4.c_str(), -1, &col4Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        if (row.type == RowType::DAILY) {
            RECT toggleRect = GetToggleRect(static_cast<int>(i), rowY);
            bool isToggled = (toggledDate_ == row.date);
            DrawToggleSwitch(hdc, toggleRect, isToggled);
        }
    }
    
    DeleteObject(hRowFont);
    DeleteObject(hRowBoldFont);
    SelectObject(hdc, hOldFont);
}
