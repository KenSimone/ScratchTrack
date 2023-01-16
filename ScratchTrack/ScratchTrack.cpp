// SimpleBudget.cpp : Defines the entry point for the application.
//

#include "framework.h"

#include "AccountInfo.h"
#include "Transaction.h"

#include "ScratchTrack.h"
#include "resource.h"
#include "TransactionDialog.h"
#include "CategoryDlg.h"
#include "AccountDlg.h"
#include "Utils.h"

#include <commctrl.h>
#include <Windows.h>
#include <windowsx.h>
#include <vector>

#pragma comment(lib, "Comctl32")
#pragma comment(lib, "Shlwapi")

#define MAX_LOADSTRING 100

// Global settings
static const double FONT_SCALE_FACTOR = 1.6;

// Global Variables:
HINSTANCE g_hInst = nullptr;                                // current instance
HWND g_hTopWindow = nullptr;
HWND g_hwndTV = nullptr, g_hwndTransactionLV = nullptr, g_hwndHeader = nullptr, g_hwndAccountLV = nullptr;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
wchar_t szOneInputText[1024] = { 0 };
double g_fontScale = 1.0;

struct BUDGET_TREE_DRAG_DETAILS
{
    BUDGET_TREE_DRAG_DETAILS() : _dragging(nullptr), _lastHitItem(nullptr) {};
    HTREEITEM  _dragging;
    HTREEITEM  _lastHitItem;
} g_btDragDetails;

struct TREE_COLUMN_SIZES
{
    TREE_COLUMN_SIZES()
    {
        _name = _budgeted = _inCategories = _balance = _available = _endSpot = 0;
    }

    int _name;
    int _budgeted;
    int _inCategories;
    int _balance;
    int _available;
    int _endSpot;
} g_columnSizes;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void updateGuiValues(HTREEITEM item = NULL);
void populateTransactions();
int serializeState();
void positionControls();
void calcColumnSpots();
void getColumnSpots();
void addAccount(Account* pAccount);
HTREEITEM treeItemFromAccount(AccountBase* pItem, HTREEITEM hItem = TVI_ROOT);
HTREEITEM addTreeItem(HTREEITEM hParentItem, AccountBase* pAccount, HTREEITEM hInsertAfter = TVI_LAST);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SIMPLEBUDGET, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLEBUDGET));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

const char* getPersistFile()
{
    static char szBuff[1024] = { 0 };
    GetModuleFileNameA(NULL, szBuff, 1024);

    char* slashSpot = strrchr(szBuff, '\\');
    *slashSpot = 0;

    strcat_s(szBuff, 1024, "\\SimpleBudget.xml");

    return szBuff;
}

void setupPaymentCategoryForAccount(Account* pAccount)
{
    if (pAccount->paidFromCategoryId() == MAXINT) {
        std::wstring catName = L"Pay ";
        catName += pAccount->getName();

        // See if already have a category 
        auto categories = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
        auto groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);

        for (size_t i = 0; i < categories.size(); i++)
        {
            if (_wcsicmp(catName.c_str(), categories[i]->getName()) == 0) {
                // This seems to be the right category
                pAccount->setPaidFromCategoryId(categories[i]->getId());
                return;
            }
        }

        // We need to create a new category. First find a group to own it.
        AccountGroup* pGroup = nullptr;
        HTREEITEM hItemGroup = nullptr;
        std::wstring groupName = L"Credit cards";

        for (size_t i = 0; i < groups.size(); i++)
        {
            if (_wcsicmp(groupName.c_str(), groups[i]->getName()) == 0) {
                pGroup = (AccountGroup * )groups[i];
                hItemGroup = treeItemFromAccount(pGroup);
            }
        }

        if (!pGroup)
        {
            pGroup = new AccountGroup;
            pGroup->setName(groupName.c_str());
            hItemGroup = addTreeItem(TVI_ROOT, pGroup);
        }


        // No existing category. Make one.
        AccountCategory* pCategory = new AccountCategory;
        pCategory->setName(catName.c_str());
        pCategory->setDefaultTransactionPayToAccount(pAccount);
        pCategory->setIsExpense(true);
        pCategory->setIsVirtualAccount(true);

        pAccount->setPaidFromCategoryId(pCategory->getId());

        pGroup->addChild(pCategory);
        addTreeItem(hItemGroup, pCategory);
    }
}

// Finds an AccountBase associated with a tree item
AccountBase* accountFromItem(HTREEITEM item)
{
    TVITEMEX tvi = { 0 };
    tvi.hItem = item;
    tvi.mask = TVIF_PARAM;

    TreeView_GetItem(g_hwndTV, &tvi);

    return (AccountBase*)tvi.lParam;
}

