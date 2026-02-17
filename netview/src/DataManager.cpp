#include "DataManager.h"
#include <sstream>
#include <iomanip>

DataManager::DataManager()
    : totalDataIn_(0)
    , totalDataOut_(0)
    , rateIn_(0)
    , rateOut_(0)
    , monitoring_(false)
    , shouldStop_(false)
    , prevDataIn_(0)
    , prevDataOut_(0)
    , baselineDataIn_(0)
    , baselineDataOut_(0)
    , smoothedRateIn_(0.0)
    , smoothedRateOut_(0.0) {
}

DataManager::~DataManager() {
    StopMonitoring();
}

std::wstring DataManager::FormatBytes(UINT64 bytes) {
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

std::wstring DataManager::GuidToString(const GUID& guid) {
    wchar_t guidString[40];
    swprintf_s(guidString, 40,
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::wstring(guidString);
}

void DataManager::AutoSelectAdapter() {
    MIB_IF_TABLE2* pIfTable = nullptr;
    if (GetIfTable2(&pIfTable) != NO_ERROR) {
        return;
    }

    // Priority 1: Active WiFi
    for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
        MIB_IF_ROW2* pRow = &pIfTable->Table[i];
        if (pRow->Type == IF_TYPE_IEEE80211 && pRow->OperStatus == IfOperStatusUp) {
            selectedAdapterGuid_ = GuidToString(pRow->InterfaceGuid);
            FreeMibTable(pIfTable);
            return;
        }
    }

    // Priority 2: Active Ethernet
    for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
        MIB_IF_ROW2* pRow = &pIfTable->Table[i];
        if (pRow->Type == IF_TYPE_ETHERNET_CSMACD && pRow->OperStatus == IfOperStatusUp) {
            selectedAdapterGuid_ = GuidToString(pRow->InterfaceGuid);
            FreeMibTable(pIfTable);
            return;
        }
    }

    // Priority 3: Any active non-loopback
    for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
        MIB_IF_ROW2* pRow = &pIfTable->Table[i];
        if (pRow->Type != IF_TYPE_SOFTWARE_LOOPBACK && pRow->OperStatus == IfOperStatusUp) {
            selectedAdapterGuid_ = GuidToString(pRow->InterfaceGuid);
            FreeMibTable(pIfTable);
            return;
        }
    }

    FreeMibTable(pIfTable);
}

void DataManager::UpdateNetworkStats() {
    // Skip if no adapter selected
    if (selectedAdapterGuid_.empty()) {
        return;
    }

    MIB_IF_TABLE2* pIfTable = nullptr;
    if (GetIfTable2(&pIfTable) != NO_ERROR) {
        return;
    }

    UINT64 currentIn = 0;
    UINT64 currentOut = 0;
    bool found = false;

    // Find the selected adapter by GUID
    for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
        MIB_IF_ROW2* pRow = &pIfTable->Table[i];
        std::wstring guid = GuidToString(pRow->InterfaceGuid);

        if (guid == selectedAdapterGuid_) {
            // Verify adapter is still operational
            if (pRow->OperStatus == IfOperStatusUp) {
                currentIn = pRow->InOctets;
                currentOut = pRow->OutOctets;
                found = true;
            }
            break;  // Found our adapter, stop searching
        }
    }

    FreeMibTable(pIfTable);

    // If adapter not found or disconnected, just return (don't update totals)
    if (!found) {
        return;
    }

    // Calculate usage since monitoring started
    UINT64 deltaSinceStart = (currentIn >= baselineDataIn_) ? (currentIn - baselineDataIn_) : 0;
    UINT64 deltaOut = (currentOut >= baselineDataOut_) ? (currentOut - baselineDataOut_) : 0;

    // Store totals
    totalDataIn_.store(deltaSinceStart, std::memory_order_release);
    totalDataOut_.store(deltaOut, std::memory_order_release);
}

