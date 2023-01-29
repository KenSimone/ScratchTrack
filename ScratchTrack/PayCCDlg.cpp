#include "PayCCDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

PayCCDlg::~PayCCDlg()
{

}

bool PayCCDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Credit Card Payments\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"Credit cards are paid from another account and are tracked through the matching payment bucket that was "
        L"automatically created when the account was created. The credit card workflow is a bit involved and is a work in progress. "
        L"When an expense is recorded against a credit card account, money is unassigned from the original expense bucket, and assigned to "
        L"the corresponding credit card payment bucket. When the credit card is paid, money is unassigned from the matching CC payment bucket.",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_PAYCC_DIALOG), g_hTopWindow, PayCCDlgProc);
    PTHIS = nullptr;
    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

INT_PTR CALLBACK PayCCDlg::PayCCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (PayCCDlg::PTHIS) {

            Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT_FROM, PTHIS->_payFromAcctId, true);

            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT_AMOUNT);
            std::wstring amt = Utils::penniesToDollarsString(abs(PayCCDlg::PTHIS->_amount));
            SetWindowText(hWnd, amt.c_str());

            hWnd = GetDlgItem(hDlg, IDC_EDIT_ACCOUNT);
            SetWindowTextW(hWnd, PTHIS->_pAccount->getName());

            hWnd = GetDlgItem(hDlg, IDC_EDIT_BUCKET);
            int bucketId = PTHIS->_pAccount->paidFromBucketId();
            AccountBase* pBucket = AccountBase::lookupById(bucketId);
            SetWindowTextW(hWnd, pBucket->getName());

            hWnd = GetDlgItem(hDlg, IDC_EDIT_DESCRIPTION);
            SetWindowTextW(hWnd, PTHIS->_description.c_str());
        }

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                wchar_t szBuff[1024] = { 0 };
                HWND hWnd = GetDlgItem(hDlg, IDC_EDIT_AMOUNT);
                GetWindowText(hWnd, szBuff, 1024);
                PTHIS->_amount = -1 * abs(Utils::dollarsStringToPennies(szBuff));

                hWnd = GetDlgItem(hDlg, IDC_EDIT_DESCRIPTION);
                GetWindowTextW(hWnd, szBuff, 1024);
                PTHIS->_description = szBuff;
                PTHIS->_payFromAcctId = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT_FROM);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

PayCCDlg* PayCCDlg::PTHIS = nullptr;
