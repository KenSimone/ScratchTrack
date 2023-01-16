#include "TreeUtils.h"
#include "resource.h"

// Global settings
static const double FONT_SCALE_FACTOR = 1.6;

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern HWND g_hwndTV;
extern HWND g_hwndHeader;
extern HWND g_hwndAccountLV;
BUDGET_TREE_DRAG_DETAILS g_btDragDetails;;

void addAccount(Account* pAccount);
int serializeState();

BUDGET_TREE_COLUMN_SIZES g_columnSizes;

// This function comes up with initial column sizes
void calcColumnSpots()
{
    static const wchar_t* szMaxNameLength = L"WWWWWWWWWWWWWWWWWWWWWWW";
    static const wchar_t* szMaxValueLength = L"In cagtegory(s): $1,000,000,000";

    HDC dc = GetDC(g_hwndTV);
    SIZE textSizeA = { 0 }, textSizeB = { 0 };
    GetTextExtentPoint(dc, szMaxNameLength, (int)wcslen(szMaxNameLength), &textSizeA);
    GetTextExtentPoint(dc, szMaxValueLength, (int)wcslen(szMaxValueLength), &textSizeB);

    ReleaseDC(g_hwndTV, dc);

    //textSizeA.cx *= 2;
    g_columnSizes._name = (int)(textSizeA.cx * 1.5);
    g_columnSizes._budgeted = textSizeB.cx;
    g_columnSizes._inCategories = textSizeB.cx;
    g_columnSizes._balance = textSizeB.cx;
    g_columnSizes._available = textSizeB.cx;
    g_columnSizes._endSpot = textSizeB.cx;;
}

COLORREF setTextColorForAmount(HDC hDC, int iAmount, bool bSelected, bool colorizeNegative)
{
    static const COLORREF TEXT_COLOR = GetSysColor(COLOR_MENUTEXT);
    static const COLORREF SEL_TEXT_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);

    static const COLORREF NEGATIVE_BALANCE_COLOR = RGB(255, 0, 0);
    static const COLORREF NEGATIVE_BALANCE_COLOR_SEL = RGB(255, 200, 200);

    COLORREF crRet = { 0 };
    if (iAmount >= 0 || !colorizeNegative) {
        crRet = SetTextColor(hDC, bSelected ? SEL_TEXT_COLOR : TEXT_COLOR);
    }
    else {
        crRet = SetTextColor(hDC, bSelected ? NEGATIVE_BALANCE_COLOR_SEL : NEGATIVE_BALANCE_COLOR);
    }
    return crRet;
}

// This function gets the current position of the columns (user drag)
void getColumnSpots()
{
    HDITEM hdi = { 0 };
    hdi.mask = HDI_WIDTH;
    Header_GetItem(g_hwndHeader, 0, &hdi);
    g_columnSizes._name = hdi.cxy;

    Header_GetItem(g_hwndHeader, 1, &hdi);
    g_columnSizes._budgeted = hdi.cxy;

    Header_GetItem(g_hwndHeader, 2, &hdi);
    g_columnSizes._inCategories = hdi.cxy;

    Header_GetItem(g_hwndHeader, 3, &hdi);
    g_columnSizes._balance = hdi.cxy;

    Header_GetItem(g_hwndHeader, 4, &hdi);
    g_columnSizes._available = hdi.cxy;

    g_columnSizes._endSpot = hdi.cxy;


}

// Finds an AccountBase associated with a tree item
AccountBase* accountFromTreeItem(HTREEITEM item)
{
    if (!item || item == TVI_ROOT) {
        return nullptr;
    }

    TVITEMEX tvi = { 0 };
    tvi.hItem = item;
    tvi.mask = TVIF_PARAM;

    TreeView_GetItem(g_hwndTV, &tvi);

    return (AccountBase*)tvi.lParam;
}

