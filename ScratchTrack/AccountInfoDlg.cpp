#include "AccountInfoDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Utils.h"
#include "TreeUtils.h"
#include <richedit.h>


extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
int serializeState();

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

void AccountInfoDlg::doUnhidePopup()
{
    // Get all items
    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    auto envelopes = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
    auto folders = AccountBase::getAllAccountsOfType(AccountBase::GROUP);

    HMENU hPopup = CreatePopupMenu();
    HMENU hAcctMenu = CreatePopupMenu();
    HMENU hFolderMenu = CreatePopupMenu();
    HMENU hEnvMenu = CreatePopupMenu();

    AppendMenu(hPopup, MF_STRING | MF_POPUP, (UINT_PTR)hAcctMenu, L"Accounts");
    AppendMenu(hPopup, MF_STRING | MF_POPUP, (UINT_PTR)hFolderMenu, L"Folders");
    AppendMenu(hPopup, MF_STRING | MF_POPUP, (UINT_PTR)hEnvMenu, L"Envelopes");

    // A return code of zero from trackpopup means nothing was selected, so we can't
    // use accounts IDs, which are zero based.
    int menuIdOffset = 1;

    // Add hidden accounts
    for (size_t i = 0; i < accounts.size(); i++) {
        AccountBase* pAcct = accounts[i];
        if (pAcct->getHidden()) {
            AppendMenu(hAcctMenu, MF_STRING | MF_POPUP, (UINT_PTR)pAcct->getId() + menuIdOffset, pAcct->getName());
        }
    }

    // Add hidden groups
    for (size_t i = 0; i < folders.size(); i++) {
        AccountBase* pAcct = folders[i];
        if (pAcct->getHidden()) {
            AppendMenu(hFolderMenu, MF_STRING | MF_POPUP, (UINT_PTR)pAcct->getId() + menuIdOffset, pAcct->getName());
        }
    }

    // Add hidden envelopes
    for (size_t i = 0; i < envelopes.size(); i++) {
        AccountBase* pAcct = envelopes[i];
        if (pAcct->getHidden()) {
            AppendMenu(hEnvMenu, MF_STRING | MF_POPUP, (UINT_PTR)pAcct->getId() + menuIdOffset, pAcct->getName());
        }
    }

    POINT pt = { 0 };
    GetCursorPos(&pt);
    DWORD dwCode = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, _hThisDialg, NULL);
    if (dwCode)
    {
        unsigned int acctId = dwCode - menuIdOffset;
        AccountBase* pAcct = AccountBase::lookupById(acctId);
        if (pAcct)
        {
            // Unhide the item
            pAcct->setHidden(false);

            // Envelope special work
            if (pAcct->getType() == AccountBase::ENVELOPE) {
                AccountBase* pParent = pAcct->getParent();

                // If its parent is hidden, warn.
                if (pParent && pParent->getHidden()) {
                    wchar_t szBuff[1024] = { 0 };
                    swprintf(szBuff, 1024, L"Item '%ls' is now marked as unhidden, but its parent '%ls' is hidden.\n", pAcct->getName(), pParent->getName());
                    MessageBox(g_hTopWindow, szBuff, L"Unide warning", MB_ICONWARNING);
                }
                else
                {
                    HTREEITEM hParent = treeItemFromAccount(pParent);
                    addTreeItem(hParent, pAcct);
                }
            }

            // Folder / group special work
            if (pAcct->getType() == AccountBase::GROUP) {
                addTreeItem(TVI_ROOT, pAcct);

            }

            // Accounts
            if (pAcct->getType() == AccountBase::ACCOUNT) {
                addTreeItem(TVI_ROOT, pAcct);
            }

            serializeState();
        }
    }

}

