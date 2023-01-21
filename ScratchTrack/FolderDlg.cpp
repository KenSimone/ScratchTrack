#include "FolderDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

bool FolderDlg::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FOLDER_DIALOG), g_hTopWindow, FolderDlgProc);
    PTHIS = nullptr;

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

        return (INT_PTR)TRUE;
    case WM_COMMAND:
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
