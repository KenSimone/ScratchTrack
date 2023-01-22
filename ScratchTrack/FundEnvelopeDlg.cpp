#include "FundEnvelopeDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

bool FundEnvelopeDlg::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FILL_ENVELOPES_DIALOG), g_hTopWindow, FundEnvelopeDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

void FundEnvelopeDlg::calcNewBalance(HWND hDlg)
{
    _newBalance = 0;
    if (SendDlgItemMessage(hDlg, IDC_ADD_BUDGETED_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = _pEnvelope->getBalance() + abs(_pEnvelope->getBudgeted());
    }
    else if (SendDlgItemMessage(hDlg, IDC_ADD_TO_TOP_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = abs(_pEnvelope->getBudgeted());
    }
    else if (SendDlgItemMessage(hDlg, IDC_SET_ANY_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_SET_ARBITRARY);
    }

}

void FundEnvelopeDlg::updateUi(HWND hDlg)
{
    calcNewBalance(hDlg);
    Utils::setDlgItemDollarText(hDlg, IDC_EDIT_NEW_BALANCE, _newBalance);

}

INT_PTR CALLBACK FundEnvelopeDlg::FundEnvelopeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (FundEnvelopeDlg::PTHIS) {
            AccountEnvelope* pEnvelope = PTHIS->_pEnvelope;

            
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_ADD_BUDGETED, abs(pEnvelope->getBudgeted()));
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_FILL_TO_BUDGETED, abs(pEnvelope->getBudgeted()) - pEnvelope->getBalance());
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_SET_ARBITRARY, 0);

            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_CUR_BALANCE, pEnvelope->getBalance());

            if (pEnvelope->getBudgeted()){
                SendDlgItemMessage(hDlg, IDC_ADD_BUDGETED_RADIO, BM_SETCHECK, BST_CHECKED, 0);
            }
            else {
                SendDlgItemMessage(hDlg, IDC_SET_ANY_RADIO, BM_SETCHECK, BST_CHECKED, 0);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ADD_BUDGETED), false);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FILL_TO_BUDGETED), false);
            }
            PTHIS->updateUi(hDlg);
        }

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_EDIT_SET_ARBITRARY && HIWORD(wParam) == EN_CHANGE) {
            PTHIS->updateUi(hDlg);
        }
        if (LOWORD(wParam) == IDC_ADD_BUDGETED_RADIO || LOWORD(wParam) == IDC_ADD_TO_TOP_RADIO || LOWORD(wParam) == IDC_SET_ANY_RADIO) {
            PTHIS->updateUi(hDlg);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                PTHIS->calcNewBalance(hDlg);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

FundEnvelopeDlg* FundEnvelopeDlg::PTHIS = nullptr;
