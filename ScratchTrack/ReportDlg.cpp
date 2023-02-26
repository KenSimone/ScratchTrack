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
extern void setGuiFont(HWND hCtrl);

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
            unsigned int graphStyle = (unsigned int)ComboBox_GetItemData(GetDlgItem(hDlg, IDC_COMBO_STYLE), ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBO_STYLE)));
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC_REPORT), graphStyle != GRAPH_STYLE_GRID);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST_REPORT), graphStyle == GRAPH_STYLE_GRID);

            InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_REPORT), nullptr, false);
            PTHIS->populateGrid();
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_SIZE:
        InvalidateRect(hDlg, nullptr, false);
        PTHIS->positionConrtols();
        break;

    }
    return (INT_PTR)FALSE;
}

ReportDlg* ReportDlg::PTHIS = nullptr;

std::map<unsigned int, int> ReportDlg::getEnvelopeIdsToTotalSpentMap(int* pElapsedDays, int* pElapsedMonths)
{
    HWND hMonthCombo = GetDlgItem(_hDlg, IDC_COMBO_MONTH);
    HWND hYearCombo = GetDlgItem(_hDlg, IDC_COMBO_YEAR);

    int month = (int)ComboBox_GetCurSel(hMonthCombo);
    int year = (int)ComboBox_GetCurSel(hYearCombo);

    month = (int)ComboBox_GetItemData(hMonthCombo, month);
    year = (int)ComboBox_GetItemData(hYearCombo, year);



    std::vector<Transaction*> transactions = Transaction::getAllTransactions();

    std::map<unsigned int, int> mapEnvIdToSpend;

    SYSTEMTIME oldest = { 0 }, newest = { 0 };

    // Newest is now
    GetLocalTime(&newest);

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

            if (oldest.wYear == 0) {
                oldest = ts;
            }
            else  {
                if (Utils::compareDates(oldest, ts) > 0) {
                    oldest = ts;
                }
            }

            auto mapitr = mapEnvIdToSpend.find(pTrans->getEnvelopeId());
            if (mapitr != mapEnvIdToSpend.end()) {
                mapitr->second += pTrans->getAmount();
            }
            else {
                mapEnvIdToSpend[pTrans->getEnvelopeId()] = pTrans->getAmount();
            }
        }
    }

    if (pElapsedMonths) {
        if (newest.wYear != oldest.wYear) {
            // This gets a little tricky
            *pElapsedMonths = (12 - oldest.wMonth)+1;
            int elapsedFullYears = newest.wYear - oldest.wYear - 1;
            *pElapsedMonths += elapsedFullYears * 12;
            *pElapsedMonths += newest.wMonth;
        }
        else {
            *pElapsedMonths = (newest.wMonth - oldest.wMonth) + 1;
        }

    }

    if (oldest.wYear == 0) {
        oldest = newest;
    }
    int elapsedDays = Utils::daysDiff(oldest, newest);
    if (pElapsedDays) {
        *pElapsedDays = elapsedDays;
    }

    if (elapsedDays <= 60 && !_displayed30dayWarn) {
        _displayed30dayWarn = true;
        wchar_t szBuff[1024] = { 0 };
        swprintf(szBuff, 1024, L"There are currently %d day(s) of data. Reports are only meaningful after two months of data.", elapsedDays);
        MessageBox(_hDlg, szBuff, L"Not enough data", MB_OK);
    }

    return mapEnvIdToSpend;
}

void ReportDlg::paintExpenseByCategoryReport(HWND hWnd)
{
    auto mapEnvIdToSpend = getEnvelopeIdsToTotalSpentMap(nullptr, nullptr);

    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);

    GdiBarChart bc;

    for (auto itr = mapEnvIdToSpend.begin(); itr != mapEnvIdToSpend.end(); itr++) {
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
    // Style combo
    HWND hStyleCombo = GetDlgItem(_hDlg, IDC_COMBO_STYLE);
    int iPos = (int)SendMessage(hStyleCombo, CB_ADDSTRING, 0, (LPARAM)L"Graph");
    SendMessage(hStyleCombo, CB_SETITEMDATA, iPos, GRAPH_STYLE_BAR);
    ComboBox_SetCurSel(hStyleCombo, iPos);

    iPos = (int)SendMessage(hStyleCombo, CB_ADDSTRING, 0, (LPARAM)L"Grid");
    SendMessage(hStyleCombo, CB_SETITEMDATA, iPos, GRAPH_STYLE_GRID);

    // Report combo
    HWND hReportCombo = GetDlgItem(_hDlg, IDC_COMBO_REPORT);
    iPos = (int)SendMessage(hReportCombo, CB_ADDSTRING, 0, (LPARAM)L"Expenses by date");
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
        iPos = (int)SendMessage(hYearCombo, CB_ADDSTRING, 0, (LPARAM)szTemp);
        SendMessage(hYearCombo, CB_SETITEMDATA, iPos, i);
    }

    iPos = (int)SendMessage(hYearCombo, CB_ADDSTRING, 0, (LPARAM)L"All Years");
    ComboBox_SetCurSel(hYearCombo, iPos);
    SendMessage(hYearCombo, CB_SETITEMDATA, iPos, -1);

    unsigned int graphStyle = (unsigned int)ComboBox_GetItemData(GetDlgItem(_hDlg, IDC_COMBO_STYLE), ComboBox_GetCurSel(GetDlgItem(_hDlg, IDC_COMBO_STYLE)));
    ListView_SetExtendedListViewStyle(GetDlgItem(_hDlg, IDC_LIST_REPORT), LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    setGuiFont(GetDlgItem(_hDlg, IDC_LIST_REPORT));

    ShowWindow(GetDlgItem(_hDlg, IDC_STATIC_REPORT), graphStyle != GRAPH_STYLE_GRID);
    ShowWindow(GetDlgItem(_hDlg, IDC_LIST_REPORT), graphStyle == GRAPH_STYLE_GRID);


    PTHIS->positionConrtols();
}

