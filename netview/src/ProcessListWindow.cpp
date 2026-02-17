#include "ProcessListWindow.h"
#include "UIHelper.h"
#include <vector>

ProcessListWindow* ProcessListWindow::instance_ = nullptr;

ProcessListWindow::ProcessListWindow(ProcessManager* pm)
    : processManager_(pm), hwnd_(NULL), closeButton_(NULL) {
    instance_ = this;
}

ProcessListWindow::~ProcessListWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
    instance_ = nullptr;
}

bool ProcessListWindow::Create() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = StaticWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WND_CLASS_PROCESSLIST;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_WINDOW_BG);
    
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = screenHeight / 2;
    int windowHeight = screenHeight - 100;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int x = screenWidth - windowWidth - 10;
    int y = 10;
    
    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WND_CLASS_PROCESSLIST,
        L"Process Control",
        WS_POPUP | WS_VISIBLE,
        x, y, windowWidth, windowHeight,
        NULL, NULL, GetModuleHandle(NULL), this
    );
    
    if (!hwnd_) return false;
    
    SetLayeredWindowAttributes(hwnd_, 0, 250, LWA_ALPHA);
    UIHelper::SetDarkTheme(hwnd_);
    
    return true;
}

void ProcessListWindow::Show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        Refresh();
    }
}

void ProcessListWindow::Hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void ProcessListWindow::Refresh() {
    if (hwnd_) {
        DestroyControls();
        CreateControls();
        InvalidateRect(hwnd_, NULL, TRUE);
    }
}

