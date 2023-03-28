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
extern void setGuiFont(HWND hCtrl);


TransactionImportDlg::TransactionImportDlg(Account* pAccount, std::vector<ImportedTransactionInfo>& transactions) : _transactions(transactions),
    _pAccount(pAccount)
{
    memset(&_maxImportDate, 0, sizeof(_maxImportDate));
}

TransactionImportDlg::~TransactionImportDlg()
{

}

bool TransactionImportDlg::haveUnmappedItems(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
    size_t entries = ListView_GetItemCount(hListView);
    for (size_t i = 0; i < entries; i++)
    {
        LVITEM item = { 0 };
        item.mask = LVIF_PARAM;
        item.iItem = (int)i;
        if (ListView_GetCheckState(hListView, i)) {
            ListView_GetItem(hListView, &item);
            ImportedTransactionInfo* pInfo = (ImportedTransactionInfo*)item.lParam;
            if (pInfo->_envelopeId == ACCOUNT_ID_INVALID) {
                return true;
            }
        }
    }
    return false;
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

    // Mapping stuff
    hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT_MAP);
    int selCount = ListView_GetSelectedCount(hListView);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EDIT_MAP_RULE), selCount == 1);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE_MAP_RULE), selCount);

    EnableWindow(GetDlgItem(hDlg, IDOK), !haveUnmappedItems(hDlg));
    

}

bool TransactionImportDlg::ShowDialog()
{
    PTHIS = this;

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_TRANSACTION_IMPORT), g_hTopWindow, TransactionImportDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

void TransactionImportDlg::updateImports(HWND hDlg)
{
    double lastImportDate = 0;
    HWND hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
    int iCount = ListView_GetItemCount(hListView);

    ImportedTransactionInfo* pInfo = nullptr;
    for (int i = 0; i < iCount; i++)
    {
        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        wchar_t szBuff[1024] = { 0 };

        //item.pszText = szBuff;
        item.iItem = i;
        ListView_GetItem(hListView, &item);
        pInfo = (ImportedTransactionInfo*)item.lParam;

        g_transactionImporter.bestGuessAtPayeeAndCat(pInfo->_payee.c_str(), pInfo->_payeeMapped, pInfo->_envelopeId);

        ListView_SetItemText(hListView, i, 1, const_cast<wchar_t*>(pInfo->_payee.c_str()));
        ListView_SetItemText(hListView, i, 2, const_cast<wchar_t*>(pInfo->_payeeMapped.c_str()));
        AccountBase* pEnvelope = AccountBase::lookupById(pInfo->_envelopeId);
        if (pEnvelope) {
            ListView_SetItemText(hListView, i, 3, const_cast<wchar_t*>(pEnvelope->getName()));
        }
        else {
            ListView_SetItemText(hListView, i, 3, const_cast<wchar_t*>(L""));
        }
    }

    for (size_t i = 0; i < 5; i++) {
        ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);
    }
    PTHIS->enableDisableUi(hDlg);
}

void TransactionImportDlg::populateImports(HWND hDlg)
{
    double lastImportDate = 0;
    HWND hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
    ListView_DeleteAllItems(hListView);

    SYSTEMTIME lastImportTime = { 0 };
    DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_DATETIMEPICKER_LAST_IMPORT), &lastImportTime);

    SystemTimeToVariantTime(&lastImportTime, &lastImportDate);
    
    bool showOnlyUnmapped = SendDlgItemMessage(hDlg, IDC_CHECK_SHOW_UNMAPPED_ONLY, BM_GETCHECK, 0, 0);

    for (size_t i = 0; i < PTHIS->_transactions.size(); i++) {
        ImportedTransactionInfo& info = PTHIS->_transactions[i];

        if (Utils::compareDates(info._date, PTHIS->_maxImportDate) == 1) {
            PTHIS->_maxImportDate = info._date;
        }

        // Figure out if this transaction predates our last import
        double transDate;
        SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&info._date), &transDate);

        bool oldTransaction = transDate <= lastImportDate;

        Transaction* pDup = g_transactionImporter.findDuplicate(PTHIS->_pAccount, info._payee.c_str(),
            info._amount, info._date, 5);

        g_transactionImporter.bestGuessAtPayeeAndCat(info._payee.c_str(), info._payeeMapped, info._envelopeId);
        AccountBase* pEnvelope = nullptr;
        if (info._envelopeId != ACCOUNT_ID_INVALID) {
            if (showOnlyUnmapped) {
                continue;
            }
            pEnvelope = AccountBase::lookupById(info._envelopeId);
        } 

        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;

        wchar_t szBuff[1024] = { 0 };
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &info._date, nullptr, szBuff, 1024, nullptr);

        item.pszText = szBuff;
        item.lParam = (LPARAM)&info;
        item.iItem = MAXINT;

        int iIndex = ListView_InsertItem(hListView, &item);
        ListView_SetItemText(hListView, iIndex, 1, const_cast<wchar_t*>(info._payee.c_str()));
        ListView_SetItemText(hListView, iIndex, 2, const_cast<wchar_t*>(info._payeeMapped.c_str()));

        if (pEnvelope) {
            ListView_SetItemText(hListView, iIndex, 3, const_cast<wchar_t*>(pEnvelope->getName()));
        }

        std::wstring amount = Utils::penniesToDollarsString(info._amount);
        ListView_SetItemText(hListView, iIndex, 4, const_cast<wchar_t*>(amount.c_str()));

        ListView_SetCheckState(hListView, iIndex, (pDup || oldTransaction) ? false : true);
    } // for each transaction

    for (size_t i = 0; i < 5; i++) {
        ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);
    }
    PTHIS->enableDisableUi(hDlg);
}

