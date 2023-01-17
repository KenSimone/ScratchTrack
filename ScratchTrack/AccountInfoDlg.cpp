#include "AccountInfoDlg.h"
#include "resource.h"


extern HINSTANCE g_hInst;

AccountInfoDlg::AccountInfoDlg() : _hParent(nullptr), _hThisDialg(nullptr)
{

}

AccountInfoDlg::~AccountInfoDlg()
{
}

HWND AccountInfoDlg::create(HWND hParent)
{
    _hParent = hParent;
    _hThisDialg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BUDGET_INFO_DIALOG), _hParent, AccountInfoDlgProc);
    SetWindowLongPtr(_hThisDialg, GWLP_USERDATA, (LONG_PTR)this);
    return _hThisDialg;
}

INT_PTR CALLBACK AccountInfoDlg::AccountInfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            AccountInfoDlg* pThis = (AccountInfoDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            UINT message = 0;
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_ADD_ACCOUNT:
                message = NM_ACCOUNT_INFO_DIALOG_ADD_ACCOUNT;
                break;
            case IDC_BUTTON_ADD_FOLDER:
                message = NM_ACCOUNT_DIALOG_ADD_FOLDER;
                break;
            }

            if (message) {
                NMHDR hdr = { 0 };
                hdr.code = message;
                hdr.idFrom = IDD_BUDGET_INFO_DIALOG;
                SendMessage(pThis->_hParent, WM_NOTIFY, 0, (LPARAM)&hdr);
            }
        }
    }
    return false;

}