HTREEITEM treeItemFromAccount(AccountBase* pItem, HTREEITEM hItem /*= TVI_ROOT*/ )
{
    if (hItem == TVI_ROOT) {
        hItem = TreeView_GetRoot(g_hwndTV);
    }

    // Check self and siblings
    while (hItem) {
        if (pItem == accountFromItem(hItem)) {
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

void setItemText(HTREEITEM hItem, const wchar_t* szText)
{
    TVITEMEX tvi = { 0 };
    tvi.hItem = hItem;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = const_cast<wchar_t*>(szText);

    SendMessageW(g_hwndTV, TVM_SETITEM, 0, (LPARAM)&tvi);
}

void setFont(HWND hCtrl)
{
    // Adjust font
    HFONT hFont = (HFONT)SendMessage(hCtrl, WM_GETFONT, NULL, NULL);
    LOGFONT lf = { 0 };
    GetObject(hFont, sizeof(lf), &lf);
    lf.lfHeight = (LONG)(lf.lfHeight * FONT_SCALE_FACTOR);
    HFONT hNewFont = CreateFontIndirect(&lf);
    SendMessage(hCtrl, WM_SETFONT, (WPARAM)hNewFont, true);
}

void createAccountListControl()
{
    // IDC_SIMPLEBUDGET_ACCOUNT_LIST
    RECT rClient = { 0 };
    GetClientRect(g_hTopWindow, &rClient);

    g_hwndAccountLV = CreateWindowEx(0, WC_LISTVIEW, TEXT("Simple Budget list"), WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_SHOWSELALWAYS | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 0,0, g_hTopWindow,
        (HMENU)IDC_SIMPLEBUDGET_ACCOUNT_LIST, g_hInst, NULL);

    ListView_SetExtendedListViewStyle(g_hwndAccountLV, LVS_EX_FULLROWSELECT);

    setFont(g_hwndAccountLV);

    LVCOLUMN column = { 0 };
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(L"Account");
    ListView_InsertColumn(g_hwndAccountLV, 0, &column);

    column.pszText = const_cast<wchar_t*>(L"Balance");
    ListView_InsertColumn(g_hwndAccountLV, 1, &column);

    for (int i = 0; i < 2; i++) {
        ListView_SetColumnWidth(g_hwndAccountLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

}
void createTransactionListControl()
{
    RECT rClient = { 0 };
    GetClientRect(g_hTopWindow, &rClient);

    g_hwndTransactionLV = CreateWindowEx(0, WC_LISTVIEW, TEXT("Simple Budget list"), WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_SHOWSELALWAYS | LVS_REPORT | LVS_SINGLESEL,
        rClient.left, rClient.bottom/2,
        rClient.right,
        rClient.bottom/2,
        g_hTopWindow,
        (HMENU)IDC_SIMPLEBUDGET_TRANSACTION_LIST,
        g_hInst,
        NULL);

    ListView_SetExtendedListViewStyle(g_hwndTransactionLV, LVS_EX_FULLROWSELECT);

    setFont(g_hwndTransactionLV);

    LVCOLUMN column = { 0 };
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(L"Date");
    ListView_InsertColumn(g_hwndTransactionLV, 0, &column);

    column.pszText = const_cast<wchar_t*>(L"Payee");
    ListView_InsertColumn(g_hwndTransactionLV, 1, &column);

    column.pszText = const_cast<wchar_t*>(L"Category");
    ListView_InsertColumn(g_hwndTransactionLV, 2, &column);

    column.pszText = const_cast<wchar_t*>(L"Description");
    ListView_InsertColumn(g_hwndTransactionLV, 3, &column);

    column.pszText = const_cast<wchar_t*>(L"Outflow");
    ListView_InsertColumn(g_hwndTransactionLV, 4, &column);

    column.pszText = const_cast<wchar_t*>(L"Inflow");
    ListView_InsertColumn(g_hwndTransactionLV, 5, &column);

    for (int i = 0; i < 6; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

void CreateTreeControl()
{
    // Create tree header
    g_hwndHeader = CreateWindowEx(0, WC_HEADER, nullptr,  WS_CHILD | HDS_FULLDRAG, 0, 0, 0, 0, g_hTopWindow, (HMENU)IDC_SIMPLEBUDGET_TREEHEADER, g_hInst, nullptr);

 
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

    g_hwndTV = CreateWindowEx(0,WC_TREEVIEW,TEXT("Simple Budget Tree"), WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        rClient.left, rClient.top,
        rClient.right,
        rClient.bottom/2,
        g_hTopWindow,
        (HMENU)IDC_SIMPLEBUDGET_TREE,
        g_hInst,
        NULL);

    // Adjust font
    setFont(g_hwndTV);
    calcColumnSpots();

    // Create tree
    HDITEM item = { 0 };
    item.cxy = g_columnSizes._name;
    item.pszText = const_cast<wchar_t*>(L"Category");
    item.mask = HDI_TEXT | HDI_WIDTH;
    Header_InsertItem(g_hwndHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Budgeted cashflow");
    item.cxy = g_columnSizes._budgeted;
    Header_InsertItem(g_hwndHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Actual cashflow");
    item.cxy = g_columnSizes._inCategories;
    Header_InsertItem(g_hwndHeader, 99, &item);

    item.pszText = const_cast<wchar_t*>(L"Virtual balance");
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

    DWORD dwAttribs = ::GetFileAttributesA(getPersistFile());
    if (dwAttribs != INVALID_FILE_ATTRIBUTES) {
        std::vector<AccountBase*> topLevelItems;
        deserialize(getPersistFile(), topLevelItems);
        for (size_t i = 0; i < topLevelItems.size(); i++)
        {
            addTreeItem(TVI_ROOT, topLevelItems[i]);
        }
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        MessageBox(g_hTopWindow, L"Unable to load file", L"Error", MB_OK);
        exit(0);
    }
    else {
        Account* pAccount = new Account;
        pAccount->setName(L"Checking");
        addTreeItem(TVI_ROOT, pAccount);
    }
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIMPLEBUDGET));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SIMPLEBUDGET);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SIMPLEBUDGET));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance; // Store instance handle in our global variable

    g_hTopWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!g_hTopWindow)
   {
      return FALSE;
   }

   //InitCommonControls();
   INITCOMMONCONTROLSEX icex = { 0 };
   icex.dwSize = sizeof(icex);
   icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
   InitCommonControlsEx(&icex);
   createAccountListControl();
   CreateTreeControl();
   createTransactionListControl();

   HTREEITEM hRoot = TreeView_GetRoot(g_hwndTV);
   if (hRoot) {
       TreeView_SelectItem(g_hwndTV, hRoot);
   }

   populateTransactions();
   positionControls();

   updateGuiValues();

   ShowWindow(g_hTopWindow, nCmdShow);
   UpdateWindow(g_hTopWindow);

   return TRUE;
}

INT_PTR CALLBACK OneInputProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        szOneInputText[0] = 0;
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_ONE_INPUT_TEXT, szOneInputText, 1024);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

bool addTransaction(HTREEITEM hItem)
{
    bool didSimonthing = false;
    AccountBase* pItem = accountFromItem(hItem);
    TransactionDialog transactionDlg;
    transactionDlg.setCategoryId(pItem->getId());
    AccountCategory* pCategory = nullptr;
    if (pItem->getType() == AccountBase::CATEGORY) {
        pCategory = (AccountCategory*)pItem;
        transactionDlg.setPayee(pCategory->getDefaultTransactionPayee());
        transactionDlg.setAmount(pCategory->getDefaultTransactionAmount());

        if (pCategory->getDefaultTransactionPayToAccount()) {
            transactionDlg.setAccountToId(pCategory->getDefaultTransactionPayToAccount()->getId());
        }
        else {
            transactionDlg.setAccountToId(MAXINT);
        }

        if (pCategory->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pCategory->getDefaultTransactionPayFromAccount()->getId());
        }
        else {
            transactionDlg.setAccountFromId(MAXINT);
        }

        transactionDlg.setIsExpense(pCategory->isExpense());
        if (pCategory->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pCategory->getDefaultTransactionPayFromAccount()->getId());
        }
    }

    if (transactionDlg.ShowDialog())
    {
        Transaction::addTransaction(transactionDlg.getAmount(), transactionDlg.getAccountFromId(), transactionDlg.getAccountToId(), transactionDlg.getCategoryId(), transactionDlg.getPayee(), transactionDlg.getDescription());

        // Apply the amount to the transaction category 
        pCategory->addBalance(transactionDlg.getAmount());

        // Apply the amount to the account
        AccountBase* acct = AccountBase::lookupById(transactionDlg.getAccountFromId());
        if (acct) {
            acct->addBalance(transactionDlg.getAmount());
        }

        AccountBase* pAccountTo = AccountBase::lookupById(transactionDlg.getAccountToId());
        if (!pAccountTo)
        {
            Account* pAccount = dynamic_cast<Account*>(acct);
            if (pAccount)
            {
                pAccountTo = AccountBase::lookupById(pAccount->paidFromCategoryId());
            }
        }
        if (pAccountTo) {
            pAccountTo->addBalance(-transactionDlg.getAmount());
        }

        updateGuiValues();
        populateTransactions();
        didSimonthing = true;
    }
    return didSimonthing;
}
bool getOneText(std::wstring& result)
{
    szOneInputText[0] = 0;
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ONE_INPUT), g_hTopWindow, OneInputProc);
    result = szOneInputText;
    return result[0] ? true : false;
}

