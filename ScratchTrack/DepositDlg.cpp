#include "DepositDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"
#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

DepositDlg::DepositDlg(Account* pAccount, int amount, const wchar_t* szPayee, const wchar_t* szDescription) 
    : _pAccount(pAccount), _amount(amount)
{
    if (szPayee) {
        _payee = szPayee;
    }

    if (szDescription) {
        _description = szDescription;
    }

}

DepositDlg::~DepositDlg()
{

}

void DepositDlg::enableDisableUi(HWND hDlg)
{

    bool enableOk = true;
    if (Utils::isDlgItemTextEmpty(hDlg, IDC_COMBO_ACCOUNT)) {
        enableOk = false;
    }

    EnableWindow(GetDlgItem(hDlg, IDOK), enableOk);
}

bool DepositDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Deposits\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"Money can be deposited via the deposit action. A deposit can be edited afer it's created through the transaction list.",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DEPOSIT_DIALOG), g_hTopWindow, DepositDlgProc);
    PTHIS = nullptr;
    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

INT_PTR CALLBACK DepositDlg::DepositDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    //IDC_COMBO_PAYEE

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        // Accounts
        Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT, PTHIS->_pAccount->getId(), true);
        SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, PTHIS->_description.c_str());
        Utils::setDlgItemDollarText(hDlg, IDC_EDIT_DEPOSIT_AMOUNT, PTHIS->_amount);
        Utils::populatePayeeCombo(hDlg, IDC_COMBO_PAYEE, PTHIS->_payee.c_str());
        PTHIS->enableDisableUi(hDlg);

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
        {
            PTHIS->enableDisableUi(hDlg);
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                PTHIS->_amount = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_DEPOSIT_AMOUNT);
                PTHIS->_description = Utils::getDlgItemText(hDlg, IDC_EDIT_DESCRIPTION);
                PTHIS->_payee = Utils::getDlgItemText(hDlg, IDC_COMBO_PAYEE);

                int id = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT);
                PTHIS->_pAccount = (Account*)AccountBase::lookupById(id);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


DepositDlg* DepositDlg::PTHIS = nullptr;
