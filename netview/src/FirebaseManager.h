#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <functional>

#pragma comment(lib, "winhttp.lib")

class FirebaseManager {
public:
    static FirebaseManager& GetInstance();
    
    struct PaymentSubmission {
        std::wstring installationKey;
        std::wstring walletAddress;
        std::wstring timestamp;
    };
    
    using SubmitCallback = std::function<void(bool success, const std::wstring& message)>;
    
    // Submit payment to Firebase
    void SubmitPayment(const PaymentSubmission& payment, SubmitCallback callback);
    
    // Check payment status
    void CheckPaymentStatus(const std::wstring& installationKey, 
                           std::function<void(bool verified)> callback);
    
private:
    FirebaseManager();
    ~FirebaseManager();
    
    std::wstring GetCurrentTimestamp();
    std::wstring BuildPaymentJson(const PaymentSubmission& payment);
    bool SendHttpRequest(const std::wstring& json, std::wstring& response);
    
    static FirebaseManager* instance_;
    
    // Firebase configuration
    // Replace with your Firebase project details
    const std::wstring firebaseUrl_ = L"your-project-id.firebaseio.com";
    const std::wstring firebasePath_ = L"/payments";
};
