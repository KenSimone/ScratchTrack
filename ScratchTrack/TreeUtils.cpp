#include "TreeUtils.h"
#include "resource.h"
#include "Utils.h"

#include <map>
void setGuiFont(HWND hCtrl);

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern HWND g_hwndTV;
extern HWND g_hwndTreeHeader;
extern HWND g_hwndAccountLV;
BUDGET_TREE_DRAG_DETAILS g_btDragDetails;

void addAccount(Account* pAccount);
int serializeState();

BUDGET_TREE_COLUMN_SIZES g_columnSizes;

std::map< BUDGET_TREE_LINK_ITEM_TYPE, RECT> g_budgetTreeLinkSpots;
HWND g_hwndTreeToolTip = nullptr;
TTTOOLINFO g_toolTip = { 0 };
bool g_isToolTipActive = false;
static bool _trackingMouse = false;

HIMAGELIST _il = nullptr;

HTREEITEM hotLinkFromPoint(POINT pt)
{
    return nullptr;
}

LRESULT CALLBACK treeViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uMsg == WM_SETCURSOR)
    {
        POINT point = { 0 };
        GetCursorPos(&point);
        ScreenToClient(g_hwndTV, &point);
        budgetTreeHandleMouseMove(point.x, point.y);
        return true;
    }
    else if (uMsg == WM_MOUSELEAVE)
    {
        SendMessage(g_hwndTreeToolTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolTip);
        _trackingMouse = FALSE;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

}

// This function comes up with initial column sizes
void calcColumnSpots()
{
    HFONT hFont = (HFONT)SendMessage(g_hwndTV, WM_GETFONT, 0, 0);
    static const wchar_t* szMaxNameLength = L"INDENT__BIG FOLDER NAME__ICONS";
    static const wchar_t* szMaxValueLength = L"DIFFERENCE";

    HDC dc = GetDC(g_hwndTV);
    int iDc = SaveDC(dc);
    SelectObject(dc, hFont);
    SIZE textSizeA = { 0 }, textSizeB = { 0 };
    GetTextExtentPoint32(dc, szMaxNameLength, (int)wcslen(szMaxNameLength), &textSizeA);
    GetTextExtentPoint32(dc, szMaxValueLength, (int)wcslen(szMaxValueLength), &textSizeB);
    RestoreDC(dc, iDc);
    ReleaseDC(g_hwndTV, dc);

    int totalSize = 0;
    g_columnSizes._name = textSizeA.cx;
    g_columnSizes._budgeted = textSizeB.cx;
    g_columnSizes._inCategories = textSizeB.cx;
    g_columnSizes._diff = textSizeB.cx;
    g_columnSizes._balance = textSizeB.cx;

    totalSize = g_columnSizes._name + g_columnSizes._budgeted + g_columnSizes._inCategories + g_columnSizes._balance + g_columnSizes._diff;

    // Info width is the remainder
    RECT rHeader = { 0 };
    GetClientRect(g_hwndTreeHeader, &rHeader);

    int headerWidth = rHeader.right - rHeader.left;
    headerWidth = headerWidth - totalSize;

    g_columnSizes._info = headerWidth;
}

