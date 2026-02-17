#pragma once

#include <windows.h>
#include <string>
#include "ProcessManager.h"

#define WND_CLASS_PROCESSLIST L"NetViewProcessListWindow"
#define BTN_ID_ACTIVE_BASE 2000
#define BTN_ID_BLOCK_BASE 3000
#define BTN_ID_CLOSE 4000

// BLACK theme colors for Process Control Window
#define COLOR_WINDOW_BG RGB(0, 0, 0)            // Pure black background
#define COLOR_CARD_BG RGB(20, 20, 20)           // Dark card background
#define COLOR_CARD_BORDER RGB(60, 60, 60)       // Grey border
#define COLOR_TEXT_WHITE RGB(255, 255, 255)     // Pure white text
#define COLOR_TEXT_GRAY RGB(180, 180, 180)      // Light grey text
#define COLOR_BTN_ACTIVE RGB(33, 150, 243)      // Blue for active
#define COLOR_BTN_INACTIVE RGB(100, 100, 100)   // Grey for inactive
#define COLOR_BTN_BLOCKED RGB(244, 67, 54)      // Red for blocked
#define COLOR_BTN_HOVER RGB(45, 165, 255)       // Hover effect

class ProcessListWindow {
public:
    ProcessListWindow(ProcessManager* pm);
    ~ProcessListWindow();
    
    bool Create();
    void Show();
    void Hide();
    void Refresh();
    
    HWND GetHandle() const { return hwnd_; }
    
private:
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void OnPaint();
    void OnCommand(WPARAM wParam);
    void CreateControls();
    void DestroyControls();
    
private:
    ProcessManager* processManager_;
    HWND hwnd_;
    
    std::vector<HWND> processLabels_;
    std::vector<HWND> pidLabels_;
    std::vector<HWND> activeButtons_;
    std::vector<HWND> blockButtons_;
    HWND closeButton_;
    
    static ProcessListWindow* instance_;
};
