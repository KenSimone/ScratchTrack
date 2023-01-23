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

// Dialogs
#include "TransactionFilterDlg.h"
#include "AccountInfoDlg.h"
#include "DepositDlg.h"
#include "TransferDlg.h"
#include "PayCCDlg.h"
#include "EnvelopeDlg.h"
#include "AccountDlg.h"
#include "FolderDlg.h"
#include "FundEnvelopeDlg.h"
#include "FundEnvelopesDlg.h"

#include <commctrl.h>
#include <Windows.h>
#include <windowsx.h>
#include <vector>

#pragma comment(lib, "Comctl32")
#pragma comment(lib, "Shlwapi")

#define MAX_LOADSTRING 100

// Global settings
static const double FONT_SCALE_FACTOR = 1.4;


// Global Variables:
HINSTANCE g_hInst = nullptr;                                // current instance
HWND g_hTopWindow = nullptr;
HWND g_hwndTV = nullptr, g_hwndTransactionLV = nullptr, g_hwndTreeHeader = nullptr, g_hwndAccountLV = nullptr;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
wchar_t szOneInputText[1024] = { 0 };
TransactionFilterDlg g_transactionFilterDlg;
AccountInfoDlg g_accountInfoDlg;

extern BUDGET_TREE_COLUMN_SIZES g_columnSizes;
extern BUDGET_TREE_DRAG_DETAILS g_btDragDetails;

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
bool doFundEnvelopes(AccountGroup* pGroup);

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

    strcat_s(szBuff, 1024, "\\ScratchTrack.xml");

    return szBuff;
}

void setupPaymentEnvelopeForAccount(Account* pAccount)
{
    if (pAccount->paidFromEnvelopeId() == INVALID_ACCOUNT_ID) {
        std::wstring catName = L"Pay ";
        catName += pAccount->getName();

        // See if already have a envelope 
        auto categories = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
        auto groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);

        for (size_t i = 0; i < categories.size(); i++)
        {
            if (_wcsicmp(catName.c_str(), categories[i]->getName()) == 0) {
                // This seems to be the right envelope
                pAccount->setPaidFromEnvelopeId(categories[i]->getId());
                pAccount->setReadOnly(true);
                return;
            }
        }

        // We need to create a new envelope. First find a group to own it.
        AccountGroup* pGroup = nullptr;
        HTREEITEM hItemGroup = nullptr;
        std::wstring groupName = L"Credit cards";

        for (size_t i = 0; i < groups.size(); i++)
        {
            if (_wcsicmp(groupName.c_str(), groups[i]->getName()) == 0) {
                pGroup = (AccountGroup * )groups[i];
                pGroup->setReadOnly(true);
                hItemGroup = treeItemFromAccount(pGroup);
            }
        }

        if (!pGroup)
        {
            pGroup = new AccountGroup;
            pGroup->setName(groupName.c_str());
            pGroup->setReadOnly(true);
            hItemGroup = addTreeItem(TVI_ROOT, pGroup);
        }


        // No existing envelope. Make one.
        AccountEnvelope* pEnvelope = new AccountEnvelope;
        pEnvelope->setName(catName.c_str());
        pEnvelope->setDefaultTransactionPayToAccount(pAccount);
        pEnvelope->setIsExpense(true);
        pEnvelope->setIsVirtualAccount(true);
        pEnvelope->setReadOnly(true);

        pAccount->setPaidFromEnvelopeId(pEnvelope->getId());

        pGroup->addChild(pEnvelope);
        addTreeItem(hItemGroup, pEnvelope);
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

    ListView_SetExtendedListViewStyle(g_hwndTransactionLV, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    setGuiFont(g_hwndTransactionLV);

    LVCOLUMN column = { 0 };
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(L"Date");
    ListView_InsertColumn(g_hwndTransactionLV, 0, &column);

    column.pszText = const_cast<wchar_t*>(L"Payee");
    ListView_InsertColumn(g_hwndTransactionLV, 1, &column);

    column.pszText = const_cast<wchar_t*>(L"Envelope");
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

       AccountGroup* pGroup = new AccountGroup;
       pGroup->setName(L"Monthly Expenses");
       addTreeItem(TVI_ROOT, pGroup);
       serializeState();
   }

   // Fix up selection / paint
   HTREEITEM hRoot = TreeView_GetRoot(g_hwndTV);
   if (hRoot) {
       TreeView_SelectItem(g_hwndTV, hRoot);
   }

   populateTransactions();
   positionControls();

   updateGuiValues();

   g_accountInfoDlg.updateReport();
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
    AccountBase* pItem = accountFromTreeItem(hItem);
    TransactionDialog transactionDlg;
    transactionDlg.setEnvelopeId(pItem->getId());
    AccountEnvelope* pEnvelope = nullptr;
    if (pItem->getType() == AccountBase::ENVELOPE) {
        pEnvelope = (AccountEnvelope*)pItem;
        transactionDlg.setPayee(pEnvelope->getDefaultTransactionPayee());
        transactionDlg.setAmount(pEnvelope->getDefaultTransactionAmount());

        if (pEnvelope->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pEnvelope->getDefaultTransactionPayFromAccount()->getId());
        }
        else {
            transactionDlg.setAccountFromId(INVALID_ACCOUNT_ID);
        }

        if (pEnvelope->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pEnvelope->getDefaultTransactionPayFromAccount()->getId());
        }
    }

    if (transactionDlg.ShowDialog())
    {
        Transaction::addTransaction(Transaction::EXPENSE, transactionDlg.getAmount(), transactionDlg.getAccountFromId(), INVALID_ACCOUNT_ID, transactionDlg.getEnvelopeId(), transactionDlg.getPayee(), transactionDlg.getDescription());

        // Apply the amount to the transaction envelope 
        pEnvelope->addBalance(transactionDlg.getAmount());

        // Apply the amount to the account
        Account* pFromAccount = dynamic_cast<Account*>(AccountBase::lookupById(transactionDlg.getAccountFromId()));
        if (pFromAccount)
        {
            // We have the "from" account. Add the transaction amount.
            pFromAccount->addBalance(transactionDlg.getAmount());

            // See if this is a payment from a CC account
            AccountEnvelope* pToEnvelope = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pFromAccount->paidFromEnvelopeId()));
            if (pToEnvelope)
            {
                pToEnvelope->addBalance(-transactionDlg.getAmount());
                if (pEnvelope->getBalance() < 0)
                {
                    pEnvelope->addBalance(-pEnvelope->getBalance());
                }
            }
        } // if pFromAccount

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