void TransactionImportDlg::populateRuleMapper(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT_MAP);
    ListView_DeleteAllItems(hListView);

    auto& map = g_transactionImporter.map();
    for (const auto& itr : map)
    {
        std::wstring nameIn = itr.first;
        const TransactionImporter::MAPPED_IMPORT_ITEM& mapInfo = itr.second;
        auto envelope = AccountBase::lookupById(mapInfo._envelopeId);
        std::wstring payee = mapInfo._payee;

        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;

        item.pszText = const_cast<wchar_t*>(nameIn.c_str());
        item.lParam = (LPARAM)&mapInfo;
        item.iItem = MAXINT;
        int iIndex = ListView_InsertItem(hListView, &item);

        ListView_SetItemText(hListView, iIndex, 1, const_cast<wchar_t*>(payee.c_str()));
        ListView_SetItemText(hListView, iIndex, 2, const_cast<wchar_t*>(envelope->getName()));

    }

    for (size_t i = 0; i < 3; i++) {
        ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);
    }

}

void TransactionImportDlg::initRuleMapper(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT_MAP);
    ListView_DeleteAllItems(hListView);
    ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    size_t columnCount = 0;
    LVCOLUMN column = { 0 };
    LPNMHDR pHeader = nullptr;

    setGuiFont(hListView);

    column.mask = LVCF_TEXT;

    column.pszText = const_cast<wchar_t*>(L"Payee match string");
    ListView_InsertColumn(hListView, columnCount++, &column);

    column.pszText = const_cast<wchar_t*>(L"ScratchTrack payee name");
    ListView_InsertColumn(hListView, columnCount++, &column);

    column.pszText = const_cast<wchar_t*>(L"Envelope");
    ListView_InsertColumn(hListView, columnCount++, &column);

    populateRuleMapper(hDlg);
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
        if (pHeader->idFrom == IDC_DATETIMEPICKER_LAST_IMPORT) {
            if (pHeader->code == DTN_DATETIMECHANGE) {
                PTHIS->populateImports(hDlg);
            } // date time change
        } // last import date picker
        else if (pHeader->idFrom == IDC_LIST_TRANS_IMPORT && pHeader->code == LVN_ITEMCHANGED)
        {
            PTHIS->enableDisableUi(hDlg);
        }
        else if (pHeader->idFrom == IDC_LIST_TRANS_IMPORT_MAP && pHeader->code == LVN_ITEMCHANGED)
        {
            PTHIS->enableDisableUi(hDlg);
        }
        else if (pHeader->idFrom == IDC_LIST_TRANS_IMPORT  && pHeader->code == NM_DBLCLK)
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
                    // TODO: Just update and add
                    PTHIS->populateRuleMapper(hDlg);
                    PTHIS->populateImports(hDlg);

                    //hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
                    //
                    //ListView_SetItemText(hListView, hi.iItem, 2, const_cast<wchar_t*>(pInfo->_payeeMapped.c_str()));

                    //auto pEnvelope = AccountBase::lookupById(pInfo->_envelopeId);
                    //if (pEnvelope) {
                    //    ListView_SetItemText(hListView, hi.iItem, 3, const_cast<wchar_t*>(pEnvelope->getName()));
                    //}
                }
            }
        }
        break;
    case WM_INITDIALOG:
        PTHIS->initRuleMapper(hDlg);
        hListView = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT);
        setGuiFont(hListView);

        SYSTEMTIME lastImportTime = PTHIS->_pAccount->getLastImportDate();
        DateTime_SetSystemtime(GetDlgItem(hDlg, IDC_DATETIMEPICKER_LAST_IMPORT), GDT_VALID, &lastImportTime);

        ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES);

        column.mask = LVCF_TEXT;

        column.pszText = const_cast<wchar_t*>(L"Date");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Payee (incoming)");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Payee (mapped)");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Envelope");
        ListView_InsertColumn(hListView, columnCount++, &column);

        column.pszText = const_cast<wchar_t*>(L"Amount");
        ListView_InsertColumn(hListView, columnCount++, &column);

        PTHIS->populateImports(hDlg);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_ADD_MAP_RULE && HIWORD(wParam) == BN_CLICKED) {
            PTHIS->addMapRule(hDlg);
        }
        else if (LOWORD(wParam) == IDC_BUTTON_EDIT_MAP_RULE && HIWORD(wParam) == BN_CLICKED) {
            PTHIS->editMapRule(hDlg);
        }
        else if (LOWORD(wParam) == IDC_BUTTON_DELETE_MAP_RULE && HIWORD(wParam) == BN_CLICKED) {
            PTHIS->deleteMapRule(hDlg);
        }
        else if (LOWORD(wParam) == IDC_CHECK_SHOW_UNMAPPED_ONLY && HIWORD(wParam) == BN_CLICKED) {
            PTHIS->populateImports(hDlg);
        }
        else if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
        {
            PTHIS->enableDisableUi(hDlg);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
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

void TransactionImportDlg::editMapRule(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT_MAP);
    int iSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (iSel >= 0) {
        wchar_t szBuff[1024] = { 0 };
        LVITEM item = { 0 };
        item.iItem = iSel;
        item.pszText = szBuff;
        item.cchTextMax = 1024;
        item.mask = LVIF_PARAM | LVIF_TEXT;

        ListView_GetItem(hList, &item);
        const TransactionImporter::MAPPED_IMPORT_ITEM* pInfo = (TransactionImporter::MAPPED_IMPORT_ITEM*)item.lParam;
        ImportedTransactionInfo it;
        it._payee = item.pszText;
        it._envelopeId = pInfo->_envelopeId;
        ImportMapDlg dlg(&it);
        if (dlg.ShowDialog(hDlg)) {
            auto& map = g_transactionImporter.map();
            auto itr = map.find(it._payeeMatchString);
            pInfo = (itr == map.end()) ? nullptr : &itr->second;
            
            ListView_SetItemText(hList, iSel, 0, const_cast<wchar_t*>(it._payeeMatchString.c_str()));
            ListView_SetItemText(hList, iSel, 1, const_cast<wchar_t*>(it._payeeMapped.c_str()));
            auto acct = AccountBase::lookupById(it._envelopeId);
            ListView_SetItemText(hList, iSel, 2, const_cast<wchar_t*>(acct->getName()));

            item.mask = LVIF_PARAM;
            item.lParam = (LPARAM)pInfo;
            ListView_SetItem(hList, &item);
            //populateImports(hDlg);
            updateImports(hDlg);
        }
    }
}

