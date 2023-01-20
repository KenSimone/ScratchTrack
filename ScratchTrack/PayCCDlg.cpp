#include "PayCCDlg.h"
#include "resource.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

PayCCDlg::PayCCDlg() : _hParent(nullptr), _hThisDialg(nullptr)
{

}

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
    PayCCDlg* pThis = (PayCCDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

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

PayCCDlg* PayCCDlg::PTHIS = nullptr;
