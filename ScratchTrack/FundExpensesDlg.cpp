#include "FundExpensesDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"

#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

bool FundExpensesDlg::ShowDialog()
{
    PTHIS = this;

    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Expense Funding\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    g_accountInfoDlg.appendText(L"In ScratchTrack funds are assigned to spending categories. When expenses are recorded in a expense, funds in that expense are reduced by the epxense amount. "
        L"When a expense is out of money, spending in that expense should stop until more funds are added.\n\n",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    g_accountInfoDlg.appendText(L"This dialog box allows you to assign money to all categories in a income. It's important to note that money stays in its account when assigned to categories. "
        L"When money is assigned, you'll have less budgetable dollars available to place in other categories, but no money actually moves. Think of categories as a way to view and organize "
        L"the money in your account(s).",
        RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FUND_GROUP_DIALOG), g_hTopWindow, FundExpensesDlgProc);
    PTHIS = nullptr;

    g_accountInfoDlg.restoreText();

    return result == IDOK;
}

void FundExpensesDlg::updateUi(HWND hDlg)
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
    int totalExpenseBalance = 0;
    for (size_t i = 0; i < groups.size(); i++) {
        AccountGroup* pGroup = dynamic_cast<AccountGroup*>(groups[i]);
        auto children = pGroup->getChildren();
        for (size_t x = 0; x < children.size(); x++)
        {
            totalExpenseBalance += children[x]->getBalance();
        }
    }

    // Get budget of this group
    int myBudget = _pGroup->budgetedToSubAccounts();

    // Total dollars available for categories
    int availabeDollars = totalBalance - totalExpenseBalance;

    // Cost to top off accounts
    int costToTopOff = 0;
    int costToFullyFund = 0;

    // Get balance of this group
    auto categories = _pGroup->getChildren();
    int myBalance = 0;
    for (size_t i = 0; i < categories.size(); i++) {
        myBalance += categories[i]->getBalance();
        costToFullyFund += -categories[i]->getBudgeted();
        costToTopOff += -categories[i]->getBudgeted() - categories[i]->getBalance();
    }

    // No whip up some sort of smart statement
    wchar_t szBuff[1024] = { 0 };

    swprintf(szBuff, 1024, L"Available dollars: %ls\nCost to top off: %ls\nCost to fully fund: %ls\n", 
        Utils::penniesToDollarsString(availabeDollars).c_str(), Utils::penniesToDollarsString(costToTopOff).c_str(), Utils::penniesToDollarsString(costToFullyFund).c_str());

    SetDlgItemText(hDlg, IDC_STATIC_NOTES, szBuff);
}

INT_PTR CALLBACK FundExpensesDlg::FundExpensesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (FundExpensesDlg::PTHIS) {
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

FundExpensesDlg* FundExpensesDlg::PTHIS = nullptr;
