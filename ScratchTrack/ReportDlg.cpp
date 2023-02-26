#include "ReportDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"
#include "Transaction.h"
#include "GdiChart.h"

#include <Windows.h>
#include <richedit.h>
#include <vector>
#include <map>
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

void ReportDlg::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_REPORT), g_hTopWindow, ReportDlgProc);
    PTHIS = nullptr;

}

INT_PTR CALLBACK ReportDlg::StaticReportProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_PAINT:
        // Do paint here.
        PTHIS->paintExpenseByCategoryReport(hWnd);
        break;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam); // Call Default Window Procedure.
}
INT_PTR CALLBACK ReportDlg::ReportDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        PTHIS->_hDlg = hDlg;
        //PTHIS->showExpenseByCategoryReport();
        HWND hReportWnd = GetDlgItem(hDlg, IDC_STATIC_REPORT);
        PTHIS->initUi();

        SetWindowLongPtr(hReportWnd, GWLP_WNDPROC, (LONG_PTR)StaticReportProc);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        { 
            InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_REPORT), nullptr, false);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_PAINT:
        int x = 6;
        break;
    }
    return (INT_PTR)FALSE;
}

ReportDlg* ReportDlg::PTHIS = nullptr;

void ReportDlg::paintExpenseByCategoryReport(HWND hWnd)
{
    HWND hReportCombo = GetDlgItem(_hDlg, IDC_COMBO_REPORT);
    HWND hMonthCombo = GetDlgItem(_hDlg, IDC_COMBO_MONTH);
    HWND hYearCombo = GetDlgItem(_hDlg, IDC_COMBO_YEAR);

    int month = (int)ComboBox_GetCurSel(hMonthCombo);
    int year = (int)ComboBox_GetCurSel(hYearCombo);

    month = (int)ComboBox_GetItemData(hMonthCombo, month);
    year = (int)ComboBox_GetItemData(hYearCombo, year);


    std::vector<Transaction*> transactions = Transaction::getAllTransactions();

    std::map<unsigned int, int> mapTransToSpend;

    for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
        Transaction* pTrans = *itr;

        if (pTrans->getType() == Transaction::EXPENSE) {
            SYSTEMTIME ts = pTrans->getTimestamp();

            if (year != -1 && year != ts.wYear) {
                continue;
            }

            if (month != -1 && month != ts.wMonth) {
                continue;
            }


            auto mapitr = mapTransToSpend.find(pTrans->getEnvelopeId());
            if (mapitr != mapTransToSpend.end()) {
                mapitr->second += pTrans->getAmount();
            }
            else {
                mapTransToSpend[pTrans->getEnvelopeId()] = pTrans->getAmount();
            }
        }
    }

    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);

    GdiBarChart bc;

    for (auto itr = mapTransToSpend.begin(); itr != mapTransToSpend.end(); itr++) {
        AccountBase* pEnv = AccountBase::lookupById(itr->first);
        if (pEnv) {
            bc.addItem(pEnv->getName(), -itr->second);
        }
    }

    bc.render(hWnd, hdc);
    
    EndPaint(hWnd, &ps);

}

void ReportDlg::initUi()
{
    // IDC_COMBO_REPORT
    // IDC_COMBO_MONTH
    // IDC_COMBO_YEAR

    // Report combo
    HWND hReportCombo = GetDlgItem(_hDlg, IDC_COMBO_REPORT);
    int iPos = (int)SendMessage(hReportCombo, CB_ADDSTRING, 0, (LPARAM)L"Expenses by date");
    ComboBox_SetCurSel(hReportCombo, iPos);

    // Month combo
    HWND hMonthCombo = GetDlgItem(_hDlg, IDC_COMBO_MONTH);
    wchar_t szTemp[1024] = { 0 };
    for (int i = 0; i < 12; i++){
        swprintf(szTemp, 1024, L"%d", i + 1);
        iPos = (int)SendMessage(hMonthCombo, CB_ADDSTRING, 0, (LPARAM)szTemp);
        SendMessage(hMonthCombo, CB_SETITEMDATA, iPos, i+1);
    }

    iPos = (int)SendMessage(hMonthCombo, CB_ADDSTRING, 0, (LPARAM)L"All Months");
    ComboBox_SetCurSel(hMonthCombo, iPos);
    SendMessage(hMonthCombo, CB_SETITEMDATA, iPos, -1);

    auto transactions = Transaction::getAllTransactions();

    SYSTEMTIME currentTime = { 0 };
    GetLocalTime(&currentTime);
    int yearMin = currentTime.wYear, yearMax = currentTime.wYear;
    for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
        SYSTEMTIME ts = (*itr)->getTimestamp();
        yearMin = min(yearMin, ts.wYear);
        yearMax = max(yearMax, ts.wYear);
    }

    HWND hYearCombo = GetDlgItem(_hDlg, IDC_COMBO_YEAR);
    for (int i = yearMin; i <= yearMax; i++) {
        swprintf(szTemp, 1024, L"%d", i);
        iPos = SendMessage(hYearCombo, CB_ADDSTRING, 0, (LPARAM)szTemp);
        SendMessage(hYearCombo, CB_SETITEMDATA, iPos, i);
    }

    iPos = (int)SendMessage(hYearCombo, CB_ADDSTRING, 0, (LPARAM)L"All Years");
    ComboBox_SetCurSel(hYearCombo, iPos);
    SendMessage(hYearCombo, CB_SETITEMDATA, iPos, -1);


}
