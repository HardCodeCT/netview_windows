#include "LicenseManager.h"
#include <sstream>
#include <iomanip>
#include <random>

LicenseManager* LicenseManager::instance_ = nullptr;

LicenseManager& LicenseManager::GetInstance() {
    if (!instance_) {
        instance_ = new LicenseManager();
    }
    return *instance_;
}

LicenseManager::LicenseManager() {
    InitializeInstallation();
}

LicenseManager::~LicenseManager() {
}

void LicenseManager::InitializeInstallation() {
    HKEY hKey;
    
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        // Check if installation key exists
        wchar_t keyBuffer[256] = {0};
        DWORD bufferSize = sizeof(keyBuffer);
        
        if (RegQueryValueExW(hKey, L"InstallationKey", NULL, NULL, 
            (LPBYTE)keyBuffer, &bufferSize) != ERROR_SUCCESS) {
            // Generate new installation key
            installationKey_ = GenerateInstallationKey();
            RegSetValueExW(hKey, L"InstallationKey", 0, REG_SZ,
                (LPBYTE)installationKey_.c_str(),
                (installationKey_.length() + 1) * sizeof(wchar_t));
        } else {
            installationKey_ = keyBuffer;
        }
        
        RegCloseKey(hKey);
    }
}

std::wstring LicenseManager::GenerateInstallationKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const wchar_t* hex = L"0123456789ABCDEF";
    std::wstring key;
    
    for (int i = 0; i < 32; i++) {
        if (i > 0 && i % 8 == 0) {
            key += L'-';
        }
        key += hex[dis(gen)];
    }
    
    return key;
}

std::wstring LicenseManager::GetInstallationKey() {
    return installationKey_;
}

void LicenseManager::SaveInstallationDate(const SYSTEMTIME& st) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        // Check if InstallDate already exists - DON'T overwrite it!
        DWORD existingDate = 0;
        DWORD size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"InstallDate", NULL, NULL,
            (LPBYTE)&existingDate, &size) == ERROR_SUCCESS && existingDate != 0) {
            // Already exists - don't overwrite!
            RegCloseKey(hKey);
            return;
        }
        
        // Doesn't exist - safe to create
        DWORD installDate = (st.wYear << 16) | (st.wMonth << 8) | st.wDay;
        RegSetValueExW(hKey, L"InstallDate", 0, REG_DWORD,
            (LPBYTE)&installDate, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
}

SYSTEMTIME LicenseManager::GetInstallationDate() {
    SYSTEMTIME st = {0};
    HKEY hKey;
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD installDate = 0;
        DWORD size = sizeof(DWORD);
        
        if (RegQueryValueExW(hKey, L"InstallDate", NULL, NULL,
            (LPBYTE)&installDate, &size) == ERROR_SUCCESS && installDate != 0) {
            
            st.wYear = (installDate >> 16) & 0xFFFF;
            st.wMonth = (installDate >> 8) & 0xFF;
            st.wDay = installDate & 0xFF;
        }
        
        RegCloseKey(hKey);
    }
    
    // If failed to read or date is invalid, return empty struct
    // Main.cpp is responsible for creating the date on first run
    return st;
}

int LicenseManager::CalculateDaysSince(const SYSTEMTIME& past) {
    // If past date is invalid (all zeros), return 0
    if (past.wYear == 0 || past.wMonth == 0 || past.wDay == 0) {
        return 0;
    }
    
    SYSTEMTIME now;
    GetLocalTime(&now);
    
    FILETIME ftPast, ftNow;
    SystemTimeToFileTime(&past, &ftPast);
    SystemTimeToFileTime(&now, &ftNow);
    
    ULARGE_INTEGER uliPast, uliNow;
    uliPast.LowPart = ftPast.dwLowDateTime;
    uliPast.HighPart = ftPast.dwHighDateTime;
    uliNow.LowPart = ftNow.dwLowDateTime;
    uliNow.HighPart = ftNow.dwHighDateTime;
    
    // Check for invalid dates (future dates or corrupted data)
    if (uliNow.QuadPart < uliPast.QuadPart) {
        return 0;
    }
    
    ULONGLONG diff = uliNow.QuadPart - uliPast.QuadPart;
    
    // Convert from 100-nanosecond intervals to days
    int days = (int)(diff / 10000000ULL / 60 / 60 / 24);
    
    return days;
}

bool LicenseManager::IsLicensed() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD licensed = 0;
        DWORD size = sizeof(DWORD);
        
        if (RegQueryValueExW(hKey, L"Licensed", NULL, NULL,
            (LPBYTE)&licensed, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return licensed == 1;
        }
        
        RegCloseKey(hKey);
    }
    return false;
}

void LicenseManager::ActivateLicense() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NetView", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        DWORD value = 1;
        RegSetValueExW(hKey, L"Licensed", 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
}

void LicenseManager::StartTrial() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    SaveInstallationDate(st);
}

bool LicenseManager::IsTrialActive() {
    if (IsLicensed()) {
        return true; // Licensed users have unlimited access
    }
    
    SYSTEMTIME installDate = GetInstallationDate();
    
    // If install date is invalid, trial is not active
    if (installDate.wYear == 0 || installDate.wMonth == 0 || installDate.wDay == 0) {
        return false;
    }
    
    int daysUsed = CalculateDaysSince(installDate);
    
    return daysUsed < 7;
}

int LicenseManager::GetTrialDaysUsed() {
    if (IsLicensed()) {
        return 0; // Licensed users don't count trial days
    }
    
    SYSTEMTIME installDate = GetInstallationDate();
    
    // If install date is invalid, return 0
    if (installDate.wYear == 0 || installDate.wMonth == 0 || installDate.wDay == 0) {
        return 0;
    }
    
    int days = CalculateDaysSince(installDate);
    
    return days < 7 ? days : 7;
}

int LicenseManager::GetTrialDaysRemaining() {
    if (IsLicensed()) {
        return 999; // Unlimited for licensed users
    }
    
    int daysUsed = GetTrialDaysUsed();
    int remaining = 7 - daysUsed;
    
    return remaining > 0 ? remaining : 0;
}
