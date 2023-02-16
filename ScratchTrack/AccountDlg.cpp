#include "AccountDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfoDlg.h"
#include <windows.h>
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

AccountDlg::AccountDlg() : _budgeted(true), _amount(0), _creditCard(false)
{

}

AccountDlg::~AccountDlg()
{

}

bool AccountDlg::ShowDialog()
{
    PTHIS = this;
    g_accountInfoDlg.saveText();
    g_accountInfoDlg.appendText(L"About Accounts\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);
    g_accountInfoDlg.appendText(L"An account in ScratchTrack has a name and a balance. The balance can be positive, zero, or negative.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);
    g_accountInfoDlg.appendText(L"If an account is in-budget, the funds in that account can be assigned to categories.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);
    g_accountInfoDlg.appendText(L"When you create a credit card account, a matching 'credit card payment' expense "
    L"will be created. When you pay expenses with the credit card, "
    L"funds move from the original expense expense to the credit card expense expense.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);
    g_accountInfoDlg.appendText(L"When you pay a credit card, funds are withdrawn from the credit card expense.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);
    g_accountInfoDlg.appendText(L"Deposits, transfers, and credit card payments are entered through action icons in the account list.\n\n", RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);


    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ACCOUNT_DIALOG), g_hTopWindow, AccountDlgProc);
    PTHIS = nullptr;
    g_accountInfoDlg.restoreText();

    return result == IDOK;

}

bool AccountDlg::validateParams(HWND hDlg)
{
    bool ret = true;
    wchar_t szBuff[1024] = { 0 };
    bool isCC = SendDlgItemMessage(hDlg, IDC_CHECK_CREDIT_CARD, BM_GETCHECK, 0, 0);
    if (isCC) {
        // Warn if positive balance on CC
        GetWindowText(GetDlgItem(hDlg, IDC_EDIT_ACCOUNT_BALANCE), szBuff, 1024);
        int balance = Utils::dollarsStringToPennies(szBuff);
        if (balance > 0) {
            ret = MessageBox(hDlg, L"Balances on credit cards are usually zero or negative. The balance for this credit card is positive. Is that intended?", L"Confirm Balance", MB_YESNO | MB_ICONWARNING) == IDYES;
        }
    }
    return ret;
}

void AccountDlg::enableDisableUi(HWND hDlg)
{
    bool isCC = SendDlgItemMessage(hDlg, IDC_CHECK_CREDIT_CARD, BM_GETCHECK, 0, 0);
    if (isCC) {
        SendDlgItemMessage(hDlg, IDC_CHECK_BUDGETED, BM_SETCHECK, BST_UNCHECKED, 0);
    }
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_BUDGETED), !isCC);

    bool enableOk = true;
    if (Utils::isDlgItemTextEmpty(hDlg, IDC_EDIT_ACCOUNT_NAME)) {
        enableOk = false;
    }

    EnableWindow(GetDlgItem(hDlg, IDOK), enableOk);
}

INT_PTR CALLBACK AccountDlg::AccountDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (PTHIS)
        {
            // Populate name
            HWND hWndCtrl = GetDlgItem(hDlg, IDC_EDIT_ACCOUNT_NAME);
            SetWindowText(hWndCtrl, PTHIS->getName());

            // Populate balance
            hWndCtrl = GetDlgItem(hDlg, IDC_EDIT_ACCOUNT_BALANCE);
            std::wstring balance = Utils::penniesToDollarsString(PTHIS->getBalance());
            SetWindowText(hWndCtrl, balance.c_str());

            // Populate budgetd checkbox
            hWndCtrl = GetDlgItem(hDlg, IDC_CHECK_BUDGETED);
            Button_SetCheck(hWndCtrl, PTHIS->getBudgeted() ? BST_CHECKED : BST_UNCHECKED);

            // Populate credit card checkbox
            hWndCtrl = GetDlgItem(hDlg, IDC_CHECK_CREDIT_CARD);
            Button_SetCheck(hWndCtrl, PTHIS->getCreditCard() ? BST_CHECKED : BST_UNCHECKED);

            PTHIS->enableDisableUi(hDlg);

        }

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
            PTHIS->enableDisableUi(hDlg);
        }

        if (HIWORD(wParam) == BN_CLICKED){
            // Income or expense
            PTHIS->enableDisableUi(hDlg);
        }

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            bool closeIt = true;
            if (LOWORD(wParam) == IDOK)
            {
                closeIt = PTHIS->validateParams(hDlg);
                if (closeIt) {
                    wchar_t szBuff[1024] = { 0 };
                    // Save name
                    HWND hWndCtrl = GetDlgItem(hDlg, IDC_EDIT_ACCOUNT_NAME);
                    GetWindowText(hWndCtrl, szBuff, 1024);
                    PTHIS->setName(szBuff);

                    // Save balance
                    hWndCtrl = GetDlgItem(hDlg, IDC_EDIT_ACCOUNT_BALANCE);
                    GetWindowText(hWndCtrl, szBuff, 1024);
                    PTHIS->setBalance(Utils::dollarsStringToPennies(szBuff));

                    // Save budgetd checkbox
                    hWndCtrl = GetDlgItem(hDlg, IDC_CHECK_BUDGETED);
                    PTHIS->setBudgeted(Button_GetCheck(hWndCtrl) == BST_CHECKED);

                    // Save credit card checkbox
                    hWndCtrl = GetDlgItem(hDlg, IDC_CHECK_CREDIT_CARD);
                    PTHIS->setCreditCard(Button_GetCheck(hWndCtrl) == BST_CHECKED);
                }
            }
            if (closeIt) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
    }
    return (INT_PTR)FALSE;
}

AccountDlg* AccountDlg::PTHIS = nullptr;

