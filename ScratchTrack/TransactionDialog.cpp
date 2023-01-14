#include "TransactionDialog.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

TransactionDialog::TransactionDialog() : _amount(0), _accountFromId(0), _categoryId(0), _accountToId(MAXINT)
{
}

TransactionDialog::~TransactionDialog()
{
}

bool TransactionDialog::ShowDialog()
{
    THIS_PTR = this;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_TRANSACTION_DIALOG), g_hTopWindow, TransactionDialogProc);
    THIS_PTR = nullptr;

    return result == IDOK;
}

INT_PTR CALLBACK TransactionDialog::TransactionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (THIS_PTR) {
            // Accounts
            HWND hAccounts = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT);
            auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
            bool selSet = false;
            for (auto itr = accounts.begin(); itr != accounts.end(); itr++){
                AccountBase* pAccount = *itr;
                int pos = (int)SendMessage(hAccounts, CB_ADDSTRING, 0, (LPARAM)pAccount->getName());
                SendMessage(hAccounts, CB_SETITEMDATA, pos, (LPARAM)pAccount);
                if (pAccount->getId() == THIS_PTR->getAccountFromId()) {
                    selSet = true;
                    ComboBox_SetCurSel(hAccounts, pos);
                }
            }
            if (!selSet) {
                ComboBox_SetCurSel(hAccounts, 0);
            }


            // Categories
            HWND hCategories = GetDlgItem(hDlg, IDC_COMBO_CATEGORY);
            auto categories = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
            for (auto itr = categories.begin(); itr != categories.end(); itr++) {
                int pos = (int)SendMessage(hCategories, CB_ADDSTRING, 0, (LPARAM)(*itr)->getName());
                SendMessage(hCategories, CB_SETITEMDATA, pos, (LPARAM)*itr);
                AccountBase* pAccount = *itr;
                if (pAccount->getId() == THIS_PTR->getCategoryId()) {
                    ComboBox_SetCurSel(hCategories, pos);
                }
            }

            // Transactions
            HWND hPayees = GetDlgItem(hDlg, IDC_COMBO_PAYEE);
            std::vector<Transaction*> transactions = Transaction::getAllTransactions();
            std::set<std::wstring> uniquePayees;
            for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
                uniquePayees.insert((*itr)->getPayee());
            }

            for (auto itr = uniquePayees.begin(); itr != uniquePayees.end(); itr++) {
                if (!itr->empty()) {
                    int pos = (int)SendMessage(hPayees, CB_ADDSTRING, 0, (LPARAM)itr->c_str());
                    if (_wcsicmp(itr->c_str(), THIS_PTR->getPayee()) == 0)
                    {
                        selSet = true;
                        ComboBox_SetCurSel(hPayees, pos);
                    }
                }
            }

            // Set expense radio
            int iAmount = THIS_PTR->getAmount();

            if (iAmount < 0 || (!iAmount && THIS_PTR->getIsExpense()) ) {
                SendDlgItemMessage(hDlg, IDC_RADIO_EXPENSE, BM_SETCHECK, BST_CHECKED, 0);
                iAmount *= -1;
            }
            else {
                SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_SETCHECK, BST_CHECKED, 0);
            }

            // Set expense text
            std::wstring amount = Utils::penniesToDollarsString(iAmount);
            SetDlgItemText(hDlg, IDC_EDIT_Transaction_AMOUNT, amount.c_str());
            ComboBox_SetText(GetDlgItem(hDlg, IDC_COMBO_PAYEE), THIS_PTR->getPayee());
            SetDlgItemText(hDlg, IDC_EDIT_Transaction_DESCRIPTION, THIS_PTR->getDescription());
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                bool isIncome = SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_GETCHECK, 0, 0);

                if (THIS_PTR) {
                    wchar_t szBuff[1024] = { 0 };
                    GetDlgItemText(hDlg, IDC_EDIT_Transaction_AMOUNT, szBuff, 1024);
                    THIS_PTR->_amount = Utils::dollarsStringToPennies(szBuff);
                    if (!isIncome) {
                        THIS_PTR->_amount *= -1;
                    }

                    GetDlgItemText(hDlg, IDC_COMBO_PAYEE, szBuff, 1024);
                    THIS_PTR->_payee = szBuff;

                    GetDlgItemText(hDlg, IDC_EDIT_Transaction_DESCRIPTION, szBuff, 1024);
                    THIS_PTR->_description = szBuff;
                }
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

TransactionDialog* TransactionDialog::THIS_PTR = nullptr;