void ProcessListWindow::CreateControls() {
    std::vector<ProcessInfo> processes = processManager_->GetAllProcesses();
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    
    int y = 50;
    int cardHeight = 70;
    int margin = 10;
    int cardWidth = clientRect.right - 2 * margin;
    
    HFONT hFont = UIHelper::CreateModernFont(11, FW_NORMAL);
    HFONT hBoldFont = UIHelper::CreateModernFont(12, FW_BOLD);
    
    for (size_t i = 0; i < processes.size(); i++) {
        const ProcessInfo& info = processes[i];
        
        HWND hProcessLabel = CreateWindowExW(
            0, L"STATIC", info.name.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin + 15, y + 10, cardWidth - 150, 20,
            hwnd_, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessage(hProcessLabel, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
        processLabels_.push_back(hProcessLabel);
        
        std::wstring pidText = L"PID: " + std::to_wstring(info.pid);
        HWND hPidLabel = CreateWindowExW(
            0, L"STATIC", pidText.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin + 15, y + 32, cardWidth - 150, 18,
            hwnd_, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessage(hPidLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
        pidLabels_.push_back(hPidLabel);
        
        HWND hActiveBtn = CreateWindowExW(
            0, L"BUTTON", L"Active",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            cardWidth - 130, y + 15, 60, 35,
            hwnd_, (HMENU)(BTN_ID_ACTIVE_BASE + i), GetModuleHandle(NULL), NULL
        );
        SendMessage(hActiveBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        activeButtons_.push_back(hActiveBtn);
        
        HWND hBlockBtn = CreateWindowExW(
            0, L"BUTTON", L"Block",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            cardWidth - 60, y + 15, 60, 35,
            hwnd_, (HMENU)(BTN_ID_BLOCK_BASE + i), GetModuleHandle(NULL), NULL
        );
        SendMessage(hBlockBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        blockButtons_.push_back(hBlockBtn);
        
        y += cardHeight + margin;
    }
    
    closeButton_ = CreateWindowExW(
        0, L"BUTTON", L"Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
        clientRect.right - 100, clientRect.bottom - 50, 90, 40,
        hwnd_, (HMENU)BTN_ID_CLOSE, GetModuleHandle(NULL), NULL
    );
    SendMessage(closeButton_, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
}

void ProcessListWindow::DestroyControls() {
    for (HWND hwnd : processLabels_) DestroyWindow(hwnd);
    for (HWND hwnd : pidLabels_) DestroyWindow(hwnd);
    for (HWND hwnd : activeButtons_) DestroyWindow(hwnd);
    for (HWND hwnd : blockButtons_) DestroyWindow(hwnd);
    if (closeButton_) DestroyWindow(closeButton_);
    
    processLabels_.clear();
    pidLabels_.clear();
    activeButtons_.clear();
    blockButtons_.clear();
    closeButton_ = NULL;
}

LRESULT CALLBACK ProcessListWindow::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ProcessListWindow* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (ProcessListWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (ProcessListWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->WindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ProcessListWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_COMMAND:
            OnCommand(wParam);
            return 0;
            
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            
            int cmdId = LOWORD(wParam);
            bool isActiveBtn = (cmdId >= BTN_ID_ACTIVE_BASE && cmdId < BTN_ID_BLOCK_BASE);
            bool isBlockBtn = (cmdId >= BTN_ID_BLOCK_BASE && cmdId < BTN_ID_CLOSE);
            bool isCloseBtn = (cmdId == BTN_ID_CLOSE);
            
            COLORREF bgColor = COLOR_BTN_INACTIVE;
            COLORREF textColor = COLOR_TEXT_WHITE;
            
            if (isCloseBtn) {
                bgColor = COLOR_BTN_BLOCKED;
            } else if (isActiveBtn || isBlockBtn) {
                int index = isActiveBtn ? (cmdId - BTN_ID_ACTIVE_BASE) : (cmdId - BTN_ID_BLOCK_BASE);
                std::vector<ProcessInfo> processes = processManager_->GetAllProcesses();
                
                if (index < (int)processes.size()) {
                    bool isBlocked = processes[index].isBlocked;
                    
                    if (isActiveBtn) {
                        bgColor = isBlocked ? COLOR_BTN_INACTIVE : COLOR_BTN_ACTIVE;
                    } else if (isBlockBtn) {
                        bgColor = isBlocked ? COLOR_BTN_BLOCKED : COLOR_BTN_INACTIVE;
                    }
                }
            }
            
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(dis->hDC, &dis->rcItem, hBrush);
            DeleteObject(hBrush);
            
            wchar_t text[64];
            GetWindowTextW(dis->hwndItem, text, 64);
            SetTextColor(dis->hDC, textColor);
            SetBkMode(dis->hDC, TRANSPARENT);
            DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            return TRUE;
        }
            
        case WM_ERASEBKGND:
            return 1;
            
        case WM_DESTROY:
            DestroyControls();
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ProcessListWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    // BLACK background
    HBRUSH bgBrush = CreateSolidBrush(COLOR_WINDOW_BG);
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    
    HFONT hTitleFont = UIHelper::CreateModernFont(16, FW_BOLD);
    HFONT hOldFont = (HFONT)SelectObject(memDC, hTitleFont);
    SetTextColor(memDC, COLOR_TEXT_WHITE);
    SetBkMode(memDC, TRANSPARENT);
    
    RECT titleRect = {20, 15, clientRect.right - 20, 40};
    DrawTextW(memDC, L"Process Control", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    std::vector<ProcessInfo> processes = processManager_->GetAllProcesses();
    int y = 50;
    int cardHeight = 70;
    int margin = 10;
    
    for (size_t i = 0; i < processes.size(); i++) {
        RECT cardRect = {margin, y, clientRect.right - margin, y + cardHeight};
        
        UIHelper::DrawRoundedRect(memDC, cardRect, 6, COLOR_CARD_BG, COLOR_CARD_BORDER, 1);
        
        y += cardHeight + margin;
    }
    
    SelectObject(memDC, hOldFont);
    DeleteObject(hTitleFont);
    
    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);
    
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    
    EndPaint(hwnd_, &ps);
}

void ProcessListWindow::OnCommand(WPARAM wParam) {
    int cmdId = LOWORD(wParam);
    
    if (cmdId == BTN_ID_CLOSE) {
        Hide();
        return;
    }
    
    std::vector<ProcessInfo> processes = processManager_->GetAllProcesses();
    
    if (cmdId >= BTN_ID_ACTIVE_BASE && cmdId < BTN_ID_BLOCK_BASE) {
        int index = cmdId - BTN_ID_ACTIVE_BASE;
        if (index < (int)processes.size()) {
            processManager_->UnblockProcess(processes[index].name);
            Refresh();
        }
    }
    
    if (cmdId >= BTN_ID_BLOCK_BASE && cmdId < BTN_ID_CLOSE) {
        int index = cmdId - BTN_ID_BLOCK_BASE;
        if (index < (int)processes.size()) {
            processManager_->BlockProcess(processes[index].name);
            Refresh();
        }
    }
}
