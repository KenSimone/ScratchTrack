// SimpleBudget.cpp : Defines the entry point for the application.
//

#include "framework.h"

#include "AccountInfo.h"
#include "Transaction.h"

#include "ScratchTrack.h"
#include "resource.h"
#include "TransactionDialog.h"
#include "EnvelopeDlg.h"
#include "AccountDlg.h"
#include "Utils.h"
#include "TreeUtils.h"
#include "AccountListViewUtils.h"
#include "TransactionFilterDlg.h"
#include "AccountInfoDlg.h"
#include "DepositDlg.h"
#include "TransferDlg.h"
#include "PayCCDlg.h"

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
                hItemGroup = treeItemFromAccount(pGroup);
            }
        }

        if (!pGroup)
        {
            pGroup = new AccountGroup;
            pGroup->setName(groupName.c_str());
            hItemGroup = addTreeItem(TVI_ROOT, pGroup);
        }


        // No existing envelope. Make one.
        AccountEnvelope* pEnvelope = new AccountEnvelope;
        pEnvelope->setName(catName.c_str());
        pEnvelope->setDefaultTransactionPayToAccount(pAccount);
        pEnvelope->setIsExpense(true);
        pEnvelope->setIsVirtualAccount(true);

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

    // Filter bar
    //HWND hHdrWnd = ListView_GetHeader(g_hwndTransactionLV);
    //SetWindowLong(hHdrWnd, GWL_STYLE, HDS_FILTERBAR);

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

    for (int i = 0; i < 6; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, LVSCW_AUTOSIZE_USEHEADER);
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

        if (pEnvelope->getDefaultTransactionPayToAccount()) {
            transactionDlg.setAccountToId(pEnvelope->getDefaultTransactionPayToAccount()->getId());
        }
        else {
            transactionDlg.setAccountToId(INVALID_ACCOUNT_ID);
        }

        if (pEnvelope->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pEnvelope->getDefaultTransactionPayFromAccount()->getId());
        }
        else {
            transactionDlg.setAccountFromId(INVALID_ACCOUNT_ID);
        }

        transactionDlg.setIsExpense(pEnvelope->isExpense());
        if (pEnvelope->getDefaultTransactionPayFromAccount()) {
            transactionDlg.setAccountFromId(pEnvelope->getDefaultTransactionPayFromAccount()->getId());
        }
    }

    if (transactionDlg.ShowDialog())
    {
        Transaction::addTransaction(transactionDlg.getAmount(), transactionDlg.getAccountFromId(), transactionDlg.getAccountToId(), transactionDlg.getEnvelopeId(), transactionDlg.getPayee(), transactionDlg.getDescription());

        // Apply the amount to the transaction envelope 
        pEnvelope->addBalance(transactionDlg.getAmount());

        // Apply the amount to the account
        Account* pFromAccount = dynamic_cast<Account*>(AccountBase::lookupById(transactionDlg.getAccountFromId()));
        if (pFromAccount)
        {
            // We have the "from" account. Add the transaction amount.
            pFromAccount->addBalance(transactionDlg.getAmount());

            // Is this a transfer?
            Account* pAccountTo = dynamic_cast<Account*>(AccountBase::lookupById(transactionDlg.getAccountToId()));
            if (pAccountTo)
            {
                // This is a transfer, so add the opposite amount to the target account
                pAccountTo->addBalance(-transactionDlg.getAmount());
            }
            else
            {
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

bool editEnvelope(AccountEnvelope* pEnvelope)
{
    EnvelopeDialog dlg;
    dlg.setBudget(pEnvelope->getBudgeted());
    dlg.setEnvelopeName(pEnvelope->getName());
    dlg.setDefaultTransactionPayFromAccount(pEnvelope->getDefaultTransactionPayFromAccount());
    dlg.setDefaultTransactionPayToAccount(pEnvelope->getDefaultTransactionPayToAccount());
    dlg.setDefaultTransactionAmount(pEnvelope->getDefaultTransactionAmount());
    dlg.setDefaultTransactionPayee(pEnvelope->getDefaultTransactionPayee());
    dlg.setIsIncome(!pEnvelope->isExpense());
    dlg.setIsVirtualAccount(pEnvelope->isVirtualAccount());
    if (dlg.ShowDialog())
    {
        pEnvelope->setBudgeted(dlg.getBudget());
        pEnvelope->setDefaultTransactionPayFromAccount(dlg.getDefaultTransactionPayFromAccount());
        pEnvelope->setDefaultTransactionPayToAccount(dlg.getDefaultTransactionPayToAccount());
        pEnvelope->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pEnvelope->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pEnvelope->setIsExpense(!dlg.getIsIncome());
        pEnvelope->setIsVirtualAccount(dlg.getIsVirtual());
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
        pEnvelope->setDefaultTransactionPayToAccount(dlg.getDefaultTransactionPayToAccount());
        pEnvelope->setDefaultTransactionAmount(dlg.getDefaultTransactionAmount());
        pEnvelope->setDefaultTransactionPayee(dlg.getDefaultTransactionPayee());
        pEnvelope->setIsExpense(!dlg.getIsIncome());
        pEnvelope->setIsVirtualAccount(dlg.getIsVirtual());
        pEnvelope->setBudgeted(dlg.getBudget());

        pParent->addChild(pEnvelope);
        addTreeItem(hParent, pEnvelope);
        ret = true;
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
    case ID_ROOT_EDITENVELOPE:
        //zzzz
        didSomething = editEnvelope((AccountEnvelope*)pItem);
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

    // Payee
    std::wstring payee = pTransaction->getPayee();
    if (pTransaction->getAccountToId() != INVALID_ACCOUNT_ID && pAccountTo) {
        wchar_t szBuff[1024] = { 0 };
        if (pAccountFrom) {
            swprintf(szBuff, 1024, L"%s->%s", pAccountFrom->getName(), pAccountTo->getName());
        }
        else{ 
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

void editTransaction(Transaction* pTransaction, int listviewItemIndex)
{
    if (pTransaction->getAccountFromId() == INVALID_ACCOUNT_ID) {
        return editPayCC(pTransaction, listviewItemIndex);
    }

    TransactionDialog dlg;
    dlg.setAmount(pTransaction->getAmount());
    dlg.setPayee(pTransaction->getPayee());
    dlg.setDescription(pTransaction->getDescription());
    dlg.setAccountFromId(pTransaction->getAccountFromId());
    dlg.setAccountToId(pTransaction->getAccountToId());
    dlg.setEnvelopeId(pTransaction->getEnvelopeId());

    if (dlg.ShowDialog())
    {
        // This is actually kind of complicated. 
        // We need to undo the original transacation first.
        AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(AccountBase::lookupById(pTransaction->getEnvelopeId()));
        Account* pAccountFrom = dynamic_cast<Account *>(AccountBase::lookupById(pTransaction->getAccountFromId()));
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
        pTransaction->setDetails(dlg.getAmount(), dlg.getPayee(), dlg.getDescription(), dlg.getEnvelopeId(), dlg.getAccountFromId(), dlg.getAccountToId());
        applyModifiedTransaction(pTransaction, listviewItemIndex);

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
    std::wstring temp;
    if (getOneText(temp)) {
        AccountGroup* pGroup = new AccountGroup;
        pGroup->setName(temp.c_str());
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
        case ID_FILE_DECREASEFONTSIZE:
            g_fontScale -= .2;
            break;
        case ID_FILE_INCREASEFONTSIZE:
            g_fontScale += .2;
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
        }
        else if (pHeader->idFrom == IDC_SIMPLEBUDGET_ACCOUNT_LIST)
        {
            if (pHeader->code == NM_CLICK)
            {
                POINT pt = { 0 };
                TVHITTESTINFO tvhi = { 0 };
                GetCursorPos(&pt);
                ScreenToClient(g_hwndAccountLV, &pt);

                Account* pHitAccount = nullptr;
                Account::ACTION_FLAG af = hitTestAccountList(pt, &pHitAccount);
                if (af == Account::ACTION_FLAG::DEPOSIT) {
                    DepositDlg dlg;
                    dlg.ShowDialog();
                    return true;
                }
                else if (af == Account::ACTION_FLAG::TRANSFER) {
                    TransferDlg dlg;
                    dlg.ShowDialog();
                    return true;
                }
                else if (af == Account::ACTION_FLAG::PAY) {
                    PayCCDlg dlg(pHitAccount, 0, 0);
                    if (dlg.ShowDialog()) {
                        Transaction::addTransaction(dlg.getAmount(), dlg.getPaidFromId(), pHitAccount->getId(), pHitAccount->paidFromEnvelopeId(), pHitAccount->getName(), dlg.getDescription());
                        pHitAccount->addBalance(-dlg.getAmount());
                        AccountEnvelope* pEnvelope = (AccountEnvelope * )Account::lookupById(pHitAccount->paidFromEnvelopeId());
                        pEnvelope->addBalance(dlg.getAmount());
                        Account* pFrom = (Account*)Account::lookupById(dlg.getPaidFromId());
                        pFrom->addBalance(dlg.getAmount());
                        serializeState();
                        updateGuiValues();
                        populateTransactions();

                    }
                    return true;
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
                    editAccount(pAccount);
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
                    if (!pEnvelope)
                    {
                        int x = 5;
                    }

                    if (editEnvelope(pEnvelope)) {
                        serializeState();
                    }
                    
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
                if (acctBase)
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
                        hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_ENVELOPE_MENU));
                        hMenu = GetSubMenu(hMenu, 0);

                        // Add the "move to group" option
                        //bool added = false;
                        //std::vector<AccountBase*> groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
                        //HMENU groupMenu = CreatePopupMenu();
                        //for (size_t i = 0; i < groups.size(); i++) {
                        //    if (pParentItem && groups[i]->getId() != pParentItem->getId()) {
                        //        AppendMenu(groupMenu, MF_STRING, ID_ROOT_ENVELOPE_MOVE_TO_START + groups[i]->getId(), groups[i]->getName());
                        //        added = true;
                        //    }
                        //}

                        //if (added) {
                        //    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)groupMenu, L"Move to group...");
                        //}

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

            std::wstring name;
            static const wchar_t* szCreditCard = (wchar_t*)(u"\U0001F4B3 ");
            static const wchar_t* szBank = (wchar_t*)u"\U0001F3E6 ";
            name = (pAccount->paidFromEnvelopeId() == INVALID_ACCOUNT_ID) ? szBank : szCreditCard;
            name += pAccount->getName();

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

    // Clear the list view
    ListView_DeleteAllItems(g_hwndTransactionLV);

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

    DWORD dwFlags = LVSCW_AUTOSIZE_USEHEADER;
    for (DWORD i = 0; i < 5; i++) {
        ListView_SetColumnWidth(g_hwndTransactionLV, i, dwFlags);
    }    

    SendMessage(g_hwndTransactionLV, WM_SETREDRAW, TRUE, 0);

}

int serializeState()
{
    std::vector<AccountBase*> topItems;

    // Collect the accounts
    for (int i = 0; i < ListView_GetItemCount(g_hwndAccountLV); i++)
    {
        LVITEM item = { 0 };
        item.iItem = i;
        item.mask = LVIF_PARAM;
        ListView_GetItem(g_hwndAccountLV, &item);
        Account* pAccount = (Account*)item.lParam;
        topItems.push_back(pAccount);
    }

    // Collect Categories and subcategories
    HTREEITEM hItem = TreeView_GetChild(g_hwndTV, TVI_ROOT);
    while (hItem){
        AccountBase* item = accountFromTreeItem(hItem);
        topItems.push_back(item);
        hItem = TreeView_GetNextSibling(g_hwndTV, hItem);
    }

    serialize(topItems, getPersistFile());

    g_accountInfoDlg.updateReport();

    return ERROR_SUCCESS;
}

void positionControls()
{
    static const double TREE_VERTICAL_SPACE = .65;
    static const double TREE_HORIZONTAL_SPACE = .70;

    RECT rClient = { 0 }, rTransFilter = { 0 };
    HDLAYOUT hdl = { 0 };
    WINDOWPOS wp = { 0 };
    GetClientRect(g_hTopWindow, &rClient);
    hdl.prc = &rClient;
    hdl.pwpos = &wp;
    SendMessage(g_hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hdl);

    GetWindowRect(g_transactionFilterDlg, &rTransFilter);

    int treeleft = (int)(rClient.right * (1.0 - TREE_HORIZONTAL_SPACE)); // Left size of the tree, the account stuff is to the right
    int accountListTop = 0;
    int accountListLeft = 0;
    int accountListRight = treeleft - 1;
    int accountListBottom = (int)(rClient.bottom * TREE_VERTICAL_SPACE) /2;

    int accountInfoTop = accountListBottom + 1; //  Below the account list
    int accountInfoBottom = (int)(rClient.bottom * TREE_VERTICAL_SPACE) - 1; // Same height as the tree

    int transFilterTop = accountInfoBottom+1; // Top of the transaction filter, which is above transactions, and below account stuff and tree
    int transListTop = transFilterTop + (rTransFilter.bottom - rTransFilter.top); // Top of transaction list, which is below the filter
    int transListHeight = rClient.bottom - transListTop; // From the top of the transaction list to the bottom of the screen

    // Envelope tree header
    SetWindowPos(g_hwndHeader, wp.hwndInsertAfter, treeleft, 0, wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);

    // Envelope tree control
    SetWindowPos(g_hwndTV, nullptr, treeleft, rClient.top, rClient.right, transFilterTop - 1 - wp.cy, SWP_NOZORDER);

    // Accounts listview
    SetWindowPos(g_hwndAccountLV, nullptr, 
        accountListLeft, accountListTop,  // Top left
        accountListRight, (accountListBottom - accountListTop) - 1,  // left edge of the tree, above the transaction filter
        SWP_NOZORDER);

    // Account info dlg
    SetWindowPos(g_accountInfoDlg, nullptr, accountListLeft, accountInfoTop, accountListRight, accountInfoBottom - accountInfoTop, SWP_NOZORDER);

    // Transaction filter
    SetWindowPos(g_transactionFilterDlg, nullptr, rClient.left, transFilterTop, rClient.right, rTransFilter.bottom - rTransFilter.top, SWP_NOZORDER);

    // Transactions list view
    SetWindowPos(g_hwndTransactionLV, nullptr, rClient.left, transListTop, rClient.right, transListHeight, SWP_NOZORDER);


    ShowWindow(g_transactionFilterDlg, SW_SHOW);
    ShowWindow(g_accountInfoDlg, SW_SHOW);

    //g_hwndAccountLV;
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