bool editCategory(AccountCategory* pCategory)
{
    CategoryDialog dlg;
    dlg.setBudget(pCategory->getBudgeted());
    dlg.setCategoryName(pCategory->getName());
    dlg.setDefaultTransactionPayFromAccount(pCategory->getDefaultTransactionPayFromAccount());
    dlg.setDefaultTransactionPayToAccount(pCategory->getDefaultTransactionPayToAccount());
    dlg.setDefaultTransactionAmount(pCategory->getDefaultTransactionAmount());
    dlg.setDefaultTransactionPayee(pCategory->getDefaultTransactionPayee());
    dlg.setIsIncome(!pCategory->isExpense());
    dlg.setIsVirtualAccount(pCategory->isVirtualAccount());
    if (dlg.ShowDialog())
    {
        pCategory->setBudgeted(dlg.getBudget());
        pCategory->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
        pCategory->setDefaultTransactionPayToAccount(dlg.getDefaultTransactionPayToAccount());
        pCategory->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pCategory->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pCategory->setIsExpense(!dlg.getIsIncome());
        pCategory->setIsVirtualAccount(dlg.getIsVirtual());
        pCategory->setName(dlg.getCategoryName());
        
        updateGuiValues();

        return true;
    }
    return false;
}

bool processContextMenu(DWORD dwCommand, HTREEITEM hItem, AccountBase* pItem)
{
    std::wstring temp;
    bool didSomething = false;
    AccountGroup* pParentGroup = nullptr;
    if (pItem->getType() == AccountBase::GROUP){
        pParentGroup = (AccountGroup*)pItem;
    }
    switch (dwCommand)
    {
    case ID_ENVELOPE_RESERVE_FUNDS:
        if (pItem && getOneText(temp)) {
            // Add funds to this item
            int amount = Utils::dollarsStringToPennies(temp.c_str());
            pItem->addBalance(amount);

            updateGuiValues();
            didSomething = true;
        }
        break;
    case ID_ROOT_EDITCATEGORY:
        //zzzz
        didSomething = editCategory((AccountCategory*)pItem);
        break;
    case ID_ROOT_ADD_TOP_LEVEL_ACCOUNT:
        if (pItem && getOneText(temp)) {
            Account* pAccount = new Account;
            pAccount->setName(temp.c_str());
            addTreeItem(TVI_ROOT, pAccount);
            didSomething = true;
        }
        break;
    case ID_ROOT_SETBUDGETEDAMOUNT:
        if (pItem && getOneText(temp)) {
            pItem->setBudgeted(Utils::dollarsStringToPennies(temp.c_str()));
            updateGuiValues();
            didSomething = true;
        }
        break;
    case ID_ROOT_ADD_GROUP:
        if (pItem && pParentGroup && getOneText(temp)) {
            AccountGroup* pGroup = new AccountGroup;
            pGroup->setName(szOneInputText);
            pParentGroup->addChild(pGroup);
            addTreeItem(hItem, pGroup);
            didSomething = true;
        }
        break;
    case ID_ROOT_ADDENVELOPE:
    {
        CategoryDialog dlg;
        if (pParentGroup && dlg.ShowDialog()) {
            AccountCategory* pCategory = new AccountCategory;
            pCategory->setName(dlg.getCategoryName());
            pCategory->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
            pCategory->setDefaultTransactionPayToAccount(dlg.getDefaultTransactionPayToAccount());
            pCategory->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
            pCategory->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
            pCategory->setIsExpense(!dlg.getIsIncome());
            pCategory->setIsVirtualAccount(dlg.getIsVirtual());
            pCategory->setBudgeted(dlg.getBudget());

            pParentGroup->addChild(pCategory);
            addTreeItem(hItem, pCategory);
            didSomething = true;
        }
        break;
    }
    case ID_ROOT_ADDTransaction:
    {
        didSomething = addTransaction(hItem);
        break;
    }
    }

    if (dwCommand >= ID_ROOT_ENVELOPE_MOVE_TO_START && dwCommand < ID_ROOT_ENVELOPE_MOVE_TO_END) {
        // This is a "move to" command. Remove from current parent and add to new parent
        AccountBase* pParent = pItem->getParent();
        AccountBase* pNewParent = AccountBase::lookupById(dwCommand - ID_ROOT_ENVELOPE_MOVE_TO_START);

        HTREEITEM hNewParent = treeItemFromAccount(pNewParent);
        HTREEITEM hSelf = treeItemFromAccount(pItem);

        if (pParent && pNewParent && pParent->removeChild(pItem)) 
        {
            pNewParent->addChild(pItem);
            TreeView_DeleteItem(g_hwndTV, hSelf);
            addTreeItem(hNewParent, pItem);
            updateGuiValues();
            didSomething = true;
        }
    }
    return didSomething;
}

