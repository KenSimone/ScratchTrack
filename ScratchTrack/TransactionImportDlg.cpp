#include "TransactionImportDlg.h"
#include "ImportMapDlg.h"
#include "TransactionImporter.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"
#include <Windows.h>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;
extern TransactionImporter g_transactionImporter;

TransactionImportDlg::TransactionImportDlg(Account* pAccount, std::vector<ImportedTransactionInfo>& transactions) : _transactions(transactions),
    _pAccount(pAccount)
{
}

TransactionImportDlg::~TransactionImportDlg()
{

}

void TransactionImportDlg::enableDisableUi(HWND hDlg)
{
}

bool TransactionImportDlg::ShowDialog()
{
    PTHIS = this;

    //g_accountInfoDlg.saveText();
    //g_accountInfoDlg.appendText(L"About Deposits\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

    //g_accountInfoDlg.appendText(L"Money can be deposited via the deposit action. A deposit can be edited afer it's created through the transaction list.",
    //    RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_TRANSACTION_IMPORT), g_hTopWindow, TransactionImportDlgProc);
    PTHIS = nullptr;
    //g_accountInfoDlg.restoreText();

    return result == IDOK;
}

INT_PTR CALLBACK TransactionImportDlg::TransactionImportDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    //IDC_COMBO_PAYEE

    UNREFERENCED_PARAMETER(lParam);
    HWND hListView = nullptr;
    size_t columnCount = 0;
    LVCOLUMN column = { 0 };
    LPNMHDR pHeader = nullptr;;

    switch (message)
    {
    case WM_NOTIFY:
        pHeader = (LPNMHDR)lParam;
        if (pHeader->idFrom == IDC_LIST_TRANS_IMPORT  && pHeader->code == NM_DBLCLK)
        {
            hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
            LVHITTESTINFO hi = { 0 };

            GetCursorPos(&hi.pt);
            ScreenToClient(hListView, &hi.pt);
            ListView_HitTest(hListView, &hi);
            if (hi.iItem == -1) {
                ListView_SubItemHitTest(hListView, &hi);
            }

            if (hi.iItem >= 0)
            {
                LVITEM item = { 0 };
                item.iItem = hi.iItem;
                item.mask = LVIF_PARAM;

                ListView_GetItem(hListView, &item);
                ImportedTransactionInfo* pInfo = (ImportedTransactionInfo*)item.lParam;

                ImportMapDlg dlg(pInfo);
                dlg.ShowDialog(hDlg);
            }
        }
        break;
    case WM_INITDIALOG:
        hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
        ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

        column.mask = LVCF_TEXT;

        column.pszText = const_cast<wchar_t*>(L"Date");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Payee (incoming)");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Payee (mapped)");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Category");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Amount");
        ListView_InsertColumn(hListView, columnCount++, &column);

        for (size_t i = 0; i < PTHIS->_transactions.size(); i++) {
            ImportedTransactionInfo& info = PTHIS->_transactions[i];

            std::wstring payeeMapped, catMapped;
            g_transactionImporter.bestGuessAtPayeeAndCat(info._payee.c_str(), payeeMapped, catMapped);
            
            LVITEM item = { 0 };
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.pszText = const_cast<wchar_t*>(info._date.c_str());
            item.lParam = (LPARAM) &info;
            item.iItem = MAXINT;

            int iIndex = ListView_InsertItem(hListView, &item);
            ListView_SetItemText(hListView, iIndex, 2, const_cast<wchar_t*>(info._payee.c_str()));
            ListView_SetItemText(hListView, iIndex, 1, const_cast<wchar_t*>(payeeMapped.c_str()));


            ListView_SetItemText(hListView, iIndex, 3, const_cast<wchar_t*>(catMapped.c_str()));
            std::wstring amount = Utils::penniesToDollarsString(info._amount);
            ListView_SetItemText(hListView, iIndex, 4, const_cast<wchar_t*>(amount.c_str()));
        }

        for (size_t i = 0; i < 5; i++) {
            ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);
        }

        // Accounts
        //Utils::populateAccountCombo(hDlg, IDC_COMBO_ACCOUNT, PTHIS->_pAccount->getId(), true);
        //SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, PTHIS->_description.c_str());
        //Utils::setDlgItemDollarText(hDlg, IDC_EDIT_DEPOSIT_AMOUNT, PTHIS->_amount);
        //Utils::populatePayeeCombo(hDlg, IDC_COMBO_PAYEE, PTHIS->_payee.c_str());
        //PTHIS->enableDisableUi(hDlg);

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
                //PTHIS->_amount = Utils::getDlgItemDollarText(hDlg, IDC_EDIT_DEPOSIT_AMOUNT);
                //PTHIS->_description = Utils::getDlgItemText(hDlg, IDC_EDIT_DESCRIPTION);
                //PTHIS->_payee = Utils::getDlgItemText(hDlg, IDC_COMBO_PAYEE);

                //int id = Utils::getAccountIdFromAccountCombo(hDlg, IDC_COMBO_ACCOUNT);
                //PTHIS->_pAccount = (Account*)AccountBase::lookupById(id);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


TransactionImportDlg* TransactionImportDlg::PTHIS = nullptr;
