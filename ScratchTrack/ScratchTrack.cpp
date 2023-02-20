// SimpleBudget.cpp : Defines the entry point for the application.
//

#include "framework.h"

#include "AccountInfo.h"
#include "Transaction.h"

#include "ScratchTrack.h"
#include "resource.h"
#include "TransactionDialog.h"
#include "Utils.h"
#include "TreeUtils.h"
#include "AccountListViewUtils.h"
#include "Version.h"
#include "TransactionImporter.h"
#include "ExcelSerializer.h"

// Dialogs
#include "TransactionFilterDlg.h"
#include "AccountInfoDlg.h"
#include "DepositDlg.h"
#include "TransferDlg.h"
#include "PayCCDlg.h"
#include "ExpenseDlg.h"
#include "AccountDlg.h"
#include "FolderDlg.h"
#include "FundExpenseDlg.h"
#include "FundExpensesDlg.h"
#include "TransactionImportDlg.h"

#include <commctrl.h>
#include <Windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <vector>
#include <Commdlg.h>

#pragma comment(lib, "Comctl32")
#pragma comment(lib, "Shlwapi")

#define MAX_LOADSTRING 100

// Global settings
static const float FONT_SCALE_FACTOR = 1.4f;
static const float FONT_SCALE_FACTOR_LOWRES = 1.1f;

// Global Variables:
HINSTANCE g_hInst = nullptr;                                // current instance
HWND g_hTopWindow = nullptr;
HWND g_hwndTV = nullptr, g_hwndTransactionLV = nullptr, g_hwndTreeHeader = nullptr, g_hwndAccountLV = nullptr;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
wchar_t szOneInputText[1024] = { 0 };
TransactionFilterDlg g_transactionFilterDlg;
AccountInfoDlg g_accountInfoDlg;

TransactionImporter g_transactionImporter;

extern BUDGET_TREE_COLUMN_SIZES g_columnSizes;
extern BUDGET_TREE_DRAG_DETAILS g_btDragDetails;

extern HIMAGELIST _il;
HTREEITEM g_hAddNewFolderItem = TVI_LAST;

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
void addAccount(Account* pAccount);
void setGuiFont(HWND hCtrl);
bool doFundCategories(AccountGroup* pGroup);
bool addExpense(HTREEITEM hItem);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CoInitialize(nullptr);

    // Is an instance already running?
    if (!Utils::isSingleInstance()) {
        MessageBox(nullptr, L"An instance of ScratchTrack is already running", L"Already running", MB_OK);
        return 0;
    }

    Utils::monitorDataFileForChanges(false);

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

    Utils::monitorDataFileForChanges(true);

    CoUninitialize();

    return (int) msg.wParam;
}

int setupPaymentExpenseForAccount(Account* pAccount)
{
    std::wstring expenseName = L"Credit card payment";

    AccountExpense* pExpense = dynamic_cast<AccountExpense*>(AccountBase::lookupById(ACCOUNT_ID_PAY_CC));

    if (!pExpense) {

#define CC_IN_FOLDER
#ifdef CC_IN_FOLDER
        auto folders = AccountBase::getAllAccountsOfType(AccountBase::GROUP);

        // Ensure we have a folder for CC payment
        AccountGroup* pFolder = nullptr;
        HTREEITEM hItemFolder = nullptr;
        std::wstring folderName = L"CC Payments";

        for (size_t i = 0; i < folders.size(); i++)
        {
            if (_wcsicmp(folderName.c_str(), folders[i]->getName()) == 0) {
                pFolder = (AccountGroup*)folders[i];
                pFolder->setReadOnly(true);
                hItemFolder = treeItemFromAccount(pFolder);
            }
        }

        if (!pFolder)
        {
            pFolder = new AccountGroup;
            pFolder->setName(folderName.c_str());
            pFolder->setReadOnly(true);

            hItemFolder = addTreeItem(TVI_ROOT, pFolder, TVI_LAST);
        }
        else if (pFolder->getHidden()) {
            pFolder->setHidden(false);
            addTreeItem(TVI_ROOT, pFolder);
        }


        // No existing expense. Make one.
        pExpense = AccountExpense::getOrCreateCCExpense();
        pExpense->setName(expenseName.c_str());
        pExpense->setDefaultTransactionPayToAccount(pAccount);
        pExpense->setIsExpense(true);
        pExpense->setIsVirtualAccount(true);

        pFolder->addChild(pExpense);
        addTreeItem(hItemFolder, pExpense);
    }
    else
    {
        // We do have a expense, update it's name as appropriate
        pExpense->setName(expenseName.c_str());

        // Ensure we're not hidden
        AccountBase* pFolder = pExpense->getParent();
        pFolder->setHidden(false);
        pExpense->setHidden(false);

        HTREEITEM hFolder = treeItemFromAccount(pFolder);
        if (!hFolder) {
            hFolder = addTreeItem(TVI_ROOT, pFolder);
        }

        HTREEITEM hExpense = treeItemFromAccount(pExpense);
        if (!hExpense) {
            addTreeItem(hFolder, pExpense);
        }

    }
#else
        pExpense = AccountExpense::getOrCreateCCExpense();
        pExpense->setName(expenseName.c_str());
        pExpense->setDefaultTransactionPayToAccount(pAccount);
        pExpense->setIsExpense(true);
        pExpense->setIsVirtualAccount(true);

        pAccount->setIsCCAccount(true);
        addTreeItem(TVI_ROOT, pExpense);

    }