void editTransaction(Transaction* pTransaction, int listviewItemIndex)
{
    TransactionDialog dlg;
    dlg.setAmount(pTransaction->getAmount());
    dlg.setPayee(pTransaction->getPayee());
    dlg.setDescription(pTransaction->getDescription());
    dlg.setAccountFromId(pTransaction->getAccountFromId());
    dlg.setAccountToId(pTransaction->getAccountToId());
    dlg.setCategoryId(pTransaction->getCategoryId());

    if (dlg.ShowDialog())
    {
        // This is actually kind of complicated. 
        // We need to undo the original transacation first.
        AccountBase* pCategory = AccountBase::lookupById(pTransaction->getCategoryId());
        AccountBase* pAccountFrom = AccountBase::lookupById(pTransaction->getAccountFromId());
        AccountBase* pAccountTo = AccountBase::lookupById(pTransaction->getAccountToId());

        if (!pAccountTo)
        {
            Account* pAccount = dynamic_cast<Account*>(pAccountFrom);
            if (pAccount)
            {
                pAccountTo = AccountBase::lookupById(pAccount->paidFromCategoryId());
            }
        }

        int origAmount = pTransaction->getAmount();
        pCategory->addBalance(-origAmount);
        if (pAccountFrom) {
            pAccountFrom->addBalance(-origAmount);
        }
        if (pAccountTo) {
            pAccountTo->addBalance(origAmount);
        }

        // Now this is basically the same as add
        pTransaction->setDetails(dlg.getAmount(), dlg.getPayee(), dlg.getDescription(), dlg.getCategoryId(), dlg.getAccountFromId(), dlg.getAccountToId());

        pCategory = AccountBase::lookupById(pTransaction->getCategoryId());
        pAccountFrom = AccountBase::lookupById(pTransaction->getAccountFromId());
        pAccountTo = AccountBase::lookupById(pTransaction->getAccountToId());

        if (!pAccountTo)
        {
            Account* pAccount = dynamic_cast<Account*>(pAccountFrom);
            if (pAccount) {
                pAccountTo = AccountBase::lookupById(pAccount->paidFromCategoryId());
            }
        }


        pCategory->addBalance(pTransaction->getAmount());

        // Add the amount to the account
        if (pAccountFrom) {
            pAccountFrom->addBalance(pTransaction->getAmount());
        }

        if (pAccountTo) {
            pAccountTo->addBalance(-pTransaction->getAmount());
        }

        // Payee
        std::wstring payee = pTransaction->getPayee();
        if (pTransaction->getAccountToId() != MAXINT && pAccountTo) {
            wchar_t szBuff[1024] = { 0 };
            swprintf(szBuff, 1024, L"%s->%s", pAccountFrom->getName(), pAccountTo->getName());
            payee = szBuff;
        }
        ListView_SetItemText(g_hwndTransactionLV, listviewItemIndex, 1, const_cast<wchar_t*>(payee.c_str()));

        // Category
        AccountBase* ati = AccountBase::lookupById(pTransaction->getCategoryId());
        if (ati) {
            ListView_SetItemText(g_hwndTransactionLV, listviewItemIndex, 2, const_cast<wchar_t*>(ati->getName()));
        }

        // Description
        ListView_SetItemText(g_hwndTransactionLV, listviewItemIndex, 3, const_cast<wchar_t*>(pTransaction->getDescription()));

        std::wstring amount = Utils::penniesToDollarsString(abs(pTransaction->getAmount()));
        ListView_SetItemText(g_hwndTransactionLV, listviewItemIndex, 4, const_cast<wchar_t*>(amount.c_str()));
        ListView_SetItemText(g_hwndTransactionLV, listviewItemIndex, 5, const_cast<wchar_t*>(L""));

        // Recalculate dollars in tree and save
        updateGuiValues();
        serializeState();
    }
}

LRESULT customDrawListControl(NMLVCUSTOMDRAW* pCustomDrawItem)
{
    LRESULT ret = CDRF_NOTIFYITEMDRAW;

    static const COLORREF SEL_COLOR = GetSysColor(COLOR_HIGHLIGHT);
    static const COLORREF SEL_TEXT_COLOR = GetSysColor(COLOR_HIGHLIGHTTEXT);
    Transaction* pItem = (Transaction*)pCustomDrawItem->nmcd.lItemlParam;

    if (pItem)
    {

        // https://learn.microsoft.com/en-us/windows/win32/controls/list-view-item-states
        //if (pCustomDrawItem->iStateId & LVIS_SELECTED)
        //{
        //}
    }


    return ret;
}

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
    else{ 
        crRet = SetTextColor(hDC, bSelected ? NEGATIVE_BALANCE_COLOR_SEL : NEGATIVE_BALANCE_COLOR );
    }
    return crRet;
}

