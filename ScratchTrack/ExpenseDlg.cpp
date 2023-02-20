#include "ExpenseDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include "Utils.h"
#include "AutoCombo.h"
#include "AccountInfoDlg.h"
#include <Windows.h>
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

ExpenseDialog::ExpenseDialog() : _amount(0), _pDefaultTransactionPayFromAccount(nullptr), _budget(0)
{

}

ExpenseDialog::~ExpenseDialog()
{

}

bool ExpenseDialog::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Categories\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"In ScratchTrack funds are assigned to spending categories. When expenses are recorded in a expense, funds in that expense are reduced by the epxense amount. "
        L"When a expense is out of money, spending in that expense should stop until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Examples of categories include 'Dining out', 'Utilities', and 'Car payment'. "
        L"Categories have a budget. This is a best guess of how much you'll spend in a month. Don't worry if you're not sure. You can adjust this value later as needed.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Expenses are paid from categories through the expense's action icon or right click menu. If a expense's "
        L"expense is the same ever time (i.e. a car payment), you can set those values here. When you "
        L"create a transaction in this expense, the settings will be filled in for you (but you can change them as needed).",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EXPENSE_DIALOG), g_hTopWindow, ExpenseDialogProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;

}

void ExpenseDialog::enableDisableUi(HWND hDlg)
{

    bool enableOk = true;
    if (Utils::isDlgItemTextEmpty(hDlg, IDC_EDIT_CAT_NAME)) {
        enableOk = false;
    }

    EnableWindow(GetDlgItem(hDlg, IDOK), enableOk);
}


INT_PTR CALLBACK ExpenseDialog::ExpenseDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (PTHIS) {

            HWND hCatNameEdit = GetDlgItem(hDlg, IDC_EDIT_CAT_NAME);
            SetWindowText(hCatNameEdit, const_cast<wchar_t*>(PTHIS->getExpenseName()));

            HWND hAccountCombo = GetDlgItem(hDlg, IDC_COMBO_CAT_ACCOUNT);
            auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
            bool selSet = false;


            // Accounts from
            for (auto itr = accounts.begin(); itr != accounts.end(); itr++) {
                AccountBase* pAccount = *itr;
                int pos = (int)SendMessage(hAccountCombo, CB_ADDSTRING, 0, (LPARAM)pAccount->getName());
                SendMessage(hAccountCombo, CB_SETITEMDATA, pos, (LPARAM)pAccount);
                if (pAccount == PTHIS->getDefaultTransactionPayFromAccount()) {
                    selSet = true;
                    ComboBox_SetCurSel(hAccountCombo, pos);
                }
            }
            if (!selSet) {
                ComboBox_SetCurSel(hAccountCombo, 0);
            }

            // Budget
            HWND hBudget = GetDlgItem(hDlg, IDC_EDIT_CAT_MONTHLY_BUDGET);
            std::wstring temp = Utils::penniesToDollarsString(abs(PTHIS->getBudget()));
            SetWindowText(hBudget, temp.c_str());


            // Payees
            HWND hPayeeCombo = GetDlgItem(hDlg, IDC_COMBO_CAT_PAYEE);
            Utils::populatePayeeCombo(hDlg, IDC_COMBO_CAT_PAYEE);
            ComboBox_SetText(hPayeeCombo, PTHIS->getDefaultTransactionPayee());


            // Default amount
            int iAmount = abs(PTHIS->getDefaultTransactionAmount());

            HWND hAmount = GetDlgItem(hDlg, IDC_EDIT_CAT_AMOUNT);
            SetWindowText(hAmount, Utils::penniesToDollarsString(iAmount).c_str());
        }

        PTHIS->enableDisableUi(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE)
        {
            PTHIS->enableDisableUi(hDlg);
        }

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                bool folder = SendDlgItemMessage(hDlg, IDC_RADIO_FOLDER, BM_GETCHECK, 0, 0);

                if (PTHIS) {
                    // Name
                    wchar_t szBuff[1024] = { 0 };
                    GetDlgItemText(hDlg, IDC_EDIT_CAT_NAME, szBuff, 1024);
                    PTHIS->setExpenseName(szBuff);

                    // Account from
                    HWND hAccountCombo = GetDlgItem(hDlg, IDC_COMBO_CAT_ACCOUNT);
                    int curSel = ComboBox_GetCurSel(hAccountCombo);
                    if (curSel >= 0) {
                        Account* pParentAccount = (Account*)ComboBox_GetItemData(hAccountCombo, curSel);
                        PTHIS->setDefaultTransactionPayFromAccount(pParentAccount);
                    }


                    // Payee
                    szBuff[0] = 0;
                    HWND hPayeeCombo = GetDlgItem(hDlg, IDC_COMBO_CAT_PAYEE);
                    GetWindowText(hPayeeCombo, szBuff, 1024);
                    PTHIS->setDefaultTransactionPayee(szBuff);

                    // Amount
                    szBuff[0] = 0;
                    HWND hAmount = GetDlgItem(hDlg, IDC_EDIT_CAT_AMOUNT);
                    GetWindowText(hAmount, szBuff, 1024);
                    int amt = abs(Utils::dollarsStringToPennies(szBuff));


                    szBuff[0] = 0;
                    HWND hBudget = GetDlgItem(hDlg, IDC_EDIT_CAT_MONTHLY_BUDGET);
                    GetWindowText(hBudget, szBuff, 1024);
                    int budgetedAmt = Utils::dollarsStringToPennies(szBuff);

                    // Budgeted amount
                    budgetedAmt *= -1;
                    amt *= -1;

                    PTHIS->setBudget(budgetedAmt);
                    PTHIS->setDefaultTransactionAmount(amt);
                }
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

ExpenseDialog* ExpenseDialog::PTHIS = nullptr;
