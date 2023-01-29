#include "FundBucketDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

bool FundBucketDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Bucket Funding\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"ScratchTrack follows the 'Bucket Budgeting' philosophy. The idea is that money is assigned to separate buckets for different spending categories. Expenses are paid from the buckets. "
        L"When an bucket is empty, spending in that category stops until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"This dialog box allows you to assign money to an bucket. It's important to note that money stays in its account when assigned to an bucket. "
        L"When money is assigned, you'll have less budgetable dollars available to place in other buckets, but no money actually moves. Think of buckets as a way to organize "
        L"the money in your account(s).",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FILL_BUCKETS_DIALOG), g_hTopWindow, FundBucketDlgProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

void FundBucketDlg::calcNewBalance(HWND hDlg)
{
    _newBalance = 0;
    if (SendDlgItemMessage(hDlg, IDC_ADD_BUDGETED_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = _pBucket->getBalance() + abs(_pBucket->getBudgeted());
    }
    else if (SendDlgItemMessage(hDlg, IDC_ADD_TO_TOP_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = abs(_pBucket->getBudgeted());
    }
    else if (SendDlgItemMessage(hDlg, IDC_SET_ANY_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_SET_ARBITRARY);
    }

}

void FundBucketDlg::updateUi(HWND hDlg)
{
    calcNewBalance(hDlg);
    Utils::setDlgItemDollarText(hDlg, IDC_EDIT_NEW_BALANCE, _newBalance);

}

INT_PTR CALLBACK FundBucketDlg::FundBucketDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (FundBucketDlg::PTHIS) {
            AccountBucket* pBucket = PTHIS->_pBucket;

            
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_ADD_BUDGETED, abs(pBucket->getBudgeted()));
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_FILL_TO_BUDGETED, abs(pBucket->getBudgeted()) - pBucket->getBalance());
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_SET_ARBITRARY, 0);

            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_CUR_BALANCE, pBucket->getBalance());

            if (pBucket->getBudgeted()){
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

FundBucketDlg* FundBucketDlg::PTHIS = nullptr;