void handleDragDrop(HTREEITEM hDragSource, HTREEITEM hDragTarget)
{
    if (hDragTarget != NULL)
    {
        // Get the associated accounts and figure out the real target
        AccountBase* dropSource = accountFromTreeItem(g_btDragDetails._dragging);
        AccountBase* dropTarget = accountFromTreeItem(hDragTarget);

        HTREEITEM hNewParent = nullptr, hInsertAfter = TVI_LAST;

        if (dropSource->getType() == AccountBase::CATEGORY && dropTarget->getType() == AccountBase::CATEGORY)
        {
            // This is the most common case, re-ordering categories
            // The real drop target is the parent of the target category
            AccountBase* pParent = dropTarget->getParent();
            hNewParent = treeItemFromAccount(pParent);
            hInsertAfter = hDragTarget;
        }
        else if (dropSource->getType() == AccountBase::CATEGORY && dropTarget->getType() == AccountBase::GROUP)
        {
            // This is the second most common case, re-ordering categories, trying to get the item into the first spot
            hNewParent = hDragTarget;
            hInsertAfter = TVI_FIRST;
        }
        else if (dropSource->getType() == AccountBase::GROUP && dropTarget->getType() == AccountBase::GROUP)
        {
            // Re-ordering groups. There's a bit of a gap here. No way to drag to the first spot
            // You can achieve the same by dragging to second spot,and then swapping.
            hNewParent = TVI_ROOT;
            hInsertAfter = hDragTarget;
        }
        else if ( dropSource->getType() == AccountBase::GROUP && dropTarget->getType() == AccountBase::CATEGORY)
        {
            // Someone has dropped a group on to a category. Assume they want to move the group after the
            // parent of the category
            AccountBase* pParent = dropTarget->getParent();
            hNewParent = TVI_ROOT;
            hInsertAfter = treeItemFromAccount(pParent);
        }  

        if (hNewParent && hDragSource && ( hNewParent != hDragSource) )
        {
            // Capture parents
            AccountBase* pMovingAccount = accountFromTreeItem(hDragSource);
            AccountBase* pOldParent = pMovingAccount ? pMovingAccount->getParent() : nullptr;
            AccountBase* pNewParent = accountFromTreeItem(hNewParent);
            
            if (pMovingAccount)
            {
                // Remove the original
                TreeView_DeleteItem(g_hwndTV, hDragSource);
                if (pOldParent) {
                    pOldParent->removeChild(pMovingAccount);
                }
                if (pNewParent) {
                    AccountGroup* pNewGroup = dynamic_cast<AccountGroup*>(pNewParent);
                    AccountBase* pInsertAfter = nullptr;
                    if (pNewGroup)
                    {
                        if (hInsertAfter == TVI_FIRST) {

                        }
                        else if (hInsertAfter == TVI_LAST) {
                            pInsertAfter = nullptr;
                        }
                        else
                        {
                            pInsertAfter = accountFromTreeItem(hInsertAfter);
                        }
                    }

                    pNewParent->addChild(pMovingAccount, pInsertAfter);
                }

                addTreeItem(hNewParent, pMovingAccount, hInsertAfter);
                serializeState();
            }
        }

    }
    ImageList_EndDrag();
    TreeView_SelectDropTarget(g_hwndTV, NULL);
    ReleaseCapture();
    ShowCursor(TRUE);
    g_btDragDetails._dragging = g_btDragDetails._lastHitItem = nullptr;
}

void budgetTreeHandleLButtonUp(LONG xCur, LONG yCur)
{
    if (g_btDragDetails._dragging)
    {
        TreeView_SetInsertMark(g_hwndTV, NULL, 0);

        TVHITTESTINFO tvht;
        POINT point;
        point.x = xCur;
        point.y = yCur;
        ClientToScreen(g_hTopWindow, &point);
        ScreenToClient(g_hwndTV, &point);
        tvht.pt.x = point.x;
        tvht.pt.y = point.y;

        // Get destination item.
        //HTREEITEM htiDest = TreeView_GetDropHilight(g_hwndTV);
        HTREEITEM htiDest = TreeView_HitTest(g_hwndTV, &tvht);
        handleDragDrop(g_btDragDetails._dragging, htiDest);
    }
}

