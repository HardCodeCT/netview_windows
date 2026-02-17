#pragma once
#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <string>

class DataManager;
class HistoryManager;
class HistoryWindow;
class PaymentWindow;

#define WND_CLASS_MONITOR L"NetViewMonitorWidget"
#define WND_CLASS_CONTACT L"NetViewContactPopup"
#define MONITOR_WIDTH 200
#define MONITOR_HEIGHT 140
#define TIMER_UPDATE 1001
#define TIMER_LICENSE_CHECK 1002

// UI Colors - Calculated as hex constants
#define COLOR_SURFACE 0x00000000           // RGB(0, 0, 0)
#define COLOR_SURFACE_LIGHT 0x00141414     // RGB(20, 20, 20)
#define COLOR_BORDER 0x00C878A0            // RGB(160, 120, 200)
#define COLOR_TEXT 0x00FFFFFF              // RGB(255, 255, 255)
#define COLOR_TEXT_DIM 0x00B4B4B4          // RGB(180, 180, 180)
#define COLOR_PRIMARY_IN 0x0000FF00        // RGB(0, 255, 0)
#define COLOR_PRIMARY_OUT 0x000000FF       // RGB(255, 0, 0)
#define COLOR_HISTORY_CARD_BG 0x00C878A0   // RGB(160, 120, 200)
#define COLOR_HISTORY_CARD_BORDER 0x00B4648C  // RGB(140, 100, 180)
#define COLOR_HISTORY_CARD_TEXT 0x00FFFFFF // RGB(255, 255, 255)
#define COLOR_PAY_CARD_BG 0x000045FF       // RGB(255, 69, 0) - Orange-red for PAY button
#define COLOR_TRIAL_WARNING 0x0000C8FF     // RGB(255, 200, 0) - Yellow for trial warning

class MonitorWidget {
public:
    MonitorWidget(DataManager* dm, HistoryManager* hm, HistoryWindow* hw);
    ~MonitorWidget();
    
    bool Create();
    void Update();
    HWND GetHandle() const { return hwnd_; }
    
    // Cumulative mode control
    void SetCumulativeMode(const std::wstring& fromDate);
    std::wstring GetCumulativeFromDate() const { return cumulativeFromDate_; }
    
    // License management
    void CheckLicenseStatus();
    void ShowPaymentWindow();

private:
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Contact popup
    static LRESULT CALLBACK StaticContactWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ContactWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ShowContactPopup();
    void HideContactPopup();
    void OnContactPaint(HWND hwnd);
    void OpenURL(const std::wstring& url);
    
    void OnPaint();
    void DrawCloseX(HDC hdc, int centerX, int centerY, int size, COLORREF color);  // NEW: X button drawer
    void DrawDownArrow(HDC hdc, int x, int y, int size, COLORREF color);
    void DrawUpArrow(HDC hdc, int x, int y, int size, COLORREF color);
    Gdiplus::Image* LoadPNGFromResource(int resourceID);
    void DrawIconFromResource(HDC hdc, int resourceID, int x, int y, int size);
    
    // Position persistence
    void SavePosition();
    void LoadPosition(int& x, int& y);
    
    // Cumulative mode persistence
    void SaveCumulativeMode();
    void LoadCumulativeMode();
    
    // Contact card visibility persistence
    void SaveContactCardVisibility();
    void LoadContactCardVisibility();

private:
    DataManager* dataManager_;
    HistoryManager* historyManager_;
    HistoryWindow* historyWindow_;
    PaymentWindow* paymentWindow_;
    
    HWND hwnd_;
    HWND contactPopupHwnd_;
    
    bool isHovered_;
    bool isHistoryHovered_;
    bool isContactHovered_;
    bool isContactCloseHovered_;
    bool contactCardVisible_;
    
    RECT historyCardRect_;
    RECT contactCardRect_;
    RECT contactCloseRect_;
    
    // Contact popup rects
    RECT twitterRect_;
    RECT whatsappRect_;
    RECT gmailRect_;
    int hoveredContactButton_;
    
    // License state
    bool isLicensed_;
    bool isTrialActive_;
    int trialDaysUsed_;
    int trialDaysRemaining_;
    
    // Cumulative mode
    std::wstring cumulativeFromDate_;
    
    // Drag and drop support
    bool isDragging_;
    POINT dragOffset_;
    
    static MonitorWidget* instance_;
};