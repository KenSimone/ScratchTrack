#include "ImportMapDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"
#include "TransactionImporter.h"
#include <Windows.h>
#include <windowsx.h>

extern HINSTANCE g_hInst;
extern AccountInfoDlg g_accountInfoDlg;
extern TransactionImporter g_transactionImporter;


ImportMapDlg::ImportMapDlg(ImportedTransactionInfo* pInfo) : _pTransactionInfo(pInfo)
{
}

ImportMapDlg::~ImportMapDlg()
{

}

void ImportMapDlg::enableDisableUi(HWND hDlg)
{
}

bool ImportMapDlg::ShowDialog(HWND hParent)
{
    PTHIS = this;

    //g_accountInfoDlg.saveText();
    //g_accountInfoDlg.appendText(L"About Deposits\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    //g_accountInfoDlg.appendText(L"Money can be deposited via the deposit action. A deposit can be edited afer it's created through the transaction list.",
    //    RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MAP_IMPORT), hParent, ImportMapDlgProc);
    PTHIS = nullptr;
    //g_accountInfoDlg.restoreText();

    return result == IDOK;
}

INT_PTR CALLBACK ImportMapDlg::ImportMapDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    //IDC_COMBO_PAYEE

    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        std::wstring payee;
        unsigned int categoryId;
        g_transactionImporter.bestGuessAtPayeeAndCat(PTHIS->_pTransactionInfo->_payee.c_str(), payee, categoryId);

        HWND hCatCombo = nullptr;
        std::vector< AccountBase*> cats;

        SetDlgItemText(hDlg, IDC_EDIT_ORIG_PAYEE, PTHIS->_pTransactionInfo->_payee.c_str());
        Utils::populatePayeeCombo(hDlg, IDC_COMBO_NEW_PAYEE, payee.c_str());

        cats = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
        hCatCombo = GetDlgItem(hDlg, IDC_COMBO_CATEGORY);
        for (size_t i = 0; i < cats.size(); i++) {
            int pos = (int)SendMessage(hCatCombo, CB_ADDSTRING, 0, (LPARAM)cats[i]->getName());
            ComboBox_SetItemData(hCatCombo, pos, cats[i]);
            if (cats[i]->getId() == categoryId) {
                ComboBox_SetCurSel(hCatCombo, pos);
            }

        }
    }
            
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
        {
            PTHIS->enableDisableUi(hDlg);
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                // Get payee
                wchar_t szBuff[1024] = { 0 };
                GetDlgItemText(hDlg, IDC_COMBO_NEW_PAYEE, szBuff, 1024);
                PTHIS->_pTransactionInfo->_payeeMapped = szBuff;
                
                // Get category
                HWND hCatCombo = GetDlgItem(hDlg, IDC_COMBO_CATEGORY);
                int iSel = ComboBox_GetCurSel(hCatCombo);
                if (iSel >= 0) {
                    AccountBase* pAccount = (AccountBase*)ComboBox_GetItemData(hCatCombo, iSel);
                    if (pAccount) {
                        PTHIS->_pTransactionInfo->_categoryId = pAccount->getId();
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


ImportMapDlg* ImportMapDlg::PTHIS = nullptr;
