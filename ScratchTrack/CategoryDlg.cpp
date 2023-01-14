#include "CategoryDlg.h"
#include "resource.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include "Utils.h"
#include <Windows.h>
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

CategoryDialog::CategoryDialog() : _amount(0), _isIncome(false), _pDefaultTransactionPayFromAccount(nullptr), _isVirtual(false), _budget(0)
{

}

CategoryDialog::~CategoryDialog()
{

}

bool CategoryDialog::ShowDialog()
{
    PTHIS = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_CATEGORY_DIALOG), g_hTopWindow, CategoryDialogProc);
    PTHIS = nullptr;

    return result == IDOK;

}

void enableDisableUi(HWND hDlg)
{
    bool income = SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_GETCHECK, 0, 0);
    HWND hBudget = GetDlgItem(hDlg, IDC_EDIT_CAT_MONTHLY_BUDGET);
    HWND hVirtAcct = GetDlgItem(hDlg, IDC_CHECK_VIRTUAL_ACCOUNT);

    EnableWindow(hBudget, !income);
    EnableWindow(hVirtAcct, !income);
}

INT_PTR CALLBACK CategoryDialog::CategoryDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (PTHIS) {
            HWND hExpenseRadio = GetDlgItem(hDlg, IDC_RADIO_EXPENSE);
            HWND hIncomeRadio = GetDlgItem(hDlg, IDC_RADIO_INCOME);
            SendDlgItemMessage(hDlg, IDC_CHECK_VIRTUAL_ACCOUNT, BM_SETCHECK, PTHIS->getIsVirtual() ? BST_CHECKED : BST_UNCHECKED, 0);

            HWND hCatNameEdit = GetDlgItem(hDlg, IDC_EDIT_CAT_NAME);
            SetWindowText(hCatNameEdit, const_cast<wchar_t*>(PTHIS->getCategoryName()));

            HWND hAccountCombo = GetDlgItem(hDlg, IDC_COMBO_CAT_ACCOUNT);
            auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
            bool selSet = false;

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
            std::wstring temp = Utils::penniesToDollarsString(PTHIS->getBudget());
            SetWindowText(hBudget, temp.c_str());


            // Payees
            HWND hPayeeCombo = GetDlgItem(hDlg, IDC_COMBO_CAT_PAYEE);
            std::vector<Transaction*> transactions = Transaction::getAllTransactions();
            std::set<std::wstring> uniquePayees;
            for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
                uniquePayees.insert((*itr)->getPayee());
            }
            bool selected = 0;
            for (auto itr = uniquePayees.begin(); itr != uniquePayees.end(); itr++) {
                if (!itr->empty()) {
                    int pos = (int)SendMessage(hPayeeCombo, CB_ADDSTRING, 0, (LPARAM)itr->c_str());
                    if (_wcsicmp(itr->c_str(), PTHIS->getDefaultTransactionPayee()) == 0)
                    {
                        ComboBox_SetCurSel(hPayeeCombo, pos);
                    }
                }
            }

            // Default amount
            int iAmount = PTHIS->getDefaultTransactionAmount();
            if (PTHIS->_isIncome) {
                SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_SETCHECK, BST_CHECKED, 0);
            }
            else {
                SendDlgItemMessage(hDlg, IDC_RADIO_EXPENSE, BM_SETCHECK, BST_CHECKED, 0);
                iAmount *= -1;
            }

            HWND hAmount = GetDlgItem(hDlg, IDC_EDIT_CAT_AMOUNT);
            SetWindowText(hAmount, Utils::penniesToDollarsString(iAmount).c_str());
            enableDisableUi(hDlg);
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDC_RADIO_EXPENSE || LOWORD(wParam) == IDC_RADIO_INCOME))
        {
            // Income or expense
            enableDisableUi(hDlg);
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                bool income = SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_GETCHECK, 0, 0);

                if (PTHIS) {
                    // Name
                    wchar_t szBuff[1024] = { 0 };
                    GetDlgItemText(hDlg, IDC_EDIT_CAT_NAME, szBuff, 1024);
                    PTHIS->setCategoryName(szBuff);

                    // Account
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

                    // Income or expense
                    bool income = SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_GETCHECK, 0, 0);
                    PTHIS->setIsIncome(income);

                    // Budgeted amount
                    if (income) {
                        PTHIS->setBudget(0);
                        PTHIS->setIsVirtualAccount(false);
                    }
                    else
                    {
                        szBuff[0] = 0;
                        HWND hBudget = GetDlgItem(hDlg, IDC_EDIT_CAT_MONTHLY_BUDGET);
                        GetWindowText(hBudget, szBuff, 1024);
                        PTHIS->setBudget(Utils::dollarsStringToPennies(szBuff));
                        PTHIS->setIsVirtualAccount(SendDlgItemMessage(hDlg, IDC_CHECK_VIRTUAL_ACCOUNT, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        amt *= -1;
                    }

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

CategoryDialog* CategoryDialog::PTHIS = nullptr;
