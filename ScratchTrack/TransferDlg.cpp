#include "TransferDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Utils.h"
#include "AccountInfoDlg.h"
#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

TransferDlg::TransferDlg(int accountFromId, int accountToId, int amount, const wchar_t* szDescription /*= nullptr*/) :
    _accountFromId(accountFromId), _accountToId(accountToId), _amount(amount)
{
    if (szDescription) {
        _description = szDescription;
    }

}

TransferDlg::~TransferDlg()
{

}

bool TransferDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Transfers\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"Money can be moved from one account to another via transfer. A transfer can be edited afer it's created through the transaction list.",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_TRANSFER_DIALOG), g_hTopWindow, TransferDlgProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

bool TransferDlg::confirmUnusualSettings(HWND hDlg)
{
    bool ret = true;
    // Make sure we have money in the from account
    int fromId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT);
    int toId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT_TO);

    AccountBase* pFrom = AccountBase::lookupById(fromId);
    AccountBase* pTo = AccountBase::lookupById(toId);

    int amount = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_AMOUNT);

    if (pFrom->getBalance() < amount) {
        wchar_t szBuff[1024] = { 0 };
        swprintf(szBuff, 1024, L"Account '%ls' has a balance of %ls and the transfer amount is %ls. This original account's resulting balance will be negative. Proceed?", 
            pFrom->getName(), Utils::penniesToDollarsString(pFrom->getBalance()).c_str(), Utils::penniesToDollarsString(amount).c_str());

        ret = MessageBox(hDlg, szBuff, L"Negative Transfer", MB_YESNO | MB_ICONWARNING) == IDYES;
    }
    return ret;
}

void TransferDlg::validateSelections(HWND hDlg)
{
    // Ensure the two accounts are not the same. If they are, disable OK.
    int fromId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT);
    int toId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT_TO);

    EnableWindow(GetDlgItem(hDlg, IDOK), fromId != toId);
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_WARN), fromId != toId ? SW_HIDE : SW_SHOW);

}

INT_PTR CALLBACK TransferDlg::TransferDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        Utils::setDlgItemDollarText(hDlg, IDC_EDIT_AMOUNT, PTHIS->_amount);
        Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT, PTHIS->getAccountFromId(), true);
        Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT_TO, PTHIS->getAccountToId(), true, PTHIS->getAccountFromId());
        SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, PTHIS->getDescription());
        PTHIS->validateSelections(hDlg);

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            PTHIS->validateSelections(hDlg);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            bool closeIt = true;
            if (LOWORD(wParam) == IDOK)
            {
                closeIt = PTHIS->confirmUnusualSettings(hDlg);
                if (closeIt) {
                    PTHIS->_amount = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_AMOUNT);
                    PTHIS->_description = Utils::getDlgItemText(hDlg, IDC_EDIT_DESCRIPTION);
                    PTHIS->_accountFromId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT);
                    PTHIS->_accountToId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT_TO);
                }
            }

            if (closeIt) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

TransferDlg* TransferDlg::PTHIS = nullptr;
