#include "MonitorWidget.h"
#include "DataManager.h"
#include "HistoryManager.h"
#include "HistoryWindow.h"
#include "AutoStartManager.h"
#include "FontManager.h"
#include "LicenseManager.h"
#include <windows.h>
#include <gdiplus.h>
#include <shlobj.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Forward declarations
bool InitializeApplication();
void ShowFirstRunDialog();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Check if launched via auto-start
    bool isAutoStart = (strstr(lpCmdLine, "/autostart") != nullptr);
    
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Initialize custom font
    FontManager::Initialize();
    
    // ============================================
    // CENTRALIZED FIRST-RUN AND TRIAL SETUP
    // ============================================
    bool isFirstRun = false;
    HKEY hKey;
    
    // Check first run status
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD firstRun = 0;
        DWORD size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"FirstRunComplete", NULL, NULL, (LPBYTE)&firstRun, &size) != ERROR_SUCCESS) {
            isFirstRun = true;
        }
        RegCloseKey(hKey);
    } else {
        isFirstRun = true;
    }
    
    // Initialize on first run
    if (isFirstRun) {
        if (!InitializeApplication()) {
            if (!isAutoStart) {
                MessageBoxW(
                    NULL,
                    L"Failed to initialize NetView.\n\n"
                    L"Please ensure the application was installed correctly.",
                    L"Initialization Error",
                    MB_OK | MB_ICONERROR
                );
            }
            FontManager::Cleanup();
            GdiplusShutdown(gdiplusToken);
            return 1;
        }
        
        // Start trial - saves installation date
        LicenseManager::GetInstance().StartTrial();
        
        // Show welcome dialog (only if manually launched)
        if (!isAutoStart) {
            ShowFirstRunDialog();
        }
        
        // Mark first run complete
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, NULL, 
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD value = 1;
            RegSetValueExW(hKey, L"FirstRunComplete", 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }
    
    // ============================================
    // CHECK LICENSE/TRIAL STATUS
    // ============================================
    LicenseManager& licMgr = LicenseManager::GetInstance();
    bool isLicensed = licMgr.IsLicensed();
    bool isTrialActive = licMgr.IsTrialActive();
    
    // Handle expired trial
    if (!isLicensed && !isTrialActive) {
        if (isAutoStart) {
            // Auto-started with expired trial - exit silently
            FontManager::Cleanup();
            GdiplusShutdown(gdiplusToken);
            return 0;
        } else {
            // Manually launched - show message and payment option
            int result = MessageBoxW(
                NULL,
                L"Your 7-day trial has expired!\n\n"
                L"To continue using NetView, please purchase a lifetime license for $9.99.\n\n"
                L"Click 'OK' to view payment options, or 'Cancel' to exit.",
                L"NetView - Trial Expired",
                MB_OKCANCEL | MB_ICONWARNING
            );
            
            if (result == IDCANCEL) {
                FontManager::Cleanup();
                GdiplusShutdown(gdiplusToken);
                return 0;
            }
            // If OK, continue to show app with payment window
        }
    }
    
    // Show trial reminder (only if manually launched, not first run, and trial active)
    if (!isFirstRun && !isAutoStart && !isLicensed && isTrialActive) {
        int daysRemaining = licMgr.GetTrialDaysRemaining();
        wchar_t message[256];
        swprintf_s(message, 
            L"Welcome back to NetView!\n\n"
            L"Trial Period: %d of 7 days remaining\n\n"
            L"Get lifetime access for just $9.99!",
            daysRemaining);
        
        MessageBoxW(
            NULL,
            message,
            L"NetView - Trial",
            MB_OK | MB_ICONINFORMATION
        );
    }
    
    // ============================================
    // START APPLICATION
    // ============================================
    
    // Create data manager
    DataManager dataManager;
    
    // Start network monitoring
    if (!dataManager.StartMonitoring()) {
        if (!isAutoStart) {
            MessageBoxW(
                NULL,
                L"Failed to start network monitoring.\n\n"
                L"Please check your network adapters and try again.",
                L"Monitoring Error",
                MB_OK | MB_ICONERROR
            );
        }
        FontManager::Cleanup();
        GdiplusShutdown(gdiplusToken);
        return 1;
    }
    
    // Create history manager
    HistoryManager historyManager;
    
    // Create history window
    HistoryWindow historyWindow(&historyManager);
    if (!historyWindow.Create()) {
        if (!isAutoStart) {
            MessageBoxW(
                NULL,
                L"Failed to create history window.",
                L"Window Error",
                MB_OK | MB_ICONWARNING
            );
        }
        // Continue anyway - history window is optional
    }
    
    // Create monitor widget
    MonitorWidget widget(&dataManager, &historyManager, &historyWindow);
    if (!widget.Create()) {
        if (!isAutoStart) {
            MessageBoxW(
                NULL,
                L"Failed to create monitor widget.",
                L"Widget Error",
                MB_OK | MB_ICONERROR
            );
        }
        dataManager.StopMonitoring();
        FontManager::Cleanup();
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    historyWindow.SetMonitorWidget(&widget);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    dataManager.StopMonitoring();
    FontManager::Cleanup();
    GdiplusShutdown(gdiplusToken);
    
    return 0;
}

bool InitializeApplication() {
    // Ensure auto-start is configured
    AutoStartManager autoStart;
    if (!autoStart.IsRegistered()) {
        if (!autoStart.Register()) {
            // Not critical, log but continue
        }
    }
    
    // Ensure application directory exists
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK) {
        std::wstring path = appDataPath;
        path += L"\\NetView";
        CreateDirectoryW(path.c_str(), NULL);
    }
    
    return true;
}

void ShowFirstRunDialog() {
    MessageBoxW(
        NULL,
        L"Welcome to NetView!\n\n"
        L"You have a 7-day FREE trial to explore all features.\n\n"
        L"NetView monitors your network activity and displays:\n"
        L"  • Real-time download/upload statistics\n"
        L"  • Daily, monthly, and yearly usage\n"
        L"  • Persistent history tracking\n\n"
        L"After 7 days, purchase lifetime access for just $9.99!\n\n"
        L"NetView will start automatically on system boot.",
        L"NetView - Welcome (7-Day Trial)",
        MB_OK | MB_ICONINFORMATION
    );
}
