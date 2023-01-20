#include "PayCCDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

PayCCDlg::~PayCCDlg()
{

}

bool PayCCDlg::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_PAYCC_DIALOG), g_hTopWindow, PayCCDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

INT_PTR CALLBACK PayCCDlg::PayCCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (PayCCDlg::PTHIS) {

            Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT_FROM, PTHIS->_payFromAcctId);

            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT_AMOUNT);
            std::wstring amt = Utils::penniesToDollarsString(abs(PayCCDlg::PTHIS->_amount));
            SetWindowText(hWnd, amt.c_str());

            hWnd = GetDlgItem(hDlg, IDC_EDIT_ACCOUNT);
            SetWindowTextW(hWnd, PTHIS->_pAccount->getName());

            hWnd = GetDlgItem(hDlg, IDC_EDIT_ENVELOPE);
            int envelopeId = PTHIS->_pAccount->paidFromEnvelopeId();
            AccountBase* pEnvelope = AccountBase::lookupById(envelopeId);
            SetWindowTextW(hWnd, pEnvelope->getName());

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
