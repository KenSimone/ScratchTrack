#include "FolderDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

void FolderDlg::enableDisableUi(HWND hDlg)
{
    bool enableOk = true;
    if (Utils::isDlgItemTextEmpty(hDlg, IDC_EDIT_FOLDER_NAME)) {
        enableOk = false;
    }

    EnableWindow(GetDlgItem(hDlg, IDOK), enableOk);
}

bool FolderDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Folders\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"ScratchTrack follows the 'Category Budgeting' philosophy. The idea is that money is assigned to separate categories for different spending categories. Expenses are paid from the categories. "
        L"When a category is empty, spending in that category stops until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Folders help organize categories. One approach is to create folders for each monthly income event. If you're paid twice a month, create "
        L"folders titled 'paycheck1' and 'paycheck2'. "
        L"Use the paycheck to fill the categories in the folder when it arrives.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"You can optionally set expense limits for folders. This helps organize "
        L"your budget. If your folders are aligned with your paychecks, set the expense limit to your paycheck amount. "
        L"As categories are added to the folder, you'll be able to see how close you are to the limit in this window.", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FOLDER_DIALOG), g_hTopWindow, FolderDlgProc);
    PTHIS = nullptr;
    g_accountInfoDlg.restoreText();
    return result == IDOK;
}

INT_PTR CALLBACK FolderDlg::FolderDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (FolderDlg::PTHIS) {
            Utils::setDlgItemDollarText(hDlg, IDC_EDIT_EXPENSE_LIMIT, PTHIS->_limit);
            SetDlgItemText(hDlg, IDC_EDIT_FOLDER_NAME, PTHIS->_name.c_str());
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
                PTHIS->_limit = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_EXPENSE_LIMIT);
                PTHIS->_name = Utils::getDlgItemText(hDlg, IDC_EDIT_FOLDER_NAME);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

FolderDlg* FolderDlg::PTHIS = nullptr;