void TransactionImportDlg::addMapRule(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT_MAP);

    ImportedTransactionInfo it;
    ImportMapDlg dlg(&it);
    if (dlg.ShowDialog(hDlg)) {
        auto& map = g_transactionImporter.map();
        g_transactionImporter.addMapping(it._payeeMatchString.c_str(), it._payeeMapped, it._envelopeId);

        // Insert
        populateRuleMapper(hDlg);
        //populateImports(hDlg);
        updateImports(hDlg);
    }
}

void TransactionImportDlg::deleteMapRule(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_TRANS_IMPORT_MAP);
    if (IDYES == MessageBox(hDlg, L"Are you sure you want to delete the selected mapping rules?", L"Confirm delete", MB_YESNO | MB_ICONWARNING))
    {
        int iSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
        while (iSel >= 0) {
            wchar_t szBuff[1024] = { 0 };
            LVITEM item = { 0 };
            item.iItem = iSel;
            item.pszText = szBuff;
            item.cchTextMax = 1024;
            item.mask = LVIF_PARAM | LVIF_TEXT;
            ListView_GetItem(hList, &item);

            item.pszText;
            g_transactionImporter.deleteMapping(item.pszText);

            ListView_DeleteItem(hList, iSel);
            iSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
        }
        populateRuleMapper(hDlg);
        populateImports(hDlg);
    }

}


TransactionImportDlg* TransactionImportDlg::PTHIS = nullptr;