#endif

    return pExpense->getId();
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

    ListView_SetExtendedListViewStyle(g_hwndTransactionLV, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    setGuiFont(g_hwndTransactionLV);

    LVCOLUMN column = { 0 };
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(L"Date");
    ListView_InsertColumn(g_hwndTransactionLV, 0, &column);

    column.pszText = const_cast<wchar_t*>(L"Payee");
    ListView_InsertColumn(g_hwndTransactionLV, 1, &column);

    column.pszText = const_cast<wchar_t*>(L"Expense");
    ListView_InsertColumn(g_hwndTransactionLV, 2, &column);

    column.pszText = const_cast<wchar_t*>(L"Description");
    ListView_InsertColumn(g_hwndTransactionLV, 3, &column);

    column.pszText = const_cast<wchar_t*>(L"Outflow");
    ListView_InsertColumn(g_hwndTransactionLV, 4, &column);

    column.pszText = const_cast<wchar_t*>(L"Inflow");
    ListView_InsertColumn(g_hwndTransactionLV, 5, &column);
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
   budgetTreeCreateTreeControl();
   createTransactionListControl();
   g_transactionFilterDlg.create(g_hTopWindow);
   g_accountInfoDlg.create(g_hTopWindow);


   // Populate data (load or create new)
   DWORD dwAttribs = ::GetFileAttributesA(Utils::getPersistFile());
   bool haveXml = dwAttribs != INVALID_FILE_ATTRIBUTES;

   dwAttribs = ::GetFileAttributesA(Utils::getPersistFileXlsx());
   bool haveXls = dwAttribs != INVALID_FILE_ATTRIBUTES;
   std::vector<AccountBase*> topLevelItems, topLevelItems2;

   int deserializeResult = ERROR_FILE_NOT_FOUND;
   if (haveXls) {
       deserializeResult = ExcelSerializer::deserialize(Utils::getPersistFileXlsx(), topLevelItems);
   }
   else if (haveXml) {
       deserializeResult = deserialize(Utils::getPersistFile(), topLevelItems);
   }

   if (deserializeResult == ERROR_SUCCESS ) {
       for (size_t i = 0; i < topLevelItems.size(); i++) {
           addTreeItem(TVI_ROOT, topLevelItems[i]);
       }
   }
   else if (deserializeResult != ERROR_FILE_NOT_FOUND) {
       MessageBox(g_hTopWindow, L"Unable to load file", L"Error", MB_OK);
       exit(0);
   }
   //else {
   //    Account* pAccount = new Account;
   //    pAccount->setName(L"Checking");
   //    addTreeItem(TVI_ROOT, pAccount);

   //    AccountGroup* pGroup = new AccountGroup;
   //    pGroup->setName(L"Monthly Expenses");
   //    addTreeItem(TVI_ROOT, pGroup);
   //    serializeState();
   //}

   // Add bottom tree item to add new folder folders
   TVINSERTSTRUCT tvi = { 0 };
   tvi.itemex.mask = TVIF_TEXT | TVIF_PARAM;
   tvi.itemex.pszText = const_cast<wchar_t*>(L"Add folder");
   tvi.hParent = TVI_ROOT;
   tvi.itemex.lParam = 0;
   tvi.hInsertAfter = TVI_LAST;

   g_hAddNewFolderItem = TreeView_InsertItem(g_hwndTV, &tvi);
   //ShowWindow(g_hwndTV, SW_SHOW);

   // Fix up selection / paint
   HTREEITEM hRoot = TreeView_GetRoot(g_hwndTV);
   if (hRoot) {
       TreeView_SelectItem(g_hwndTV, hRoot);
   }

   populateTransactions();
   positionControls();

   updateGuiValues();

   g_accountInfoDlg.updateReport();
   ShowWindow(g_hTopWindow, SW_MAXIMIZE);
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

bool getOneText(std::wstring& result)
{
    szOneInputText[0] = 0;
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ONE_INPUT), g_hTopWindow, OneInputProc);
    result = szOneInputText;
    return result[0] ? true : false;
}

bool editFolder(AccountGroup* pGroup)
{
    FolderDlg dlg(pGroup->getName(), pGroup->getExpenseLimit());

    if (dlg.ShowDialog())
    {
        pGroup->setName(dlg.getName());
        pGroup->setExpenseLimit(dlg.getLimit());

        updateGuiValues();

        return true;
    }
    return false;
}

bool editExpense(AccountExpense* pExpense)
{
    ExpenseDialog dlg;
    dlg.setBudget(pExpense->getBudgeted());
    dlg.setExpenseName(pExpense->getName());
    dlg.setDefaultTransactionPayFromAccount(pExpense->getDefaultTransactionPayFromAccount());
    dlg.setDefaultTransactionAmount(pExpense->getDefaultTransactionAmount());
    dlg.setDefaultTransactionPayee(pExpense->getDefaultTransactionPayee());
    if (dlg.ShowDialog())
    {
        pExpense->setBudgeted(dlg.getBudget());
        pExpense->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
        pExpense->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pExpense->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pExpense->setName(dlg.getExpenseName());
        
        updateGuiValues();

        return true;
    }
    return false;
}

bool doExpenseAdd(AccountGroup* pParent, HTREEITEM hParent)
{
    bool ret = false;
    ExpenseDialog dlg;
    if (pParent && dlg.ShowDialog()) {
        AccountExpense* pExpense = new AccountExpense;
        pExpense->setName(dlg.getExpenseName());
        pExpense->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
        pExpense->setDefaultTransactionPayToAccount(nullptr);
        pExpense->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pExpense->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pExpense->setIsExpense(true);
        pExpense->setIsVirtualAccount(true);
        pExpense->setBudgeted(dlg.getBudget());

        pParent->addChild(pExpense);
        addTreeItem(hParent, pExpense);
        ret = true;
    }
    return ret;
}

bool doFundCategories(AccountGroup* pGroup)
{
    FundExpensesDlg dlg(pGroup);
    bool ret = dlg.ShowDialog();
    if (ret) {
        auto children = pGroup->getChildren();
        for (size_t i = 0; i < children.size(); i++) {
            AccountExpense* pExpense = dynamic_cast<AccountExpense*>(children[i]);
            if (pExpense) {
                if (dlg.getResetBalance()) {
                    pExpense->addBalance(-pExpense->getBalance() + -pExpense->getBudgeted());
                }
                else {
                    pExpense->addBalance(-pExpense->getBudgeted());
                }
            }
        }
    }

    return ret;
}

bool processContextMenu(DWORD dwCommand, HTREEITEM hItem, AccountBase* pItem)
{
    std::wstring temp;
    bool didSomething = false;
    AccountGroup* pParentGroup = nullptr;
    if (pItem->getType() == AccountBase::GROUP){
        pParentGroup = (AccountGroup*)pItem;
    }

    if (dwCommand >= ID_ROOT_EXPENSE_MOVE_TO_START && dwCommand < ID_ROOT_EXPENSE_MOVE_TO_END) {
        // This is a move
        AccountGroup* pDestGroup = dynamic_cast<AccountGroup*>(AccountBase::lookupById(dwCommand - ID_ROOT_EXPENSE_MOVE_TO_START));
        AccountGroup* pOrigGroup = dynamic_cast<AccountGroup*>(pItem->getParent());
        HTREEITEM hNewParent = treeItemFromAccount(pDestGroup);

        if (pDestGroup && hNewParent && pOrigGroup->removeChild(pItem)) {
            pDestGroup->addChild(pItem);
            TreeView_DeleteItem(g_hwndTV, hItem);
            addTreeItem(hNewParent, pItem);
            didSomething = true;
        }
    }

    switch (dwCommand)
    {
    case ID_ROOT_HIDEEXPENSE:
    {
        if (Utils::confirmHide(pItem)) {
            pItem->setHidden(true);
            TreeView_DeleteItem(g_hwndTV, hItem);
            didSomething = true;
        }

        break;
    }
    case ID_EXPENSE_RESERVE_FUNDS:
    {
        AccountExpense* pExpense = dynamic_cast<AccountExpense*>(pItem);
        FundExpenseDlg dlg(pExpense);
        if (pExpense && dlg.ShowDialog()) {
            pExpense->addBalance(-pExpense->getBalance() + dlg.getNewBalance());
            didSomething = true;
        }
        break;
    }
    case ID_ROOT_EDITEXPENSE:
        didSomething = editExpense((AccountExpense*)pItem);
        break;
    case ID_ROOT_SETBUDGETEDAMOUNT:
        if (pItem && getOneText(temp)) {
            pItem->setBudgeted(Utils::dollarsStringToPennies(temp.c_str()));
            didSomething = true;
        }
        break;
    case IDM_EDIT_FOLDER:
    {
        didSomething = editFolder((AccountGroup*)pItem);
        break;
    }
    case ID_ROOT_FUNDCATEGORIES:
    {
        didSomething = doFundCategories((AccountGroup*)pItem);
        break;
    }
    case ID_ROOT_ADDEXPENSE:
    {
        didSomething = doExpenseAdd(pParentGroup, hItem);
        break;
    }
    case ID_ROOT_ADDTransaction:
    {
        didSomething = addExpense(hItem);
        break;
    }
    }

    if (didSomething) {
        updateGuiValues();
        serializeState();
    }

    return didSomething;
}

