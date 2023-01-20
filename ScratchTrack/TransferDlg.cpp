#include "TransferDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Utils.h"
#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

TransferDlg::TransferDlg(int accountToId, int accountFromId, int amount, const wchar_t* szDescription /*= nullptr*/) :
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
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_TRANSFER_DIALOG), g_hTopWindow, TransferDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

INT_PTR CALLBACK TransferDlg::TransferDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TransferDlg* pThis = (TransferDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        Utils::setDlgItemDollarText(hDlg, IDC_EDIT_AMOUNT, PTHIS->_amount);
        Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT, PTHIS->getAccountFromId());
        Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT_TO, PTHIS->getAccountToId());
        SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, PTHIS->getDescription());

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                PTHIS->_amount = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_AMOUNT);
                PTHIS->_description = Utils::getDlgItemText(hDlg, IDC_EDIT_DESCRIPTION);
                PTHIS->_accountFromId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT);
                PTHIS->_accountToId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT_TO);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

TransferDlg* TransferDlg::PTHIS = nullptr;