// Adds an account item to the tree
HTREEITEM addTreeItem(HTREEITEM hParentItem, AccountBase* pAccount, HTREEITEM hInsertAfter /*= TVI_LAST*/)
{
    static const DWORD IMG_ACCOUNT = 0;
    static const DWORD IMG_FOLDER = 1;
    static const DWORD IMG_CATEGORY = 2;
    static const DWORD IMG_ADDMONEY = 3;
    static const DWORD IMG_SUBTRACTMONEY = 4;


    TVINSERTSTRUCT tvi = { 0 };
    tvi.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.itemex.pszText = const_cast<wchar_t*>(pAccount->getName());
    tvi.hParent = hParentItem;
    tvi.itemex.lParam = (LPARAM)pAccount;
    tvi.hInsertAfter = hInsertAfter;

    switch (pAccount->getType())
    {
    case AccountBase::ACCOUNT:
        tvi.itemex.iSelectedImage = tvi.itemex.iImage = IMG_ACCOUNT;
        break;
    case AccountBase::GROUP:
        tvi.itemex.iSelectedImage = tvi.itemex.iImage = IMG_FOLDER;
        break;
    case AccountBase::CATEGORY:
        if (((AccountCategory*)pAccount)->isExpense()) {
            tvi.itemex.iSelectedImage = tvi.itemex.iImage = IMG_SUBTRACTMONEY;
        }
        else {
            tvi.itemex.iSelectedImage = tvi.itemex.iImage = IMG_ADDMONEY;
        }
        break;
    };

    HTREEITEM ret = TVI_ROOT;
    if (pAccount->getType() == AccountBase::ACCOUNT) {
        addAccount((Account*)pAccount);
    }
    else {
        ret = (HTREEITEM)SendMessageW(g_hwndTV, TVM_INSERTITEM, 0, (LPARAM)&tvi);
    }

    // Add any children
    if (pAccount->getType() == AccountBase::GROUP)
    {
        AccountGroup* pGroup = (AccountGroup*)pAccount;
        auto children = pGroup->getChildren();
        for (size_t i = 0; i < children.size(); i++)
        {
            addTreeItem(ret, children[i]);
        }

    }


    TreeView_Expand(g_hwndTV, hParentItem, TVE_EXPAND);

    return ret;

}

void budgetTreeHandleMouseMove(LONG xCur, LONG yCur)
{
    HTREEITEM htiTarget;  // Handle to target item. 
    TVHITTESTINFO tvht;   // Hit test information. 

    if (g_btDragDetails._dragging)
    {
        // Drag the item to the current position of the mouse pointer. 
        // First convert the dialog coordinates to control coordinates. 
        POINT point;
        point.x = xCur;
        point.y = yCur;
        ClientToScreen(g_hTopWindow, &point);
        ScreenToClient(g_hwndTV, &point);
        ImageList_DragMove(point.x, point.y);

        // Find out if the pointer is on the item. If it is, 
        // highlight the item as a drop target. 
        tvht.pt.x = point.x;
        tvht.pt.y = point.y;
        if ((htiTarget = TreeView_HitTest(g_hwndTV, &tvht)) != NULL)
        {
            //TreeView_SelectDropTarget(g_hwndTV, htiTarget);
            if (g_btDragDetails._lastHitItem != htiTarget)
            {
                ImageList_DragShowNolock(FALSE);
                TreeView_SetInsertMark(g_hwndTV, htiTarget, true);
                ImageList_DragShowNolock(TRUE);
                g_btDragDetails._lastHitItem = htiTarget;
            }
        }
    }
    return;
}