bool editEnvelope(AccountEnvelope* pEnvelope)
{
    EnvelopeDialog dlg;
    dlg.setBudget(pEnvelope->getBudgeted());
    dlg.setEnvelopeName(pEnvelope->getName());
    dlg.setDefaultTransactionPayFromAccount(pEnvelope->getDefaultTransactionPayFromAccount());
    dlg.setDefaultTransactionAmount(pEnvelope->getDefaultTransactionAmount());
    dlg.setDefaultTransactionPayee(pEnvelope->getDefaultTransactionPayee());
    if (dlg.ShowDialog())
    {
        pEnvelope->setBudgeted(dlg.getBudget());
        pEnvelope->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
        pEnvelope->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pEnvelope->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pEnvelope->setName(dlg.getEnvelopeName());
        
        updateGuiValues();

        return true;
    }
    return false;
}

bool doEnvelopeAdd(AccountGroup* pParent, HTREEITEM hParent)
{
    bool ret = false;
    EnvelopeDialog dlg;
    if (pParent && dlg.ShowDialog()) {
        AccountEnvelope* pEnvelope = new AccountEnvelope;
        pEnvelope->setName(dlg.getEnvelopeName());
        pEnvelope->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
        pEnvelope->setDefaultTransactionPayToAccount(nullptr);
        pEnvelope->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pEnvelope->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pEnvelope->setIsExpense(true);
        pEnvelope->setIsVirtualAccount(true);
        pEnvelope->setBudgeted(dlg.getBudget());

        pParent->addChild(pEnvelope);
        addTreeItem(hParent, pEnvelope);
        ret = true;
    }
    return ret;
}