COLORREF setTextColorForAmount(HDC hDC, int iAmount, bool bSelected, bool colorizeNegative)
{
    static const COLORREF TEXT_COLOR = GetSysColor(COLOR_MENUTEXT);
    static const COLORREF SEL_TEXT_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);

    static const COLORREF NEGATIVE_BALANCE_COLOR = RGB(255, 0, 0);
    static const COLORREF NEGATIVE_BALANCE_COLOR_SEL = RGB(255, 200, 200);

    static const COLORREF TEXT_COLOR_ZERO = GetSysColor(COLOR_GRAYTEXT);

    COLORREF crRet = { 0 };
    if (iAmount == 0) {
        crRet = SetTextColor(hDC, bSelected ? SEL_TEXT_COLOR : TEXT_COLOR_ZERO);
    }
    else if (iAmount >= 0 || !colorizeNegative) {
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
    Header_GetItem(g_hwndTreeHeader, 0, &hdi);
    g_columnSizes._name = hdi.cxy;

    Header_GetItem(g_hwndTreeHeader, 1, &hdi);
    g_columnSizes._budgeted = hdi.cxy;

    Header_GetItem(g_hwndTreeHeader, 2, &hdi);
    g_columnSizes._inCategories = hdi.cxy;

    Header_GetItem(g_hwndTreeHeader, 3, &hdi);
    g_columnSizes._diff = hdi.cxy;

    Header_GetItem(g_hwndTreeHeader, 4, &hdi);
    g_columnSizes._balance = hdi.cxy;

    Header_GetItem(g_hwndTreeHeader, 5, &hdi);
    g_columnSizes._info = hdi.cxy;
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
    if (pAccount->getHidden()) {
        return nullptr;
    }


    TVINSERTSTRUCT tvi = { 0 };
    tvi.itemex.mask = TVIF_TEXT | TVIF_PARAM;

    // Note that setting the text here will invalidate the rectangle, but the name is rendered in POSTPAINT
    // This text can be anything.
    tvi.itemex.pszText = const_cast<wchar_t*>(pAccount->getName());
    tvi.hParent = hParentItem;
    tvi.itemex.lParam = (LPARAM)pAccount;
    tvi.hInsertAfter = hInsertAfter;

    HTREEITEM ret = TVI_ROOT;
    if (pAccount->getType() == AccountBase::ACCOUNT) {
        addAccount((Account*)pAccount);
    }
    else {
        ret = TreeView_InsertItem(g_hwndTV, &tvi);
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

HTREEITEM treeViewHitTestLinks(LONG xCur, LONG yCur, BUDGET_TREE_LINK_ITEM_TYPE& itemType, AccountBase** pAccountItem /*= nullptr*/)
{
    TVHITTESTINFO tvhi = { 0 };
    tvhi.pt.x = xCur;
    tvhi.pt.y = yCur;
    itemType = BUDGET_TREE_LINK_ITEM_TYPE::INVALID_LINK_ITEM_TYPE;

    HTREEITEM hItem = TreeView_HitTest(g_hwndTV, &tvhi);
    AccountBase* pItem = accountFromTreeItem(hItem);

    if (pItem && !pItem->getReadOnly())
    {
        RECT r = { 0 };
        TreeView_GetItemRect(g_hwndTV, hItem, &r, false);

        for (auto itr = g_budgetTreeLinkSpots.begin(); itr != g_budgetTreeLinkSpots.end(); itr++)
        {
            // Weed out the wrong types
            if (pItem->getType() == AccountBase::CATEGORY && (itr->first < ENV_FIRST || itr->first > ENV_LAST))
            {
                continue;

            }
            else if (pItem->getType() == AccountBase::GROUP && (itr->first < GRP_FIRST || itr->first > GRP_LAST))
            {
                continue;
            }

            RECT rHitArea = itr->second;
            rHitArea.top = r.top;
            rHitArea.bottom = r.bottom;
            if (PtInRect(&rHitArea, tvhi.pt))
            {
                // A bit hacky, but exclude new transaction from Pay CC envelope
                itemType = itr->first;
                if (itemType == ENV_TRANS_ADD && pItem->getId() == ACCOUNT_ID_PAY_CC) {
                }
                else 
                {
                    if (pAccountItem) {
                        *pAccountItem = pItem;
                    }
                    return hItem;
                }
            }
        }
    }

    if (pAccountItem) {
        *pAccountItem = nullptr;
    }
    return nullptr;
}

bool budgetTreeHandleMouseMove(LONG xCur, LONG yCur)
{
    HTREEITEM htiTarget;  // Handle to target item. 
    TVHITTESTINFO tvht;   // Hit test information. 

    // Translate point
    POINT point;
    point.x = xCur;
    point.y = yCur;

    if (g_btDragDetails._dragging)
    {
        // Drag the item to the current position of the mouse pointer. 
        // First convert the dialog coordinates to control coordinates. 
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
    else
    {
        if (!_trackingMouse)
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
            tme.hwndTrack = g_hwndTV;
            tme.dwFlags = TME_LEAVE;

            TrackMouseEvent(&tme);
            _trackingMouse = true;
        }
        // Hotspot check
        HCURSOR hCursor = nullptr;

        BUDGET_TREE_LINK_ITEM_TYPE type = INVALID_LINK_ITEM_TYPE;

        static wchar_t szBuffer[1024] = { 0 };
        AccountBase* pHitItem = nullptr;
        if (treeViewHitTestLinks(point.x, point.y, type, &pHitItem)){
            hCursor = LoadCursor(nullptr, IDC_HAND);

            // Show tool tip
            wcscpy_s(szBuffer, 1024, L"No hint. File a bug.");
            switch (type) 
            {
            case ENV_CATEGORY_EDIT:
                swprintf(szBuffer, 1024, L"Edit the '%ls' category", pHitItem->getName());
                break;
            case ENV_TRANS_ADD:
                swprintf(szBuffer, 1024, L"Record expense in the '%ls' category", pHitItem->getName());
                break;
            case ENV_MONEY_ADD:
                swprintf(szBuffer, 1024, L"Assign money to the '%ls' category", pHitItem->getName());
                break;
            case GRP_CATEGORY_ADD:
                swprintf(szBuffer, 1024, L"Add a new category to the '%ls' folder", pHitItem->getName());
                break;
            case GRP_FOLDER_EDIT:
                swprintf(szBuffer, 1024, L"Edit the '%ls' folder", pHitItem->getName());
                break;
            case GRP_FUND_CHILDREN:
                swprintf(szBuffer, 1024, L"Assign money to the categories in the '%ls' folder", pHitItem->getName());
                break;
            }
            g_toolTip.lpszText = szBuffer;
            ClientToScreen(g_hwndTV, &point);
            static POINT old = { 0 };
            if (memcmp(&old, &point, sizeof(point)) != 0 ) {
                SendMessage(g_hwndTreeToolTip, TTM_SETTOOLINFO, 0, (LPARAM)&g_toolTip);
                SendMessage(g_hwndTreeToolTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x + 10, point.y - 20));
                if (!g_isToolTipActive) {
                    SendMessage(g_hwndTreeToolTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&g_toolTip);
                    g_isToolTipActive = true;
                }
                old = point;
            }
        }
        else
        {
            if (g_isToolTipActive) {
                SendMessage(g_hwndTreeToolTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolTip);
                g_isToolTipActive = false;
            }
        }

        if ( !hCursor ) {
            hCursor = LoadCursor(nullptr, IDC_ARROW);
        }

        SetCursor(hCursor);
    }
    return false;
}

int addLinkText(HTREEITEM hItem, HDC hDc, const wchar_t* szLinkText, BUDGET_TREE_LINK_ITEM_TYPE type, int rightEdge, int top)
{
    static const int iconSpacing = 5;
    SIZE textSize = { 0 };
    GetTextExtentPoint(hDc, szLinkText, (int)wcslen(szLinkText), &textSize);

    RECT rHitArea = { 0 };
    rHitArea.left = rightEdge - textSize.cx - iconSpacing;
    rHitArea.top = top;
    rHitArea.right = rHitArea.left + textSize.cx;
    rHitArea.bottom = rHitArea.top + textSize.cy;

    TextOut(hDc, rHitArea.left, rHitArea.top, szLinkText, (int)wcslen(szLinkText));

    g_budgetTreeLinkSpots[type] = rHitArea;

    return rHitArea.left;
}

LRESULT budgetTreeCustomDrawTreeItem(NMTVCUSTOMDRAW* pCustomDrawItem)
{
    LRESULT ret = CDRF_NOTIFYITEMDRAW;
    AccountBase* pItem = (AccountBase*)pCustomDrawItem->nmcd.lItemlParam;

    //static const wchar_t* szAddTransaction = L"\u271A"; // big plus
    static const wchar_t* szAddTransaction = L"\U0001F9FE"; // Receipt
    
    //static const wchar_t* szAddTransaction = L"\U0001F4B8"; // money flying away
    //static const wchar_t* szAddTransaction = L"\U0001F4E4"; // outbox tray
    // static const wchar_t* szAddTransaction = L"\U0001F4B5"; // cash stack
    //static const wchar_t* szAddTransaction = L"\U0001F4B2"; // dollar sign
    //static const wchar_t* szAddTransaction = L"\U0001f4c9"; // downward graph
    //static const wchar_t* szAddTransaction = L"\U0001F4E9"; // Category in

    
    static const wchar_t* szEditCategory= L"\u270E";  // pencil
    //static const wchar_t* szAddCategory = L"\u271A"; // plus sign
    //static const wchar_t* szAddCategory = L"\U0001F95B"; // Category
    //static const wchar_t* szAddCategory = L"\U0001F195"; // Category
    static const wchar_t* szAddCategory = L"\U0001F7BC"; // Category
    
    static const wchar_t* szAddMoney = L"\U0001F4B2";
    //static const wchar_t* szHide = L"\U0001F576"; // sunglasses
    static const wchar_t* szHide = L"\U0001F441"; // eye

    static const COLORREF ACCOUNT_COLOR = RGB(230, 240, 250);
    static const COLORREF GROUP_COLOR = RGB(230, 240, 250);
    static const COLORREF CATEGORY_COLOR = RGB(255, 255, 255);

    static const COLORREF SEL_COLOR = GetSysColor(COLOR_HIGHLIGHT);
    static const COLORREF LINK_COLOR = GetSysColor(COLOR_HOTLIGHT);
    static const COLORREF SEL_LINK_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);
    static const COLORREF SEL_TEXT_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);

    wchar_t szInfotText[1024] = { 0 };

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
                cr = CATEGORY_COLOR;
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
            std::wstring budgeted = Utils::penniesToDollarsString(abs(iBudget));

            // Money allocated to this item
            int iCategories = pItem->getType() == AccountBase::CATEGORY ? pItem->getBalance() : pItem->allocatedToSubAccounts();
            std::wstring allocated = Utils::penniesToDollarsString(iCategories);

            // Sum of all transactions for this item
            int iTransactionSum = pItem->getCurrentMonthTransacationTotal();
            std::wstring transactionSum = Utils::penniesToDollarsString(iTransactionSum);

            int iDiff = abs(iBudget) - abs(iTransactionSum);
            std::wstring diff = Utils::penniesToDollarsString(iDiff);

            // Calculate column positions
            getColumnSpots();

            int nameSpot = rcText.left;
            int budgetedSpot = g_columnSizes._name;
            int inCategoriesSpot = budgetedSpot + g_columnSizes._budgeted;
            int diffSpot = inCategoriesSpot + g_columnSizes._inCategories;
            int balanceSpot = diffSpot + g_columnSizes._diff;
            int infoSpot = balanceSpot + g_columnSizes._balance;
            int endSpot = infoSpot + g_columnSizes._info;

            HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
            SelectObject(pCustomDrawItem->nmcd.hdc, GetStockObject(HOLLOW_BRUSH));
            SelectObject(pCustomDrawItem->nmcd.hdc, pen);
            SetBkMode(pCustomDrawItem->nmcd.hdc, TRANSPARENT);

            // Name
            const wchar_t* szFolderOpen = L"\U0001F5C1 ";
            const wchar_t* szFolderClosed = L"\U0001F5C0 ";
            const wchar_t* szCategory = L"\U0001F95B ";
            std::wstring name;
            if (TreeView_GetItemState(g_hwndTV, hItem, TVIF_STATE) & TVIS_EXPANDED) {
                name = szFolderOpen;
            }
            else if (pItem->getType() == AccountBase::CATEGORY) {
                //name = szCategory;
            }
            else {
                name = szFolderClosed;
            }

            RECT rText = { 0 };
            rText.bottom = rcText.bottom; rText.top = rcText.top;

            DWORD dwDrawTextFlags = DT_NOPREFIX | DT_END_ELLIPSIS;

            // Budgeted dollars
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iBudget, selected, false);
            rText.left = budgetedSpot; rText.right = inCategoriesSpot;
            DrawText(pCustomDrawItem->nmcd.hdc, budgeted.c_str(), (int)budgeted.length(), &rText, dwDrawTextFlags);

            // Monthly transaction sum
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iTransactionSum, selected, false);
            rText.left = inCategoriesSpot; rText.right = diffSpot;
            DrawText(pCustomDrawItem->nmcd.hdc, transactionSum.c_str(), (int)transactionSum.length(), &rText, dwDrawTextFlags);

            // Difference
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iDiff, selected, true);
            rText.left = diffSpot; rText.right = balanceSpot;
            DrawText(pCustomDrawItem->nmcd.hdc, diff.c_str(), (int)diff.length(), &rText, dwDrawTextFlags);

            // Balance
            if (pItem->getType() == AccountBase::CATEGORY)
            {
                AccountCategory* pCategory = (AccountCategory*)pItem;
                if (pCategory->isVirtualAccount()) {
                    setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iBalance, selected, true);
                    rText.left = balanceSpot; rText.right = infoSpot;
                    DrawText(pCustomDrawItem->nmcd.hdc, balance.c_str(), (int)balance.length(), &rText, dwDrawTextFlags);
                }
            } 
            else if ( pItem->getType() == AccountBase::GROUP && !pItem->getReadOnly())
            {
                AccountGroup* pGroup = dynamic_cast<AccountGroup*>(pItem);
                if (pGroup->getExpenseLimit()) {
                    std::wstring limit = Utils::penniesToDollarsString(-pGroup->getExpenseLimit());
                    std::wstring budgeted = Utils::penniesToDollarsString(abs(pGroup->budgetedToSubAccounts()));
                    std::wstring diff = Utils::penniesToDollarsString(-(pGroup->getExpenseLimit() - pGroup->budgetedToSubAccounts()));
                    wnsprintf(szInfotText, 1024, L"  %ls of %ls budgeted [%ls]", budgeted.c_str(), limit.c_str(), diff.c_str());
                    setTextColorForAmount(pCustomDrawItem->nmcd.hdc, 0, selected, true);
                    rText.left = infoSpot; rText.right = infoSpot + g_columnSizes._info;
                    DrawText(pCustomDrawItem->nmcd.hdc, szInfotText, (int)wcslen(szInfotText), &rText, dwDrawTextFlags);
                }
            }

            // Create font for add/edit links
            HFONT hTreeFont = (HFONT)GetCurrentObject(pCustomDrawItem->nmcd.hdc, OBJ_FONT);
            LOGFONT lf = { 0 };
            GetObject(hTreeFont, sizeof(LOGFONT), &lf);
            //lf.lfUnderline = true;
            HFONT hFontItalic = CreateFontIndirect(&lf);
            HFONT hOrigFont = (HFONT)SelectObject(pCustomDrawItem->nmcd.hdc, hFontItalic);
            COLORREF oldColor = SetTextColor(pCustomDrawItem->nmcd.hdc, selected ? SEL_LINK_COLOR : LINK_COLOR);

            int rightEdge = budgetedSpot;
            switch (pItem->getType())
            {
            case AccountBase::CATEGORY:
            {
                AccountCategory* pCategory = dynamic_cast<AccountCategory*>(pItem);
                if (!pCategory->getReadOnly()) {
#ifdef ICON_HIDE_ITEM
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szHide, ENV_HIDE, rightEdge, rcText.top);
#endif
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szEditCategory, ENV_CATEGORY_EDIT, rightEdge, rcText.top);
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szAddMoney, ENV_MONEY_ADD, rightEdge, rcText.top);

                    if (pCategory->getId() != ACCOUNT_ID_PAY_CC) {
                        rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szAddTransaction, ENV_TRANS_ADD, rightEdge, rcText.top);
                    }

                }
                break;
            }
            case AccountBase::GROUP:
                if (!pItem->getReadOnly()) {
#ifdef ICON_HIDE_ITEM
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szHide, GRP_FOLDER_HIDE, rightEdge, rcText.top);
#endif
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szEditCategory, GRP_FOLDER_EDIT, rightEdge, rcText.top);
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szAddMoney, GRP_FUND_CHILDREN, rightEdge, rcText.top);
                    rightEdge = addLinkText(hItem, pCustomDrawItem->nmcd.hdc, szAddCategory, GRP_CATEGORY_ADD, rightEdge, rcText.top);
                }
                break;

            }

            // Name
            if (pItem->getId() == ACCOUNT_ID_PAY_CC) {
                static const wchar_t* szCreditCard = (wchar_t*)(u"\U0001F4B3 ");
                name = szCreditCard;
            }
            name += pItem->getName();
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, 1, selected, false);
            rText.left = nameSpot; rText.top = rcText.top;
            rText.bottom = rcText.bottom; rText.right = rightEdge;
            DrawText(pCustomDrawItem->nmcd.hdc, name.c_str(), (int)name.length(), &rText, dwDrawTextFlags);

            // Clean up
            SelectObject(pCustomDrawItem->nmcd.hdc, hOrigFont);
            SetTextColor(pCustomDrawItem->nmcd.hdc, oldColor);
            DeleteObject(hFontItalic);


            // Draw a horizontal line above the item, across the full span
            MoveToEx(pCustomDrawItem->nmcd.hdc, pCustomDrawItem->nmcd.rc.left, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, pCustomDrawItem->nmcd.rc.right, pCustomDrawItem->nmcd.rc.top);

            // Draw vertical column lines
            MoveToEx(pCustomDrawItem->nmcd.hdc, budgetedSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, budgetedSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, inCategoriesSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, inCategoriesSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, balanceSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, balanceSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, diffSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, diffSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, infoSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, infoSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, endSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, endSpot - 2, pCustomDrawItem->nmcd.rc.bottom);


            RestoreDC(pCustomDrawItem->nmcd.hdc, oldDcState);

            DeleteObject(pen);
        }

    } // if item
    return ret;
}

