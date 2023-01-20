#include "AccountListViewUtils.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Utils.h"

#include <map>
#include <string>

// externs
extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern HWND g_hwndAccountLV ;

// Forward defines
void setGuiFont(HWND hCtrl);

// Globals
std::map<Account::ACTION_FLAG, RECT> g_actionRects;
static const wchar_t* szPay = L"Pay";
static const wchar_t* szDeposit = L"Deposit";
static const wchar_t* szTransfer = L"Transfer";

Account::ACTION_FLAG hitTestAccountList(POINT pt, Account** ppAccount /*= nullptr*/)
{
    LVHITTESTINFO hitInfo = { 0 };
    hitInfo.pt = pt;
    ListView_SubItemHitTestEx(g_hwndAccountLV, &hitInfo);
    Account::ACTION_FLAG ret = Account::ACTION_FLAG::NO_ACTION;
    Account* pAccount = nullptr;


    if (hitInfo.iSubItem == 2 && hitInfo.flags & LVHT_ONITEMLABEL)
    {
        LVITEM item = { 0 };
        item.iItem = hitInfo.iItem;
        item.mask = LVIF_PARAM;
        ListView_GetItem(g_hwndAccountLV, &item);
        pAccount = (Account*)item.lParam;
        if (pAccount)
        {
            bool hitPay = false, hitDeposit = false, hitTransfer = false;
            RECT rSubItem = { 0 };
            ListView_GetSubItemRect(g_hwndAccountLV, item.iItem, hitInfo.iSubItem, LVIR_BOUNDS, &rSubItem);

            RECT rDeposit = g_actionRects[Account::ACTION_FLAG::DEPOSIT];
            RECT rPay = g_actionRects[Account::ACTION_FLAG::PAY];
            RECT rTransfer = g_actionRects[Account::ACTION_FLAG::TRANSFER];

            // Top and bottom is set to current rect. We just care about left and right
            rTransfer.top = rPay.top = rDeposit.top = rSubItem.top;
            rTransfer.bottom = rPay.bottom = rDeposit.bottom = rSubItem.bottom;

            if (pAccount->paidFromEnvelopeId() != INVALID_ACCOUNT_ID)
            {
                if (pAccount->getActionFlags() & Account::ACTION_FLAG::PAY && PtInRect(&rPay, hitInfo.pt) && pAccount->paidFromEnvelopeId() != INVALID_ACCOUNT_ID)
                {
                    ret = Account::ACTION_FLAG::PAY;
                }
            }
            else
            {
                if (pAccount->getActionFlags() & Account::ACTION_FLAG::DEPOSIT && PtInRect(&rDeposit, hitInfo.pt))
                {
                    ret = Account::ACTION_FLAG::DEPOSIT;
                }
                else if (pAccount->getActionFlags() & Account::ACTION_FLAG::TRANSFER && PtInRect(&rTransfer, hitInfo.pt))
                {
                    ret = Account::ACTION_FLAG::TRANSFER;
                }
            }
        }
    }

    if (ppAccount){
        *ppAccount = pAccount;
    }

    return ret;
}


bool accountListHandleMouseMove(LONG xCur, LONG yCur)
{
    // Translate point
    POINT pt = { 0 };
    pt.x = xCur;
    pt.y = yCur;

    HCURSOR hCursor = nullptr;

    bool hit = hitTestAccountList(pt) != Account::ACTION_FLAG::NO_ACTION;
    hCursor = LoadCursor(nullptr, hit ? IDC_HAND : IDC_ARROW);

    if (hCursor) {
        SetCursor(hCursor);
    }

    return true;
}


