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
    bool enable = true;
    std::wstring warning;
    std::wstring matchString = Utils::getDlgItemText(hDlg, IDC_EDIT_MATCH_STRING);
    if (matchString.empty()) {
        enable = false;
    }

    std::wstring origPayee = Utils::getDlgItemText(hDlg, IDC_EDIT_ORIG_PAYEE);

    // Get envelope
    HWND hCatCombo = GetDlgItem(hDlg, IDC_COMBO_ENVELOPE);
    int iSel = ComboBox_GetCurSel(hCatCombo);
    if (iSel < 0) {
        enable = false;
    }

    // New payee
    std::wstring newPayee = Utils::getDlgItemText(hDlg, IDC_COMBO_NEW_PAYEE);
    if (newPayee.empty()) {
        enable = false;
    }

    if (!origPayee.empty()) {
        bool isGlobMatch = Utils::isGlobMatch(origPayee.c_str(), matchString.c_str());
        enable = enable && isGlobMatch;
        if (!isGlobMatch) {
            warning = L"ERROR! The pattern DOES NOT match the original payee [";
            warning += origPayee;
            warning += L"].";
        }
    }

    std::wstring testText = Utils::getDlgItemText(hDlg, IDC_EDIT_MATCH_TEST);
    if (!testText.empty()) {
        if (!Utils::isGlobMatch(testText.c_str(), matchString.c_str()))
        {
            if (!warning.empty()) {
                warning = L"\n";
            }
            warning += L"Warning! The pattern DOES NOT match the test payee [";
            warning += testText;
            warning += L"]";
        }
    }

    if (warning.empty()) {
        warning = L"The match string matches the current test text";
    }
    
    SetDlgItemText(hDlg, IDC_STATIC_TEST_RESULTS, warning.c_str());

    EnableWindow(GetDlgItem(hDlg, IDOK), enable);

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
        unsigned int envelopeId;
        g_transactionImporter.bestGuessAtPayeeAndCat(PTHIS->_pTransactionInfo->_payee.c_str(), payee, envelopeId);

        HWND hCatCombo = nullptr;
        std::vector< AccountBase*> cats;

        std::wstring temp(PTHIS->_pTransactionInfo->_payee);
        g_transactionImporter.sanitizePayee(temp);
        if (PTHIS->_pTransactionInfo->_payee == temp) {
            SetDlgItemText(hDlg, IDC_EDIT_ORIG_PAYEE, PTHIS->_pTransactionInfo->_payee.c_str());
        }
        else {
            SetDlgItemText(hDlg, IDC_EDIT_MATCH_TEST, temp.c_str());
        }
        SetDlgItemText(hDlg, IDC_EDIT_MATCH_STRING, PTHIS->_pTransactionInfo->_payee.c_str());
        
        Utils::populatePayeeCombo(hDlg, IDC_COMBO_NEW_PAYEE, payee.c_str());

        cats = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
        hCatCombo = GetDlgItem(hDlg, IDC_COMBO_ENVELOPE);
        for (size_t i = 0; i < cats.size(); i++) {
            int pos = (int)SendMessage(hCatCombo, CB_ADDSTRING, 0, (LPARAM)cats[i]->getName());
            ComboBox_SetItemData(hCatCombo, pos, cats[i]);
            if (cats[i]->getId() == envelopeId) {
                ComboBox_SetCurSel(hCatCombo, pos);
            }
        }

        PTHIS->enableDisableUi(hDlg);
    }
            
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == EN_CHANGE )
        {
            PTHIS->enableDisableUi(hDlg);
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                ImportedTransactionInfo* pInfo = PTHIS->_pTransactionInfo;
                g_transactionImporter.deleteMapping(pInfo->_payeeMatchString.c_str());
                g_transactionImporter.deleteMapping(pInfo->_payee.c_str());

                // Get payee
                wchar_t szBuff[1024] = { 0 };
                GetDlgItemText(hDlg, IDC_COMBO_NEW_PAYEE, szBuff, 1024);

                pInfo->_payeeMapped = szBuff;
                pInfo->_payeeMatchString = Utils::getDlgItemText(hDlg, IDC_EDIT_MATCH_STRING);
                pInfo->_payee = Utils::getDlgItemText(hDlg, IDC_EDIT_ORIG_PAYEE);
                
                // Get envelope
                HWND hCatCombo = GetDlgItem(hDlg, IDC_COMBO_ENVELOPE);
                int iSel = ComboBox_GetCurSel(hCatCombo);
                if (iSel >= 0) {
                    AccountBase* pAccount = (AccountBase*)ComboBox_GetItemData(hCatCombo, iSel);
                    if (pAccount) {
                        pInfo->_envelopeId = pAccount->getId();
                    }
                }

                if (pInfo->_payeeMatchString.empty()) {
                    pInfo->_payeeMatchString = pInfo->_payee;
                }
                g_transactionImporter.addMapping(pInfo->_payeeMatchString.c_str(), pInfo->_payeeMapped, pInfo->_envelopeId);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

ImportMapDlg* ImportMapDlg::PTHIS = nullptr;
