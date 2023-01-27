#include "EnvelopeDlg.h"
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

EnvelopeDialog::EnvelopeDialog() : _amount(0), _pDefaultTransactionPayFromAccount(nullptr), _budget(0)
{

}

EnvelopeDialog::~EnvelopeDialog()
{

}

bool EnvelopeDialog::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Envelopes\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"ScratchTrack follows the 'Envelope Budgeting' philosophy. The idea is that money is assigned to separate envelopes for different spending categories. Expenses are paid from the envelopes. "
        L"When an envelope is empty, spending in that category stops until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Examples of typical envelopes include 'Dining out', 'Utilities', and 'Car payment'. "
        L"Envelopes have a budget. This is a best guess of how much you'll spend in a month. Don't worry if you're not sure. You can adjust this value as needed at a later time.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Expenses are paid from envelopes through the envelope's action icon or right click menu. If an envelope "
        L"is paid from the same account, to the same payee, or is the same amount each time, you can set those values here. When you "
        L"create a transaction in this envelope, those settings will be filled in for you.",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ENVELOPE_DIALOG), g_hTopWindow, EnvelopeDialogProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;

}

INT_PTR CALLBACK EnvelopeDialog::EnvelopeDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (PTHIS) {

            HWND hCatNameEdit = GetDlgItem(hDlg, IDC_EDIT_CAT_NAME);
            SetWindowText(hCatNameEdit, const_cast<wchar_t*>(PTHIS->getEnvelopeName()));

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
            MakeAutocompleteCombo(hPayeeCombo);
            std::vector<Transaction*> transactions = Transaction::getAllTransactions();
            std::set<std::wstring> uniquePayees;
            for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
                uniquePayees.insert((*itr)->getPayee());
            }

            for (auto itr = uniquePayees.begin(); itr != uniquePayees.end(); itr++) {
                if (!itr->empty()) {
                    int pos = (int)SendMessage(hPayeeCombo, CB_ADDSTRING, 0, (LPARAM)itr->c_str());
                }
            }

            ComboBox_SetText(hPayeeCombo, PTHIS->getDefaultTransactionPayee());


            // Default amount
            int iAmount = abs(PTHIS->getDefaultTransactionAmount());

            HWND hAmount = GetDlgItem(hDlg, IDC_EDIT_CAT_AMOUNT);
            SetWindowText(hAmount, Utils::penniesToDollarsString(iAmount).c_str());
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                bool income = SendDlgItemMessage(hDlg, IDC_RADIO_INCOME, BM_GETCHECK, 0, 0);

                if (PTHIS) {
                    // Name
                    wchar_t szBuff[1024] = { 0 };
                    GetDlgItemText(hDlg, IDC_EDIT_CAT_NAME, szBuff, 1024);
                    PTHIS->setEnvelopeName(szBuff);

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

EnvelopeDialog* EnvelopeDialog::PTHIS = nullptr;
