#pragma once
#include <windows.h>
#include "HistoryManager.h"

// Forward declare GDI+ types
namespace Gdiplus {
    class Image;
}

#define WND_CLASS_HISTORY L"NetViewHistoryWindow"
#define HISTORY_WIDTH 700
#define HISTORY_HEIGHT 600
#define TIMER_HISTORY_REFRESH 2001

// Forward declaration
class MonitorWidget;

// Display row types
enum class RowType {
    DAILY,
    MONTHLY_TOTAL,
    YEARLY_TOTAL
};

struct DisplayRow {
    RowType type;
    std::wstring text1;  // Date or label
    std::wstring text2;  // Download or empty
    std::wstring text3;  // Upload or empty
    std::wstring text4;  // Total
    std::wstring date;   // Actual date (for toggling)
    
    DisplayRow(RowType t, const std::wstring& t1, const std::wstring& t2,
               const std::wstring& t3, const std::wstring& t4, const std::wstring& d = L"")
        : type(t), text1(t1), text2(t2), text3(t3), text4(t4), date(d) {}
};

struct SummaryStats {
    UINT64 todayTotal;
    UINT64 last7DaysTotal;
    UINT64 thisMonthTotal;
    UINT64 lastMonthTotal;
};

class HistoryWindow {
public:
    HistoryWindow(HistoryManager* historyMgr);
    ~HistoryWindow();
    
    bool Create();
    void Show();
    void Hide();
    bool IsVisible() const;
    
    void SetMonitorWidget(MonitorWidget* widget) { monitorWidget_ = widget; }
    
    // Restore toggle state from MonitorWidget's saved cumulative mode
    void RestoreToggleState(const std::wstring& toggledDate) { toggledDate_ = toggledDate; }

private:
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void OnPaint();
    void DrawTitleBar(HDC hdc, const RECT& clientRect);
    void DrawContent(HDC hdc, const RECT& clientRect);
    void DrawToggleSwitch(HDC hdc, const RECT& toggleRect, bool isToggled);
    void DrawSummaryCards(HDC hdc, const RECT& clientRect);
    
    // Generate display rows with monthly/yearly totals inserted
    std::vector<DisplayRow> GenerateDisplayRows();
    
    // Calculate monthly and yearly totals
    void CalculateTotals(const std::vector<DailyUsage>& daily,
                        std::map<std::wstring, std::pair<UINT64, UINT64>>& monthlyTotals,
                        std::map<std::wstring, std::pair<UINT64, UINT64>>& yearlyTotals);
    
    // Calculate summary statistics for the cards
    void CalculateSummaryStats(SummaryStats& stats);
    
    // Toggle handling
    void HandleToggleClick(int x, int y);
    RECT GetToggleRect(int rowIndex, int rowY);
    
    // Social media
    void OpenURL(const std::wstring& url);
    Gdiplus::Image* LoadPNGFromResource(int resourceID);

private:
    HistoryManager* historyManager_;
    MonitorWidget* monitorWidget_;
    HWND hwnd_;
    
    int scrollOffset_;
    int maxScrollOffset_;
    
    // Toggle state
    std::wstring toggledDate_;  // Empty = no toggle, otherwise the date that's toggled
    
    // Social media icon rectangles
    RECT twitterIconRect_;
    RECT whatsappIconRect_;
    RECT gmailIconRect_;
    int hoveredSocialIcon_;  // -1 = none, 0 = twitter, 1 = whatsapp, 2 = gmail
    
    static HistoryWindow* instance_;
};
