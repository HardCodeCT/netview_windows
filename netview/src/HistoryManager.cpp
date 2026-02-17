#include "HistoryManager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <shlobj.h>  // For getting AppData folder

HistoryManager::HistoryManager()
    : todayDownload_(0)
    , todayUpload_(0)
    , lastSavedDownload_(0)
    , lastSavedUpload_(0)
    , lastSaveTime_(0)
    , sessionBaseDownload_(0)
    , sessionBaseUpload_(0) {
    todayDate_ = GetTodayDate();
    LoadHistory();
}

HistoryManager::~HistoryManager() {
    SaveTodayUsageSafe();  // Use safe version on exit
}

std::wstring HistoryManager::GetTodayDate() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    wchar_t buffer[32];
    swprintf_s(buffer, 32, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return std::wstring(buffer);
}

std::wstring HistoryManager::GetCurrentMonth() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    wchar_t buffer[32];
    swprintf_s(buffer, 32, L"%04d-%02d", st.wYear, st.wMonth);
    return std::wstring(buffer);
}

std::wstring HistoryManager::GetCurrentYear() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    wchar_t buffer[32];
    swprintf_s(buffer, 32, L"%04d", st.wYear);
    return std::wstring(buffer);
}

void HistoryManager::ParseDate(const std::wstring& date, int& year, int& month, int& day) {
    year = month = day = 0;
    if (date.length() >= 10) {
        year = _wtoi(date.substr(0, 4).c_str());
        month = _wtoi(date.substr(5, 2).c_str());
        day = _wtoi(date.substr(8, 2).c_str());
    }
}

std::wstring HistoryManager::GetHistoryFilePath() {
    // Get AppData folder
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring path = appDataPath;
        path += L"\\NetView";
        
        // Create directory if it doesn't exist
        CreateDirectoryW(path.c_str(), NULL);
        
        path += L"\\history.txt";
        return path;
    }
    
    // Fallback to current directory
    return L"netview_history.txt";
}

void HistoryManager::LoadHistory() {
    std::wstring filePath = GetHistoryFilePath();
    std::wifstream file(filePath);
    
    if (!file.is_open()) {
        // File doesn't exist yet (first run)
        wchar_t msg[512];
        swprintf_s(msg, 512, L"[HistoryManager] No history file found at: %s (first run)\n", filePath.c_str());
        OutputDebugStringW(msg);
        return;
    }
    
    std::wstring line;
    int lineCount = 0;
    
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        // Parse CSV format: DATE,DOWNLOAD,UPLOAD
        size_t firstComma = line.find(L',');
        size_t secondComma = line.find(L',', firstComma + 1);
        
        if (firstComma != std::wstring::npos && secondComma != std::wstring::npos) {
            std::wstring date = line.substr(0, firstComma);
            std::wstring downloadStr = line.substr(firstComma + 1, secondComma - firstComma - 1);
            std::wstring uploadStr = line.substr(secondComma + 1);
            
            UINT64 download = _wcstoui64(downloadStr.c_str(), NULL, 10);
            UINT64 upload = _wcstoui64(uploadStr.c_str(), NULL, 10);
            
            // Store in memory
            dailyHistory_[date].first = download;
            dailyHistory_[date].second = upload;
            
            // If this is today's data, restore it
            if (date == todayDate_) {
                todayDownload_ = download;
                todayUpload_ = upload;
                lastSavedDownload_ = download;
                lastSavedUpload_ = upload;
                
                // Store as session baseline
                sessionBaseDownload_ = download;
                sessionBaseUpload_ = upload;
                
                wchar_t msg[256];
                swprintf_s(msg, 256, L"[HistoryManager] Restored today's data: %llu down, %llu up\n", 
                          download, upload);
                OutputDebugStringW(msg);
            }
            
            lineCount++;
        }
    }
    
    file.close();
    
    // Debug output
    wchar_t msg[256];
    swprintf_s(msg, 256, L"[HistoryManager] Loaded %d records from file\n", lineCount);
    OutputDebugStringW(msg);
}