LRESULT customDrawTreeItem(NMTVCUSTOMDRAW* pCustomDrawItem)
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
        HTREEITEM hItem = (HTREEITEM)pCustomDrawItem->nmcd.dwItemSpec;
        HTREEITEM hSelItem = (HTREEITEM)TreeView_GetSelection(g_hwndTV);
        COLORREF cr = { 0 }, crText = RGB(0,0,0);

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
        
        pItem->getType() == AccountBase::ACCOUNT ? ACCOUNT_COLOR : GROUP_COLOR;
        bool selected = hItem == hSelItem;

        if (selected) {
            cr = SEL_COLOR;
            crText = SEL_TEXT_COLOR;
        }
        COLORREF oldTextColor = SetTextColor(pCustomDrawItem->nmcd.hdc, crText);

        if (pCustomDrawItem->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
        {
            HBRUSH brush = CreateSolidBrush(cr);
            HBRUSH oldBrush = (HBRUSH)SelectObject(pCustomDrawItem->nmcd.hdc, brush);

            HPEN pen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
            HBRUSH oldpen = (HBRUSH)SelectObject(pCustomDrawItem->nmcd.hdc, pen);

            RECT rcFull = { 0 }, rcText = { 0 };
            TreeView_GetItemRect(g_hwndTV, hItem, &rcText, true);
            TreeView_GetItemRect(g_hwndTV, hItem, &rcFull, false);

            Rectangle(pCustomDrawItem->nmcd.hdc, rcText.left, rcFull.top, rcFull.right, rcFull.bottom + 1);
            SelectObject(pCustomDrawItem->nmcd.hdc, oldBrush);
            SelectObject(pCustomDrawItem->nmcd.hdc, oldpen);
            DeleteObject(brush);
            DeleteObject(pen);

           
            ret |= CDRF_NOTIFYPOSTPAINT;
        }
        else if (pCustomDrawItem->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
        {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
            HGDIOBJ oldBrush = SelectObject(pCustomDrawItem->nmcd.hdc, GetStockObject(HOLLOW_BRUSH));
            HGDIOBJ oldPen = SelectObject(pCustomDrawItem->nmcd.hdc, pen);
            int oldBkMode = SetBkMode(pCustomDrawItem->nmcd.hdc, TRANSPARENT);

            //int dbalance = pItem->getBalance() + pItem->allocatedToSubAccounts();
            int iBalance = pItem->getBalance();
            std::wstring balance = Utils::penniesToDollarsString(iBalance);

            int iBudget = pItem->getType() == AccountBase::CATEGORY ? pItem->getBudgeted() : pItem->budgetedToSubAccounts();
            std::wstring budgeted = Utils::penniesToDollarsString(iBudget);

            int iEnvelopes = pItem->getType() == AccountBase::CATEGORY ? pItem->getBalance() : pItem->allocatedToSubAccounts();
            std::wstring allocated = Utils::penniesToDollarsString(iEnvelopes);

            int iAvailable = pItem->getBalance() - abs(pItem->allocatedToSubAccounts());
            std::wstring availableBalance = Utils::penniesToDollarsString(iAvailable);

            int iTransactionSum = pItem->getCurrentMonthTransacationTotal();
            std::wstring transactionSum = Utils::penniesToDollarsString(iTransactionSum);

            getColumnSpots();

            int budgetedSpot = g_columnSizes._name;
            int inEnvelopes = budgetedSpot + g_columnSizes._budgeted;
            int balanceSpot = inEnvelopes + g_columnSizes._inCategories;
            int availableBalanceSpot = balanceSpot + g_columnSizes._balance;
            int endSpot = availableBalanceSpot + g_columnSizes._endSpot;

            // Budgeted dollars
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iBudget, selected, false);
            TextOut(pCustomDrawItem->nmcd.hdc, budgetedSpot, pCustomDrawItem->nmcd.rc.top, budgeted.c_str(), (int)budgeted.length());

            // Monthly transaction sum
            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iTransactionSum, selected, false);
            TextOut(pCustomDrawItem->nmcd.hdc, inEnvelopes, pCustomDrawItem->nmcd.rc.top, transactionSum.c_str(), (int)transactionSum.length());


            if (pItem->getType() == AccountBase::CATEGORY)
            {
                AccountCategory* pCategory = (AccountCategory*)pItem;
                if (pCategory->isVirtualAccount()) {
                    setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iBalance, selected, true);
                    TextOut(pCustomDrawItem->nmcd.hdc, balanceSpot, pCustomDrawItem->nmcd.rc.top, balance.c_str(), (int)balance.length());
                }
            }

            //if (pItem->getType() == AccountBase::ACCOUNT)
            //{
            //    setTextColorForAmount(pCustomDrawItem->nmcd.hdc, iAvailable, selected, true);
            //    TextOut(pCustomDrawItem->nmcd.hdc, availableBalanceSpot, pCustomDrawItem->nmcd.rc.top, availableBalance.c_str(), (int)availableBalance.length());

            //}

            // Draw line above
            MoveToEx(pCustomDrawItem->nmcd.hdc, pCustomDrawItem->nmcd.rc.left, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, pCustomDrawItem->nmcd.rc.right, pCustomDrawItem->nmcd.rc.top);

            // Draw columns line
            MoveToEx(pCustomDrawItem->nmcd.hdc, budgetedSpot-2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, budgetedSpot-2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, inEnvelopes-2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, inEnvelopes-2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, balanceSpot-2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, balanceSpot-2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, availableBalanceSpot-2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, availableBalanceSpot-2, pCustomDrawItem->nmcd.rc.bottom);

            MoveToEx(pCustomDrawItem->nmcd.hdc, endSpot - 2, pCustomDrawItem->nmcd.rc.top, nullptr);
            LineTo(pCustomDrawItem->nmcd.hdc, endSpot - 2, pCustomDrawItem->nmcd.rc.bottom);

            RECT rcText = { 0 }, rcFull = { 0 };

            TreeView_GetItemRect(g_hwndTV, hItem, &rcText, true);
            TreeView_GetItemRect(g_hwndTV, hItem, &rcFull, false);

            HBRUSH brush = CreateSolidBrush(cr);
            SelectObject(pCustomDrawItem->nmcd.hdc, brush);

            DeleteObject(pen);
            pen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
            SelectObject(pCustomDrawItem->nmcd.hdc, pen);

            Rectangle(pCustomDrawItem->nmcd.hdc, rcText.left, rcFull.top + 1, rcText.right + 1, rcFull.bottom);

            setTextColorForAmount(pCustomDrawItem->nmcd.hdc, 0, selected, false);
            TextOut(pCustomDrawItem->nmcd.hdc, rcText.left, rcText.top, pItem->getName(), (int)wcslen(pItem->getName()));
            SetTextColor(pCustomDrawItem->nmcd.hdc, oldTextColor);

            SelectObject(pCustomDrawItem->nmcd.hdc, oldBrush);
            SelectObject(pCustomDrawItem->nmcd.hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);


            SetBkMode(pCustomDrawItem->nmcd.hdc, oldBkMode);
            DeleteObject(pen);
        }

    } // if item
    return ret;
}

void doTreeDrag(LPNMTREEVIEW nmTreeItem)
{
    // Tell the tree-view control to create an image to use 
    // for dragging. 
    HIMAGELIST himl = TreeView_CreateDragImage(g_hwndTV, nmTreeItem->itemNew.hItem);
//    HIMAGELIST himl = TreeView_GetImageList(g_hwndTV, TVSIL_NORMAL);

    // Start the drag operation. 
    ImageList_BeginDrag(himl, 0, 0, 0);
    ImageList_DragEnter(g_hwndTV, nmTreeItem->ptDrag.x, nmTreeItem->ptDrag.x);

    // Hide the mouse pointer, and direct mouse input to the 
    // parent window. 
    ShowCursor(FALSE);
    SetCapture(GetParent(g_hwndTV));
    g_btDragDetails._dragging = nmTreeItem->itemNew.hItem;

    return;
}

void handleMouseMove(LONG xCur, LONG yCur)
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

