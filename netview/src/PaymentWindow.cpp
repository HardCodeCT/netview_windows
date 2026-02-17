#include <windows.h>
#include <objbase.h>
#include "PaymentWindow.h"
#include "FirebaseManager.h"
#include "LicenseManager.h"
#include "UIHelper.h"
#include <gdiplus.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;

PaymentWindow* PaymentWindow::instance_ = nullptr;

PaymentWindow::PaymentWindow() 
    : hwnd_(NULL)
    , walletAddressEdit_(NULL)
    , confirmButton_(NULL)
    , scrollPos_(0)
    , isVisible_(false) {
    
    instance_ = this;
    
    // Initialize crypto wallets (same as Android app)
    wallets_ = {
        {L"Bitcoin", L"BTC", L"0.0006 BTC", L"bc1qaqdwv7tzfr4m597pad4f894m69gf83dqze93ck", 0},
        {L"Ethereum", L"ETH", L"0.016 ETH", L"0xF18022fE8D3a432464B7740392e16793C41AD746", 0},
        {L"Tether", L"USDT", L"40 USDT", L"0xF18022fE8D3a432464B7740392e16793C41AD746", 0},
        {L"Binance Coin", L"BNB", L"0.06 BNB", L"0xF18022fE8D3a432464B7740392e16793C41AD746", 0},
        {L"Solana", L"SOL", L"0.30 SOL", L"CWnyw7pFhBFY8HYoo3sQx1gyGjbNLi28oq1UAqsabkDv", 0},
        {L"Litecoin", L"LTC", L"0.4 LTC", L"ltc1qeuwatekvym4txerz5fa23lajw2y4t2ttx8zzj9", 0}
    };
}

PaymentWindow::~PaymentWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
    instance_ = nullptr;
}

bool PaymentWindow::Create(HWND parentHwnd) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = StaticWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WND_CLASS_PAYMENT;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(PAYMENT_COLOR_BACKGROUND);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    int windowWidth = 500;
    int windowHeight = 700;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WND_CLASS_PAYMENT,
        L"NetView - Payment ($9.99 Lifetime Access)",
        WS_POPUP | WS_VISIBLE | WS_VSCROLL,
        x, y, windowWidth, windowHeight,
        parentHwnd,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (!hwnd_) return false;
    
    UIHelper::SetDarkTheme(hwnd_);
    
    // Create wallet address input
    int inputY = 580;
    walletAddressEdit_ = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_MULTILINE,
        20, inputY, windowWidth - 40, 50,
        hwnd_,
        (HMENU)1001,
        GetModuleHandle(NULL),
        NULL
    );
    
    HFONT hEditFont = UIHelper::CreateModernFont(12, FW_NORMAL);
    SendMessage(walletAddressEdit_, WM_SETFONT, (WPARAM)hEditFont, TRUE);
    
    // Create confirm button
    confirmButton_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"CONFIRM PAYMENT",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, inputY + 60, windowWidth - 40, 50,
        hwnd_,
        (HMENU)1002,
        GetModuleHandle(NULL),
        NULL
    );
    
    HFONT hButtonFont = UIHelper::CreateModernFont(14, FW_BOLD);
    SendMessage(confirmButton_, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
    
    return true;
}

void PaymentWindow::Show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        isVisible_ = true;
    }
}

void PaymentWindow::Hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
        isVisible_ = false;
    }
}

bool PaymentWindow::IsVisible() const {
    return isVisible_;
}