void updateTransactionUiAndSave(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAccountFrom = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountFromId()));
    Account* pAccountTo = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountToId()));

    // Payee
    std::wstring payee = pTransaction->getPayee();
    if (pTransaction->getAccountToId() != ACCOUNT_ID_INVALID && pAccountTo) {
        wchar_t szBuff[1024] = { 0 };
        if (pAccountFrom) {
            swprintf(szBuff, 1024, L"%s->%s", pAccountFrom->getName(), pAccountTo->getName());
        }
        else {
            swprintf(szBuff, 1024, L"%s", pAccountTo->getName());
        }
        payee = szBuff;
    }
    ListView_SetItemText(g_hwndTransactionLV, listviewItemIndex, 1, const_cast<wchar_t*>(payee.c_str()));

    // Expense
    AccountBase* ati = AccountBase::lookupById(pTransaction->getExpenseId());
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

void doTransfer(Account* pAccount)
{
    // Ensure we have enough accounts
    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    size_t transferAccts = 0;
    for (size_t i = 0; i < accounts.size() && transferAccts < 2; i++) {
        Account* pAccount = dynamic_cast<Account*>(accounts[i]);
        if (!pAccount || pAccount->isCCAccount()) {
            continue;
        }
        transferAccts++;
    }

    if (transferAccts < 2) {
        MessageBox(g_hTopWindow, L"You must have two or more accounts to perform a transfer", L"Transfer", MB_OK);
        return;
    }

    TransferDlg dlg(pAccount->getId(), ACCOUNT_ID_INVALID, 0, nullptr);
    if (dlg.ShowDialog()) {
        Account* pAccountFrom = (Account*)AccountBase::lookupById(dlg.getAccountFromId());
        Account* pAccountTo = (Account*)AccountBase::lookupById(dlg.getAccountToId());
        if (pAccountFrom && pAccountTo) {
            Transaction* pTransaction = Transaction::addTransaction(Transaction::TRANSFER, dlg.getAmount(), dlg.getAccountFromId(), dlg.getAccountToId(), ACCOUNT_ID_INVALID, L"Transfer", dlg.getDescription());
            pTransaction->apply();
            serializeState();
            updateGuiValues();
            populateTransactions();

        }
    }
}

void editTransfer(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAcctTo = (Account*)AccountBase::lookupById(pTransaction->getAccountToId());
    Account* pAcctFrom = (Account*)AccountBase::lookupById(pTransaction->getAccountFromId());
    if (pAcctTo && pAcctFrom) {
        TransferDlg dlg(pAcctFrom->getId(), pAcctTo->getId(), pTransaction->getAmount(), pTransaction->getDescription());
        if (dlg.ShowDialog()) {

            pTransaction->undo();
            pTransaction->setDetails(dlg.getAmount(), pTransaction->getPayee(), dlg.getDescription(), pTransaction->getExpenseId(), dlg.getAccountFromId(), dlg.getAccountToId());
            pTransaction->apply();

            updateTransactionUiAndSave(pTransaction, listviewItemIndex);
        }
    }
}

void addDeposit(Account* pAccount)
{
    DepositDlg dlg(pAccount, 0, nullptr, nullptr);
    if (dlg.ShowDialog()) {
        Transaction* pTransaction = Transaction::addTransaction(Transaction::DEPOSIT, dlg.getAmount(), ACCOUNT_ID_INVALID, pAccount->getId(), ACCOUNT_ID_INVALID, dlg.getPayee(), dlg.getDescription());
        pTransaction->apply();
        serializeState();
        updateGuiValues();
        populateTransactions();
    }
}

void editDeposit(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAcct = (Account*)AccountBase::lookupById(pTransaction->getAccountToId());
    if (pAcct) {
        DepositDlg dlg(pAcct, pTransaction->getAmount(), pTransaction->getPayee(), pTransaction->getDescription());
        if (dlg.ShowDialog()) {
            pTransaction->undo();
            pTransaction->setDetails(dlg.getAmount(), dlg.getPayee(), dlg.getDescription(), pTransaction->getExpenseId(), pTransaction->getAccountFromId(), dlg.getAccount()->getId());
            pTransaction->apply();
            updateTransactionUiAndSave(pTransaction, listviewItemIndex);
        }
    }
}

void doPayCC(Account* pAccount)
{
    // Ensure we have enough accounts
    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    size_t accts = 0;
    for (size_t i = 0; i < accounts.size() && !accts; i++) {
        Account* pAccount = dynamic_cast<Account*>(accounts[i]);
        if (!pAccount || pAccount->isCCAccount()) {
            continue;
        }
        accts++;
    }

    if (!accts) {
        MessageBox(g_hTopWindow, L"You must have at least one non-credit card account to make this payment", L"Cedit Card Payment", MB_OK);
        return;
    }

    PayCCDlg dlg(pAccount, abs(pAccount->getBalance()), 0);
    if (dlg.ShowDialog()) {
        Transaction* pTransaction = Transaction::addTransaction(Transaction::CCPAYMENT, dlg.getAmount(), dlg.getPaidFromId(), pAccount->getId(), pAccount->isCCAccount() ? ACCOUNT_ID_PAY_CC : ACCOUNT_ID_INVALID, pAccount->getName(), dlg.getDescription());
        pTransaction->apply();
        serializeState();
        updateGuiValues();
        populateTransactions();
    }
}