LRESULT budgetTreeCustomDrawTreeItem(NMTVCUSTOMDRAW* pCustomDrawItem)
{
    LRESULT ret = CDRF_NOTIFYITEMDRAW;
    AccountBase* pItem = (AccountBase*)pCustomDrawItem->nmcd.lItemlParam;

    static const wchar_t* szMaxNameLength = L"WWWWWWWWWWWWWWWWWWWWWWW";
    static const wchar_t* szMaxValueLength = L"In categories: $1,000,000,000";

    static const COLORREF ACCOUNT_COLOR = RGB(230, 240, 250);
    static const COLORREF GROUP_COLOR = RGB(230, 240, 250);
    static const COLORREF ENVELOPE_COLOR = RGB(255, 255, 255);

    static const COLORREF SEL_COLOR = GetSysColor(COLOR_HIGHLIGHT);
    static const COLORREF SEL_TEXT_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);

    if (pItem)
    {
        static HFONT hNewFont = nullptr;

        if (pCustomDrawItem->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
        {
            // We want post paint notifications
            ret |= CDRF_NOTIFYPOSTPAINT;
        }
        else if (pCustomDrawItem->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
        {
            // Figure out which item we're drawing and figure out render colors
            HTREEITEM hItem = (HTREEITEM)pCustomDrawItem->nmcd.dwItemSpec;
            COLORREF cr = { 0 }, crText = RGB(0, 0, 0);

            // Figure out color based on account type
            switch (pItem->getType())
            {
            case AccountBase::ACCOUNT:
                cr = ACCOUNT_COLOR;
                break;
            case AccountBase::GROUP:
                cr = GROUP_COLOR;
                break;
            case AccountBase::CATEGORY:
                cr = ENVELOPE_COLOR;
                break;
            }

            // Is this item selected?
            bool selected = hItem == (HTREEITEM)TreeView_GetSelection(g_hwndTV);
            if (selected) {
                cr = SEL_COLOR;
                crText = SEL_TEXT_COLOR;
            }

            // Save the original DC state
            int oldDcState = SaveDC(pCustomDrawItem->nmcd.hdc);

            // Create our backgroundbrush and select it
            HBRUSH brush = CreateSolidBrush(cr);
            SelectObject(pCustomDrawItem->nmcd.hdc, brush);

            // NULL brush (no border, we'll draw grids later)
            SelectObject(pCustomDrawItem->nmcd.hdc, GetStockObject(NULL_PEN)/*pen*/);

            // This is a little tricky. We want to cover the 'full' area (all the way to the right)
            // but we want to start from the left edge of th text.
            RECT rcFull = { 0 }, rcText = { 0 };
            TreeView_GetItemRect(g_hwndTV, hItem, &rcText, true);
            TreeView_GetItemRect(g_hwndTV, hItem, &rcFull, false);

            Rectangle(pCustomDrawItem->nmcd.hdc, rcText.left, rcFull.top, rcFull.right, rcFull.bottom + 1);
            DeleteObject(brush);

            // Get the text for the columns
            int iBalance = pItem->getBalance();
            std::wstring balance = Utils::penniesToDollarsString(iBalance);

            // Amount budgeted for this item
            int iBudget = pItem->getType() == AccountBase::CATEGORY ? pItem->getBudgeted() : pItem->budgetedToSubAccounts();
            std::wstring budgeted = Utils::penniesToDollarsString(iBudget);

            // Money allocated to this item
            int iEnvelopes = pItem->getType() == AccountBase::CATEGORY ? pItem->getBalance() : pItem->allocatedToSubAccounts();
            std::wstring allocated = Utils::penniesToDollarsString(iEnvelopes);

            // Sum of all transactions for this item
            int iTransactionSum = pItem->getCurrentMonthTransacationTotal();
            std::wstring transactionSum = Utils::penniesToDollarsString(iTransactionSum);

            // Calculate column positions
            getColumnSpots();

            int nameSpot = rcText.left;
            int budgetedSpot = g_columnSizes._name;
            int inEnvelopes = budgetedSpot + g_columnSizes._budgeted;
            int balanceSpot = inEnvelopes + g_columnSizes._inCategories;
            int availableBalanceSpot = balanceSpot + g_columnSizes._balance;

            HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
            SelectObject(pCustomDrawItem->nmcd.hdc, GetStockObject(HOLLOW_BRUSH));
            SelectObject(pCustomDrawItem->nmcd.hdc, pen);
            SetBkMode(pCustomDrawItem->nmcd.hdc, TRANSPARENT);

            // Name
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, 0, selected, false);
            TextOut(pCustomDrawItem->nmcd.hdc, nameSpot /*rcText.left*/, rcText.top, pItem->getName(), (int)wcslen(pItem->getName()));

            // Budgeted dollars
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iBudget, selected, false);
            TextOut(pCustomDrawItem->nmcd.hdc, budgetedSpot, pCustomDrawItem->nmcd.rc.top, budgeted.c_str(), (int)budgeted.length());

            // Monthly transaction sum
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iTransactionSum, selected, false);
            TextOut(pCustomDrawItem->nmcd.hdc, inEnvelopes, pCustomDrawItem->nmcd.rc.top, transactionSum.c_str(), (int)transactionSum.length());

            // Balance
            if (pItem->getType() == AccountBase::CATEGORY)
            {
                AccountCategory* pCategory = (AccountCategory*)pItem;
                if (pCategory->isVirtualAccount()) {
                    setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iBalance, selected, true);
                    TextOut(pCustomDrawItem->nmcd.hdc, balanceSpot, pCustomDrawItem->nmcd.rc.top, balance.c_str(), (int)balance.length());
                }
            }

            // Draw a horizontal line above the item, across the full span
            MoveToEx(pCustomDrawItem->nmcd.hdc, pCustomDrawItem->nmcd.rc.left, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, pCustomDrawItem->nmcd.rc.right, pCustomDrawItem->nmcd.rc.top);

            // Draw vertical column lines
            MoveToEx(pCustomDrawItem->nmcd.hdc, budgetedSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, budgetedSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, inEnvelopes - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, inEnvelopes - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, balanceSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, balanceSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, availableBalanceSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, availableBalanceSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            RestoreDC(pCustomDrawItem->nmcd.hdc, oldDcState);

            DeleteObject(pen);
        }

    } // if item
    return ret;
}

