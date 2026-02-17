#pragma once

// CRITICAL: Include order matters for Windows networking!
// winsock2.h MUST come before windows.h
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tcpmib.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <tlhelp32.h>
#include <fstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Process information structure
struct ProcessInfo {
    std::wstring name;
    DWORD pid;
    bool isBlocked;
    bool isActive;
    
    ProcessInfo() : pid(0), isBlocked(false), isActive(false) {}
    ProcessInfo(const std::wstring& n, DWORD p, bool blocked, bool active)
        : name(n), pid(p), isBlocked(blocked), isActive(active) {}
};

// Thread-safe process manager (Chrome-style architecture)
class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();
    
    // Lifecycle
    void StartMonitoring();
    void StopMonitoring();
    
    // Active process count for UI
    int GetActiveProcessCount() const;
    
    // Process list (thread-safe copy)
    std::vector<ProcessInfo> GetAllProcesses() const;
    
    // Block/Unblock operations
    void BlockProcess(const std::wstring& processName);
    void UnblockProcess(const std::wstring& processName);
    bool IsProcessBlocked(const std::wstring& processName) const;
    
private:
    // Background monitoring thread (runs every 5 seconds)
    void MonitorThread();
    
    // Core operations
    void ScanActiveProcesses();
    void EnforceBlocking();
    
    // Process utilities
    std::set<DWORD> GetProcessIdsByName(const std::wstring& processName);
    void TerminateNetworkConnections(DWORD pid);
    
    // Persistence
    void SaveBlockedProcesses();
    void LoadBlockedProcesses();
    std::wstring GetConfigFilePath();
    
private:
    // Thread synchronization
    mutable std::mutex mutex_;
    std::atomic<bool> shouldStop_;
    std::thread monitorThread_;
    
    // Active processes map: processName -> ProcessInfo
    std::map<std::wstring, ProcessInfo> activeProcesses_;
    
    // Blocked process names (persistent)
    std::set<std::wstring> blockedProcessNames_;
    
    // Currently blocked PIDs (runtime tracking)
    std::set<DWORD> blockedPids_;
    
    // Configuration
    static constexpr int SCAN_INTERVAL_MS = 5000; // 5 seconds
    static constexpr const wchar_t* CONFIG_FILENAME = L"blocked_processes.cfg";
};