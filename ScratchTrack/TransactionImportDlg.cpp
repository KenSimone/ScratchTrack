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
    HWND hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
    unsigned int numItems =  ListView_GetItemCount(hListView);

    int amountImported = 0;
    for (unsigned int i = 0; i < numItems; i++)
    {
        LVITEM item = { 0 };
        item.mask = LVIF_PARAM;
        item.iItem = i;
        if (ListView_GetCheckState(hListView, i)) {
            ListView_GetItem(hListView, &item);
            ImportedTransactionInfo* pInfo = (ImportedTransactionInfo*)item.lParam;
            amountImported += pInfo->_amount;
        }
    }
    wchar_t szBuff[1024] = { 0 };
    std::wstring newBal = Utils::penniesToDollarsString(amountImported + _pAccount->getBalance());
    swprintf(szBuff, 1024, L"Account %ls will have a balance of %ls after importing selected transactions\n",
        _pAccount->getName(), newBal.c_str());

    SetDlgItemText(hDlg, IDC_STATIC_BALANCE, szBuff);
}

bool TransactionImportDlg::ShowDialog()
{
    PTHIS = this;

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_TRANSACTION_IMPORT), g_hTopWindow, TransactionImportDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

INT_PTR CALLBACK TransactionImportDlg::TransactionImportDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hListView = nullptr;
    size_t columnCount = 0;
    LVCOLUMN column = { 0 };
    LPNMHDR pHeader = nullptr;;

    switch (message)
    {
    case WM_NOTIFY:
        pHeader = (LPNMHDR)lParam;
        if (pHeader->idFrom == IDC_LIST_TRANS_IMPORT && pHeader->code == LVN_ITEMCHANGED)
        {
            PTHIS->enableDisableUi(hDlg);
        }
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
                if (dlg.ShowDialog(hDlg))
                {
                    hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
                    
                    ListView_SetItemText(hListView, hi.iItem, 2, const_cast<wchar_t*>(pInfo->_payeeMapped.c_str()));

                    auto pCategory = AccountBase::lookupById(pInfo->_categoryId);
                    if (pCategory) {
                        ListView_SetItemText(hListView, hi.iItem, 3, const_cast<wchar_t*>(pCategory->getName()));
                    }
                }
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

            Transaction* pDup = g_transactionImporter.findDuplicate(PTHIS->_pAccount, info._payee.c_str(),
                info._amount, info._date, 5);

            g_transactionImporter.bestGuessAtPayeeAndCat(info._payee.c_str(), info._payeeMapped, info._categoryId);
            AccountBase* pCategory = nullptr;
            if (info._categoryId != ACCOUNT_ID_INVALID) {
                pCategory = AccountBase::lookupById(info._categoryId);
            }
            
            LVITEM item = { 0 };
            item.mask = LVIF_TEXT | LVIF_PARAM;

            wchar_t szBuff[1024] = { 0 };
            GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &info._date, nullptr, szBuff, 1024, nullptr);

            item.pszText = szBuff;
            item.lParam = (LPARAM) &info;
            item.iItem = MAXINT;

            int iIndex = ListView_InsertItem(hListView, &item);
            ListView_SetItemText(hListView, iIndex, 1, const_cast<wchar_t*>(info._payee.c_str()));
            ListView_SetItemText(hListView, iIndex, 2, const_cast<wchar_t*>(info._payeeMapped.c_str()));

            if (pCategory) {
                ListView_SetItemText(hListView, iIndex, 3, const_cast<wchar_t*>(pCategory->getName()));
            }

            std::wstring amount = Utils::penniesToDollarsString(info._amount);
            ListView_SetItemText(hListView, iIndex, 4, const_cast<wchar_t*>(amount.c_str()));

            ListView_SetCheckState(hListView, iIndex, pDup ? false : true);
        } // for each transaction

        for (size_t i = 0; i < 5; i++) {
            ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);
        }
        PTHIS->enableDisableUi(hDlg);

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
                hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
                size_t entries = ListView_GetItemCount(hListView);
                for (size_t i = 0; i < entries; i++)
                {
                    LVITEM item = { 0 };
                    item.mask = LVIF_PARAM;
                    item.iItem = (int)i;
                    if (ListView_GetCheckState(hListView, i)) {
                        ListView_GetItem(hListView, &item);
                        ImportedTransactionInfo* pInfo = (ImportedTransactionInfo*)item.lParam;
                        g_transactionImporter.addMapping(pInfo->_payee.c_str(), pInfo->_payeeMapped, pInfo->_categoryId);
                        PTHIS->_transactionsOut.push_back(*pInfo);
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


TransactionImportDlg* TransactionImportDlg::PTHIS = nullptr;