void editPayCC(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAcct = (Account*)AccountBase::lookupById(pTransaction->getAccountToId());
    if (pAcct) {
        PayCCDlg dlg(pAcct, pTransaction->getAmount(), pTransaction->getAccountFromId(), pTransaction->getDescription());
        if (dlg.ShowDialog()) {
            pTransaction->undo();
            // Apply new balance to the expense
            pTransaction->setDetails(dlg.getAmount(), pTransaction->getPayee(), dlg.getDescription(), pTransaction->getExpenseId(), pTransaction->getAccountFromId(), pTransaction->getAccountToId());
            pTransaction->apply();
            updateTransactionUiAndSave(pTransaction, listviewItemIndex);
        }
    }
}

bool addExpense(HTREEITEM hItem)
{
    bool didSimonthing = false;
    AccountBase* pItem = accountFromTreeItem(hItem);
    TransactionDialog transactionDlg(true);
    transactionDlg.setExpenseId(pItem->getId());
    AccountExpense* pExpense = nullptr;
    if (pItem->getType() == AccountBase::EXPENSE) {
        pExpense = (AccountExpense*)pItem;
        transactionDlg.setPayee(pExpense->getDefaultTransactionPayee());
        transactionDlg.setAmount(pExpense->getDefaultTransactionAmount());

        if (pExpense->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pExpense->getDefaultTransactionPayFromAccount()->getId());
        }
        else {
            transactionDlg.setAccountFromId(ACCOUNT_ID_INVALID);
        }

        if (pExpense->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pExpense->getDefaultTransactionPayFromAccount()->getId());
        }
    }

    if (transactionDlg.ShowDialog())
    {
        Transaction* pTransaction = Transaction::addTransaction(Transaction::EXPENSE, transactionDlg.getAmount(), transactionDlg.getAccountFromId(), ACCOUNT_ID_INVALID, transactionDlg.getExpenseId(), transactionDlg.getPayee(), transactionDlg.getDescription());
        pTransaction->apply();

        updateGuiValues();
        populateTransactions();
        didSimonthing = true;
    }
    return didSimonthing;
}

void editExpense(Transaction* pTransaction, int listviewItemIndex)
{
    TransactionDialog dlg(false /* turn off validation in edit. Undo logic needs to be considered */);

    dlg.setAmount(pTransaction->getAmount());
    dlg.setPayee(pTransaction->getPayee());
    dlg.setDescription(pTransaction->getDescription());
    dlg.setAccountFromId(pTransaction->getAccountFromId());
    dlg.setExpenseId(pTransaction->getExpenseId());

    if (dlg.ShowDialog())
    {
        pTransaction->undo();
        pTransaction->setDetails(dlg.getAmount(), dlg.getPayee(), dlg.getDescription(), dlg.getExpenseId(), dlg.getAccountFromId(), ACCOUNT_ID_INVALID);
        pTransaction->apply();
        updateTransactionUiAndSave(pTransaction, listviewItemIndex);
    }
}

void editTransaction(Transaction* pTransaction, int listviewItemIndex)
{
    switch (pTransaction->getType())
    {
    case Transaction::CCPAYMENT:
        editPayCC(pTransaction, listviewItemIndex);
        break;
    case Transaction::EXPENSE:
        editExpense(pTransaction, listviewItemIndex);
        break;
    case Transaction::DEPOSIT:
        editDeposit(pTransaction, listviewItemIndex);
        break;
    case Transaction::TRANSFER:
        editTransfer(pTransaction, listviewItemIndex);
        break;
    }
}