bool doFundEnvelopes(AccountGroup* pGroup)
{
    FundEnvelopesDlg dlg(pGroup);
    bool ret = dlg.ShowDialog();
    if (ret) {
        auto children = pGroup->getChildren();
        for (size_t i = 0; i < children.size(); i++) {
            AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(children[i]);
            if (pEnvelope) {
                if (dlg.getResetBalance()) {
                    pEnvelope->addBalance(-pEnvelope->getBalance() + -pEnvelope->getBudgeted());
                }
                else {
                    pEnvelope->addBalance(-pEnvelope->getBudgeted());
                }
            }
        }
        updateGuiValues();
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

    if (dwCommand >= ID_ROOT_ENVELOPE_MOVE_TO_START && dwCommand < ID_ROOT_ENVELOPE_MOVE_TO_END) {
        // This is a move
        AccountGroup* pDestGroup = dynamic_cast<AccountGroup*>(AccountBase::lookupById(dwCommand - ID_ROOT_ENVELOPE_MOVE_TO_START));
        AccountGroup* pOrigGroup = dynamic_cast<AccountGroup*>(pItem->getParent());
        HTREEITEM hNewParent = treeItemFromAccount(pDestGroup);

        if (pDestGroup && hNewParent && pOrigGroup->removeChild(pItem)) {
            pDestGroup->addChild(pItem);
            TreeView_DeleteItem(g_hwndTV, hItem);
            addTreeItem(hNewParent, pItem);
            updateGuiValues();
            serializeState();
        }
        
    }
    switch (dwCommand)
    {
    case ID_ENVELOPE_RESERVE_FUNDS:
    {
        AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(pItem);
        FundEnvelopeDlg dlg(pEnvelope);
        if (pEnvelope && dlg.ShowDialog()) {
            pEnvelope->addBalance(-pEnvelope->getBalance() + dlg.getNewBalance());
            updateGuiValues();
            didSomething = true;
        }
        break;
    }
    case ID_ROOT_EDITENVELOPE:
        didSomething = editEnvelope((AccountEnvelope*)pItem);
        break;
    case ID_ROOT_SETBUDGETEDAMOUNT:
        if (pItem && getOneText(temp)) {
            pItem->setBudgeted(Utils::dollarsStringToPennies(temp.c_str()));
            updateGuiValues();
            didSomething = true;
        }
        break;
    case IDM_EDIT_FOLDER:
    {
        didSomething = editFolder((AccountGroup*)pItem);
        break;
    }
    case ID_ROOT_FUNDENVELOPES:
    {
        didSomething = doFundEnvelopes((AccountGroup*)pItem);
        break;
    }
    case ID_ROOT_ADDENVELOPE:
    {
        didSomething = doEnvelopeAdd(pParentGroup, hItem);
        break;
    }
    case ID_ROOT_ADDTransaction:
    {
        didSomething = addTransaction(hItem);
        break;
    }
    }

    return didSomething;
}

void updateTransactionUiAndSave(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAccountFrom = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountFromId()));
    Account* pAccountTo = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountToId()));

    // Payee
    std::wstring payee = pTransaction->getPayee();
    if (pTransaction->getAccountToId() != INVALID_ACCOUNT_ID && pAccountTo) {
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

    // Envelope
    AccountBase* ati = AccountBase::lookupById(pTransaction->getEnvelopeId());
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

void applyModifiedTransaction(Transaction* pTransaction, int listviewItemIndex)
{
    AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pTransaction->getEnvelopeId()));
    Account* pAccountFrom = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountFromId()));
    Account* pAccountTo = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountToId()));

    AccountEnvelope* pEnvelopeTo = nullptr;
    if (pAccountFrom) {
        pEnvelopeTo = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pAccountFrom->paidFromEnvelopeId()));
    }

    // Record the expense in the envelope
    if (pEnvelope) {
        pEnvelope->addBalance(pTransaction->getAmount());
    }

    // Record the expense in the account
    if (pAccountFrom) {
        pAccountFrom->addBalance(pTransaction->getAmount());
    }

    // If this is a transfer to a envelope, record the reverse of the expense in the target account 
    if (pAccountTo) {
        pAccountTo->addBalance(-pTransaction->getAmount());
    }

    // If this is a transfer to a envelope, record the reverse of the expense in the target envelope
    if (pEnvelopeTo) {
        pEnvelopeTo->addBalance(-pTransaction->getAmount());
    }

    updateTransactionUiAndSave(pTransaction, listviewItemIndex);
}

void doPayCC(Account* pAccount)
{
    PayCCDlg dlg(pAccount, 0, 0);
    if (dlg.ShowDialog()) {
        Transaction::addTransaction(Transaction::CCPAYMENT, dlg.getAmount(), dlg.getPaidFromId(), pAccount->getId(), pAccount->paidFromEnvelopeId(), pAccount->getName(), dlg.getDescription());
        pAccount->addBalance(-dlg.getAmount());
        AccountEnvelope* pEnvelope = (AccountEnvelope*)Account::lookupById(pAccount->paidFromEnvelopeId());
        pEnvelope->addBalance(dlg.getAmount());
        Account* pFrom = (Account*)Account::lookupById(dlg.getPaidFromId());
        pFrom->addBalance(dlg.getAmount());
        serializeState();
        updateGuiValues();
        populateTransactions();

    }
}

