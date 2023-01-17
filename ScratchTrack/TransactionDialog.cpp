#include "TransactionDialog.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include "AutoCombo.h"
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

TransactionDialog::TransactionDialog() : _amount(0), _accountFromId(0), _envelopeId(0), _accountToId(MAXINT)
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

void TransactionDialog::updateUi(HWND hDlg)
{
    bool isTransfer = SendDlgItemMessage(hDlg, IDC_CHECK_PAY_TO_ACCOUNT, BM_GETCHECK, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_ACCOUNT_TO), isTransfer);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_PAYEE), !isTransfer);
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
            HWND hAccountsTo = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT_TO);
            auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
            bool selSet = false, toSelSet = false;
            for (auto itr = accounts.begin(); itr != accounts.end(); itr++){
                AccountBase* pAccount = *itr;

                // Account from
                int pos = (int)SendMessage(hAccounts, CB_ADDSTRING, 0, (LPARAM)pAccount->getName());
                SendMessage(hAccounts, CB_SETITEMDATA, pos, (LPARAM)pAccount);
                if (pAccount->getId() == THIS_PTR->getAccountFromId()) {
                    selSet = true;
                    ComboBox_SetCurSel(hAccounts, pos);
                }

                // Account to
                pos = (int)SendMessage(hAccountsTo, CB_ADDSTRING, 0, (LPARAM)pAccount->getName());
                SendMessage(hAccountsTo, CB_SETITEMDATA, pos, (LPARAM)pAccount);
                if (pAccount->getId() == THIS_PTR->getAccountToId()) {
                    toSelSet = true;
                    ComboBox_SetCurSel(hAccountsTo, pos);
                }

            }
            if (!selSet) {
                ComboBox_SetCurSel(hAccounts, 0);
            }

            if (!toSelSet) {
                ComboBox_SetCurSel(hAccountsTo, 0);
            }

            // Transfer checkbox
            SendDlgItemMessage(hDlg, IDC_CHECK_PAY_TO_ACCOUNT, BM_SETCHECK, THIS_PTR->getAccountToId() != MAXINT ? BST_CHECKED : BST_UNCHECKED, 0);

            // Categories
            HWND hCategories = GetDlgItem(hDlg, IDC_COMBO_ENVELOPE);
            auto categories = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
            for (auto itr = categories.begin(); itr != categories.end(); itr++) {
                int pos = (int)SendMessage(hCategories, CB_ADDSTRING, 0, (LPARAM)(*itr)->getName());
                SendMessage(hCategories, CB_SETITEMDATA, pos, (LPARAM)*itr);
                AccountBase* pAccount = *itr;
                if (pAccount->getId() == THIS_PTR->getEnvelopeId()) {
                    ComboBox_SetCurSel(hCategories, pos);
                }
            }

            // Transactions
            HWND hPayees = GetDlgItem(hDlg, IDC_COMBO_PAYEE);
            MakeAutocompleteCombo(hPayees);

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

            updateUi(hDlg);
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDC_CHECK_PAY_TO_ACCOUNT))
        {
            updateUi(hDlg);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                bool isIncome = SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_GETCHECK, 0, 0);

                if (THIS_PTR) {

                    HWND hAccountsFrom = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT);
                    HWND hAccountsTo = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT_TO);

                    // Set amount
                    wchar_t szBuff[1024] = { 0 };
                    GetDlgItemText(hDlg, IDC_EDIT_Transaction_AMOUNT, szBuff, 1024);
                    THIS_PTR->_amount = Utils::dollarsStringToPennies(szBuff);
                    if (!isIncome) {
                        THIS_PTR->_amount *= -1;
                    }

                    // Set payee
                    GetDlgItemText(hDlg, IDC_COMBO_PAYEE, szBuff, 1024);
                    THIS_PTR->_payee = szBuff;

                    // Set description
                    GetDlgItemText(hDlg, IDC_EDIT_Transaction_DESCRIPTION, szBuff, 1024);
                    THIS_PTR->_description = szBuff;

                    // Set account from
                    HWND hWndFrom = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT);
                    int iSel = ComboBox_GetCurSel(hWndFrom);
                    if (iSel >= 0) {
                        Account* pAcct = (Account*)ComboBox_GetItemData(hWndFrom, iSel);
                        if (pAcct) {
                            THIS_PTR->setAccountFromId(pAcct->getId());
                        }
                    }


                    // Set account to
                    bool isTransfer = SendDlgItemMessage(hDlg, IDC_CHECK_PAY_TO_ACCOUNT, BM_GETCHECK, 0, 0);
                    if (!isTransfer) {
                        THIS_PTR->_accountToId = MAXINT;
                    }
                    else
                    {
                        HWND hWndTo = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT_TO);
                        int iSel = ComboBox_GetCurSel(hWndTo);
                        if (iSel >= 0) {
                            Account* pAcct = (Account * )ComboBox_GetItemData(hWndTo, iSel);
                            if (pAcct) {
                                THIS_PTR->setAccountToId(pAcct->getId());
                            }
                        }
                    }
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