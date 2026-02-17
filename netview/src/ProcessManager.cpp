// CRITICAL: Include order matters for Windows networking!
// winsock2.h MUST come before windows.h
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "ProcessManager.h"
#include <algorithm>
#include <shlobj.h>

ProcessManager::ProcessManager() 
    : shouldStop_(true) {
    LoadBlockedProcesses();
}

ProcessManager::~ProcessManager() {
    StopMonitoring();
    SaveBlockedProcesses();
}

void ProcessManager::StartMonitoring() {
    if (!shouldStop_.load()) {
        return; // Already running
    }
    
    shouldStop_.store(false);
    monitorThread_ = std::thread(&ProcessManager::MonitorThread, this);
}

void ProcessManager::StopMonitoring() {
    shouldStop_.store(true);
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}

int ProcessManager::GetActiveProcessCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (const auto& pair : activeProcesses_) {
        if (pair.second.isActive) {
            count++;
        }
    }
    return count;
}

std::vector<ProcessInfo> ProcessManager::GetAllProcesses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessInfo> result;
    result.reserve(activeProcesses_.size());
    
    for (const auto& pair : activeProcesses_) {
        result.push_back(pair.second);
    }
    
    return result;
}

void ProcessManager::BlockProcess(const std::wstring& processName) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        blockedProcessNames_.insert(processName);
        
        // Update existing process info
        auto it = activeProcesses_.find(processName);
        if (it != activeProcesses_.end()) {
            it->second.isBlocked = true;
        }
    }
    
    SaveBlockedProcesses();
    
    // Immediately enforce blocking
    EnforceBlocking();
}

void ProcessManager::UnblockProcess(const std::wstring& processName) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        blockedProcessNames_.erase(processName);
        
        // Update existing process info
        auto it = activeProcesses_.find(processName);
        if (it != activeProcesses_.end()) {
            it->second.isBlocked = false;
        }
        
        // Remove from blocked PIDs
        std::set<DWORD> pidsToRemove;
        for (DWORD pid : blockedPids_) {
            auto procIt = activeProcesses_.find(processName);
            if (procIt != activeProcesses_.end() && procIt->second.pid == pid) {
                pidsToRemove.insert(pid);
            }
        }
        
        for (DWORD pid : pidsToRemove) {
            blockedPids_.erase(pid);
        }
    }
    
    SaveBlockedProcesses();
}

bool ProcessManager::IsProcessBlocked(const std::wstring& processName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blockedProcessNames_.find(processName) != blockedProcessNames_.end();
}

void ProcessManager::MonitorThread() {
    while (!shouldStop_.load()) {
        ScanActiveProcesses();
        EnforceBlocking();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(SCAN_INTERVAL_MS));
    }
}

void ProcessManager::ScanActiveProcesses() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Mark all as inactive first
    for (auto& pair : activeProcesses_) {
        pair.second.isActive = false;
    }
    
    // Get TCP connections
    PMIB_TCPTABLE_OWNER_PID pTcpTable = nullptr;
    DWORD dwSize = 0;
    
    // Get required buffer size
    GetExtendedTcpTable(nullptr, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    
    pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(dwSize);
    if (!pTcpTable) return;
    
    if (GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, 
                            TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        
        std::set<DWORD> activePids;
        
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            DWORD pid = pTcpTable->table[i].dwOwningPid;
            if (pid > 0) {
                activePids.insert(pid);
            }
        }
        
        // Get process names for active PIDs
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32W);
            
            if (Process32FirstW(hSnapshot, &pe32)) {
                do {
                    if (activePids.find(pe32.th32ProcessID) != activePids.end()) {
                        std::wstring processName = pe32.szExeFile;
                        
                        auto it = activeProcesses_.find(processName);
                        if (it == activeProcesses_.end()) {
                            // New process detected
                            bool isBlocked = blockedProcessNames_.find(processName) != 
                                           blockedProcessNames_.end();
                            
                            ProcessInfo info(processName, pe32.th32ProcessID, isBlocked, true);
                            activeProcesses_[processName] = info;
                        } else {
                            // Update existing
                            it->second.isActive = true;
                            it->second.pid = pe32.th32ProcessID;
                        }
                    }
                } while (Process32NextW(hSnapshot, &pe32));
            }
            
            CloseHandle(hSnapshot);
        }
    }
    
    free(pTcpTable);
}

void ProcessManager::EnforceBlocking() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const std::wstring& blockedName : blockedProcessNames_) {
        std::set<DWORD> pids = GetProcessIdsByName(blockedName);
        
        for (DWORD pid : pids) {
            if (blockedPids_.find(pid) == blockedPids_.end()) {
                // New instance of blocked process
                TerminateNetworkConnections(pid);
                blockedPids_.insert(pid);
            }
        }
    }
    
    // Clean up dead PIDs
    std::set<DWORD> deadPids;
    for (DWORD pid : blockedPids_) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) {
            deadPids.insert(pid);
        } else {
            DWORD exitCode;
            if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                deadPids.insert(pid);
            }
            CloseHandle(hProcess);
        }
    }
    
    for (DWORD pid : deadPids) {
        blockedPids_.erase(pid);
    }
}

std::set<DWORD> ProcessManager::GetProcessIdsByName(const std::wstring& processName) {
    std::set<DWORD> pids;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                pids.insert(pe32.th32ProcessID);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return pids;
}

void ProcessManager::TerminateNetworkConnections(DWORD pid) {
    // Get TCP connections for this PID
    PMIB_TCPTABLE_OWNER_PID pTcpTable = nullptr;
    DWORD dwSize = 0;
    
    GetExtendedTcpTable(nullptr, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(dwSize);
    
    if (!pTcpTable) return;
    
    if (GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, 
                            TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            if (pTcpTable->table[i].dwOwningPid == pid) {
                // Set connection to delete state
                MIB_TCPROW row;
                row.dwState = MIB_TCP_STATE_DELETE_TCB;
                row.dwLocalAddr = pTcpTable->table[i].dwLocalAddr;
                row.dwLocalPort = pTcpTable->table[i].dwLocalPort;
                row.dwRemoteAddr = pTcpTable->table[i].dwRemoteAddr;
                row.dwRemotePort = pTcpTable->table[i].dwRemotePort;
                
                SetTcpEntry(&row);
            }
        }
    }
    
    free(pTcpTable);
}

void ProcessManager::SaveBlockedProcesses() {
    std::wstring configPath = GetConfigFilePath();
    
    std::wofstream file(configPath);
    if (!file.is_open()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const std::wstring& name : blockedProcessNames_) {
        file << name << L"\n";
    }
    
    file.close();
}

void ProcessManager::LoadBlockedProcesses() {
    std::wstring configPath = GetConfigFilePath();
    
    std::wifstream file(configPath);
    if (!file.is_open()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    blockedProcessNames_.clear();
    
    std::wstring line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            blockedProcessNames_.insert(line);
        }
    }
    
    file.close();
}

std::wstring ProcessManager::GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK) {
        std::wstring path = appDataPath;
        path += L"\\NetView\\";
        
        // Create directory if it doesn't exist
        CreateDirectoryW(path.c_str(), NULL);
        
        path += CONFIG_FILENAME;
        return path;
    }
    
    return CONFIG_FILENAME;
}