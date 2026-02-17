#include "AutoStartManager.h"

std::wstring AutoStartManager::GetExecutablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

bool AutoStartManager::Register() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_WRITE,
        &hKey
    );
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    std::wstring exePath = GetExecutablePath();
    
    // Add quotes around path AND auto-start flag to handle spaces in path
    std::wstring quotedPath = L"\"" + exePath + L"\" /autostart";
    
    result = RegSetValueExW(
        hKey,
        APP_NAME,
        0,
        REG_SZ,
        (BYTE*)quotedPath.c_str(),
        (DWORD)((quotedPath.length() + 1) * sizeof(wchar_t))
    );
    
    RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS;
}

bool AutoStartManager::Unregister() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_WRITE,
        &hKey
    );
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    result = RegDeleteValueW(hKey, APP_NAME);
    
    RegCloseKey(hKey);
    
    // ERROR_FILE_NOT_FOUND means it wasn't registered, which is fine
    return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

bool AutoStartManager::IsRegistered() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_READ,
        &hKey
    );
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    wchar_t value[MAX_PATH];
    DWORD size = sizeof(value);
    DWORD type;
    
    result = RegQueryValueExW(
        hKey,
        APP_NAME,
        NULL,
        &type,
        (LPBYTE)value,
        &size
    );
    
    RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS && type == REG_SZ;
}