INT_PTR CALLBACK AccountInfoDlg::AccountInfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    AccountInfoDlg* pThis = (AccountInfoDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
            UINT message = 0;
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_ADD_ACCOUNT:
                message = NM_ACCOUNT_INFO_DIALOG_ADD_ACCOUNT;
                break;
            case IDC_BUTTON_ADD_FOLDER:
                message = NM_ACCOUNT_DIALOG_ADD_FOLDER;
                break;
            case IDC_BUTTON_UNHIDE:
                pThis->doUnhidePopup();
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
    SendMessage(hRichText, EM_SETSEL, INVALID_ACCOUNT_ID, INVALID_ACCOUNT_ID);

    SendMessageW(hRichText, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(hRichText, EM_REPLACESEL, 0, (LPARAM)szText);
    

}

void AccountInfoDlg::getMasterSummary(std::wstring& summary, bool& isError)
{
    // Calc new window caption;
    int iReservedDollars = 0, iBalance = 0;
    std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    std::vector<AccountBase*> envelopes = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);

    for (size_t i = 0; i < accounts.size(); i++) {
        Account* pAccount = (Account*)accounts[i];
        if (pAccount->getContributesToBudget()) {
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
    static const int HEADER_SIZE = 14;
    static const int TEXT_SIZE = 11;
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    //SendMessageW(hRichText, WM_SETREDRAW, false, 0);
    SendMessageW(hRichText, WM_SETTEXT, 0, (LPARAM)L"");

    appendText(L"State of the budget\n", RGB(0, 0, 0), false, true, HEADER_SIZE);

    std::wstring summary;
    bool isErrorState = false;
    getMasterSummary(summary, isErrorState);
    appendText(summary.c_str(), isErrorState ? RGB(255, 0, 0) : RGB(0,127,0), false, false, TEXT_SIZE);

    auto groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
    for (size_t i = 0; i < groups.size(); i++) {
        AccountGroup* group = dynamic_cast<AccountGroup* >(groups[i]);
        int limit = group->getExpenseLimit();
        if (!limit) {
            continue;
        }
        int budgeted = -1*group->budgetedToSubAccounts();

        bool errorState = limit < budgeted;
        COLORREF cr = errorState ? RGB(255, 0, 0) : RGB(0, 0, 255);
        appendText(L"Folder '", cr, false, false, TEXT_SIZE);
        appendText(group->getName(), cr, false, false, TEXT_SIZE);
        appendText(L"'\n\tSet expense limit: ", cr, false, false, TEXT_SIZE);
        std::wstring amt = Utils::penniesToDollarsString(limit);
        appendText(amt.c_str(), cr, false, false, TEXT_SIZE);
        appendText(L"\n\tBudgeted expenses: ", cr, false, false, TEXT_SIZE);
        amt = Utils::penniesToDollarsString(budgeted);
        appendText(amt.c_str(), cr, false, false, TEXT_SIZE);

        int delta = limit - budgeted;
        amt = Utils::penniesToDollarsString(budgeted);
        if (errorState) {
            appendText(L"\n\tOVERBUDGETED BY ", cr, false, false, TEXT_SIZE);
        }
        else {
            appendText(L"\n\tUnder budget by ", cr, false, false, TEXT_SIZE);
        }
        appendText(amt.c_str(), cr, false, false, TEXT_SIZE);
        appendText(L"\n", cr, false, false, TEXT_SIZE);
    }
    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    for (size_t i = 0; i < accounts.size(); i++)
    {
        auto account = accounts[i];
        if (account->getBalance() < 0)
        {
            std::wstring balance = Utils::penniesToDollarsString(account->getBalance());
            appendText(L"\u26A0 : ", RGB(255, 0, 0), false, false, 13);
            appendText(L"Account '", RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(account->getName(), RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(L"' negative balance ", RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(balance.c_str(), RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(L"\n", RGB(255, 0, 0), false, false, TEXT_SIZE);
        }
    }

    auto categories = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
    for (size_t i = 0; i < categories.size(); i++)
    {
        auto envelope = categories[i];
        if (envelope->getBalance() < 0)
        {
            std::wstring balance = Utils::penniesToDollarsString(envelope->getBalance());
            appendText(L"\u26A0 : ", RGB(255, 0, 0), false, false, 13);
            appendText(L"Envelope '", RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(envelope->getName(), RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(L"' negative balance ", RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(balance.c_str(), RGB(255, 0, 0), false, false, TEXT_SIZE);
            appendText(L"\n", RGB(255, 0, 0), false, false, TEXT_SIZE);
        }
        
    }
    //SendMessageW(hRichText, WM_SETREDRAW, true, 0);

}