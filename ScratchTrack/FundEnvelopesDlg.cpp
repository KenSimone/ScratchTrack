#include "FundEnvelopesDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

bool FundEnvelopesDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Envelope Funding\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"ScratchTrack follows the 'Envelope Budgeting' philosophy. The idea is that money is assigned to separate envelopes for different spending categories. Expenses are paid from the envelopes. "
        L"When an envelope is empty, spending in that category stops until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"This dialog box allows you to assign money to all envelopes in a folder. It's important to note that money stays in its account when assigned to envelopes. "
        L"When money is assigned, you'll have less budgetable dollars available to place in other envelopes, but no money actually moves. Think of envelopes as a way to organize "
        L"the money in your account(s).",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FUND_GROUP_DIALOG), g_hTopWindow, FundEnvelopesDlgProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

void FundEnvelopesDlg::updateUi(HWND hDlg)
{
    // Get all available dollars
    auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
    int totalBalance = 0;
    for (size_t i = 0; i < accounts.size(); i++)
    {
        Account* pAccount = dynamic_cast<Account*>(accounts[i]);
        if (pAccount->isCCAccount()) {
            continue;
        }

        if (!pAccount->getContributesToBudget()) {
            continue;
        }
        totalBalance += pAccount->getBalance();
    }

    // Get budgeted dollars
    auto groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
    int totalEnvelopeBalance = 0;
    for (size_t i = 0; i < groups.size(); i++) {
        AccountGroup* pGroup = dynamic_cast<AccountGroup*>(groups[i]);
        auto children = pGroup->getChildren();
        for (size_t x = 0; x < children.size(); x++)
        {
            totalEnvelopeBalance += children[x]->getBalance();
        }
    }

    // Get budget of this group
    int myBudget = _pGroup->budgetedToSubAccounts();

    // Total dollars available for envelopes
    int availabeDollars = totalBalance - totalEnvelopeBalance;

    // Cost to top off accounts
    int costToTopOff = 0;
    int costToFullyFund = 0;

    // Get balance of this group
    auto envelopes = _pGroup->getChildren();
    int myBalance = 0;
    for (size_t i = 0; i < envelopes.size(); i++) {
        myBalance += envelopes[i]->getBalance();
        costToFullyFund += -envelopes[i]->getBudgeted();
        costToTopOff += -envelopes[i]->getBudgeted() - envelopes[i]->getBalance();
    }

    // No whip up some sort of smart statement
    wchar_t szBuff[1024] = { 0 };

    swprintf(szBuff, 1024, L"Available dollars: %ls\nCost to top off: %ls\nCost to fully fund: %ls\n", 
        Utils::penniesToDollarsString(availabeDollars).c_str(), Utils::penniesToDollarsString(costToTopOff).c_str(), Utils::penniesToDollarsString(costToFullyFund).c_str());

    SetDlgItemText(hDlg, IDC_STATIC_NOTES, szBuff);
}

INT_PTR CALLBACK FundEnvelopesDlg::FundEnvelopesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (FundEnvelopesDlg::PTHIS) {
            SendDlgItemMessage(hDlg, IDC_RADIO_ADD_TO_EXISTING_BALANCE, BM_SETCHECK, BST_CHECKED, 0);
            PTHIS->updateUi(hDlg);
        }

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                PTHIS->_resetBalance = !SendDlgItemMessage(hDlg, IDC_RADIO_ADD_TO_EXISTING_BALANCE, BM_GETCHECK, 0, 0);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

FundEnvelopesDlg* FundEnvelopesDlg::PTHIS = nullptr;