void doTreeDrag(LPNMTREEVIEW nmTreeItem)
{
    AccountBase* pItem = accountFromTreeItem(nmTreeItem->itemNew.hItem);
    if (!pItem || (pItem && pItem->getId() == ACCOUNT_ID_PAY_CC)) {
        return;
    }

    // Tell the tree-view control to create an image to use 
    // for dragging. 
    HIMAGELIST himl = TreeView_CreateDragImage(g_hwndTV, nmTreeItem->itemNew.hItem);


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

void editAccount(Account* pAccount)
{
    AccountDlg dlg;
    dlg.setBalance(pAccount->getBalance());
    dlg.setBudgeted(pAccount->getContributesToBudget());
    dlg.setName(pAccount->getName());
    dlg.setCreditCard(pAccount->isCCAccount());
    if (dlg.ShowDialog())
    {
        // Did this change "types" (cc to non CC)
        pAccount->addBalance(-pAccount->getBalance());
        pAccount->addBalance(dlg.getBalance());
        pAccount->setName(dlg.getName());
        pAccount->setContributesToBudget(dlg.getBudgeted());
        if (dlg.getCreditCard()) {
            pAccount->setIsCCAccount(true);
            setupPaymentExpenseForAccount(pAccount);
        }
        else if (pAccount->isCCAccount())
        {
            // This account isn't a CC account anymore
            pAccount->setIsCCAccount(false);

            // Find the CC payment expense
            AccountExpense* pExpense = dynamic_cast<AccountExpense * >(AccountBase::lookupById(ACCOUNT_ID_PAY_CC));

            // If there aren't any other CC accounts, hide the payment folders
            auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
            bool allHidden = true;
            for (size_t i = 0; i < accounts.size(); i++) {
                Account* pAccount = dynamic_cast<Account*>(accounts[i]);
                if (pAccount->isCCAccount() && !pAccount->getHidden()) {
                    allHidden = false;
                    break;
                }
            }

            if (allHidden) {
                pExpense->addBalance(-pExpense->getBalance());
                pExpense->setHidden(true);
                HTREEITEM hExpense = treeItemFromAccount(pExpense);
                HTREEITEM hParent = TreeView_GetParent(g_hwndTV, hExpense);
                if (hParent) {
                    AccountBase* pParent = accountFromTreeItem(hParent);
                    pParent->setHidden(true);
                    TreeView_DeleteItem(g_hwndTV, hParent);
                }
            }
        }
        InvalidateRect(g_hwndAccountLV, nullptr, true);
        updateGuiValues();
        positionControls();
        populateTransactions();
        serializeState();
    }
}

void doAddAccountDlg()
{
    AccountDlg dlg;
    dlg.setBudgeted(true);
    if (dlg.ShowDialog()) {
        Account* pAccount = new Account;
        pAccount->setName(dlg.getName());
        pAccount->addBalance(dlg.getBalance());
        pAccount->setContributesToBudget(dlg.getBudgeted());
        if (dlg.getCreditCard()) {
            pAccount->setIsCCAccount(true);
            setupPaymentExpenseForAccount(pAccount);
        }
        addTreeItem(TVI_ROOT, pAccount);
        serializeState();
    }
}

void doAddFolder()
{
    FolderDlg dlg(L"", 0);
    if (dlg.ShowDialog()) {
        AccountGroup* pGroup = new AccountGroup;
        pGroup->setName(dlg.getName());
        pGroup->setExpenseLimit(dlg.getLimit());
        addTreeItem(TVI_ROOT, pGroup, TVI_LAST);
        serializeState();
    }
}

void doImportTransactions(Account* pAccount)
{
    OPENFILENAME of = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };
    of.lStructSize = sizeof(of);
    of.hwndOwner = g_hTopWindow;
    of.lpstrFilter = _T("QIF\0*.qif\0\0");
    of.lpstrFile = szFile;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&of)) {
        std::vector<ImportedTransactionInfo> transactions;
        if (ERROR_SUCCESS == g_transactionImporter.readTransactions(szFile, transactions) && !transactions.empty())
        {
            TransactionImportDlg dlg(pAccount, transactions);
            if (dlg.ShowDialog())
            {
                auto transactionsNew = dlg.transactionsOut();
                for (size_t i = 0; i < transactionsNew.size(); i++) {
                    std::wstring payee = transactionsNew[i]._payeeMapped.empty() ? transactionsNew[i]._payee : transactionsNew[i]._payeeMapped;
                    Transaction* pTransaction = Transaction::addTransaction(Transaction::EXPENSE, transactionsNew[i]._amount, 
                        pAccount->getId(), ACCOUNT_ID_INVALID, transactionsNew[i]._expenseId, payee.c_str(), nullptr, &transactionsNew[i]._date);
                    pTransaction->apply();

                    // If the max imported date is greater than our current last import date, update it
                    SYSTEMTIME maxImport = dlg.maxImportedDate();
                    if (Utils::compareDates(maxImport, pAccount->getLastImportDate()) == 1) {
                        pAccount->setLastImportDate(maxImport);
                    }

                } // for each new transaction

                Transaction::sortAllTransactions();

                updateGuiValues();
                populateTransactions();
                serializeState();
            }
        }
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
    case WM_CREATE:
        break;
    case WM_MOUSEMOVE:
        if (g_btDragDetails._dragging) {
            POINT pt = { 0 };
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hWnd, &pt);
            ScreenToClient(g_hwndTV, &pt);
            budgetTreeHandleMouseMove(pt.x, pt.y);
        }

        break;

    case WM_LBUTTONUP:
        budgetTreeHandleLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
        case ID_FILE_ADDACCOUNT:
        {
            doAddAccountDlg();
            break;
        }
        case ID_FILE_ADDEXPENSE:
            doAddFolder();
            break;
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
        if (pHeader->idFrom == IDD_BUDGET_INFO_DIALOG) {
            if (pHeader->code == AccountInfoDlg::NM_ACCOUNT_INFO_DIALOG_ADD_ACCOUNT)
            {
                doAddAccountDlg();

            }
            else if (pHeader->code == AccountInfoDlg::NM_ACCOUNT_DIALOG_ADD_FOLDER)
            {
                doAddFolder();
            }

        }
        else if (pHeader->idFrom == IDD_TRANSACTION_FILTER_DIALOG && pHeader->code == TransactionFilterDlg::NM_TRANS_DIALOG_FILTER_CHANGED)
        {
            populateTransactions();
            positionControls();
        }
        else if (pHeader->idFrom == IDC_SIMPLEBUDGET_ACCOUNT_LIST)
        {
            if (pHeader->code == LVN_ITEMCHANGED)
            {
                NMLISTVIEW* pStateChange = reinterpret_cast<NMLISTVIEW*>(pHeader);
                if (g_transactionFilterDlg.filterToAccount()) {
                    if ( (pStateChange->uNewState ^ pStateChange->uOldState) & LVIS_SELECTED ) {
                        populateTransactions();
                    }
                }
            }
            else if (pHeader->code == NM_CLICK)
            {
                killAccountListToolTips();
                POINT pt = { 0 };
                TVHITTESTINFO tvhi = { 0 };
                GetCursorPos(&pt);
                ScreenToClient(g_hwndAccountLV, &pt);

                Account* pHitAccount = nullptr;
                int index = -1;
                Account::ACTION_FLAG af = hitTestAccountList(pt, &pHitAccount, &index);
                if (af == Account::ACTION_FLAG::DEPOSIT) {
                    addDeposit(pHitAccount);
                    return true;
                }
                else if (af == Account::ACTION_FLAG::TRANSFER) {
                    doTransfer(pHitAccount);
                    return true;
                }
                else if (af == Account::ACTION_FLAG::PAY) {
                    doPayCC(pHitAccount);
                    return true;
                }
                else if (af == Account::HIDE) {
                    if (Utils::confirmHide(pHitAccount)) {
                        pHitAccount->setHidden(true);
                        ListView_DeleteItem(g_hwndAccountLV, index);
                        serializeState();

                    }
                }
                else if (af == Account::EDIT) {
                    editAccount(pHitAccount);
                }
            }
            else if (pHeader->code == NM_CUSTOMDRAW)
            {
                return accountListCustomDraw((LPNMLVCUSTOMDRAW)lParam);
            }
            else if (pHeader->code == NM_DBLCLK)
            {
                // See if we hit an item
                POINT pt = { 0 };
                LVHITTESTINFO lvhi = { 0 };
                GetCursorPos(&pt);
                lvhi.pt = pt;
                ScreenToClient(g_hwndAccountLV, &lvhi.pt);
                ListView_HitTest(g_hwndAccountLV, &lvhi);
                if (lvhi.iItem == -1) {
                    ListView_SubItemHitTest(g_hwndAccountLV, &lvhi);
                }

                if (lvhi.iItem != -1)
                {
                    LVITEM item = { 0 };
                    item.iItem = lvhi.iItem;
                    item.mask = LVIF_PARAM;
                    ListView_GetItem(g_hwndAccountLV, &item);

                    Account* pAccount = (Account*)item.lParam;
                    if (!pAccount->getReadOnly()) {
                        editAccount(pAccount);
                    }
                    return true;
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
                    ListView_SubItemHitTest(g_hwndAccountLV, &lvhi);
                }

                if (lvhi.iItem != -1) 
                {
                    LVITEM item = { 0 };
                    item.iItem = lvhi.iItem;
                    item.mask = LVIF_PARAM;
                    ListView_GetItem(g_hwndAccountLV, &item);
                    Account* pAccount = (Account*)item.lParam;

                    HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(pAccount->isCCAccount() ? IDR_MENU_ACCOUNT_CC : IDR_MENU_ACCOUNT));
                    HMENU hSubMenu = GetSubMenu(hMenu, 0);
                    DWORD dwCode = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                    std::wstring temp;

                    switch (dwCode)
                    {
                    case ID_ROOT_HIDEACCOUNT:
                    {
                        if (Utils::confirmHide(pAccount)) {
                            pAccount->setHidden(true);
                            ListView_DeleteItem(g_hwndAccountLV, lvhi.iItem);
                            serializeState();
                        }

                        break;
                    }
                    case ID_ROOT_DEPOSIT:
                        addDeposit(pAccount);
                        break;
                    case ID_ROOT_TRANSFER:
                        doTransfer(pAccount);
                        break;
                    case ID_ROOT_PAYCREDITCARD:
                        doPayCC(pAccount);
                        break;
                    case ID_ACCOUNT_RENAME:
                        if (pAccount && getOneText(temp)) {
                            pAccount->setName(temp.c_str());
                            item.mask = LVIF_TEXT;
                            item.pszText = const_cast<wchar_t*>(temp.c_str());
                            ListView_SetItem(g_hwndAccountLV, &item);
                            serializeState();
                        }

                        break;
                    case ID_ROOT_IMPORTTRANSACTIONS:
                        if (pAccount) {
                            doImportTransactions(pAccount);
                        }
                        break;
                    case ID_ROOT_EDITACCOUNT:
                        if (pAccount) {
                            editAccount(pAccount);
                        }
                        break;
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
                if (g_transactionFilterDlg.filterToExpenseSelection()) {
                    populateTransactions();
                }
            }
            else if (pHeader->code == TVN_BEGINDRAG)
            {
                // https://learn.microsoft.com/en-us/windows/win32/controls/drag-a-tree-view-item
                doTreeDrag((LPNMTREEVIEW)lParam);
            }
            else if (pHeader->code == NM_CUSTOMDRAW)
            {
                return budgetTreeCustomDrawTreeItem((NMTVCUSTOMDRAW*)lParam);
            }
            else if (pHeader->code == NM_CLICK)
            {
                killTreeToolTips();
                POINT pt = { 0 };
                TVHITTESTINFO tvhi = { 0 };
                GetCursorPos(&pt);
                ScreenToClient(g_hwndTV, &pt);
                tvhi.pt = pt;

                BUDGET_TREE_LINK_ITEM_TYPE type = INVALID_LINK_ITEM_TYPE;
                HTREEITEM hItem = treeViewHitTestLinks(pt.x, pt.y, type);
                if (hItem && type == ENV_TRANS_ADD)
                {
                    processContextMenu(ID_ROOT_ADDTransaction, hItem, accountFromTreeItem(hItem));
                }
                else if (hItem && type == ENV_MONEY_ADD)
                {
                    processContextMenu(ID_EXPENSE_RESERVE_FUNDS, hItem, accountFromTreeItem(hItem));
                }
                else if (hItem && type == GRP_EXPENSE_ADD)
                {
                    processContextMenu(ID_ROOT_ADDEXPENSE, hItem, accountFromTreeItem(hItem));
                }
                else if (hItem && type == ENV_EXPENSE_EDIT)
                {
                    processContextMenu(ID_ROOT_EDITEXPENSE, hItem, accountFromTreeItem(hItem));
                }
                else if (hItem && type == GRP_FOLDER_EDIT) {
                    processContextMenu(IDM_EDIT_FOLDER, hItem, accountFromTreeItem(hItem));
                }
                else if (hItem && (type == GRP_FUND_CHILDREN)) {
                    processContextMenu(ID_ROOT_FUNDCATEGORIES, hItem, accountFromTreeItem(hItem));
                }
                else if (hItem && (type == GLB_ADD_FOLDER)) {
                    doAddFolder();
                }
                else
                {
                    TreeView_HitTest(g_hwndTV, &tvhi);
                    if (tvhi.hItem) {
                        TreeView_Select(g_hwndTV, tvhi.hItem, TVGN_CARET);
                    }
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
                auto acctBase = accountFromTreeItem(hItem);
                if (acctBase && !acctBase->getReadOnly())
                {
                    AccountExpense* pExpense = dynamic_cast<AccountExpense*>(acctBase);
                    if (pExpense && editExpense(pExpense) ){
                        serializeState();
                    }
                    return true;
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
                    pItem = accountFromTreeItem(hItem);
                    HTREEITEM hParent = TreeView_GetParent(g_hwndTV, hItem);
                    pParentItem = accountFromTreeItem(hParent);
                }

                // Show context menu
                if (pItem)
                {
                    HMENU hMenu = nullptr, hSubMenu = nullptr;
                    if (pItem->getType() == AccountBase::GROUP && !pItem->getReadOnly()) {
                        hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_GROUP_MENU));
                        hSubMenu = GetSubMenu(hMenu, 0);
                    }
                    else if (pItem->getType() == AccountBase::EXPENSE) {

                        bool addMove = !pItem->getReadOnly() && pItem->getId() != ACCOUNT_ID_PAY_CC;

                        if (!pItem->getReadOnly()) {
                            hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_EXPENSE_MENU));
                            hSubMenu = GetSubMenu(hMenu, 0);
                        }

                        // Add the "move to group" option
                        if (addMove) {
                            bool added = false;
                            std::vector<AccountBase*> groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
                            HMENU groupMenu = CreatePopupMenu();
                            for (size_t i = 0; i < groups.size(); i++) {
                                if (pParentItem && groups[i]->getId() != pParentItem->getId() && !groups[i]->getReadOnly()) {
                                    AppendMenu(groupMenu, MF_STRING, ID_ROOT_EXPENSE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                                    added = true;
                                }
                            }

                            if (added) {
                                AppendMenu(hSubMenu, MF_MENUBREAK, 0, 0);
                                AppendMenu(hSubMenu, MF_POPUP, (UINT_PTR)groupMenu, L"Move to group...");
                            }
                            //// Add the "move to account" option
                            //groups = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
                            //HMENU accountMenu = CreatePopupMenu();
                            //for (size_t i = 0; i < groups.size(); i++) {
                            //    if (pParentItem && groups[i]->getId() != pParentItem->getId()) {
                            //        added = true;
                            //        AppendMenu(accountMenu, MF_STRING, ID_ROOT_EXPENSE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                            //    }
                            //}
                            //if (added) {
                            //    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)accountMenu, L"Move to account...");
                            //}
                        } // if adding move options

                    }

                    if (hSubMenu) {
                        DWORD dwCode = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                        DestroyMenu(hMenu);
                        if (dwCode) {
                            processContextMenu(dwCode, hItem, pItem);
                        }
                    }
                }
            }
        }
        else if (pHeader->idFrom == IDC_SIMPLEBUDGET_TRANSACTION_LIST)
        {
            if (pHeader->code == NM_CUSTOMDRAW)
            {
                return Utils::customDrawListViewAlternatingColors((NMLVCUSTOMDRAW*)lParam);
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
                        HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_TRANSACTION));
                        HMENU hSubhMenu = GetSubMenu(hMenu, 0);
                        DWORD dwCode = TrackPopupMenu(hSubhMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                        DestroyMenu(hMenu);
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
        SetDlgItemTextA(hDlg, IDC_VERSION, "ScratchTrack " VER_STRING);
        SetDlgItemTextA(hDlg, IDC_STATIC_COPYRIGHT, COPYRIGHT_STRING);
        
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

            std::wstring name = Utils::accountGuiName(pAccount);
            ListView_SetItemText(g_hwndAccountLV, i, 0, const_cast<wchar_t*>(name.c_str()));

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
        if (pAccount) {
            budgetTreeSetItemText(item, pAccount->getName());

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
        }

        if (topLevel) {
            InvalidateRect(g_hwndTV, NULL, true);
            SendMessage(g_hwndTV, WM_SETREDRAW, TRUE, 0);
        }
    }

    if (topLevel)
    {

        SendMessage(g_hwndAccountLV, WM_SETREDRAW, TRUE, 0);

    }
}

