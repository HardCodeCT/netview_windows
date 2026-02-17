#pragma once
#include <windows.h>
#include <string>
#include <vector>

#define WND_CLASS_PAYMENT L"NetViewPaymentWindow"

// UI Colors - Using #define macros (100% compatible with MSVC)
#define PAYMENT_COLOR_BACKGROUND       RGB(0, 0, 0)
#define PAYMENT_COLOR_CARD_BG          RGB(26, 26, 26)
#define PAYMENT_COLOR_CARD_ADDRESS_BG  RGB(13, 13, 13)
#define PAYMENT_COLOR_TEXT_PRIMARY     RGB(255, 255, 255)
#define PAYMENT_COLOR_TEXT_SECONDARY   RGB(176, 176, 176)
#define PAYMENT_COLOR_TEXT_DIM         RGB(128, 128, 128)
#define PAYMENT_COLOR_BUTTON_BG        RGB(42, 42, 42)
#define PAYMENT_COLOR_BUTTON_TEXT      RGB(100, 181, 246)
#define PAYMENT_COLOR_BORDER           RGB(60, 60, 60)

class PaymentWindow {
public:
    PaymentWindow();
    ~PaymentWindow();
    
    bool Create(HWND parentHwnd = NULL);
    void Show();
    void Hide();
    bool IsVisible() const;
    HWND GetHandle() const { return hwnd_; }
    
private:
    struct CryptoWallet {
        std::wstring name;
        std::wstring symbol;
        std::wstring minAmount;
        std::wstring address;
        int logoResourceId;
    };
    
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void OnPaint();
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void DrawWallet(HDC hdc, int yPos, const CryptoWallet& wallet);
    void OnCopyAddress(const std::wstring& address, const std::wstring& symbol);
    void OnConfirmPayment();
    
    HWND hwnd_;
    HWND walletAddressEdit_;
    HWND confirmButton_;
    
    std::vector<CryptoWallet> wallets_;
    std::vector<HWND> copyButtons_;
    
    int scrollPos_;
    bool isVisible_;
    
    static PaymentWindow* instance_;
};