void handleLButtonUp(LONG xCur, LONG yCur)
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
        if (htiDest != NULL)
        {
            auto dropSource = accountFromItem(g_btDragDetails._dragging);
            auto dropTarget = accountFromItem(htiDest);

            HTREEITEM hParent = TreeView_GetParent(g_hwndTV, g_btDragDetails._dragging);
            HTREEITEM hNewParent = TreeView_GetParent(g_hwndTV, htiDest);

            if (dropSource != dropTarget && hParent && hNewParent ) {
                // Move in the data structure
                auto sourceParent = dropSource->getParent();
                auto destParent = dropTarget->getParent();
                if (sourceParent && destParent) {

                    // Remove from the old location
                    sourceParent->removeChild(dropSource);
                    TreeView_DeleteItem(g_hwndTV, g_btDragDetails._dragging);

                    // Add to the new location
                    destParent->addChild(dropSource, dropTarget);
                    addTreeItem(hNewParent, dropSource, htiDest);

                    // Save
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
    return;
}

void editAccount(Account* pAccount)
{
    AccountDlg dlg;
    dlg.setBalance(pAccount->getBalance());
    dlg.setBudgeted(pAccount->isBudgeted());
    dlg.setName(pAccount->getName());
    dlg.setCreditCard(pAccount->paidFromCategoryId() != MAXINT);
    if (dlg.ShowDialog())
    {
        pAccount->addBalance(-pAccount->getBalance());
        pAccount->addBalance(dlg.getBalance());
        pAccount->setName(dlg.getName());
        pAccount->setIsBudgeted(dlg.getBudgeted());
        if (dlg.getCreditCard()) {
            setupPaymentCategoryForAccount(pAccount);
        }
        updateGuiValues();
        serializeState();
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_LBUTTONUP:
        handleLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    case WM_MOUSEMOVE:
        handleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_FILE_DECREASEFONTSIZE:
            g_fontScale -= .2;
            break;
        case ID_FILE_INCREASEFONTSIZE:
            g_fontScale += .2;
            break;
        case ID_FILE_ADDACCOUNT:
        {
            AccountDlg dlg;
            dlg.setBudgeted(true);
            if (dlg.ShowDialog()) {
                Account* pAccount = new Account;
                pAccount->setName(dlg.getName());
                pAccount->addBalance(dlg.getBalance());
                if (dlg.getCreditCard()) {
                    setupPaymentCategoryForAccount(pAccount);
                }
                addTreeItem(TVI_ROOT, pAccount);
                serializeState();
            }
            break;
        }
        case ID_FILE_ADDCATEGORY:
        {//zzzz
            std::wstring temp;
            if (getOneText(temp)) {
                AccountGroup* pGroup = new AccountGroup;
                pGroup->setName(temp.c_str());
                addTreeItem(TVI_ROOT, pGroup);
                serializeState();
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_NOTIFY:
    {
        LPNMHDR pHeader = (LPNMHDR)lParam;
        if (pHeader->idFrom == IDC_SIMPLEBUDGET_ACCOUNT_LIST)
        {
            if (pHeader->code == NM_DBLCLK)
            {
                // See if we hit an item
                POINT pt = { 0 };
                LVHITTESTINFO lvhi = { 0 };
                GetCursorPos(&pt);
                lvhi.pt = pt;
                ScreenToClient(g_hwndAccountLV, &lvhi.pt);
                ListView_HitTest(g_hwndAccountLV, &lvhi);
                if (lvhi.iItem == -1) {
                    ListView_SubItemHitTest(g_hwndTransactionLV, &lvhi);
                }

                if (lvhi.iItem != -1)
                {
                    LVITEM item = { 0 };
                    item.iItem = lvhi.iItem;
                    item.mask = LVIF_PARAM;
                    ListView_GetItem(g_hwndAccountLV, &item);
                    Account* pAccount = (Account*)item.lParam;
                    editAccount(pAccount);
                }

            }
            else if (pHeader->code == NM_RCLICK) 
            {
                // See if we hit an item
                POINT pt = { 0 };
                LVHITTESTINFO lvhi = { 0 };
                GetCursorPos(&pt);
                lvhi.pt = pt;
                ScreenToClient(g_hwndAccountLV, &lvhi.pt);
                ListView_HitTest(g_hwndAccountLV, &lvhi);
                if (lvhi.iItem == -1) {
                    ListView_SubItemHitTest(g_hwndTransactionLV, &lvhi);
                }

                if (lvhi.iItem != -1) 
                {
                    HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_ACCOUNT));
                    hMenu = GetSubMenu(hMenu, 0);
                    DWORD dwCode = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    LVITEM item = { 0 };
                    item.iItem = lvhi.iItem;
                    item.mask = LVIF_PARAM;
                    ListView_GetItem(g_hwndAccountLV, &item);
                    Account* pAccount = (Account*)item.lParam;
                    std::wstring temp;
                    switch (dwCode)
                    {
                    case ID_ACCOUNT_RENAME:
                        if (pAccount && getOneText(temp)) {
                            pAccount->setName(temp.c_str());
                            item.mask = LVIF_TEXT;
                            item.pszText = const_cast<wchar_t*>(temp.c_str());
                            ListView_SetItem(g_hwndAccountLV, &item);
                            serializeState();
                        }

                        break;
                    case ID_ROOT_EDITACCOUNT:
                    {
                        editAccount(pAccount);
                        break;
                    }
                    }
                }
            }

        }
        if (pHeader->idFrom == IDC_SIMPLEBUDGET_TREEHEADER)
        {
            if (pHeader->code == HDN_ENDTRACK)
            {
                // Redraw the tree
                RedrawWindow(g_hwndTV, nullptr, nullptr, RDW_INVALIDATE);
            }
        }
        if (pHeader->idFrom == IDC_SIMPLEBUDGET_TREE)
        {
            if (pHeader->code == TVN_SELCHANGED)
            {
                populateTransactions();
            }
            else if (pHeader->code == TVN_BEGINDRAG)
            {
                // https://learn.microsoft.com/en-us/windows/win32/controls/drag-a-tree-view-item
                doTreeDrag((LPNMTREEVIEW)lParam);
            }
            else if (pHeader->code == NM_CUSTOMDRAW)
            {
                return customDrawTreeItem((NMTVCUSTOMDRAW*)lParam);
            }
            else if (pHeader->code == NM_CLICK)
            {
                POINT pt = { 0 };
                TVHITTESTINFO tvhi = { 0 };
                GetCursorPos(&pt);
                tvhi.pt = pt;
                ScreenToClient(g_hwndTV, &tvhi.pt);
                TreeView_HitTest(g_hwndTV, &tvhi);
                if (tvhi.hItem) {
                    TreeView_Select(g_hwndTV, tvhi.hItem, TVGN_CARET);
                }

            }
            else if (pHeader->code == NM_DBLCLK)
            {
                AccountBase* pItem = nullptr, * pParentItem = nullptr;
                POINT pt = { 0 };
                TVHITTESTINFO tvhi = { 0 };
                GetCursorPos(&pt);
                tvhi.pt = pt;
                ScreenToClient(g_hwndTV, &tvhi.pt);

                TreeView_HitTest(g_hwndTV, &tvhi);
                HTREEITEM hItem = tvhi.hItem;
                auto envelope = accountFromItem(hItem);
                if (envelope)
                {
                    if (addTransaction(hItem)){
                        serializeState();
                    }
                }

            }
            else if (pHeader->code == NM_RCLICK)
            {
                AccountBase* pItem = nullptr, *pParentItem = nullptr;
                POINT pt = { 0 };
                TVHITTESTINFO tvhi = { 0 };
                GetCursorPos(&pt);
                tvhi.pt = pt;
                ScreenToClient(g_hwndTV, &tvhi.pt);

                TreeView_HitTest(g_hwndTV, &tvhi);
                HTREEITEM hItem = tvhi.hItem;
                if (tvhi.hItem) {
                    TreeView_Select(g_hwndTV, hItem, TVGN_CARET);
                    pItem = accountFromItem(hItem);
                    HTREEITEM hParent = TreeView_GetParent(g_hwndTV, hItem);
                    pParentItem = accountFromItem(hParent);
                }

                // Show context menu
                if (pItem)
                {
                    HMENU hMenu = nullptr;
                    if (pItem->getType() == AccountBase::GROUP) {
                        hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_GROUP_MENU));
                        hMenu = GetSubMenu(hMenu, 0);
                    }
                    else if (pItem->getType() == AccountBase::CATEGORY) {
                        hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_ENVELOPE_MENU));
                        hMenu = GetSubMenu(hMenu, 0);

                        // Add the "move to group" option
                        bool added = false;
                        std::vector<AccountBase*> groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
                        HMENU groupMenu = CreatePopupMenu();
                        for (size_t i = 0; i < groups.size(); i++) {
                            if (pParentItem && groups[i]->getId() != pParentItem->getId()) {
                                AppendMenu(groupMenu, MF_STRING, ID_ROOT_ENVELOPE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                                added = true;
                            }
                        }

                        if (added) {
                            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)groupMenu, L"Move to group...");
                        }

                        // Add the "move to account" option
                        groups = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
                        HMENU accountMenu = CreatePopupMenu();
                        for (size_t i = 0; i < groups.size(); i++) {
                            if (pParentItem && groups[i]->getId() != pParentItem->getId()) {
                                added = true;
                                AppendMenu(accountMenu, MF_STRING, ID_ROOT_ENVELOPE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                            }
                        }
                        if (added) {
                            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)accountMenu, L"Move to account...");
                        }
                    }
                    else {
                        hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_TREEVIEW));
                        hMenu = GetSubMenu(hMenu, 0);
                    }

                    DWORD dwCode = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    if (dwCode && processContextMenu(dwCode, hItem, pItem)) {
                        updateGuiValues();
                        serializeState();
                    }
                }
            }
        }
        else if (pHeader->idFrom == IDC_SIMPLEBUDGET_TRANSACTION_LIST)
        {
            if (pHeader->code == NM_CUSTOMDRAW)
            {
                return customDrawListControl((NMLVCUSTOMDRAW*)lParam);
            }
            else if (pHeader->code == NM_DBLCLK || pHeader->code == NM_RCLICK )
            {
                // See if we hit an item
                POINT pt = { 0 };
                LVHITTESTINFO lvhi = { 0 };
                GetCursorPos(&pt);
                lvhi.pt = pt;
                ScreenToClient(g_hwndTransactionLV, &lvhi.pt);
                ListView_HitTest(g_hwndTransactionLV, &lvhi);
                if (lvhi.iItem == -1) {
                    ListView_SubItemHitTest(g_hwndTransactionLV, &lvhi);
                }

                if (lvhi.iItem != -1) {
                    LVITEM item = { 0 };
                    item.mask = LVIF_PARAM;
                    item.iItem = lvhi.iItem;
                    ListView_GetItem(g_hwndTransactionLV, &item);
                    Transaction* pTransaction = (Transaction*)item.lParam;

                    if (pTransaction && pHeader->code == NM_DBLCLK) {
                        editTransaction(pTransaction, lvhi.iItem);
                    }
                    else if (pTransaction && pHeader->code == NM_RCLICK) {
                        HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_Transaction));
                        hMenu = GetSubMenu(hMenu, 0);
                        DWORD dwCode = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                        if (dwCode == ID_ROOT_EDITTransaction) {
                            editTransaction(pTransaction, lvhi.iItem);
                        }
                    }
                }
            }
        }
        break;
    }
    case WM_SIZE:
        positionControls();
        break;
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void updateGuiValues(HTREEITEM item /*= NULL*/)
{
    bool topLevel = item == nullptr;
    if (!item)
    {
        SendMessage(g_hwndAccountLV, WM_SETREDRAW, FALSE, 0);
        item = TreeView_GetRoot(g_hwndTV);

        // Update account info
        int nAccounts = ListView_GetItemCount(g_hwndAccountLV);

        for (int i = 0; i < nAccounts; i++)
        {
            LVITEM listitem = { 0 };
            listitem.mask = LVIF_PARAM;
            listitem.iItem = i;
            ListView_GetItem(g_hwndAccountLV, &listitem);
            Account* pAccount = (Account*)listitem.lParam;
            ListView_SetItemText(g_hwndAccountLV, i, 0, const_cast<wchar_t*>(pAccount->getName()));

            std::wstring amount = Utils::penniesToDollarsString(pAccount->getBalance());
            ListView_SetItemText(g_hwndAccountLV, i, 1, const_cast<wchar_t*>(amount.c_str()));

        }
    }

    if (item)
    {
        if (topLevel) {
            SendMessage(g_hwndTV, WM_SETREDRAW, FALSE, 0);
        }

        // Get self
        TVITEMEX tvi = { 0 };
        tvi.hItem = item;
        tvi.mask = TVIF_PARAM;

        TreeView_GetItem(g_hwndTV, &tvi);

        Account* pAccount = (Account*)tvi.lParam;
        setItemText(item, pAccount->getName());

        // Get children
        HTREEITEM child = TreeView_GetChild(g_hwndTV, item);
        if (child)
        {
            updateGuiValues(child);
        }

        // Get next
        item = TreeView_GetNextSibling(g_hwndTV, item);
        if (item)
        {
            updateGuiValues(item);
        }

        if (topLevel) {
            InvalidateRect(g_hwndTV, NULL, true);
            SendMessage(g_hwndTV, WM_SETREDRAW, TRUE, 0);
        }
    }

    if (topLevel)
    {
        // Calc new window caption;
        int iReservedDollars = 0, iBalance = 0;
        std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
        std::vector<AccountBase*> envelopes = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);

        for (size_t i = 0; i < accounts.size(); i++) {
            Account* pAccount = (Account * )accounts[i];
            if (pAccount->isBudgeted()) {
                iBalance += accounts[i]->getBalance();
            }
        }

        for (size_t i = 0; i < envelopes.size(); i++) {
            // This is a confusing operation. If a virtual account has a negative balance
            // we conceptually treat it as if it had a bigger balance since the whole thing is
            // a "how much do I deduct from what's available" operation. Hence the abs() here.
            iReservedDollars += abs(envelopes[i]->getBalance());
        }

        int iAvailable = iBalance - iReservedDollars;

        std::wstring totalDollars = Utils::penniesToDollarsString(iBalance);
        std::wstring availableDollars = Utils::penniesToDollarsString(iBalance - iReservedDollars);

        wchar_t szCaption[1024] = { 0 };
        swprintf(szCaption, 1024, L"ScratchTrack: %s of %s available for spending", availableDollars.c_str(), totalDollars.c_str());

        SendMessage(g_hwndAccountLV, WM_SETREDRAW, TRUE, 0);
        SetWindowText(g_hTopWindow, szCaption);

    }

}

