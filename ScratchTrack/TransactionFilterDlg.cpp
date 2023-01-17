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
    //switch (Message)
    //{
    //case WM_COMMAND:
    //    switch (LOWORD(wParam))
    //default:
    //    return FALSE;
    //}
    //return TRUE;
}

bool TransactionFilterDlg::filterToEnvelopeSelection()
{
    return SendDlgItemMessage(_hThisDialg, IDC_CHECK_FILTER_TO_ENVELOPE, BM_GETCHECK, 0, 0);
}

std::wstring TransactionFilterDlg::filterText()
{
    wchar_t szBuffer[1024] = { 0 };
    GetWindowText(GetDlgItem(_hThisDialg, IDC_EDIT_TRANSACTION_FILTER), szBuffer, 1024);
    return std::wstring(szBuffer);
}