void doTransfer(Account* pAccount)
{
    TransferDlg dlg(pAccount->getId(), INVALID_ACCOUNT_ID, 0, L"Transfer");
    if (dlg.ShowDialog()) {
        Account* pAccountFrom = (Account*)AccountBase::lookupById(dlg.getAccountFromId());
        Account* pAccountTo = (Account*)AccountBase::lookupById(dlg.getAccountToId());
        if (pAccountFrom && pAccountTo) {
            Transaction::addTransaction(Transaction::TRANSFER, dlg.getAmount(), dlg.getAccountFromId(), dlg.getAccountToId(), INVALID_ACCOUNT_ID, L"Transfer", dlg.getDescription());
            pAccountFrom->addBalance(-dlg.getAmount());
            pAccountTo->addBalance(dlg.getAmount());
            serializeState();
            updateGuiValues();
            populateTransactions();

        }
    }
}
void addDeposit(Account* pAccount)
{
    DepositDlg dlg(pAccount, 0, nullptr, L"Deposit");
    if (dlg.ShowDialog()) {
        Transaction::addTransaction(Transaction::DEPOSIT, dlg.getAmount(), INVALID_ACCOUNT_ID, pAccount->getId(), INVALID_ACCOUNT_ID, dlg.getPayee(), dlg.getDescription());
        pAccount->addBalance(dlg.getAmount());
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
            // Undo original deposit
            Account* pOrigAcct = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountToId()));
            int origAmnt = pTransaction->getAmount();
            pOrigAcct->addBalance(-origAmnt);

            // Apply new version
            Account* pNewAcct = dlg.getAccount();
            pNewAcct->addBalance(dlg.getAmount());

            // Apply new balance to the envelope
            pTransaction->setDetails(dlg.getAmount(), dlg.getPayee(), dlg.getDescription(), pTransaction->getEnvelopeId(), pTransaction->getAccountFromId(), dlg.getAccount()->getId());
            updateTransactionUiAndSave(pTransaction, listviewItemIndex);
        }
    }
}

void editTransfer(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAcctTo = (Account*)AccountBase::lookupById(pTransaction->getAccountToId());
    Account* pAcctFrom = (Account*)AccountBase::lookupById(pTransaction->getAccountFromId());
    if (pAcctTo && pAcctFrom) {
        TransferDlg dlg(pAcctTo->getId(), pAcctFrom->getId(), pTransaction->getAmount(), pTransaction->getDescription());
        if (dlg.ShowDialog()) {

            // Put money back in the "from account"
            int origAmount = pTransaction->getAmount();
            pAcctFrom->addBalance(origAmount);

            // Take money away from the 'to' account
            pAcctTo->addBalance(-origAmount);

            // Apply new balance to the envelope
            pAcctFrom->addBalance(-dlg.getAmount());
            pAcctTo->addBalance(dlg.getAmount());
            pTransaction->setDetails(dlg.getAmount(), pTransaction->getPayee(), dlg.getDescription(), pTransaction->getEnvelopeId(), dlg.getAccountFromId(), dlg.getAccountToId());
            updateTransactionUiAndSave(pTransaction, listviewItemIndex);
        }
    }
}

void editPayCC(Transaction* pTransaction, int listviewItemIndex)
{
    Account* pAcct = (Account*)AccountBase::lookupById(pTransaction->getAccountToId());
    if (pAcct) {
        PayCCDlg dlg(pAcct, pTransaction->getAmount(), pTransaction->getAccountFromId(), pTransaction->getDescription());
        if (dlg.ShowDialog()) {
            AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pTransaction->getEnvelopeId()));
            Account* pAccountTo = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountToId()));
            Account* pAccountFrom = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountFromId()));

            // Put money back in the envelope and "from account"
            int origAmount = pTransaction->getAmount();
            pEnvelope->addBalance(-origAmount);
            pAccountFrom->addBalance(-origAmount);

            // Take money away from the CC account
            pAccountTo->addBalance(origAmount);

            // Apply new balance to the envelope
            pTransaction->setDetails(dlg.getAmount(), pTransaction->getPayee(), dlg.getDescription(), pTransaction->getEnvelopeId(), pTransaction->getAccountFromId(), pTransaction->getAccountToId());
            applyModifiedTransaction(pTransaction, listviewItemIndex);

        }
    }
}