void populateTransactions()
{

    // Clear the list view
    ListView_DeleteAllItems(g_hwndTransactionLV);

    HTREEITEM hItem = (HTREEITEM)TreeView_GetSelection(g_hwndTV);
    if (!hItem) {
        return;
    }

    AccountBase* pItem = accountFromItem(hItem);
    if (!pItem){
        return;
    }

    // Get Transactions for item
    std::vector<Transaction*> Transactions = Transaction::getTransactionsForItem(pItem);

    // Add them to the list view
    SendMessage(g_hwndTransactionLV, WM_SETREDRAW, FALSE, 0);

    for (auto itr = Transactions.begin(); itr != Transactions.end(); itr++ )
    {
        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        std::wstring timeStamp;
        (*itr)->getLocalTimeStamp(timeStamp);
        item.pszText = const_cast<wchar_t*>(timeStamp.c_str());
        item.lParam = (LPARAM)*itr;

        // Date
        ListView_InsertItem(g_hwndTransactionLV, &item);

        // Payee

        Transaction* pTransaction = *itr;
        std::wstring payee = pTransaction->getPayee();
        AccountBase* pAccountTo = AccountBase::lookupById(pTransaction->getAccountToId());
        if (pAccountTo) {
            AccountBase* pAccountFrom = AccountBase::lookupById(pTransaction->getAccountFromId());
            wchar_t szBuff[1024] = { 0 };
            swprintf(szBuff, 1024, L"%s->%s", pAccountFrom->getName(), pAccountTo->getName());
            payee = szBuff;
        }

        ListView_SetItemText(g_hwndTransactionLV, 0, 1, const_cast<wchar_t*>(payee.c_str()));

        // Category
        AccountBase* ati = AccountBase::lookupById((*itr)->getCategoryId());
        if (ati) {
            ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(ati->getName()));
        }

        // Description
        ListView_SetItemText(g_hwndTransactionLV, 0, 3, const_cast<wchar_t*>((*itr)->getDescription()));

        int iAmount = abs((*itr)->getAmount());
        std::wstring amount = Utils::penniesToDollarsString(iAmount);
        ListView_SetItemText(g_hwndTransactionLV, 0, 5, const_cast<wchar_t*>(amount.c_str()));
    }

    DWORD dwFlags = LVSCW_AUTOSIZE_USEHEADER;
    for (DWORD i = 0; i < 5; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, dwFlags);
    }    

    SendMessage(g_hwndTransactionLV, WM_SETREDRAW, TRUE, 0);

}

