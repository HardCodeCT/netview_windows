#pragma once
#include <windows.h>
#include <string>

// Manages application auto-start registration
class AutoStartManager {
public:
    AutoStartManager() = default;
    ~AutoStartManager() = default;
    
    // Register app to start on system boot
    bool Register();
    
    // Unregister app from auto-start
    bool Unregister();
    
    // Check if app is registered
    bool IsRegistered();
    
private:
    std::wstring GetExecutablePath();
    
    static constexpr const wchar_t* REGISTRY_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    static constexpr const wchar_t* APP_NAME = L"NetView";
};