void ReportDlg::positionConrtols()
{
    HWND hStaticReport = GetDlgItem(_hDlg, IDC_STATIC_REPORT);
    HWND hListReport = GetDlgItem(_hDlg, IDC_LIST_REPORT);

    RECT rMain, rStatic;

    GetClientRect(_hDlg, &rMain);
    GetClientRect(hStaticReport, &rStatic);

    POINT ptTopRight;
    ptTopRight.x = rStatic.left;
    ptTopRight.y = rStatic.top;
    ClientToScreen(hStaticReport, &ptTopRight);
    ScreenToClient(_hDlg, &ptTopRight);

    POINT ptBottomRight;
    ptBottomRight.x = (rMain.right - ptTopRight.x) - ptTopRight.x;
    ptBottomRight.y = (rMain.bottom - ptTopRight.y) - ptTopRight.x;

    SetWindowPos(hStaticReport, nullptr, ptTopRight.x, ptTopRight.y, ptBottomRight.x, ptBottomRight.y, SWP_NOZORDER);
    SetWindowPos(hListReport, nullptr, ptTopRight.x, ptTopRight.y, ptBottomRight.x, ptBottomRight.y, SWP_NOZORDER);
}

void ReportDlg::populateGrid()
{
    // Listview macros: https://learn.microsoft.com/en-us/windows/win32/controls/bumper-list-view-control-reference-macros
    // Header macros: https://learn.microsoft.com/en-us/windows/win32/controls/bumper-header-control-reference-macros

    HWND hListControl = GetDlgItem(_hDlg, IDC_LIST_REPORT);
    HWND hHeader = ListView_GetHeader(hListControl);

    // Clear header items
    ListView_DeleteAllItems(hListControl);
    while (Header_GetItemCount(hHeader)) {
        ListView_DeleteColumn(hListControl, 0);
    }

    // Add header items
    LVCOLUMN lvCol = { 0 };
    lvCol.mask = LVCF_TEXT;

    lvCol.pszText = const_cast<wchar_t*>(L"Envelope");
    ListView_InsertColumn(hListControl, MAXINT, &lvCol);

    lvCol.pszText = const_cast<wchar_t*>(L"Budgeted");
    ListView_InsertColumn(hListControl, MAXINT, &lvCol);

    lvCol.pszText = const_cast<wchar_t*>(L"Avg per day");
    ListView_InsertColumn(hListControl, MAXINT, &lvCol);

    lvCol.pszText = const_cast<wchar_t*>(L"Avg per 30 days");
    ListView_InsertColumn(hListControl, MAXINT, &lvCol);

    HWND hMonthCombo = GetDlgItem(_hDlg, IDC_COMBO_MONTH);
    HWND hYearCombo = GetDlgItem(_hDlg, IDC_COMBO_YEAR);

    int month = (int)ComboBox_GetCurSel(hMonthCombo);
    int year = (int)ComboBox_GetCurSel(hYearCombo);

    month = (int)ComboBox_GetItemData(hMonthCombo, month);
    year = (int)ComboBox_GetItemData(hYearCombo, year);


    auto envelopes = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
    for (size_t i = 0; i < envelopes.size(); i++) {
        AccountEnvelope* pEnvelope = (AccountEnvelope*)envelopes[i];

        LVITEM item = { 0 };
        item.mask = LVIF_PARAM | LVIF_TEXT;
        item.lParam = (LPARAM)pEnvelope;
        item.pszText = const_cast<wchar_t*>(pEnvelope->getName());

        int elapsedDays = 0, monthlyAvg = 0;
        int dailyAvg = pEnvelope->getAverageDailySpend(elapsedDays, monthlyAvg, false, month, year);

        // Name
        int iItem = ListView_InsertItem(hListControl, &item);

        item.mask = LVIF_TEXT;
        int iSubItem = 0;

        // Budgeted
        std::wstring text = Utils::penniesToDollarsString(-pEnvelope->getBudgeted());
        item.pszText = const_cast<wchar_t*>(text.c_str());
        ListView_SetItemText(hListControl, iItem, ++iSubItem, const_cast<wchar_t*>(text.c_str()));

        // Avg per day
        text = Utils::penniesToDollarsString(-dailyAvg);
        item.pszText = const_cast<wchar_t*>(text.c_str());
        ListView_SetItemText(hListControl, iItem, ++iSubItem, const_cast<wchar_t*>(text.c_str()));

        // Avg per 30 days
        text = Utils::penniesToDollarsString(-monthlyAvg);
        item.pszText = const_cast<wchar_t*>(text.c_str());
        ListView_SetItemText(hListControl, iItem, ++iSubItem, const_cast<wchar_t*>(text.c_str()));
    }

    int nCols = (int)Header_GetItemCount(hHeader);
    for (int i = 0; i < nCols; i++) {
        ListView_SetColumnWidth(hListControl, i, LVSCW_AUTOSIZE_USEHEADER);
    }


}