void DataManager::MonitorThread() {
    // Set initial baseline for the selected adapter
    MIB_IF_TABLE2* pIfTable = nullptr;
    if (GetIfTable2(&pIfTable) == NO_ERROR) {
        for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
            MIB_IF_ROW2* pRow = &pIfTable->Table[i];
            if (GuidToString(pRow->InterfaceGuid) == selectedAdapterGuid_) {
                baselineDataIn_ = pRow->InOctets;
                baselineDataOut_ = pRow->OutOctets;
                break;
            }
        }
        FreeMibTable(pIfTable);
    }

    // Initialize tracking
    UpdateNetworkStats();
    prevDataIn_ = totalDataIn_.load(std::memory_order_acquire);
    prevDataOut_ = totalDataOut_.load(std::memory_order_acquire);
    smoothedRateIn_ = 0.0;
    smoothedRateOut_ = 0.0;

    int reconnectCounter = 0;
    const int RECONNECT_CHECK_INTERVAL = 5;  // Check every 5 seconds

    while (!shouldStop_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        reconnectCounter++;

        // Get previous totals before update
        UINT64 prevIn = totalDataIn_.load(std::memory_order_acquire);
        UINT64 prevOut = totalDataOut_.load(std::memory_order_acquire);

        // Update network stats
        UpdateNetworkStats();

        // Get current totals after update
        UINT64 currentIn = totalDataIn_.load(std::memory_order_acquire);
        UINT64 currentOut = totalDataOut_.load(std::memory_order_acquire);

        // Check if adapter disconnected (totals didn't change)
        if (currentIn == prevIn && currentOut == prevOut && reconnectCounter >= RECONNECT_CHECK_INTERVAL) {
            // ============================================================
            // CRITICAL FIX: PRESERVE ACCUMULATED DATA BEFORE SWITCHING!
            // ============================================================
            UINT64 preservedIn = currentIn;
            UINT64 preservedOut = currentOut;
            
            wchar_t msg[256];
            swprintf_s(msg, 256, L"[DataManager] Adapter disconnect detected. Preserving %llu in, %llu out\n", 
                      preservedIn, preservedOut);
            OutputDebugStringW(msg);
            
            // Try to reconnect to a different adapter
            std::wstring oldGuid = selectedAdapterGuid_;
            AutoSelectAdapter();
            
            // If we selected a new adapter, adjust baseline to preserve accumulated data
            if (!selectedAdapterGuid_.empty() && selectedAdapterGuid_ != oldGuid) {
                MIB_IF_TABLE2* pTable = nullptr;
                if (GetIfTable2(&pTable) == NO_ERROR) {
                    for (ULONG i = 0; i < pTable->NumEntries; i++) {
                        MIB_IF_ROW2* pRow = &pTable->Table[i];
                        if (GuidToString(pRow->InterfaceGuid) == selectedAdapterGuid_) {
                            // ============================================================
                            // CRITICAL FIX: Adjust baseline to preserve accumulated data
                            // ============================================================
                            // Formula: newBaseline = newAdapterValue - preservedTotal
                            // This ensures: total = newAdapterValue - newBaseline = preservedTotal
                            
                            UINT64 newAdapterIn = pRow->InOctets;
                            UINT64 newAdapterOut = pRow->OutOctets;
                            
                            // Set baseline such that we continue from preserved values
                            if (newAdapterIn >= preservedIn) {
                                baselineDataIn_ = newAdapterIn - preservedIn;
                            } else {
                                // New adapter has lower counter (wrapped or new interface)
                                // Set baseline to 0 and manually set the total
                                baselineDataIn_ = 0;
                                totalDataIn_.store(preservedIn, std::memory_order_release);
                            }
                            
                            if (newAdapterOut >= preservedOut) {
                                baselineDataOut_ = newAdapterOut - preservedOut;
                            } else {
                                baselineDataOut_ = 0;
                                totalDataOut_.store(preservedOut, std::memory_order_release);
                            }
                            
                            // Reset rate tracking (not totals!)
                            prevDataIn_ = preservedIn;
                            prevDataOut_ = preservedOut;
                            smoothedRateIn_ = 0.0;
                            smoothedRateOut_ = 0.0;
                            
                            wchar_t reconnectMsg[512];
                            swprintf_s(reconnectMsg, 512, 
                                      L"[DataManager] ✓ Reconnected! New baseline: %llu in, %llu out. "
                                      L"Continuing from: %llu in, %llu out\n",
                                      baselineDataIn_, baselineDataOut_, preservedIn, preservedOut);
                            OutputDebugStringW(reconnectMsg);
                            
                            break;
                        }
                    }
                    FreeMibTable(pTable);
                }
            }
            reconnectCounter = 0;
        }

        // Calculate delta (bytes transferred in last second)
        UINT64 rawDeltaIn = (currentIn > prevDataIn_) ? (currentIn - prevDataIn_) : 0;
        UINT64 rawDeltaOut = (currentOut > prevDataOut_) ? (currentOut - prevDataOut_) : 0;

        // Apply exponential smoothing to reduce jitter
        smoothedRateIn_ = (smoothedRateIn_ * (1.0 - SMOOTHING_FACTOR)) +
                         (static_cast<double>(rawDeltaIn) * SMOOTHING_FACTOR);
        smoothedRateOut_ = (smoothedRateOut_ * (1.0 - SMOOTHING_FACTOR)) +
                          (static_cast<double>(rawDeltaOut) * SMOOTHING_FACTOR);

        // Clamp small values to zero (remove background noise)
        const UINT64 NOISE_THRESHOLD = 100;  // bytes/sec
        UINT64 finalRateIn = (smoothedRateIn_ < NOISE_THRESHOLD) ? 0 : static_cast<UINT64>(smoothedRateIn_);
        UINT64 finalRateOut = (smoothedRateOut_ < NOISE_THRESHOLD) ? 0 : static_cast<UINT64>(smoothedRateOut_);

        // Store rates
        rateIn_.store(finalRateIn, std::memory_order_release);
        rateOut_.store(finalRateOut, std::memory_order_release);

        // Update previous values for next iteration
        prevDataIn_ = currentIn;
        prevDataOut_ = currentOut;
    }
}

bool DataManager::StartMonitoring() {
    if (monitoring_.load(std::memory_order_acquire)) {
        return true;  // Already monitoring
    }

    // Auto-select best adapter
    AutoSelectAdapter();

    if (selectedAdapterGuid_.empty()) {
        return false;  // No adapter available
    }

    shouldStop_.store(false, std::memory_order_release);
    monitoring_.store(true, std::memory_order_release);

    monitorThread_ = std::thread(&DataManager::MonitorThread, this);

    return true;
}

void DataManager::StopMonitoring() {
    if (!monitoring_.load(std::memory_order_acquire)) {
        return;
    }

    shouldStop_.store(true, std::memory_order_release);
    monitoring_.store(false, std::memory_order_release);

    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}
