#include "FirebaseManager.h"
#include <sstream>
#include <iomanip>
#include <thread>

FirebaseManager* FirebaseManager::instance_ = nullptr;

FirebaseManager& FirebaseManager::GetInstance() {
    if (!instance_) {
        instance_ = new FirebaseManager();
    }
    return *instance_;
}

FirebaseManager::FirebaseManager() {
}

FirebaseManager::~FirebaseManager() {
}

std::wstring FirebaseManager::GetCurrentTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    wchar_t buffer[128];
    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    return buffer;
}

std::wstring FirebaseManager::BuildPaymentJson(const PaymentSubmission& payment) {
    // Build JSON manually (simple approach without dependencies)
    std::wostringstream json;
    json << L"{";
    json << L"\"installationKey\":\"" << payment.installationKey << L"\",";
    json << L"\"walletAddress\":\"" << payment.walletAddress << L"\",";
    json << L"\"timestamp\":\"" << payment.timestamp << L"\",";
    json << L"\"status\":\"pending\",";
    json << L"\"amount\":\"4.99 USD\"";
    json << L"}";
    
    return json.str();
}

bool FirebaseManager::SendHttpRequest(const std::wstring& json, std::wstring& response) {
    bool success = false;
    
    HINTERNET hSession = WinHttpOpen(
        L"NetView/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) return false;
    
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        firebaseUrl_.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );
    
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Build full path with .json extension (Firebase requirement)
    std::wstring fullPath = firebasePath_ + L".json";
    
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        fullPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Set headers
    std::wstring headers = L"Content-Type: application/json\r\n";
    
    // Convert JSON to UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, NULL, 0, NULL, NULL);
    char* utf8Buffer = new char[utf8Size];
    WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, utf8Buffer, utf8Size, NULL, NULL);
    
    // Send request
    BOOL requestSent = WinHttpSendRequest(
        hRequest,
        headers.c_str(),
        -1,
        utf8Buffer,
        utf8Size - 1,
        utf8Size - 1,
        0
    );
    
    if (requestSent) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            
            WinHttpQueryHeaders(
                hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL,
                &statusCode,
                &statusCodeSize,
                NULL
            );
            
            if (statusCode == 200) {
                DWORD bytesAvailable = 0;
                std::string responseData;
                
                do {
                    bytesAvailable = 0;
                    if (WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                        if (bytesAvailable > 0) {
                            char* buffer = new char[bytesAvailable + 1];
                            DWORD bytesRead = 0;
                            
                            if (WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {
                                buffer[bytesRead] = 0;
                                responseData += buffer;
                            }
                            
                            delete[] buffer;
                        }
                    }
                } while (bytesAvailable > 0);
                
                // Convert response to wide string
                int wideSize = MultiByteToWideChar(CP_UTF8, 0, responseData.c_str(), -1, NULL, 0);
                wchar_t* wideBuffer = new wchar_t[wideSize];
                MultiByteToWideChar(CP_UTF8, 0, responseData.c_str(), -1, wideBuffer, wideSize);
                response = wideBuffer;
                delete[] wideBuffer;
                
                success = true;
            }
        }
    }
    
    delete[] utf8Buffer;
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return success;
}

void FirebaseManager::SubmitPayment(const PaymentSubmission& payment, SubmitCallback callback) {
    // Build JSON
    std::wstring json = BuildPaymentJson(payment);
    
    // Send request in background thread
    std::thread([this, json, callback]() {
        std::wstring response;
        bool success = SendHttpRequest(json, response);
        
        if (callback) {
            if (success) {
                callback(true, L"Payment submitted successfully");
            } else {
                callback(false, L"Failed to connect to server");
            }
        }
    }).detach();
}

void FirebaseManager::CheckPaymentStatus(const std::wstring& installationKey,
                                        std::function<void(bool verified)> callback) {
    // This would query Firebase to check if payment is verified
    // For now, we'll implement a simple version
    
    std::thread([this, installationKey, callback]() {
        HINTERNET hSession = WinHttpOpen(
            L"NetView/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );
        
        if (!hSession) {
            if (callback) callback(false);
            return;
        }
        
        HINTERNET hConnect = WinHttpConnect(
            hSession,
            firebaseUrl_.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );
        
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            if (callback) callback(false);
            return;
        }
        
        // Query specific installation key
        std::wstring queryPath = firebasePath_ + L".json?orderBy=\"installationKey\"&equalTo=\"" 
                                + installationKey + L"\"";
        
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            queryPath.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );
        
        bool verified = false;
        
        if (hRequest) {
            if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                  WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                if (WinHttpReceiveResponse(hRequest, NULL)) {
                    DWORD bytesAvailable = 0;
                    std::string responseData;
                    
                    do {
                        bytesAvailable = 0;
                        if (WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                            if (bytesAvailable > 0) {
                                char* buffer = new char[bytesAvailable + 1];
                                DWORD bytesRead = 0;
                                
                                if (WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {
                                    buffer[bytesRead] = 0;
                                    responseData += buffer;
                                }
                                
                                delete[] buffer;
                            }
                        }
                    } while (bytesAvailable > 0);
                    
                    // Simple check if response contains "verified":true
                    if (responseData.find("\"status\":\"verified\"") != std::string::npos) {
                        verified = true;
                    }
                }
            }
            
            WinHttpCloseHandle(hRequest);
        }
        
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
        if (callback) callback(verified);
    }).detach();
}
