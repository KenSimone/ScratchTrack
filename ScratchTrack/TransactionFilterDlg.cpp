#include "TransactionFilterDlg.h"
#include "resource.h"


extern HINSTANCE g_hInst;

TransactionFilterDlg::TransactionFilterDlg() : _hParent(nullptr), _hThisDialg(nullptr)
{

}

TransactionFilterDlg::~TransactionFilterDlg()
{
}

HWND TransactionFilterDlg::create(HWND hParent)
{
    _hParent = hParent;
    _hThisDialg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_TRANSACTION_FILTER_DIALOG), _hParent, TransactionFilterDlgProc);
    SetWindowLongPtr(_hThisDialg, GWLP_USERDATA, (LONG_PTR )this);
    return _hThisDialg;
}

INT_PTR CALLBACK TransactionFilterDlg::TransactionFilterDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        return SendDlgItemMessage(hwnd, IDC_CHECK_SHOW_TRANSACTION_LIST, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            TransactionFilterDlg* pThis = (TransactionFilterDlg * )GetWindowLongPtr(hwnd, GWLP_USERDATA);

            NMHDR hdr = { 0 };
            hdr.code = NM_TRANS_DIALOG_FILTER_CHANGED;
            hdr.idFrom = IDD_TRANSACTION_FILTER_DIALOG;
            SendMessage(pThis->_hParent, WM_NOTIFY, 0, (LPARAM) &hdr);
        }
    }
    return false;
}

bool TransactionFilterDlg::filterToExpenseSelection()
{
    return SendDlgItemMessage(_hThisDialg, IDC_CHECK_FILTER_TO_EXPENSE, BM_GETCHECK, 0, 0);
}

std::wstring TransactionFilterDlg::filterText()
{
    wchar_t szBuffer[1024] = { 0 };
    GetWindowText(GetDlgItem(_hThisDialg, IDC_EDIT_TRANSACTION_FILTER), szBuffer, 1024);
    return std::wstring(szBuffer);
}

bool TransactionFilterDlg::showTransactionList()
{
    return SendDlgItemMessage(_hThisDialg, IDC_CHECK_SHOW_TRANSACTION_LIST, BM_GETCHECK, 0, 0);
}

bool TransactionFilterDlg::filterToAccount()
{
    return SendDlgItemMessage(_hThisDialg, IDC_CHECK_FILTER_TO_ACCOUNT, BM_GETCHECK, 0, 0);
}