void HistoryManager::SaveTodayUsageSafe() {
    // Only save if usage has changed
    if (todayDownload_ == lastSavedDownload_ && todayUpload_ == lastSavedUpload_) {
        return;
    }
    
    // Update in-memory map
    dailyHistory_[todayDate_].first = todayDownload_;
    dailyHistory_[todayDate_].second = todayUpload_;
    
    // Get file paths
    std::wstring filePath = GetHistoryFilePath();
    std::wstring tempPath = filePath + L".tmp";
    
    // STEP 1: Write to temporary file first (atomic write protection)
    std::wofstream file(tempPath);
    
    if (!file.is_open()) {
        wchar_t msg[512];
        swprintf_s(msg, 512, L"[HistoryManager] ERROR: Could not open temp file for writing: %s\n", tempPath.c_str());
        OutputDebugStringW(msg);
        return;
    }
    
    // Write all records sorted by date (oldest first for easier reading)
    std::vector<std::wstring> dates;
    for (const auto& entry : dailyHistory_) {
        dates.push_back(entry.first);
    }
    std::sort(dates.begin(), dates.end());
    
    for (const auto& date : dates) {
        const auto& data = dailyHistory_[date];
        file << date << L"," << data.first << L"," << data.second << L"\n";
    }
    
    // STEP 2: Flush the stream buffer
    file.flush();
    file.close();
    
    // STEP 3: Force Windows to commit the file to disk (CRITICAL for crash resistance)
    HANDLE hFile = CreateFileW(
        tempPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(hFile);  // This forces the OS to write to physical disk
        CloseHandle(hFile);
    }
    
    // STEP 4: Atomically replace the old file with the new file
    // This prevents corruption if a crash happens during the write
    if (!MoveFileExW(tempPath.c_str(), filePath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        wchar_t msg[512];
        swprintf_s(msg, 512, L"[HistoryManager] ERROR: Could not replace file (error %d)\n", GetLastError());
        OutputDebugStringW(msg);
        DeleteFileW(tempPath.c_str());  // Clean up temp file
        return;
    }
    
    // Update saved state
    lastSavedDownload_ = todayDownload_;
    lastSavedUpload_ = todayUpload_;
    lastSaveTime_ = GetTickCount();
    
    // Debug output
    wchar_t msg[512];
    swprintf_s(msg, 512, L"[HistoryManager] SAFE SAVE: %zu records to: %s (Today: %llu down, %llu up)\n", 
              dailyHistory_.size(), filePath.c_str(), todayDownload_, todayUpload_);
    OutputDebugStringW(msg);
}

void HistoryManager::SaveTodayUsage() {
    // Use the safe version for all saves
    SaveTodayUsageSafe();
}

void HistoryManager::UpdateTodayUsage(UINT64 sessionDownload, UINT64 sessionUpload) {
    // Check if date has changed (new day started)
    std::wstring currentDate = GetTodayDate();
    if (currentDate != todayDate_) {
        // Save previous day's final data
        SaveTodayUsageSafe();
        
        // Reset for new day
        todayDate_ = currentDate;
        todayDownload_ = 0;
        todayUpload_ = 0;
        lastSavedDownload_ = 0;
        lastSavedUpload_ = 0;
        lastSaveTime_ = GetTickCount();
        
        // Reset session baseline
        sessionBaseDownload_ = 0;
        sessionBaseUpload_ = 0;
        
        OutputDebugStringW(L"[HistoryManager] New day started, reset counters\n");
    }
    
    // ADD session data to baseline (don't overwrite!)
    todayDownload_ = sessionBaseDownload_ + sessionDownload;
    todayUpload_ = sessionBaseUpload_ + sessionUpload;
    
    // Also update in-memory map - CRITICAL for cumulative mode
    dailyHistory_[todayDate_].first = todayDownload_;
    dailyHistory_[todayDate_].second = todayUpload_;
    
    // Auto-save every 2 seconds if there's new data
    DWORD currentTime = GetTickCount();
    UINT64 totalChange = (todayDownload_ + todayUpload_) - (lastSavedDownload_ + lastSavedUpload_);
    
    // Save if: (1) data changed AND (2) 2 seconds passed
    // This handles network reconnects gracefully
    if (totalChange > 0 && (currentTime - lastSaveTime_) >= SAVE_INTERVAL_MS) {
        SaveTodayUsageSafe();
    }
}

std::vector<DailyUsage> HistoryManager::GetAllDailyRecords() {
    std::vector<DailyUsage> result;
    
    // Convert map to vector
    for (const auto& entry : dailyHistory_) {
        result.push_back(DailyUsage(
            entry.first,                    // date
            entry.second.first,             // download
            entry.second.second             // upload
        ));
    }
    
    // Sort by date descending (newest first)
    std::sort(result.begin(), result.end(), 
              [](const DailyUsage& a, const DailyUsage& b) {
                  return a.date > b.date;  // Descending order
              });
    
    return result;
}

UINT64 HistoryManager::GetTodayTotal() const {
    return todayDownload_ + todayUpload_;
}

UINT64 HistoryManager::GetThisMonthTotal() const {
    std::wstring currentMonth = const_cast<HistoryManager*>(this)->GetCurrentMonth();
    UINT64 total = 0;
    
    for (const auto& entry : dailyHistory_) {
        if (entry.first.length() >= 7 && entry.first.substr(0, 7) == currentMonth) {
            total += entry.second.first + entry.second.second;
        }
    }
    
    return total;
}

UINT64 HistoryManager::GetThisYearTotal() const {
    std::wstring currentYear = const_cast<HistoryManager*>(this)->GetCurrentYear();
    UINT64 total = 0;
    
    for (const auto& entry : dailyHistory_) {
        if (entry.first.length() >= 4 && entry.first.substr(0, 4) == currentYear) {
            total += entry.second.first + entry.second.second;
        }
    }
    
    return total;
}

UINT64 HistoryManager::GetAllTimeTotal() const {
    UINT64 total = 0;
    for (const auto& entry : dailyHistory_) {
        total += entry.second.first + entry.second.second;
    }
    return total;
}

std::pair<UINT64, UINT64> HistoryManager::GetCumulativeTotalsFrom(const std::wstring& fromDate) {
    UINT64 totalDownload = 0;
    UINT64 totalUpload = 0;
    
    // Include all dates >= fromDate (including today)
    // The dailyHistory_ map is kept up-to-date with today's live values in UpdateTodayUsage()
    for (const auto& entry : dailyHistory_) {
        if (entry.first >= fromDate) {
            totalDownload += entry.second.first;
            totalUpload += entry.second.second;
        }
    }
    
    return std::make_pair(totalDownload, totalUpload);
}

std::wstring HistoryManager::FormatBytes(UINT64 bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unitIndex = 0;
    double value = static_cast<double>(bytes);
    
    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        unitIndex++;
    }
    
    std::wostringstream oss;
    oss.precision(2);
    oss << std::fixed;
    
    if (unitIndex == 0) {
        oss << bytes << L" " << units[unitIndex];
    } else {
        oss << value << L" " << units[unitIndex];
    }
    
    return oss.str();
}
