#include "IncomeDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

void IncomeDlg::enableDisableUi(HWND hDlg)
{
    bool enableOk = true;
    if (Utils::isDlgItemTextEmpty(hDlg, IDC_EDIT_INCOME_NAME)) {
        enableOk = false;
    }

    EnableWindow(GetDlgItem(hDlg, IDOK), enableOk);
}

bool IncomeDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Incomes\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"In ScratchTrack funds are assigned to spending categories. When expenses are recorded in a expense, funds in that expense are reduced by the epxense amount. "
        L"When a expense is out of money, spending in that expense should stop until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Incomes help organize categories. One approach is to create incomes for each monthly income event. If you're paid twice a month, create "
        L"incomes titled 'paycheck1' and 'paycheck2'. "
        L"Use the paycheck to fund the categories in the income when it arrives.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"You can optionally set expense limits for incomes. This helps organize "
        L"the budget. If your incomes are aligned with your paychecks, set the expense limit to your paycheck amount. "
        L"As categories are added to the income, you'll be able to see how close you are to the limit.", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INCOME_DIALOG), g_hTopWindow, IncomeDlgProc);
    PTHIS = nullptr;
    g_accountInfoDlg.restoreText();
    return result == IDOK;
}

INT_PTR CALLBACK IncomeDlg::IncomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (IncomeDlg::PTHIS) {
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_EXPENSE_LIMIT, abs(PTHIS->_limit));
            SetDlgItemText(hDlg, IDC_EDIT_INCOME_NAME, PTHIS->_name.c_str());
        }

        PTHIS->enableDisableUi(hDlg);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE)
        {
            PTHIS->enableDisableUi(hDlg);
        }

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                PTHIS->_limit = -Utils::getDlgItemDollarText(hDlg, IDC_EDIT_EXPENSE_LIMIT);
                PTHIS->_name = Utils::getDlgItemText(hDlg, IDC_EDIT_INCOME_NAME);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

IncomeDlg* IncomeDlg::PTHIS = nullptr;
