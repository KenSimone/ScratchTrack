#include "TransferDlg.h"
#include "resource.h"
#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

TransferDlg::TransferDlg() : _hParent(nullptr), _hThisDialg(nullptr)
{

}

TransferDlg::~TransferDlg()
{

}

bool TransferDlg::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DEPOSIT_DIALOG), g_hTopWindow, TransferDlgProc);
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

TransferDlg* TransferDlg::PTHIS = nullptr;
