#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>

struct DailyUsage {
    std::wstring date;      // Format: YYYY-MM-DD
    UINT64 downloadBytes;   // Download only
    UINT64 uploadBytes;     // Upload only
    UINT64 totalBytes;      // Combined

    DailyUsage() : downloadBytes(0), uploadBytes(0), totalBytes(0) {}
    DailyUsage(const std::wstring& d, UINT64 down, UINT64 up) 
        : date(d), downloadBytes(down), uploadBytes(up), totalBytes(down + up) {}
};

struct MonthlyUsage {
    std::wstring month;     // Format: YYYY-MM
    UINT64 downloadBytes;
    UINT64 uploadBytes;
    UINT64 totalBytes;

    MonthlyUsage() : downloadBytes(0), uploadBytes(0), totalBytes(0) {}
    MonthlyUsage(const std::wstring& m, UINT64 down, UINT64 up) 
        : month(m), downloadBytes(down), uploadBytes(up), totalBytes(down + up) {}
};

struct YearlyUsage {
    std::wstring year;      // Format: YYYY
    UINT64 downloadBytes;
    UINT64 uploadBytes;
    UINT64 totalBytes;

    YearlyUsage() : downloadBytes(0), uploadBytes(0), totalBytes(0) {}
    YearlyUsage(const std::wstring& y, UINT64 down, UINT64 up) 
        : year(y), downloadBytes(down), uploadBytes(up), totalBytes(down + up) {}
};

class HistoryManager {
public:
    HistoryManager();
    ~HistoryManager();

    // Update today's usage (called every 500ms by MonitorWidget)
    void UpdateTodayUsage(UINT64 totalDownload, UINT64 totalUpload);

    // Save today's usage (now public so MonitorWidget can call it)
    void SaveTodayUsage();

    // Get all daily records (sorted by date descending - newest first)
    std::vector<DailyUsage> GetAllDailyRecords();

    // Get totals
    UINT64 GetTodayTotal() const;
    UINT64 GetThisMonthTotal() const;
    UINT64 GetThisYearTotal() const;
    UINT64 GetAllTimeTotal() const;

    // Get today's individual values (needed for cumulative mode)
    UINT64 GetTodayDownload() const { return todayDownload_; }
    UINT64 GetTodayUpload() const { return todayUpload_; }

    // Utility
    std::wstring FormatBytes(UINT64 bytes);
    std::pair<UINT64, UINT64> GetCumulativeTotalsFrom(const std::wstring& fromDate);

private:
    void LoadHistory();
    void SaveTodayUsageSafe();  // Atomic save with flush (internal use)
    std::wstring GetTodayDate();
    std::wstring GetCurrentMonth();
    std::wstring GetCurrentYear();
    std::wstring GetHistoryFilePath();

    // Parse date string (YYYY-MM-DD) into components
    void ParseDate(const std::wstring& date, int& year, int& month, int& day);

private:
    // Store download and upload separately
    std::map<std::wstring, std::pair<UINT64, UINT64>> dailyHistory_;  // date -> (download, upload)
    std::wstring todayDate_;
    UINT64 todayDownload_;
    UINT64 todayUpload_;
    UINT64 lastSavedDownload_;
    UINT64 lastSavedUpload_;

    // Session baseline (values restored from file)
    UINT64 sessionBaseDownload_;
    UINT64 sessionBaseUpload_;

    // Time-based auto-save - 2 seconds for crash resistance
    DWORD lastSaveTime_;
    static const DWORD SAVE_INTERVAL_MS = 2000;  // Save every 2 seconds
};