LRESULT CALLBACK acctListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uMsg == WM_SETCURSOR)
    {
        POINT point = { 0 };
        GetCursorPos(&point);
        ScreenToClient(g_hwndAccountLV, &point);
        accountListHandleMouseMove(point.x, point.y);
        return true;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void calcActionRects(HDC hdc)
{
    // Figure out the total area
    RECT rSubItem = { 0 }, rTemp = { 0 };
    ListView_GetSubItemRect(g_hwndAccountLV, 0, 2, LVIR_BOUNDS, &rSubItem);

    // Credit card accounts
    SIZE size = { 0 };
    GetTextExtentPoint(hdc, szPay, (int)wcslen(szPay), &size);
    rTemp = rSubItem;
    rTemp.right = rSubItem.left + size.cx;
    g_actionRects[Account::ACTION_FLAG::PAY] = rTemp;

    // Regular accounts
    GetTextExtentPoint(hdc, szDeposit, (int)wcslen(szDeposit), &size);
    rTemp = rSubItem;
    rTemp.right = rSubItem.left + size.cx;
    g_actionRects[Account::ACTION_FLAG::DEPOSIT] = rTemp;

    std::wstring temp(szDeposit);
    temp += L"  ";
    SIZE sizePrefix = { 0 };
    GetTextExtentPoint(hdc, temp.c_str(), (int)temp.length(), &sizePrefix);
    GetTextExtentPoint(hdc, szTransfer, (int)wcslen(szTransfer), &size);
    rTemp = rSubItem;
    rTemp.left += sizePrefix.cx;
    rTemp.right = rTemp.left + size.cx;
    g_actionRects[Account::ACTION_FLAG::TRANSFER] = rTemp;
}

LRESULT accountListCustomDraw(LPNMLVCUSTOMDRAW  pCustomDraw)
{
    LRESULT ret = CDRF_NOTIFYITEMDRAW;
    static const COLORREF SEL_LINK_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);
    static const COLORREF LINK_COLOR = GetSysColor(COLOR_HIGHLIGHT);

    switch (pCustomDraw->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
    {
        calcActionRects(pCustomDraw->nmcd.hdc);
        return CDRF_NOTIFYSUBITEMDRAW;
    }
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
    {
        if (pCustomDraw->iSubItem == 2)
        {
            int dcState = SaveDC(pCustomDraw->nmcd.hdc);

            // Get the original font and create an underlined version
            HFONT hOrigFont = (HFONT)GetCurrentObject(pCustomDraw->nmcd.hdc, OBJ_FONT);
            LOGFONT lf = { 0 };
            GetObject(hOrigFont, sizeof(LOGFONT), &lf);
            lf.lfUnderline = true;
            HFONT hNewFont = CreateFontIndirect(&lf);
            SelectObject(pCustomDraw->nmcd.hdc, hNewFont);

            LVITEM item = { 0 };
            item.iItem = (int)pCustomDraw->nmcd.dwItemSpec;
            item.mask = LVIF_PARAM;
            ListView_GetItem(g_hwndAccountLV, &item);
            Account* pAccount = (Account*)item.lParam;
            RECT iR = { 0 };
            ListView_GetSubItemRect(pCustomDraw->nmcd.hdr.hwndFrom, pCustomDraw->nmcd.dwItemSpec, pCustomDraw->iSubItem, LVIR_BOUNDS, &iR);
            int iPos = ListView_GetNextItem(g_hwndAccountLV, -1, LVNI_SELECTED);
            bool selected = iPos == item.iItem;
            SetTextColor(pCustomDraw->nmcd.hdc, selected ? SEL_LINK_COLOR : GetSysColor(COLOR_HOTLIGHT));

            // Draw our background rect
            SelectObject(pCustomDraw->nmcd.hdc, GetStockObject(NULL_PEN));
            HBRUSH brush = CreateSolidBrush(selected ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW));
            HBRUSH hOldBrush = (HBRUSH )SelectObject(pCustomDraw->nmcd.hdc, brush);
            Rectangle(pCustomDraw->nmcd.hdc, iR.left, iR.top, iR.right, iR.bottom);
            SelectObject(pCustomDraw->nmcd.hdc, hOldBrush);
            DeleteObject(brush);


            if (pAccount->paidFromEnvelopeId() != INVALID_ACCOUNT_ID)
            {
                // This is a credit card
                TextOut(pCustomDraw->nmcd.hdc, iR.left, iR.top, L"Pay", 3);
            }
            else
            {
                SIZE size = { 0 };
                GetTextExtentPoint(pCustomDraw->nmcd.hdc, szDeposit, (int)wcslen(szDeposit), &size);
                TextOut(pCustomDraw->nmcd.hdc, iR.left, iR.top, szDeposit, (int)wcslen(szDeposit));

                SIZE size2 = { 0 };
                GetTextExtentPoint(pCustomDraw->nmcd.hdc, L"  ", 2, &size2);
                iR.left += size.cx + size2.cx;
                TextOut(pCustomDraw->nmcd.hdc, iR.left, iR.top, szTransfer, (int)wcslen(szTransfer));
            }


            RestoreDC(pCustomDraw->nmcd.hdc, dcState);
            DeleteObject(hNewFont);


            ret = CDRF_SKIPDEFAULT;

        }
        break;
    }
    }

    return ret;

}