void addAccount(Account* pAccount)
{

    // Add them to the list view
    SendMessage(g_hwndAccountLV, WM_SETREDRAW, FALSE, 0);

    LVITEM item = { 0 };
    item.mask = LVIF_TEXT | LVIF_PARAM;
    std::wstring timeStamp;
    item.pszText = const_cast<wchar_t*>(pAccount->getName());
    item.lParam = (LPARAM)pAccount;
    item.iItem = MAXINT;

    // Account name
    ListView_InsertItem(g_hwndAccountLV, &item);

    // Account balance
    std::wstring balance = Utils::penniesToDollarsString(pAccount->getBalance());
    ListView_SetItemText(g_hwndAccountLV, 0, 1, const_cast<wchar_t*>(balance.c_str()));


    ListView_SetColumnWidth(g_hwndAccountLV, 0, 200);
    ListView_SetColumnWidth(g_hwndAccountLV, 1, 200);

    SendMessage(g_hwndAccountLV, WM_SETREDRAW, TRUE, 0);

}

int serializeState()
{
    std::vector<AccountBase*> topItems;

    // Serialize accounts
    for (int i = 0; i < ListView_GetItemCount(g_hwndAccountLV); i++)
    {
        LVITEM item = { 0 };
        item.iItem = i;
        item.mask = LVIF_PARAM;
        ListView_GetItem(g_hwndAccountLV, &item);
        Account* pAccount = (Account*)item.lParam;
        topItems.push_back(pAccount);
    }

    // Serialize Categories and subcategories
    HTREEITEM hItem = TreeView_GetChild(g_hwndTV, TVI_ROOT);
    while (hItem){
        AccountBase* item = accountFromItem(hItem);
        topItems.push_back(item);
        hItem = TreeView_GetNextSibling(g_hwndTV, hItem);
    }

    serialize(topItems, getPersistFile());

    return ERROR_SUCCESS;
}

void positionControls()
{
    static const double TREE_VERTICAL_SPACE = .65;
    static const double TREE_HORIZONTAL_SPACE = .80;

    RECT rClient = { 0 };
    HDLAYOUT hdl = { 0 };
    WINDOWPOS wp = { 0 };
    GetClientRect(g_hTopWindow, &rClient);
    hdl.prc = &rClient;
    hdl.pwpos = &wp;
    SendMessage(g_hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hdl);

    int treeleft = (int)(rClient.right * (1.0 - TREE_HORIZONTAL_SPACE));

    int listtop = (int)(rClient.bottom * TREE_VERTICAL_SPACE);
    int listheight = rClient.bottom - listtop;

    SetWindowPos(g_hwndHeader, wp.hwndInsertAfter, treeleft, 0, wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
    SetWindowPos(g_hwndTV, nullptr, treeleft, rClient.top, rClient.right, listtop - 1 - wp.cy, SWP_NOZORDER);
    SetWindowPos(g_hwndAccountLV, nullptr, 0, 0, treeleft-1, listtop - 1, SWP_NOZORDER);
    SetWindowPos(g_hwndTransactionLV, nullptr, rClient.left, listtop, rClient.right, listheight, SWP_NOZORDER);

    //g_hwndAccountLV;
}
