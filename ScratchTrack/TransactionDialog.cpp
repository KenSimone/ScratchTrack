#include "TransactionDialog.h"
#include "AccountInfoDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include "AutoCombo.h"
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

TransactionDialog::TransactionDialog() : _amount(0), _accountFromId(ACCOUNT_ID_INVALID), _bucketId(ACCOUNT_ID_INVALID)
{
}

TransactionDialog::~TransactionDialog()
{
}

bool TransactionDialog::ShowDialog()
{
    THIS_PTR = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Transactions\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"A transaction is a record of an expense. Transactions are paid from accounts and tracked against buckets. "
        L"You can edit transactions by double clicking them or through their right click menu.",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_TRANSACTION_DIALOG), g_hTopWindow, TransactionDialogProc);
    THIS_PTR = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

void TransactionDialog::updateUi(HWND hDlg)
{
}

INT_PTR CALLBACK TransactionDialog::TransactionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (THIS_PTR) {
            // Accounts
            bool selSet = false, toSelSet = false;

            // Account from
            Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT, THIS_PTR->getAccountFromId());

            // Categories
            HWND hCategories = GetDlgItem(hDlg, IDC_COMBO_BUCKET);
            auto categories = AccountBase::getAllAccountsOfType(AccountBase::BUCKET);
            for (auto itr = categories.begin(); itr != categories.end(); itr++) {
                int pos = (int)SendMessage(hCategories, CB_ADDSTRING, 0, (LPARAM)(*itr)->getName());
                SendMessage(hCategories, CB_SETITEMDATA, pos, (LPARAM)*itr);
                AccountBase* pAccount = *itr;
                if (pAccount->getId() == THIS_PTR->getBucketId()) {
                    ComboBox_SetCurSel(hCategories, pos);
                }
            }

            // Transactions
            Utils::populatePayeeCombo(hDlg, IDC_COMBO_PAYEE, THIS_PTR->getPayee());

            // Set expense radio
            int iAmount = THIS_PTR->getAmount() * -1;

            // Set expense text
            std::wstring amount = Utils::penniesToDollarsString(iAmount);
            SetDlgItemText(hDlg, IDC_EDIT_Transaction_AMOUNT, amount.c_str());
            ComboBox_SetText(GetDlgItem(hDlg, IDC_COMBO_PAYEE), THIS_PTR->getPayee());
            SetDlgItemText(hDlg, IDC_EDIT_Transaction_DESCRIPTION, THIS_PTR->getDescription());

            updateUi(hDlg);
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {

                if (THIS_PTR) {

                    HWND hAccountsFrom = GetDlgItem(hDlg, IDC_COMBO_ACCOUNT);
                    HWND hCategories = GetDlgItem(hDlg, IDC_COMBO_BUCKET);

                    // Set amount
                    wchar_t szBuff[1024] = { 0 };
                    GetDlgItemText(hDlg, IDC_EDIT_Transaction_AMOUNT, szBuff, 1024);
                    THIS_PTR->_amount = Utils::dollarsStringToPennies(szBuff) * -1;

                    // Set payee
                    GetDlgItemText(hDlg, IDC_COMBO_PAYEE, szBuff, 1024);
                    THIS_PTR->_payee = szBuff;

                    // Set category
                    int iSel = ComboBox_GetCurSel(hCategories);
                    if (iSel >= 0) {
                        AccountBucket* pBucket = (AccountBucket*)ComboBox_GetItemData(hCategories, iSel);
                        THIS_PTR->setBucketId(pBucket->getId());
                    }

                    // Set description
                    GetDlgItemText(hDlg, IDC_EDIT_Transaction_DESCRIPTION, szBuff, 1024);
                    THIS_PTR->_description = szBuff;

                    // Set account from
                    THIS_PTR->setAccountFromId(Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT));
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