LRESULT CALLBACK PaymentWindow::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PaymentWindow* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (PaymentWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (PaymentWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->WindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT PaymentWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;
            
        case WM_VSCROLL: {
            SCROLLINFO si = {0};
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd_, SB_VERT, &si);
            
            int prevPos = si.nPos;
            
            switch (LOWORD(wParam)) {
                case SB_LINEUP:
                    si.nPos -= 20;
                    break;
                case SB_LINEDOWN:
                    si.nPos += 20;
                    break;
                case SB_PAGEUP:
                    si.nPos -= si.nPage;
                    break;
                case SB_PAGEDOWN:
                    si.nPos += si.nPage;
                    break;
                case SB_THUMBTRACK:
                    si.nPos = si.nTrackPos;
                    break;
            }
            
            si.fMask = SIF_POS;
            SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
            GetScrollInfo(hwnd_, SB_VERT, &si);
            
            if (si.nPos != prevPos) {
                ScrollWindow(hwnd_, 0, prevPos - si.nPos, NULL, NULL);
                UpdateWindow(hwnd_);
            }
            
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            SCROLLINFO si = {0};
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd_, SB_VERT, &si);
            
            si.nPos -= (delta / WHEEL_DELTA) * 40;
            si.fMask = SIF_POS;
            SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
            
            InvalidateRect(hwnd_, NULL, TRUE);
            return 0;
        }
        
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                Hide();
                return 0;
            }
            break;
            
        case WM_CLOSE:
            Hide();
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void PaymentWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(PAYMENT_COLOR_BACKGROUND);
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Get scroll position
    SCROLLINFO si = {0};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    GetScrollInfo(hwnd_, SB_VERT, &si);
    int scrollY = si.nPos;
    
    SetBkMode(hdc, TRANSPARENT);
    
    // Draw header
    HFONT hHeaderFont = UIHelper::CreateModernFont(16, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hHeaderFont);
    SetTextColor(hdc, PAYMENT_COLOR_TEXT_SECONDARY);
    
    RECT headerRect = {20, 20 - scrollY, clientRect.right - 20, 80 - scrollY};
    DrawTextW(hdc, L"Send minimum $9.99 equivalent to any wallet below", -1,
              &headerRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
    
    DeleteObject(hHeaderFont);
    
    // Draw wallets
    int yPos = 100;
    for (const auto& wallet : wallets_) {
        DrawWallet(hdc, yPos - scrollY, wallet);
        yPos += 140;
    }
    
    // Draw instructions
    HFONT hSmallFont = UIHelper::CreateModernFont(11, FW_NORMAL);
    SelectObject(hdc, hSmallFont);
    SetTextColor(hdc, PAYMENT_COLOR_TEXT_DIM);
    
    RECT instructRect = {20, 520 - scrollY, clientRect.right - 20, 570 - scrollY};
    DrawTextW(hdc, 
        L"Enter Your Wallet Address:\n"
        L"Paste the wallet address you sent payment from",
        -1, &instructRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
    
    DeleteObject(hSmallFont);
    
    // Draw footer text
    HFONT hFooterFont = UIHelper::CreateModernFont(10, FW_NORMAL);
    SelectObject(hdc, hFooterFont);
    SetTextColor(hdc, PAYMENT_COLOR_TEXT_DIM);
    
    RECT footerRect = {20, 650 - scrollY, clientRect.right - 20, 690 - scrollY};
    DrawTextW(hdc,
        L"After payment, enter your wallet address and tap 'Confirm Payment'.\n"
        L"Verification usually takes 1-24 hours.",
        -1, &footerRect, DT_CENTER | DT_TOP | DT_WORDBREAK);
    
    DeleteObject(hFooterFont);
    
    SelectObject(hdc, hOldFont);
    
    // Setup scrollbar
    SCROLLINFO scrollInfo = {0};
    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = 750;
    scrollInfo.nPage = clientRect.bottom;
    SetScrollInfo(hwnd_, SB_VERT, &scrollInfo, TRUE);
    
    EndPaint(hwnd_, &ps);
}

void PaymentWindow::DrawWallet(HDC hdc, int yPos, const CryptoWallet& wallet) {
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    
    int cardX = 20;
    int cardWidth = clientRect.right - 40;
    int cardHeight = 130;
    
    // Draw card background with rounded corners
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    
    SolidBrush cardBrush(Color(255, 26, 26, 26));
    Pen cardPen(Color(255, 60, 60, 60), 1.0f);
    
    GraphicsPath path;
    int radius = 12;
    path.AddArc(cardX, yPos, radius * 2, radius * 2, 180, 90);
    path.AddArc(cardX + cardWidth - radius * 2, yPos, radius * 2, radius * 2, 270, 90);
    path.AddArc(cardX + cardWidth - radius * 2, yPos + cardHeight - radius * 2, radius * 2, radius * 2, 0, 90);
    path.AddArc(cardX, yPos + cardHeight - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();
    
    graphics.FillPath(&cardBrush, &path);
    graphics.DrawPath(&cardPen, &path);
    
    // Draw wallet name
    HFONT hNameFont = UIHelper::CreateModernFont(15, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hNameFont);
    SetTextColor(hdc, PAYMENT_COLOR_TEXT_PRIMARY);
    
    std::wstring nameText = wallet.name + L" (" + wallet.symbol + L")";
    RECT nameRect = {cardX + 16, yPos + 12, cardX + cardWidth - 16, yPos + 35};
    DrawTextW(hdc, nameText.c_str(), -1, &nameRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
    
    // Draw min amount
    HFONT hAmountFont = UIHelper::CreateModernFont(12, FW_NORMAL);
    SelectObject(hdc, hAmountFont);
    SetTextColor(hdc, PAYMENT_COLOR_TEXT_SECONDARY);
    
    std::wstring minText = L"Min: " + wallet.minAmount + L" (~$9.99)";
    RECT minRect = {cardX + 16, yPos + 35, cardX + cardWidth - 16, yPos + 55};
    DrawTextW(hdc, minText.c_str(), -1, &minRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
    
    // Draw address background
    HBRUSH addrBgBrush = CreateSolidBrush(PAYMENT_COLOR_CARD_ADDRESS_BG);
    RECT addrBgRect = {cardX + 16, yPos + 60, cardX + cardWidth - 16, yPos + 85};
    FillRect(hdc, &addrBgRect, addrBgBrush);
    DeleteObject(addrBgBrush);
    
    // Draw address
    HFONT hAddrFont = CreateFontW(
        12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Consolas"
    );
    SelectObject(hdc, hAddrFont);
    SetTextColor(hdc, RGB(224, 224, 224));
    
    RECT addrRect = {cardX + 24, yPos + 63, cardX + cardWidth - 24, yPos + 82};
    DrawTextW(hdc, wallet.address.c_str(), -1, &addrRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Create copy button
    static int buttonId = 2000;
    HWND copyBtn = CreateWindowExW(
        0,
        L"BUTTON",
        (L"COPY " + wallet.symbol + L" ADDRESS").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        cardX + 16, yPos + 92, cardWidth - 32, 32,
        hwnd_,
        (HMENU)(LONG_PTR)buttonId++,
        GetModuleHandle(NULL),
        NULL
    );
    
    HFONT hBtnFont = UIHelper::CreateModernFont(11, FW_BOLD);
    SendMessage(copyBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    
    copyButtons_.push_back(copyBtn);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hNameFont);
    DeleteObject(hAmountFont);
    DeleteObject(hAddrFont);
}

void PaymentWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
    int id = LOWORD(wParam);
    
    if (id == 1002) {  // Confirm button
        OnConfirmPayment();
        return;
    }
    
    // Copy button handlers (id >= 2000)
    if (id >= 2000 && id < 2000 + (int)wallets_.size()) {
        int index = id - 2000;
        if (index < wallets_.size()) {
            OnCopyAddress(wallets_[index].address, wallets_[index].symbol);
        }
    }
}

void PaymentWindow::OnCopyAddress(const std::wstring& address, const std::wstring& symbol) {
    if (OpenClipboard(hwnd_)) {
        EmptyClipboard();
        
        SIZE_T size = (address.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        
        if (hGlobal) {
            void* pGlobal = GlobalLock(hGlobal);
            if (pGlobal) {
                memcpy(pGlobal, address.c_str(), size);
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        
        CloseClipboard();
        
        std::wstring message = symbol + L" address copied to clipboard!";
        MessageBoxW(hwnd_, message.c_str(), L"Copied", MB_OK | MB_ICONINFORMATION);
    }
}

void PaymentWindow::OnConfirmPayment() {
    wchar_t buffer[512];
    GetWindowTextW(walletAddressEdit_, buffer, 512);
    std::wstring walletAddress = buffer;
    
    // Trim whitespace
    size_t start = walletAddress.find_first_not_of(L" \t\r\n");
    size_t end = walletAddress.find_last_not_of(L" \t\r\n");
    
    if (start == std::wstring::npos || end == std::wstring::npos) {
        MessageBoxW(hwnd_, L"Please enter your wallet address!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    walletAddress = walletAddress.substr(start, end - start + 1);
    
    if (walletAddress.length() < 20) {
        MessageBoxW(hwnd_, L"Invalid wallet address!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Get installation key
    std::wstring installKey = LicenseManager::GetInstance().GetInstallationKey();
    
    // Prepare payment submission
    FirebaseManager::PaymentSubmission payment;
    payment.installationKey = installKey;
    payment.walletAddress = walletAddress;
    payment.timestamp = L""; // Will be set by FirebaseManager
    
    // Show processing message
    MessageBoxW(hwnd_, 
        L"Submitting payment information...\n\n"
        L"Verification typically takes 1-24 hours.\n"
        L"You will receive full access once verified.",
        L"Processing", MB_OK | MB_ICONINFORMATION);
    
    // Submit to Firebase
    FirebaseManager::GetInstance().SubmitPayment(payment,
        [this](bool success, const std::wstring& message) {
            if (success) {
                MessageBoxW(hwnd_,
                    L"Payment submitted successfully!\n\n"
                    L"Your submission is being verified.\n"
                    L"This usually takes 1-24 hours.\n\n"
                    L"Thank you for supporting NetView!",
                    L"Success", MB_OK | MB_ICONINFORMATION);
                
                SetWindowTextW(walletAddressEdit_, L"");
                Hide();
            } else {
                MessageBoxW(hwnd_,
                    (L"Failed to submit payment:\n" + message).c_str(),
                    L"Error", MB_OK | MB_ICONERROR);
            }
        });
}