void editExpense(Transaction* pTransaction, int listviewItemIndex)
{
    TransactionDialog dlg;
    dlg.setAmount(pTransaction->getAmount());
    dlg.setPayee(pTransaction->getPayee());
    dlg.setDescription(pTransaction->getDescription());
    dlg.setAccountFromId(pTransaction->getAccountFromId());
    dlg.setEnvelopeId(pTransaction->getEnvelopeId());

    if (dlg.ShowDialog())
    {
        // This is actually kind of complicated. 
        // We need to undo the original transacation first.
        AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pTransaction->getEnvelopeId()));
        Account* pAccountFrom = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountFromId()));
        Account* pAccountTo = dynamic_cast<Account*>(AccountBase::lookupById(pTransaction->getAccountToId()));
        AccountEnvelope* pEnvelopeTo = nullptr;

        pEnvelopeTo = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pAccountFrom->paidFromEnvelopeId()));

        int origAmount = pTransaction->getAmount();
        if (pEnvelope) {
            pEnvelope->addBalance(-origAmount);
        }

        // Put the money back in the original account
        pAccountFrom->addBalance(-origAmount);

        // Take it out of the target account (if any)
        if (pAccountTo) {
            pAccountTo->addBalance(origAmount);
        }

        // Take it out of the target envelope (if any)
        if (pEnvelopeTo) {
            pEnvelopeTo->addBalance(origAmount);
        }

        // Now this is basically the same as add
        pTransaction->setDetails(dlg.getAmount(), dlg.getPayee(), dlg.getDescription(), dlg.getEnvelopeId(), dlg.getAccountFromId(), INVALID_ACCOUNT_ID);
        applyModifiedTransaction(pTransaction, listviewItemIndex);

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

void editAccount(Account* pAccount)
{
    AccountDlg dlg;
    dlg.setBalance(pAccount->getBalance());
    dlg.setBudgeted(pAccount->getContributesToBudget());
    dlg.setName(pAccount->getName());
    dlg.setCreditCard(pAccount->paidFromEnvelopeId() != INVALID_ACCOUNT_ID);
    if (dlg.ShowDialog())
    {
        pAccount->addBalance(-pAccount->getBalance());
        pAccount->addBalance(dlg.getBalance());
        pAccount->setName(dlg.getName());
        pAccount->setContributesToBudget(dlg.getBudgeted());
        if (dlg.getCreditCard()) {
            setupPaymentEnvelopeForAccount(pAccount);
        }
        updateGuiValues();
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
            setupPaymentEnvelopeForAccount(pAccount);
        }
        addTreeItem(TVI_ROOT, pAccount);
        serializeState();
    }
}