void budgetTreeCreateTreeControl()
{
    // Create tree header
    g_hwndTreeHeader = CreateWindowEx(0, WC_HEADER, nullptr, WS_CHILD | HDS_FULLDRAG, 0, 0, 0, 0, g_hTopWindow, (HMENU)IDC_SIMPLEBUDGET_TREEHEADER, g_hInst, nullptr);


    RECT rClient = { 0 };
    HDLAYOUT hdl = { 0 };
    WINDOWPOS wp = { 0 };
    GetClientRect(g_hTopWindow, &rClient);
    hdl.prc = &rClient;
    hdl.pwpos = &wp;

    HWND lvHeader = ListView_GetHeader(g_hwndAccountLV);
    HFONT hFont = (HFONT)SendMessageW(lvHeader, WM_GETFONT, 0, 0);
    SendMessageW(g_hwndTreeHeader, WM_SETFONT, (WPARAM)hFont, 0);


    SendMessage(g_hwndTreeHeader, HDM_LAYOUT, 0, (LPARAM)&hdl);
    SetWindowPos(g_hwndTreeHeader, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);

    g_hwndTV = CreateWindowEx(0, WC_TREEVIEW, TEXT("Simple Budget Tree"), WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | TVS_HASBUTTONS  | TVS_HASLINES | TVS_LINESATROOT,
        rClient.left, rClient.top, rClient.right, rClient.bottom / 2,
        g_hTopWindow, (HMENU)IDC_SIMPLEBUDGET_TREE, g_hInst, NULL);

    // Adjust font
    setGuiFont(g_hwndTV);
    calcColumnSpots();

    SetWindowSubclass(g_hwndTV, treeViewSubclass, 1, 0);

    // Create tree
    HDITEM item = { 0 };
    item.cxy = g_columnSizes._name;
    item.pszText = const_cast<wchar_t*>(L"Category");
    item.mask = HDI_TEXT | HDI_WIDTH;
    Header_InsertItem(g_hwndTreeHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Budgeted");
    item.cxy = g_columnSizes._budgeted;
    Header_InsertItem(g_hwndTreeHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Outflows");
    item.cxy = g_columnSizes._inCategories;
    Header_InsertItem(g_hwndTreeHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Difference");
    item.cxy = g_columnSizes._balance;
    Header_InsertItem(g_hwndTreeHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Balance");
    item.cxy = g_columnSizes._balance;
    Header_InsertItem(g_hwndTreeHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Information");
    item.cxy = g_columnSizes._info;
    Header_InsertItem(g_hwndTreeHeader, 99, &item);

    SendMessage(g_hwndTV, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);

    _il = ImageList_Create(1, 1, ILC_COLOR32 | ILC_MASK, 1, 0);

    //HICON h = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SIMPLEBUDGET));
    //ImageList_AddIcon(_il, h);


    //TreeView_SetImageList(g_hwndTV, _il, TVSIL_NORMAL);

    // Create a tool tip
    g_hwndTreeToolTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        g_hwndTV, NULL, g_hInst, NULL);

    g_toolTip.cbSize = TTTOOLINFO_V1_SIZE; 
    g_toolTip.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    g_toolTip.hwnd = g_hwndTV;
    g_toolTip.hinst = g_hInst;
    g_toolTip.lpszText = const_cast<wchar_t*>(L"");
    g_toolTip.uId = (UINT_PTR)g_hwndTV;

    GetClientRect(g_hwndTV, &g_toolTip.rect);
    SendMessage(g_hwndTreeToolTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&g_toolTip);
}

void budgetTreeSetItemText(HTREEITEM hItem, const wchar_t* szText)
{
    // Note that the name is rendered in POSTPAINT. Setting the text here will force a redraw
    // but won't actually change the name.
    TVITEMEX tvi = { 0 };
    tvi.hItem = hItem;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = const_cast<wchar_t*>(szText);

    SendMessageW(g_hwndTV, TVM_SETITEM, 0, (LPARAM)&tvi);
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

void killTreeToolTips()
{
    if (g_isToolTipActive) {
        SendMessage(g_hwndTreeToolTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolTip);
        g_isToolTipActive = false;
    }
}
