#pragma once
#include <windows.h>
#include <string>

class LicenseManager {
public:
    static LicenseManager& GetInstance();
    
    // Trial management
    bool IsTrialActive();
    int GetTrialDaysRemaining();
    int GetTrialDaysUsed();
    void StartTrial();
    
    // License management
    bool IsLicensed();
    void ActivateLicense();
    
    // Installation tracking
    std::wstring GetInstallationKey();
    
private:
    LicenseManager();
    ~LicenseManager();
    
    void InitializeInstallation();
    std::wstring GenerateInstallationKey();
    SYSTEMTIME GetInstallationDate();
    void SaveInstallationDate(const SYSTEMTIME& st);
    int CalculateDaysSince(const SYSTEMTIME& past);
    
    static LicenseManager* instance_;
    std::wstring installationKey_;
};