void budgetTreeCreateTreeControl()
{
    // Create tree header
    g_hwndHeader = CreateWindowEx(0, WC_HEADER, nullptr, WS_CHILD | HDS_FULLDRAG, 0, 0, 0, 0, g_hTopWindow, (HMENU)IDC_SIMPLEBUDGET_TREEHEADER, g_hInst, nullptr);


    RECT rClient = { 0 };
    HDLAYOUT hdl = { 0 };
    WINDOWPOS wp = { 0 };
    GetClientRect(g_hTopWindow, &rClient);
    hdl.prc = &rClient;
    hdl.pwpos = &wp;

    HWND lvHeader = ListView_GetHeader(g_hwndAccountLV);
    HFONT hFont = (HFONT)SendMessageW(lvHeader, WM_GETFONT, 0, 0);
    SendMessageW(g_hwndHeader, WM_SETFONT, (WPARAM)hFont, 0);


    SendMessage(g_hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hdl);
    SetWindowPos(g_hwndHeader, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);

    g_hwndTV = CreateWindowEx(0, WC_TREEVIEW, TEXT("Simple Budget Tree"), WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASBUTTONS  | TVS_HASLINES | TVS_LINESATROOT,
        rClient.left, rClient.top,
        rClient.right,
        rClient.bottom / 2,
        g_hTopWindow,
        (HMENU)IDC_SIMPLEBUDGET_TREE,
        g_hInst,
        NULL);

    // Adjust font
    budgetTreeSetFont(g_hwndTV);
    calcColumnSpots();

    // Create tree
    HDITEM item = { 0 };
    item.cxy = g_columnSizes._name;
    item.pszText = const_cast<wchar_t*>(L"Category");
    item.mask = HDI_TEXT | HDI_WIDTH;
    Header_InsertItem(g_hwndHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Budgeted");
    item.cxy = g_columnSizes._budgeted;
    Header_InsertItem(g_hwndHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Cashflow");
    item.cxy = g_columnSizes._inCategories;
    Header_InsertItem(g_hwndHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Balance");
    item.cxy = g_columnSizes._balance;
    Header_InsertItem(g_hwndHeader, 99, &item);

    //item.pszText = const_cast<wchar_t*>(L"Available balance");
    ////item.cxy = d - c;
    //item.mask = HDI_TEXT;
    //Header_InsertItem(g_hwndHeader, 99, &item);

    SendMessage(g_hwndTV, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);

    HIMAGELIST il = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);

    HICON h = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ACCOUNT));
    ImageList_AddIcon(il, h);

    h = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_FOLDER));
    ImageList_AddIcon(il, h);

    h = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ENVELOPE));
    ImageList_AddIcon(il, h);

    h = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ADD_MONEYICON));
    ImageList_AddIcon(il, h);


    h = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SUBTRACT_MONEY_ICON));
    ImageList_AddIcon(il, h);


    TreeView_SetImageList(g_hwndTV, il, TVSIL_NORMAL);
}

void budgetTreeSetItemText(HTREEITEM hItem, const wchar_t* szText)
{
    TVITEMEX tvi = { 0 };
    tvi.hItem = hItem;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = const_cast<wchar_t*>(szText);

    SendMessageW(g_hwndTV, TVM_SETITEM, 0, (LPARAM)&tvi);
}

void budgetTreeSetFont(HWND hCtrl)
{
    // Adjust font
    HFONT hFont = (HFONT)SendMessage(hCtrl, WM_GETFONT, NULL, NULL);
    LOGFONT lf = { 0 };
    GetObject(hFont, sizeof(lf), &lf);
    lf.lfHeight = (LONG)(lf.lfHeight * FONT_SCALE_FACTOR);
    HFONT hNewFont = CreateFontIndirect(&lf);
    SendMessage(hCtrl, WM_SETFONT, (WPARAM)hNewFont, true);
}

HTREEITEM treeItemFromAccount(AccountBase* pItem, HTREEITEM hItem /*= TVI_ROOT*/)
{
    if (hItem == TVI_ROOT) {
        hItem = TreeView_GetRoot(g_hwndTV);
    }

    // Check self and siblings
    while (hItem) {
        if (pItem == accountFromTreeItem(hItem)) {
            return hItem;
        }

        // Check children
        HTREEITEM hChild = TreeView_GetChild(g_hwndTV, hItem);
        if (hChild) {
            HTREEITEM hRet = treeItemFromAccount(pItem, hChild);
            if (hRet) {
                return hRet;
            }
        }

        // Move to next
        hItem = TreeView_GetNextSibling(g_hwndTV, hItem);
    }

    // Oops. Couldn't find it.
    return nullptr;

}