void populateTransactions()
{
    static bool wasShowing = true;

    // Clear the list view
    ListView_DeleteAllItems(g_hwndTransactionLV);

    if (wasShowing && !g_transactionFilterDlg.showTransactionList()) {
        positionControls();
        wasShowing = false;
        return;
    }

    if (!wasShowing && g_transactionFilterDlg.showTransactionList()) {
        positionControls();
        wasShowing = true;
    }


    HTREEITEM hItem = (HTREEITEM)TreeView_GetSelection(g_hwndTV);
    AccountBase* pItem = accountFromTreeItem(hItem);
    Account* pSelectedAccount = getSelectedAccount();

    // Filter to expense if selected, otherwise all
    std::vector<Transaction*> Transactions = g_transactionFilterDlg.filterToExpenseSelection() ? Transaction::getTransactionsForItem(pItem) : Transaction::getAllTransactions();

    // Add them to the list view
    SendMessage(g_hwndTransactionLV, WM_SETREDRAW, FALSE, 0);

    for (auto itr = Transactions.begin(); itr != Transactions.end(); itr++ )
    {
        Transaction* pTransaction = *itr;
        // Filter to account of requested
        if (g_transactionFilterDlg.filterToAccount()) {
            if (g_transactionFilterDlg.filterToAccount() && !pSelectedAccount) {
                break;
            }
            if (pSelectedAccount->getId() != pTransaction->getAccountToId() &&
                pSelectedAccount->getId() != pTransaction->getAccountFromId()) {
                continue;
            }
        }

        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        std::wstring timeStamp;
        pTransaction->getLocalTimeStamp(timeStamp);

        Transaction* pCurTransaction = *itr;

        int iAmount = pTransaction->getAmount();
        bool isFolder = pTransaction->getAccountToId() != ACCOUNT_ID_INVALID && pTransaction->getType() != Transaction::CCPAYMENT;
        iAmount = abs(iAmount);
        std::wstring amount = Utils::penniesToDollarsString(iAmount);

        std::wstring filterText = g_transactionFilterDlg.filterText();
        if (!filterText.empty())
        {
            bool matched = Utils::searchText(timeStamp.c_str(), filterText.c_str());
            matched = matched || Utils::searchText(pCurTransaction->getDescription(), filterText.c_str());
            matched = matched || Utils::searchText(pCurTransaction->getPayee(), filterText.c_str());
            matched = matched || Utils::searchText(amount.c_str(), filterText.c_str());

            AccountBase* pCat = AccountBase::lookupById(pCurTransaction->getExpenseId());
            if (pCat) {
                matched = matched || Utils::searchText(pCat->getName(), filterText.c_str());
            }
            if (!matched) {
                continue;
            }
        }

        item.pszText = const_cast<wchar_t*>(timeStamp.c_str());
        item.lParam = (LPARAM)*itr;

        // Date
        ListView_InsertItem(g_hwndTransactionLV, &item);

        // Payee
        std::wstring payee = pTransaction->getPayee();
        AccountBase* pAccountTo = AccountBase::lookupById(pTransaction->getAccountToId());

        if (pTransaction->getType() != Transaction::EXPENSE) {
            AccountBase* pAccountFrom = AccountBase::lookupById(pTransaction->getAccountFromId());
            wchar_t szBuff[1024] = { 0 };
            if (pAccountFrom) {
                swprintf(szBuff, 1024, L"%s->%s", pAccountFrom->getName(), pAccountTo->getName());
            }
            else if (wcslen(pTransaction->getPayee())) {
                swprintf(szBuff, 1024, L"%s->%s", pTransaction->getPayee(), pAccountTo->getName());
            }
            else {
                swprintf(szBuff, 1024, L"%s", pAccountTo->getName());
            }

            payee = szBuff;
        }

        ListView_SetItemText(g_hwndTransactionLV, 0, 1, const_cast<wchar_t*>(payee.c_str()));

        // Expense
        AccountBase* ati = AccountBase::lookupById(pTransaction->getExpenseId());
        if (ati) {
            ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(ati->getName()));
        }
        else
        {
            switch (pTransaction->getType())
            {
            case Transaction::EXPENSE:
                ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(L"--expense--"));
                break;
            case Transaction::DEPOSIT:
                ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(L"--deposit--"));
                break;
            case Transaction::TRANSFER:
                ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(L"--transfer--"));
                break;
            case Transaction::CCPAYMENT:
                ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(L"--CC Payment--"));
                break;
            }
        }

        // Description
        ListView_SetItemText(g_hwndTransactionLV, 0, 3, const_cast<wchar_t*>(pTransaction->getDescription()));
        ListView_SetItemText(g_hwndTransactionLV, 0, isFolder ? 5 : 4, const_cast<wchar_t*>(amount.c_str()));
    }

    for (size_t i = 0; i < 6; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

    SendMessage(g_hwndTransactionLV, WM_SETREDRAW, TRUE, 0);

}

