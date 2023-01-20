#include "DepositDlg.h"
#include "resource.h"
#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

DepositDlg::DepositDlg() : _hParent(nullptr), _hThisDialg(nullptr)
{

}

DepositDlg::~DepositDlg()
{

}

bool DepositDlg::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DEPOSIT_DIALOG), g_hTopWindow, DepositDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

INT_PTR CALLBACK DepositDlg::DepositDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DepositDlg* pThis = (DepositDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (pThis) {
            // Accounts
        }

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


DepositDlg* DepositDlg::PTHIS = nullptr;
