#include "FundCategoryDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

bool FundCategoryDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Category Funding\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"In ScratchTrack funds are assigned to spending categories. When expenses are recorded in a category, funds in that category are reduced by the epxense amount. "
        L"When a category is out of money, spending in that category should stop until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"This dialog box allows you to assign money to a category. It's important to note that money stays in its account when assigned to a category. "
        L"When money is assigned, you'll have less budgetable dollars available to place in other categories, but no money actually moves. Think of categories as a way to view and organize "
        L"the money in your account(s).",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FILL_CATEGORIES_DIALOG), g_hTopWindow, FundCategoryDlgProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

void FundCategoryDlg::calcNewBalance(HWND hDlg)
{
    _newBalance = 0;
    if (SendDlgItemMessage(hDlg, IDC_ADD_BUDGETED_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = _pCategory->getBalance() + abs(_pCategory->getBudgeted());
    }
    else if (SendDlgItemMessage(hDlg, IDC_ADD_TO_TOP_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = abs(_pCategory->getBudgeted());
    }
    else if (SendDlgItemMessage(hDlg, IDC_SET_ANY_RADIO, BM_GETCHECK, 0, 0))
    {
        _newBalance = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_SET_ARBITRARY);
    }

}

void FundCategoryDlg::updateUi(HWND hDlg)
{
    calcNewBalance(hDlg);
    Utils::setDlgItemDollarText(hDlg, IDC_EDIT_NEW_BALANCE, _newBalance);

}

INT_PTR CALLBACK FundCategoryDlg::FundCategoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (FundCategoryDlg::PTHIS) {
            AccountCategory* pCategory = PTHIS->_pCategory;

            
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_ADD_BUDGETED, abs(pCategory->getBudgeted()));
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_FILL_TO_BUDGETED, abs(pCategory->getBudgeted()) - pCategory->getBalance());
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_SET_ARBITRARY, 0);

            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_CUR_BALANCE, pCategory->getBalance());

            if (pCategory->getBudgeted()){
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

FundCategoryDlg* FundCategoryDlg::PTHIS = nullptr;
