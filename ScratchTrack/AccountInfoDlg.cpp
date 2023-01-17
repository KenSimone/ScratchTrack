#include "AccountInfoDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Utils.h"
#include <richedit.h>


extern HINSTANCE g_hInst;

AccountInfoDlg::AccountInfoDlg() : _hParent(nullptr), _hThisDialg(nullptr)
{

}

AccountInfoDlg::~AccountInfoDlg()
{
}

HWND AccountInfoDlg::create(HWND hParent)
{
    LoadLibrary(L"Riched20.dll");

    _hParent = hParent;
    _hThisDialg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BUDGET_INFO_DIALOG), _hParent, AccountInfoDlgProc);
    SetWindowLongPtr(_hThisDialg, GWLP_USERDATA, (LONG_PTR)this);

    updateReport();
    return _hThisDialg;
}

INT_PTR CALLBACK AccountInfoDlg::AccountInfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_SIZE:
    {
        // Move the rich text control
        HWND hRichText = GetDlgItem(hwnd, IDC_RICHEDIT_SUMMARY);
        RECT rDialog, rControl;
        GetWindowRect(hwnd, &rDialog);
        GetWindowRect(hRichText, &rControl);
        POINT ptControl;
        ptControl.x = rControl.left;
        ptControl.y = rControl.top;
        ScreenToClient(hwnd, &ptControl);

        LONG width = rDialog.right - rDialog.left;
        LONG height = rDialog.bottom - rDialog.top;

        SetWindowPos(hRichText, nullptr, ptControl.x, ptControl.y, width - (2*ptControl.x), height - ptControl.y, SWP_NOZORDER);

        break;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            AccountInfoDlg* pThis = (AccountInfoDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            UINT message = 0;
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_ADD_ACCOUNT:
                message = NM_ACCOUNT_INFO_DIALOG_ADD_ACCOUNT;
                break;
            case IDC_BUTTON_ADD_FOLDER:
                message = NM_ACCOUNT_DIALOG_ADD_FOLDER;
                break;
            }

            if (message) {
                NMHDR hdr = { 0 };
                hdr.code = message;
                hdr.idFrom = IDD_BUDGET_INFO_DIALOG;
                SendMessage(pThis->_hParent, WM_NOTIFY, 0, (LPARAM)&hdr);
            }
        }
    }
    return false;

}

void AccountInfoDlg::appendText(const wchar_t* szText, COLORREF cr, bool bold, bool underline, int sizeInPoints)
{
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);

    // EM_SETCHARFORMAT
    CHARFORMAT cf = { 0 };
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE | CFM_UNDERLINE;
    if (bold) {
        cf.dwEffects = CFE_BOLD;
    }

    if (underline) {
        cf.dwEffects |= CFE_UNDERLINE;
    }
    

    static const LONG FONT_POINT_SCALE = 20;
    LONG size = FONT_POINT_SCALE * sizeInPoints;
    cf.yHeight = size;

    cf.crTextColor = cr;

    // Move selection to the end
    SendMessage(hRichText, EM_SETSEL, MAXINT, MAXINT);

    SendMessageW(hRichText, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(hRichText, EM_REPLACESEL, 0, (LPARAM)szText);
    

}

void AccountInfoDlg::getMasterSummary(std::wstring& summary, bool& isError)
{
    // Calc new window caption;
    int iReservedDollars = 0, iBalance = 0;
    std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    std::vector<AccountBase*> envelopes = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);

    for (size_t i = 0; i < accounts.size(); i++) {
        Account* pAccount = (Account*)accounts[i];
        if (pAccount->isBudgeted()) {
            iBalance += accounts[i]->getBalance();
        }
    }

    for (size_t i = 0; i < envelopes.size(); i++) {
        // This is a confusing operation. If a virtual account has a negative balance
        // we conceptually treat it as if it had a bigger balance since the whole thing is
        // a "how much do I deduct from what's available" operation. Hence the abs() here.
        iReservedDollars += abs(envelopes[i]->getBalance());
    }

    int iAvailable = iBalance - iReservedDollars;

    std::wstring totalDollars = Utils::penniesToDollarsString(iBalance);
    std::wstring availableDollars = Utils::penniesToDollarsString(iBalance - iReservedDollars);

    wchar_t szCaption[1024] = { 0 };
    swprintf(szCaption, 1024, L"%s: %s of %s available for budget\n", iAvailable >= 0 ? L"\u2713" : L"\u26A0", availableDollars.c_str(), totalDollars.c_str());

    summary = szCaption;
    isError = (iBalance - iReservedDollars) < 0;
}


void AccountInfoDlg::updateReport()
{
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    //SendMessageW(hRichText, WM_SETREDRAW, false, 0);
    SendMessageW(hRichText, WM_SETTEXT, 0, (LPARAM)L"");

    appendText(L"State of the budget\n", RGB(0, 0, 0), false, true, 14);

    std::wstring summary;
    bool isErrorState = false;
    getMasterSummary(summary, isErrorState);
    appendText(summary.c_str(), isErrorState ? RGB(255, 0, 0) : RGB(0,127,0), false, false, 13);

    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    for (size_t i = 0; i < accounts.size(); i++)
    {
        auto account = accounts[i];
        if (account->getBalance() < 0)
        {
            std::wstring balance = Utils::penniesToDollarsString(account->getBalance());
            appendText(L"\u26A0 : ", RGB(255, 0, 0), false, false, 13);
            appendText(L"Account '", RGB(255, 0, 0), false, false, 12);
            appendText(account->getName(), RGB(255, 0, 0), false, false, 12);
            appendText(L"' negative balance ", RGB(255, 0, 0), false, false, 12);
            appendText(balance.c_str(), RGB(255, 0, 0), false, false, 12);
            appendText(L"\n", RGB(255, 0, 0), false, false, 12);
        }
    }

    auto categories = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
    for (size_t i = 0; i < categories.size(); i++)
    {
        auto category = categories[i];
        if (category->getBalance() < 0)
        {
            std::wstring balance = Utils::penniesToDollarsString(category->getBalance());
            appendText(L"\u26A0 : ", RGB(255, 0, 0), false, false, 13);
            appendText(L"Category '", RGB(255, 0, 0), false, false, 12);
            appendText(category->getName(), RGB(255, 0, 0), false, false, 12);
            appendText(L"' negative balance ", RGB(255, 0, 0), false, false, 12);
            appendText(balance.c_str(), RGB(255, 0, 0), false, false, 12);
            appendText(L"\n", RGB(255, 0, 0), false, false, 12);
        }
        
    }
    //SendMessageW(hRichText, WM_SETREDRAW, true, 0);

}