int serializeState()
{
    std::vector<AccountBase*> topItems;

    // Collect the accounts in order from the UI
    for (int i = 0; i < ListView_GetItemCount(g_hwndAccountLV); i++)
    {
        LVITEM item = { 0 };
        item.iItem = i;
        item.mask = LVIF_PARAM;
        ListView_GetItem(g_hwndAccountLV, &item);
        Account* pAccount = (Account*)item.lParam;
        topItems.push_back(pAccount);
    }

    // Collect folders from the UI
    HTREEITEM hItem = TreeView_GetChild(g_hwndTV, TVI_ROOT);
    while (hItem){
        AccountBase* item = accountFromTreeItem(hItem);
        if (item) {
            topItems.push_back(item);
        }
        hItem = TreeView_GetNextSibling(g_hwndTV, hItem);
    }

    // Collect hidden accounts
    auto accts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    for (size_t i = 0; i < accts.size(); i++) {
        if (accts[i]->getHidden()) {
            topItems.push_back(accts[i]);
        }
    }

    // Collect hidden folders
    accts = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
    for (size_t i = 0; i < accts.size(); i++) {
        if (accts[i]->getHidden()) {
            topItems.push_back(accts[i]);
        }
    }


    //serialize(topItems, Utils::getPersistFile());
    int ret = ExcelSerializer::serialize(Utils::getPersistFileXlsx(), topItems);
    if (ret != ERROR_SUCCESS)
    {
        const char* msg = ExcelSerializer::libxslErrorToText(ret);
        wchar_t szMessage[1024] = { 0 };
        swprintf(szMessage, 1024, L"Failed to save budget [%hs] [%hs]!", Utils::getPersistFileXlsx(), msg);
        MessageBox(g_hTopWindow, szMessage, L"Error saving", MB_ICONWARNING);
    }

    g_accountInfoDlg.updateReport();

    return ERROR_SUCCESS;
}

