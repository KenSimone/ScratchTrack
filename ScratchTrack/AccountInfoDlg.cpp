#include "AccountInfoDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Utils.h"
#include "TreeUtils.h"
#include "Transaction.h"
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

    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    SendMessage(hRichText, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));

    updateReport();
    return _hThisDialg;
}

void AccountInfoDlg::doUnhidePopup()
{
    // Get all items
    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    auto categories = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
    auto folders = AccountBase::getAllAccountsOfType(AccountBase::GROUP);

    HMENU hPopup = CreatePopupMenu();
    HMENU hAcctMenu = CreatePopupMenu();
    HMENU hFolderMenu = CreatePopupMenu();
    HMENU hEnvMenu = CreatePopupMenu();

    AppendMenu(hPopup, MF_STRING | MF_POPUP, (UINT_PTR)hAcctMenu, L"Accounts");
    AppendMenu(hPopup, MF_STRING | MF_POPUP, (UINT_PTR)hFolderMenu, L"Folders");
    AppendMenu(hPopup, MF_STRING | MF_POPUP, (UINT_PTR)hEnvMenu, L"Categories");

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

    // Add hidden categories
    for (size_t i = 0; i < categories.size(); i++) {
        AccountBase* pAcct = categories[i];
        if (pAcct->getHidden()) {
            AppendMenu(hEnvMenu, MF_STRING | MF_POPUP, (UINT_PTR)pAcct->getId() + menuIdOffset, pAcct->getName());
        }
    }

    POINT pt = { 0 };
    GetCursorPos(&pt);
    DWORD dwCode = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, _hThisDialg, NULL);
    DestroyMenu(hPopup);
    DestroyMenu(hAcctMenu);
    DestroyMenu(hFolderMenu);
    DestroyMenu(hEnvMenu);


    if (dwCode)
    {
        unsigned int acctId = dwCode - menuIdOffset;
        AccountBase* pAcct = AccountBase::lookupById(acctId);
        if (pAcct)
        {
            // Unhide the item
            pAcct->setHidden(false);

            // Category special work
            if (pAcct->getType() == AccountBase::CATEGORY) {
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

void AccountInfoDlg::appendText(const wchar_t* szText, COLORREF cr, bool bold, bool underline, int sizeInPoints, bool deleteExisting /*= false*/)
{
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    if (deleteExisting) {
        SetWindowText(hRichText, L"");
    }

    // EM_SETCHARFORMAT
    CHARFORMAT cf = { 0 };
    cf.cbSize = sizeof(cf);
    //SendMessageW(hRichText, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    //cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE | CFM_UNDERLINE | CFM_FACE;
    cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE | CFM_UNDERLINE | CFM_COLOR | CFM_FACE;
    wcscpy_s(cf.szFaceName, L"consolas");

    if (bold) {
        cf.dwEffects = CFE_BOLD;
    }

    if (underline) {
        cf.dwEffects |= CFE_UNDERLINE;
    }
    

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    
    static const LONG FONT_POINT_SCALE = 20;
    static const LONG FONT_POINT_SCALE_LOW_RES = 16;

    LONG size = (screenWidth < 1920) ? FONT_POINT_SCALE_LOW_RES * sizeInPoints : FONT_POINT_SCALE * sizeInPoints;
    cf.yHeight = size;

    cf.crTextColor = cr;

    // Move selection to the end
    SendMessage(hRichText, EM_SETSEL, ACCOUNT_ID_INVALID, ACCOUNT_ID_INVALID);

    SendMessageW(hRichText, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(hRichText, EM_REPLACESEL, 0, (LPARAM)szText);
}

void AccountInfoDlg::printMasterSummary()
{
    // Calc new window caption;
    int iReservedDollars = 0, iBalance = 0;
    bool isError = false;
    std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    std::vector<AccountBase*> folders = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
    std::vector<AccountBase*> categories = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);

    wchar_t szCaption[1024] = { 0 };

    if (accounts.empty()) {
        swprintf(szCaption, 1024, L"It looks like you don't have any accounts. Accounts represent money (or debt).\n\n"
        L"Examples of common account types are checking, savings, and credit card.\n\n"
        L"Click 'Add Account...' to add an account to ScratchTrack.\n\n");
    }
    else if (folders.empty()) {
        swprintf(szCaption, 1024, L"It looks like you don't have any folders. Folders help organize expenses.\n\n"
            L"A common strategy is to create a folder for each pay peried. For example, if you're paid on the 15th and 31st "
            L"you might create folders titled '15th' and '31st'.\n\n"
            L"Click 'Add Folder...' to add a folder to ScratchTrack.\n\n");
    }
    else if (categories.empty()) {
        swprintf(szCaption, 1024, L"It looks like you don't have any expense categories. Categories are used to reserve money "
            L"for expenses. Categories have a name and a monthly budget.\n\n"
            L"You'll add money to your categories and then pay expenses from those categories as needed. If your "
            L"category runs out of money you'll need to replenish that category.\n\n"
            L"Click the new category icon next to a folder name to create a category.\n\n");
    }
    else {

        // Get the balance of all accounts that contribute to the budget
        for (size_t i = 0; i < accounts.size(); i++) {
            Account* pAccount = (Account*)accounts[i];
            if (pAccount->getBalance() < 0 && !pAccount->isCCAccount() ) {
                swprintf(szCaption, 1024, L"Account '%ls' has a negative balance %ls\n\n",
                    pAccount->getName(),
                    Utils::penniesToDollarsString(pAccount->getBalance()).c_str());
                appendText(szCaption, TEXT_COLOR_NEGATIVE, false, false, TEXT_SIZE);
                szCaption[0] = 0;
            }

            if (pAccount->getContributesToBudget()) {
                iBalance += accounts[i]->getBalance();
            }
        }

        // Get all reserved dollars
        for (size_t i = 0; i < categories.size(); i++) {
            // This is a confusing operation. If a virtual account has a negative balance
            // we conceptually treat it as if it had a bigger balance since the whole thing is
            // a "how much do I deduct from what's available" operation. Hence the abs() here.
            iReservedDollars += abs(categories[i]->getBalance());
        }

        int iAvailable = iBalance - iReservedDollars;

        std::wstring totalDollars = Utils::penniesToDollarsString(iBalance);
        std::wstring reservedDollars = Utils::penniesToDollarsString(iReservedDollars);
        std::wstring difference = Utils::penniesToDollarsString(abs(iBalance - iReservedDollars));

        isError = (iBalance - iReservedDollars) < 0;

        if (isError) {
            if (iReservedDollars != 0) {
                swprintf(szCaption, 1024, L"You have %ls assigned to categories but only %ls in in-budget accounts. "
                    L"Deposit %ls or remove funds from categories to correct this issue.\n\n",
                    reservedDollars.c_str(), totalDollars.c_str(), difference.c_str());
            }
        }
        else
        {
            swprintf(szCaption, 1024, L"You have %s of %s available for budgeting\n\n", difference.c_str(), totalDollars.c_str());
        }
    }
    appendText(szCaption, isError ? TEXT_COLOR_NEGATIVE : TEXT_COLOR_NORMAL, false, false, TEXT_SIZE);
}


void AccountInfoDlg::updateReport()
{
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    //SendMessageW(hRichText, WM_SETREDRAW, false, 0);
    SendMessageW(hRichText, WM_SETTEXT, 0, (LPARAM)L"");

    appendText(L"State of the Budget\n", TEXT_COLOR_NORMAL, true, true, HEADER_SIZE);

    printMasterSummary();

    //appendText(L"\nFolder Status\n", TEXT_COLOR_NORMAL, true, true, HEADER_SIZE);
    auto folders = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
    for (size_t i = 0; i < folders.size(); i++) {
        AccountGroup* folder = dynamic_cast<AccountGroup*>(folders[i]);
        if (folder->getReadOnly()) {
            continue;
        }

        int iMonthlySpend = folder->getCurrentMonthTransacationTotal();
        int iMonthlyBudget = folder->budgetedToSubAccounts();
        int iMonthlyLimit = folder->getExpenseLimit();

        wchar_t szBuff[1024] = { 0 };

        bool isError = iMonthlyBudget > iMonthlySpend;
        const wchar_t* szPreposition = isError ? L"over" : L"under";
        COLORREF cr = isError ? TEXT_COLOR_NEGATIVE : TEXT_COLOR_NORMAL;

        if (isError) {
            swprintf(szBuff, 1024, L"Folder '%ls' - Monthly expenses are %ls budget by %ls\n\n", folder->getName(), szPreposition,
                Utils::penniesToDollarsString(iMonthlyBudget - iMonthlySpend).c_str());
            appendText(szBuff, cr, false, false, TEXT_SIZE);
        }


        if (iMonthlyLimit) {
            isError = iMonthlyLimit > iMonthlySpend;
            szPreposition = isError ? L"over" : L"under";
            cr = isError ? TEXT_COLOR_NEGATIVE : TEXT_COLOR_NORMAL;

            if (isError) {
                swprintf(szBuff, 1024, L"Folder '%ls' - Monthly expenses are %ls limit by %ls\n\n", folder->getName(), szPreposition,
                    Utils::penniesToDollarsString(iMonthlyLimit - iMonthlySpend).c_str());
                appendText(szBuff, cr, false, false, TEXT_SIZE);
            }

            isError = iMonthlyLimit > iMonthlyBudget;
            szPreposition = isError ? L"over" : L"under";
            cr = isError ? TEXT_COLOR_NEGATIVE : TEXT_COLOR_NORMAL;

            if (isError) {
                swprintf(szBuff, 1024, L"Folder '%ls' - Monthly budget is %ls limit by %ls\n\n", folder->getName(), szPreposition,
                    Utils::penniesToDollarsString(iMonthlyLimit - iMonthlyBudget).c_str());
                appendText(szBuff, cr, false, false, TEXT_SIZE);
            }
        }
    }

    //appendText(L"\nCategory Status\n", TEXT_COLOR_NORMAL, true, true, HEADER_SIZE);
    auto categories = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
    for (size_t i = 0; i < categories.size(); i++)
    {
        AccountCategory* category = dynamic_cast<AccountCategory*>(categories[i]);
        if (category->getId() == ACCOUNT_ID_PAY_CC) {
            continue;
        }

        int ibudgeted = category->getBudgeted();
        int spent = category->getCurrentMonthTransacationTotal();
        std::wstring diff = Utils::penniesToDollarsString (abs(ibudgeted - spent));
        bool error = ibudgeted > spent;
        wchar_t szBuff[1024] = { 0 };
        if (error) {
            swprintf(szBuff, 1024, L"Category '%ls' - Monthly expenses are over budget by %ls\n\n", category->getName(), diff.c_str());
            appendText(szBuff, error ? TEXT_COLOR_NEGATIVE : TEXT_COLOR_NORMAL, false, false, TEXT_SIZE);
        }
        //else {
        //    swprintf(szBuff, 1024, L"%ls - Monthly expenses are currently under budget by %ls\n", category->getName(), diff.c_str());
        //}
    }

}

DWORD CALLBACK AccountInfoDlg::EditStreamCallbackOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    AccountInfoDlg* pThis = (AccountInfoDlg*)dwCookie;

    pThis->_textBuff.insert(pThis->_textBuff.end(), pbBuff, pbBuff + cb);
    *pcb = cb;

    return 0;
}

DWORD CALLBACK AccountInfoDlg::EditStreamCallbackIn(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    AccountInfoDlg* pThis = (AccountInfoDlg*)dwCookie;

    size_t toCopy = min(pThis->_textBuff.size(), (size_t)cb);

    if (pThis->_textBuff.size()) {
        unsigned char* pBuff = &pThis->_textBuff[0];
        memcpy(pbBuff, pBuff, toCopy);
        *pcb = (LONG)toCopy;
    }

    pThis->_textBuff.erase(pThis->_textBuff.begin(), pThis->_textBuff.begin() + toCopy);

    return 0;
}

void AccountInfoDlg::saveText()
{
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    EDITSTREAM es = { 0 };
    es.dwCookie = (DWORD_PTR)this;
    es.pfnCallback = AccountInfoDlg::EditStreamCallbackOut;

    SendMessage(hRichText, EM_STREAMOUT, SF_RTF, (LPARAM) & es);
}

void AccountInfoDlg::restoreText()
{
    HWND hRichText = GetDlgItem(_hThisDialg, IDC_RICHEDIT_SUMMARY);
    EDITSTREAM es = { 0 };
    es.dwCookie = (DWORD_PTR)this;
    es.pfnCallback = AccountInfoDlg::EditStreamCallbackIn;

    SendMessage(hRichText, EM_STREAMIN, SF_RTF, (LPARAM)&es);
}

