#pragma once

// CRITICAL: Define Windows version FIRST before any includes
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7
#endif

// Windows headers in correct order
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>

// Standard library
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

class DataManager {
public:
    DataManager();
    ~DataManager();
    
    // Control
    bool StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const { return monitoring_.load(std::memory_order_acquire); }
    
    // Getters (thread-safe)
    UINT64 GetDataIn() const { return totalDataIn_.load(std::memory_order_acquire); }
    UINT64 GetDataOut() const { return totalDataOut_.load(std::memory_order_acquire); }
    UINT64 GetRateIn() const { return rateIn_.load(std::memory_order_acquire); }
    UINT64 GetRateOut() const { return rateOut_.load(std::memory_order_acquire); }
    
    // Utility
    std::wstring FormatBytes(UINT64 bytes);
    
private:
    void MonitorThread();
    void UpdateNetworkStats();
    void AutoSelectAdapter();
    std::wstring GuidToString(const GUID& guid);
    
private:
    // Cumulative counters (data used since app started)
    std::atomic<UINT64> totalDataIn_;
    std::atomic<UINT64> totalDataOut_;
    
    // Rates (smoothed, bytes per second)
    std::atomic<UINT64> rateIn_;
    std::atomic<UINT64> rateOut_;
    
    // Control
    std::atomic<bool> monitoring_;
    std::atomic<bool> shouldStop_;
    std::thread monitorThread_;
    
    // For rate calculation
    UINT64 prevDataIn_;
    UINT64 prevDataOut_;
    
    // Baseline (starting point when monitoring began)
    UINT64 baselineDataIn_;
    UINT64 baselineDataOut_;
    
    // For smoothing (reduces jitter)
    double smoothedRateIn_;
    double smoothedRateOut_;
    static constexpr double SMOOTHING_FACTOR = 0.3; // 30% new, 70% old
    
    // Store which adapter we're monitoring
    std::wstring selectedAdapterGuid_;
};
