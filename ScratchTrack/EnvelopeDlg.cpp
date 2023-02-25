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

    g_accountInfoDlg.appendText(L"Envelopes are used to plan and track how money is spent. Examples of envelopes include 'Dining out', 'Utilities', and 'Car payment'. "
        L"Envelopes have a budget. This is a best guess of how much you'll spend in a month. You can adjust this value at any time by editing the Envelope.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"In ScratchTrack funds are assigned to envelopes. When transactions are recorded, funds assigned to the selected envelope are reduced by the transaction amount. "
        L"When an envelope is out of money, spending in that envelope should stop until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"Transactions are recorded by clicking the 'record expense' icon next to the envelope, or through the right click menu. If the details of transactions in an envelope "
        L"are often the same (i.e. a car payment), you can set those values here. When you "
        L"create a transaction in this envelope the settings will be filled in for you. You can adjust them as needed when recording new transactions.",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ENVELOPE_DIALOG), g_hTopWindow, EnvelopeDialogProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;

}

void EnvelopeDialog::enableDisableUi(HWND hDlg)
{

    bool enableOk = true;
    if (Utils::isDlgItemTextEmpty(hDlg, IDC_EDIT_CAT_NAME)) {
        enableOk = false;
    }

    bool isSavingsGoal = SendDlgItemMessage(hDlg, IDC_CHECK_SAVINGS_GOAL, BM_GETCHECK, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SAVINGS_GOAL), isSavingsGoal);

    if (isSavingsGoal && enableOk ) {
        enableOk = !Utils::isDlgItemTextEmpty(hDlg, IDC_EDIT_SAVINGS_GOAL);
    }
    EnableWindow(GetDlgItem(hDlg, IDOK), enableOk);
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
            Utils::populatePayeeCombo(hDlg, IDC_COMBO_CAT_PAYEE);
            ComboBox_SetText(hPayeeCombo, PTHIS->getDefaultTransactionPayee());


            // Default amount
            int iAmount = abs(PTHIS->getDefaultTransactionAmount());

            HWND hAmount = GetDlgItem(hDlg, IDC_EDIT_CAT_AMOUNT);
            SetWindowText(hAmount, Utils::penniesToDollarsString(iAmount).c_str());

            // Savings goal
            if (PTHIS->getSavingsGoal()) {
                SendDlgItemMessage(hDlg, IDC_CHECK_SAVINGS_GOAL, BM_SETCHECK, BST_CHECKED, 0);
                Utils::setDlgItemDollarText(hDlg, IDC_EDIT_SAVINGS_GOAL, PTHIS->getSavingsGoal());
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SAVINGS_GOAL), true);
            }
            else {
                SendDlgItemMessage(hDlg, IDC_CHECK_SAVINGS_GOAL, BM_SETCHECK, BST_UNCHECKED, 0);
                SetDlgItemText(hDlg, IDC_EDIT_SAVINGS_GOAL, L"");
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SAVINGS_GOAL), false);
            }
        }

        PTHIS->enableDisableUi(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == BN_CLICKED)
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

                    // Savings goal
                    bool isSavingsGoal = SendDlgItemMessage(hDlg, IDC_CHECK_SAVINGS_GOAL, BM_GETCHECK, 0, 0);
                    if (isSavingsGoal) {
                        PTHIS->setSavingsGoal(Utils::getDlgItemDollarText(hDlg, IDC_EDIT_SAVINGS_GOAL));
                    }
                    else {
                        PTHIS->setSavingsGoal(0);
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

EnvelopeDialog* EnvelopeDialog::PTHIS = nullptr;