void createAccountListControl()
{
    // IDC_SIMPLEBUDGET_ACCOUNT_LIST
    RECT rClient = { 0 };
    GetClientRect(g_hTopWindow, &rClient);

    g_hwndAccountLV = CreateWindowEx(0, WC_LISTVIEW, TEXT("Simple Budget list"), WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_SHOWSELALWAYS | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
        0, 0, 0, 0, g_hTopWindow,
        (HMENU)IDC_SIMPLEBUDGET_ACCOUNT_LIST, g_hInst, NULL);

    ListView_SetExtendedListViewStyle(g_hwndAccountLV, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    setGuiFont(g_hwndAccountLV);
    SetWindowSubclass(g_hwndAccountLV, acctListViewSubclass, 1, 0);

    LVCOLUMN column = { 0 };
    int columnCount = 0;
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(L"Account");
    ListView_InsertColumn(g_hwndAccountLV, columnCount++, &column);

    column.pszText = const_cast<wchar_t*>(L"Balance");
    ListView_InsertColumn(g_hwndAccountLV, columnCount++, &column);

    column.pszText = const_cast<wchar_t*>(L"Actions");
    ListView_InsertColumn(g_hwndAccountLV, columnCount++, &column);

    for (int i = 0; i < columnCount; i++) {
        ListView_SetColumnWidth(g_hwndAccountLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

}

void addAccount(Account* pAccount)
{

    // Add them to the list view
    SendMessage(g_hwndAccountLV, WM_SETREDRAW, FALSE, 0);

    LVITEM item = { 0 };
    item.mask = LVIF_TEXT | LVIF_PARAM;
    std::wstring timeStamp;
    //item.pszText = const_cast<wchar_t*>(pAccount->getName());
    item.lParam = (LPARAM)pAccount;
    item.iItem = INVALID_ACCOUNT_ID;

    // Account name
    std::wstring name = Utils::accountGuiName(pAccount);
    item.pszText = const_cast<wchar_t*>(name.c_str());
    int iIndex = ListView_InsertItem(g_hwndAccountLV, &item);

    // Account balance
    std::wstring balance = Utils::penniesToDollarsString(pAccount->getBalance());
    ListView_SetItemText(g_hwndAccountLV, iIndex, 1, const_cast<wchar_t*>(balance.c_str()));

    // Actions
    if (pAccount->paidFromEnvelopeId() == INVALID_ACCOUNT_ID ){
        ListView_SetItemText(g_hwndAccountLV, iIndex, 2, const_cast<wchar_t*>(L"Deposit  Transfer"));
    }
    else {
        ListView_SetItemText(g_hwndAccountLV, iIndex, 2, const_cast<wchar_t*>(L"Pay"));
    }


    ListView_SetColumnWidth(g_hwndAccountLV, 0, 200);
    ListView_SetColumnWidth(g_hwndAccountLV, 1, 200);
    ListView_SetColumnWidth(g_hwndAccountLV, 2, 160); //LVSCW_AUTOSIZE_USEHEADER
    SendMessage(g_hwndAccountLV, WM_SETREDRAW, TRUE, 0);

}