void doAddCategory()
{
    FolderDlg dlg(L"", 0);
    if (dlg.ShowDialog()) {
        AccountGroup* pGroup = new AccountGroup;
        pGroup->setName(dlg.getName());
        pGroup->setExpenseLimit(dlg.getLimit());
        addTreeItem(TVI_ROOT, pGroup);
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
        case ID_FILE_ADDENVELOPE:
            doAddCategory();
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
                doAddCategory();
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
                    hMenu = GetSubMenu(hMenu, 0);
                    DWORD dwCode = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    std::wstring temp;

                    switch (dwCode)
                    {
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
                    case ID_ROOT_EDITACCOUNT:
                    {
                        if (pAccount) {
                            editAccount(pAccount);
                        }
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
                if (g_transactionFilterDlg.filterToEnvelopeSelection()) {
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
                    // We've hit a hotlink
                    if (addTransaction(hItem)) {
                        serializeState();
                    }
                    return true;
                }
                else if (hItem && type == ENV_MONEY_ADD)
                {
                    AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(accountFromTreeItem(hItem));
                    std::wstring temp;
                    FundEnvelopeDlg dlg(pEnvelope);
                    if (pEnvelope && dlg.ShowDialog()) {
                        // Add funds to this item
                        int newBalance = dlg.getNewBalance();
                        pEnvelope->addBalance(-pEnvelope->getBalance() + newBalance);

                        updateGuiValues();
                        serializeState();
                    }

                }
                else if (hItem && type == GRP_ENVELOPE_ADD)
                {
                    AccountGroup* pGroup = dynamic_cast<AccountGroup*>(accountFromTreeItem(hItem));
                    if (doEnvelopeAdd(pGroup, hItem)) {
                        serializeState();
                    }
                }
                else if (hItem && type == ENV_ENVELOPE_EDIT)
                {
                    AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(accountFromTreeItem(hItem));
                    if (editEnvelope(pEnvelope)) {
                        serializeState();
                    }
                    
                }
                else if (hItem && type == GRP_FOLDER_EDIT) {
                    AccountGroup* pGroup = dynamic_cast<AccountGroup*>(accountFromTreeItem(hItem));
                    if (editFolder(pGroup)) {
                        serializeState();
                    }
                }
                else if (hItem && (type == GRP_FOLDER_HIDE || type == ENV_HIDE)) {
                    AccountBase* pAcct = accountFromTreeItem(hItem);
                    if (Utils::confirmHide(pAcct)) {
                        TreeView_DeleteItem(g_hwndTV, hItem);
                        pAcct->setHidden(true);
                        serializeState();
                    }
                }
                else if (hItem && (type == GRP_FUND_CHILDREN)) {
                    AccountBase* pAcct = accountFromTreeItem(hItem);
                    doFundEnvelopes((AccountGroup*)pAcct);
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
                    AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(acctBase);
                    if (pEnvelope && editEnvelope(pEnvelope) ){
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
                    HMENU hMenu = nullptr;
                    if (pItem->getType() == AccountBase::GROUP) {
                        hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_GROUP_MENU));
                        hMenu = GetSubMenu(hMenu, 0);
                    }
                    else if (pItem->getType() == AccountBase::ENVELOPE) {
                        if (!pItem->getReadOnly()) {
                            hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_ENVELOPE_MENU));
                            hMenu = GetSubMenu(hMenu, 0);
                        }

                        // Add the "move to group" option
                        bool added = false;
                        std::vector<AccountBase*> groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
                        HMENU groupMenu = CreatePopupMenu();
                        for (size_t i = 0; i < groups.size(); i++) {
                            if (pParentItem && groups[i]->getId() != pParentItem->getId() && !groups[i]->getReadOnly()) {
                                AppendMenu(groupMenu, MF_STRING, ID_ROOT_ENVELOPE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                                added = true;
                            }
                        }

                        if (added) {
                            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)groupMenu, L"Move to group...");
                        }

                        //// Add the "move to account" option
                        //groups = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
                        //HMENU accountMenu = CreatePopupMenu();
                        //for (size_t i = 0; i < groups.size(); i++) {
                        //    if (pParentItem && groups[i]->getId() != pParentItem->getId()) {
                        //        added = true;
                        //        AppendMenu(accountMenu, MF_STRING, ID_ROOT_ENVELOPE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                        //    }
                        //}
                        //if (added) {
                        //    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)accountMenu, L"Move to account...");
                        //}
                    }

                    if (hMenu) {
                        DWORD dwCode = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                        if (dwCode && processContextMenu(dwCode, hItem, pItem)) {
                            updateGuiValues();
                            serializeState();
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
    if (!hItem) {
        return;
    }

    AccountBase* pItem = accountFromTreeItem(hItem);
    if (!pItem){
        return;
    }

    // Get Transactions for item
    std::vector<Transaction*> Transactions = g_transactionFilterDlg.filterToEnvelopeSelection() ? Transaction::getTransactionsForItem(pItem) : Transaction::getAllTransactions();

    // Filter to account, if requested
    if (g_transactionFilterDlg.filterToAccount()) {
        std::vector<Transaction*> temp;
        int iCurAccountSel = ListView_GetNextItem(g_hwndAccountLV, -1, LVNI_SELECTED);
        if (iCurAccountSel >= 0) {
            LVITEM item = { 0 };
            item.iItem = iCurAccountSel;
            item.mask = LVIF_PARAM;
            ListView_GetItem(g_hwndAccountLV, &item);

            Account* pAccount = (Account*)item.lParam;
            for (size_t i = 0; i < Transactions.size(); i++) {
                Transaction* pCurTrans = Transactions[i];
                if (pCurTrans->getAccountToId() == pAccount->getId() || pCurTrans->getAccountFromId() == pAccount->getId())
                {
                    temp.push_back(pCurTrans);
                }
            }
        }
        Transactions = temp;
    }

    // Add them to the list view
    SendMessage(g_hwndTransactionLV, WM_SETREDRAW, FALSE, 0);

    for (auto itr = Transactions.begin(); itr != Transactions.end(); itr++ )
    {
        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        std::wstring timeStamp;
        (*itr)->getLocalTimeStamp(timeStamp);

        Transaction* pCurTransaction = *itr;

        int iAmount = (*itr)->getAmount();
        bool isIncome = iAmount > 0;
        iAmount = abs(iAmount);
        std::wstring amount = Utils::penniesToDollarsString(iAmount);

        std::wstring filterText = g_transactionFilterDlg.filterText();
        if (!filterText.empty())
        {
            bool matched = Utils::searchText(timeStamp.c_str(), filterText.c_str());
            matched = matched || Utils::searchText(pCurTransaction->getDescription(), filterText.c_str());
            matched = matched || Utils::searchText(pCurTransaction->getPayee(), filterText.c_str());
            matched = matched || Utils::searchText(amount.c_str(), filterText.c_str());

            AccountBase* pCat = AccountBase::lookupById(pCurTransaction->getEnvelopeId());
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
        Transaction* pTransaction = *itr;
        std::wstring payee = pTransaction->getPayee();
        AccountBase* pAccountTo = AccountBase::lookupById(pTransaction->getAccountToId());
        if (pAccountTo) {
            AccountBase* pAccountFrom = AccountBase::lookupById(pTransaction->getAccountFromId());
            wchar_t szBuff[1024] = { 0 };
            if (pAccountFrom) {
                swprintf(szBuff, 1024, L"%s->%s", pAccountFrom->getName(), pAccountTo->getName());
            }
            else {
                swprintf(szBuff, 1024, L"%s", pAccountTo->getName());
            }
            payee = szBuff;
        }

        ListView_SetItemText(g_hwndTransactionLV, 0, 1, const_cast<wchar_t*>(payee.c_str()));

        // Envelope
        AccountBase* ati = AccountBase::lookupById((*itr)->getEnvelopeId());
        if (ati) {
            ListView_SetItemText(g_hwndTransactionLV, 0, 2, const_cast<wchar_t*>(ati->getName()));
        }

        // Description
        ListView_SetItemText(g_hwndTransactionLV, 0, 3, const_cast<wchar_t*>((*itr)->getDescription()));

        ListView_SetItemText(g_hwndTransactionLV, 0, isIncome ? 5 : 4, const_cast<wchar_t*>(amount.c_str()));
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

    // Collect Categories and subcategories from the UI
    HTREEITEM hItem = TreeView_GetChild(g_hwndTV, TVI_ROOT);
    while (hItem){
        AccountBase* item = accountFromTreeItem(hItem);
        topItems.push_back(item);
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


    serialize(topItems, getPersistFile());

    g_accountInfoDlg.updateReport();

    return ERROR_SUCCESS;
}

void setGuiFont(HWND hCtrl)
{
    // Adjust font
    HFONT hFont = (HFONT)SendMessage(hCtrl, WM_GETFONT, NULL, NULL);
    LOGFONT lf = { 0 };
    GetObject(hFont, sizeof(lf), &lf);
    lf.lfHeight = (LONG)(lf.lfHeight * FONT_SCALE_FACTOR);
    HFONT hNewFont = CreateFontIndirect(&lf);
    SendMessage(hCtrl, WM_SETFONT, (WPARAM)hNewFont, true);
}

void newPositionControls()
{
    static const float HORIZONTAL_SPLIT = .4F;

    RECT rClient = { 0 };
    HDLAYOUT headerLayout = { 0 };

    // Get the area of our workspace
    GetClientRect(g_hTopWindow, &rClient);

    int leftEdge = rClient.left;
    int leftWidth = (int)((rClient.right - rClient.left) * HORIZONTAL_SPLIT);
    int middleEdge = leftEdge + leftWidth;
    int rSideWidth = rClient.right - middleEdge;

    int top = rClient.top;

// Stuff on the left
    static const float ACCOUNTLIST = 0.15F;
    static const float BUDGETINFO = 0.33F;

    int totalHeigth = rClient.bottom - rClient.top;

    // Accounts listview
    int accountListBottom = (int)(totalHeigth * ACCOUNTLIST);
    int accountListHeight = accountListBottom - top;
    SetWindowPos(g_hwndAccountLV, nullptr, leftEdge, top, leftWidth, accountListHeight, SWP_NOZORDER);

    // Budget info pane (includes add account / add category / unhide / state of the budget)
    int budgetInfoTop = accountListBottom + 1;
    int budgetInfoHeight = (int)(totalHeigth * BUDGETINFO);
    int budgetInfotBottom = budgetInfoTop + budgetInfoHeight;
    SetWindowPos(g_accountInfoDlg, nullptr, leftEdge, budgetInfoTop, leftWidth, budgetInfoHeight, SWP_NOZORDER);

    // Transaction filter
    RECT rTransFilter = { 0 };
    GetWindowRect(g_transactionFilterDlg, &rTransFilter);
    int transactionFilterTop = budgetInfotBottom + 1;
    int transactionFilterHeight = rTransFilter.bottom - rTransFilter.top;
    int transactionFilterBottom = transactionFilterTop + transactionFilterHeight;
    SetWindowPos(g_transactionFilterDlg, nullptr, leftEdge, transactionFilterTop, leftWidth, transactionFilterHeight, SWP_NOZORDER);

    // Transaction list
    int transactionListTop = transactionFilterBottom + 1;
    int transactionListHeigth = rClient.bottom - transactionListTop;
    SetWindowPos(g_hwndTransactionLV, nullptr, leftEdge, transactionListTop, leftWidth, transactionListHeigth, SWP_NOZORDER);


// Stuff on the right
    // Figure out the height of the tree header
    HDLAYOUT treeHeaderLayout = { 0 };
    WINDOWPOS wpTreeHeaderSize = { 0 };
    RECT rTree = rClient;
    treeHeaderLayout.prc = &rTree; // The top is adjusted to accomodate the header
    treeHeaderLayout.pwpos = &wpTreeHeaderSize; // This is the suggested size of the header. We care about the height.

    SendMessage(g_hwndTreeHeader, HDM_LAYOUT, 0, (LPARAM)&treeHeaderLayout);


    // Envelope tree header
    SetWindowPos(g_hwndTreeHeader, wpTreeHeaderSize.hwndInsertAfter, middleEdge, top, rSideWidth, wpTreeHeaderSize.cy, wpTreeHeaderSize.flags | SWP_SHOWWINDOW);

    // Envelope tree control
    int envelopeTreeHeigth = rClient.bottom - rClient.top - wpTreeHeaderSize.cy;
    SetWindowPos(g_hwndTV, nullptr, middleEdge, rTree.top, rSideWidth, envelopeTreeHeigth, SWP_NOZORDER);


    for (size_t i = 0; i < 3; i++) {
        ListView_SetColumnWidth(g_hwndAccountLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

    for (size_t i = 0; i < 6; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

    ShowWindow(g_transactionFilterDlg, SW_SHOW);
    ShowWindow(g_accountInfoDlg, SW_SHOW);


}


void positionControls()
{
#ifdef NEW_POS_CONTROLS
    newPositionControls();
    return;
#endif

    bool showTransactionList = g_transactionFilterDlg.showTransactionList();

    static const float HORIZONTAL_SPLIT = .4F;

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
    static const float ACCOUNTLIST = 0.15F;
    static const float BUDGETINFO = 0.33F;

    int totalHeigth = rClient.bottom - rClient.top;

    // Transaction filter size
    RECT rTransFilter = { 0 };
    GetWindowRect(g_transactionFilterDlg, &rTransFilter);


    // Accounts listview
    int accountListBottom = (int)(totalHeigth * ACCOUNTLIST);
    int accountListHeight = accountListBottom - top;
    SetWindowPos(g_hwndAccountLV, nullptr, leftEdge, top, leftWidth, accountListHeight, SWP_NOZORDER);

    // Budget info pane (includes add account / add category / unhide / state of the budget)
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

    // Envelope tree header
    SetWindowPos(g_hwndTreeHeader, wpTreeHeaderSize.hwndInsertAfter, middleEdge, top, rSideWidth, wpTreeHeaderSize.cy, wpTreeHeaderSize.flags | SWP_SHOWWINDOW);

    // Envelope tree control
    int envelopeTreeHeigth = 0;
    if (showTransactionList) {
        envelopeTreeHeigth = transactionFilterTop - rClient.top - wpTreeHeaderSize.cy;
    } else{ 
        envelopeTreeHeigth = rClient.bottom - rClient.top - wpTreeHeaderSize.cy - transactionFilterHeight;
    }

    SetWindowPos(g_hwndTV, nullptr, middleEdge, rTree.top, rSideWidth, envelopeTreeHeigth, SWP_NOZORDER);


    for (size_t i = 0; i < 3; i++) {
        ListView_SetColumnWidth(g_hwndAccountLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

    ShowWindow(g_transactionFilterDlg, SW_SHOW);
    ShowWindow(g_hwndTransactionLV, showTransactionList ? SW_SHOW : SW_HIDE);
    ShowWindow(g_accountInfoDlg, SW_SHOW);

    for (size_t i = 0; i < 6; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, LVSCW_AUTOSIZE_USEHEADER);
    }

}
