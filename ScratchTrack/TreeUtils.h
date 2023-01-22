#pragma once

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "AccountInfo.h"

struct BUDGET_TREE_DRAG_DETAILS
{
    BUDGET_TREE_DRAG_DETAILS() : _dragging(nullptr), _lastHitItem(nullptr) {};
    HTREEITEM  _dragging;
    HTREEITEM  _lastHitItem;
};

struct BUDGET_TREE_COLUMN_SIZES
{
    BUDGET_TREE_COLUMN_SIZES()
    {
        _name = _budgeted = _inCategories = _balance = _available = _endSpot = 0;
    }

    int _name;
    int _budgeted;
    int _inCategories;
    int _balance;
    int _available;
    int _endSpot;
};

enum BUDGET_TREE_LINK_ITEM_TYPE
{
    INVALID_LINK_ITEM_TYPE,

    // Envelope operations
    ENV_FIRST,
    ENV_ENVELOPE_EDIT = ENV_FIRST,
    ENV_TRANS_ADD,
    ENV_MONEY_ADD,
    ENV_HIDE,

    ENV_LAST = ENV_HIDE,

    // Group operations
    GRP_FIRST,
    GRP_ENVELOPE_ADD = GRP_FIRST,
    GRP_FOLDER_EDIT,
    GRP_FUND_CHILDREN,
    GRP_FOLDER_HIDE,
    GRP_LAST = GRP_FOLDER_HIDE
};

AccountBase* accountFromTreeItem(HTREEITEM item);
HTREEITEM treeItemFromAccount(AccountBase* pItem, HTREEITEM hItem = TVI_ROOT);
HTREEITEM addTreeItem(HTREEITEM hParentItem, AccountBase* pAccount, HTREEITEM hInsertAfter = TVI_LAST);
void budgetTreeHandleLButtonUp(LONG xCur, LONG yCur);
bool budgetTreeHandleMouseMove(LONG xCur, LONG yCur);
LRESULT budgetTreeCustomDrawTreeItem(NMTVCUSTOMDRAW* pCustomDrawItem);
void budgetTreeCreateTreeControl();
void budgetTreeSetItemText(HTREEITEM hItem, const wchar_t* szText);
HTREEITEM hotLinkFromPoint(POINT pt);
HTREEITEM treeViewHitTestLinks(LONG xCur, LONG yCur, BUDGET_TREE_LINK_ITEM_TYPE& itemType, AccountBase** pAccountItem = nullptr);
void killTreeToolTips();