void setGuiFont(HWND hCtrl)
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    bool lowRes = screenWidth < 1920;

    // Adjust font
    float scaleFactor = lowRes ? FONT_SCALE_FACTOR_LOWRES : FONT_SCALE_FACTOR;

    HFONT hFont = (HFONT)SendMessage(hCtrl, WM_GETFONT, NULL, NULL);
    LOGFONT lf = { 0 };
    GetObject(hFont, sizeof(lf), &lf);
    lf.lfHeight = (LONG)(lf.lfHeight * scaleFactor);
    HFONT hNewFont = CreateFontIndirect(&lf);
    SendMessage(hCtrl, WM_SETFONT, (WPARAM)hNewFont, true);
}

void positionControls()
{
    bool showTransactionList = g_transactionFilterDlg.showTransactionList();

    static const float HORIZONTAL_SPLIT = .35F;

    RECT rClient = { 0 };
    HDLAYOUT headerLayout = { 0 };

    // Get the area of our workspace
    GetClientRect(g_hTopWindow, &rClient);

    int leftEdge = rClient.left;
    int leftWidth = (int)((rClient.right - rClient.left) * HORIZONTAL_SPLIT);
    int middleEdge = leftEdge + leftWidth;
    int rSideWidth = rClient.right - middleEdge;
    int fullWidth = rClient.right - rClient.left;

    int top = rClient.top;

    // Stuff on the left
    static const float ACCOUNTLIST = 0.25F;
    static const float BUDGETINFO = 0.4F;

    int totalHeigth = rClient.bottom - rClient.top;

    // Transaction filter size
    RECT rTransFilter = { 0 };
    GetWindowRect(g_transactionFilterDlg, &rTransFilter);


    // Accounts listview
    int accountListBottom = (int)(totalHeigth * ACCOUNTLIST);
    int accountListHeight = accountListBottom - top;
    SetWindowPos(g_hwndAccountLV, nullptr, leftEdge, top, leftWidth, accountListHeight, SWP_NOZORDER);

    // Budget info pane (includes add account / add expense / unhide / state of the budget)
    int budgetInfoTop = accountListBottom + 1;
    int budgetInfoHeight = 0, budgetInfotBottom = 0;
    if (showTransactionList) {
        budgetInfoHeight = ((int)(totalHeigth * BUDGETINFO));
    }
    else {
        budgetInfoHeight = rClient.bottom - budgetInfoTop - (rTransFilter.bottom - rTransFilter.top);
    }
    budgetInfotBottom = budgetInfoTop + budgetInfoHeight;
    SetWindowPos(g_accountInfoDlg, nullptr, leftEdge, budgetInfoTop, leftWidth, budgetInfoHeight, SWP_NOZORDER);

    // Transaction filter
    int transactionFilterTop = budgetInfotBottom + 1;
    int transactionFilterHeight = rTransFilter.bottom - rTransFilter.top;
    int transactionFilterBottom = transactionFilterTop + transactionFilterHeight;
    SetWindowPos(g_transactionFilterDlg, nullptr, leftEdge, transactionFilterTop, fullWidth, transactionFilterHeight, SWP_NOZORDER);

    // Transaction list
    int transactionListTop = transactionFilterBottom + 1;
    int transactionListHeigth = rClient.bottom - transactionListTop;
    SetWindowPos(g_hwndTransactionLV, nullptr, leftEdge, transactionListTop, fullWidth, transactionListHeigth, SWP_NOZORDER);

    // Stuff on the right
    // Figure out the height of the tree header
    HDLAYOUT treeHeaderLayout = { 0 };
    WINDOWPOS wpTreeHeaderSize = { 0 };
    RECT rTree = rClient;
    treeHeaderLayout.prc = &rTree; // The top is adjusted to accomodate the header
    treeHeaderLayout.pwpos = &wpTreeHeaderSize; // This is the suggested size of the header. We care about the height.
    SendMessage(g_hwndTreeHeader, HDM_LAYOUT, 0, (LPARAM)&treeHeaderLayout);

    // Expense tree header
    SetWindowPos(g_hwndTreeHeader, wpTreeHeaderSize.hwndInsertAfter, middleEdge, top, rSideWidth, wpTreeHeaderSize.cy, wpTreeHeaderSize.flags | SWP_SHOWWINDOW);

    // Expense tree control
    int expenseTreeHeigth = 0;
    if (showTransactionList) {
        expenseTreeHeigth = transactionFilterTop - rClient.top - wpTreeHeaderSize.cy;
    } else{ 
        expenseTreeHeigth = rClient.bottom - rClient.top - wpTreeHeaderSize.cy - transactionFilterHeight;
    }

    SetWindowPos(g_hwndTV, nullptr, middleEdge, rTree.top, rSideWidth, expenseTreeHeigth, SWP_NOZORDER);


    for (size_t i = 0; i < 3; i++) {
        ListView_SetColumnWidth(g_hwndAccountLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

    for (size_t i = 0; i < 6; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

    // Calc tree column spots and adjust the last header width to max
    calcColumnSpots();
    HDITEM hItem = { 0 };
    hItem.cxy = g_columnSizes._info;
    hItem.mask = HDI_WIDTH;
    Header_SetItem(g_hwndTreeHeader, 5, &hItem);

    ShowWindow(g_transactionFilterDlg, SW_SHOW);
    ShowWindow(g_hwndTransactionLV, showTransactionList ? SW_SHOW : SW_HIDE);
    ShowWindow(g_accountInfoDlg, SW_SHOW);
}
