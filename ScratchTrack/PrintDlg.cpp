#include "PrintDlg.h"
#include "resource.h"
#include "Utils.h"
#include "AccountInfo.h"

#include <Windows.h>
#include <richedit.h>
#include <vector>
#include <string>

extern HINSTANCE g_hInst;
extern HWND g_hTopWindow;

ScratchTrackPrintDlg::~ScratchTrackPrintDlg()
{

}

std::wstring getRowText(std::vector<std::wstring> row, int colSpace, int numCols)
{
    int charsUsed = 0;
    std::wstring ret;

    for (size_t i = 0; i < row.size(); i++)
    {
        std::wstring curItem = row[i];
        if (curItem.length() > colSpace) {
            curItem = curItem.substr(0, colSpace-1);
        }
        size_t spacesRequired = colSpace - curItem.length();

        ret += curItem;
        for (size_t j = 0; j < spacesRequired; j++) {
            ret += L" ";
        }
    }

    int totalSize = colSpace * numCols;
    while (ret.length() < totalSize) {
        ret += L" ";
    }
    ret += L"\n";
    return ret;
}

void ScratchTrackPrintDlg::populateReport(HWND hDlg)
{
    HWND hText = GetDlgItem(hDlg, IDC_RICHEDIT_PRINT);

    static const int BLACK = RGB(0, 0, 0);
    static const int WHITE = RGB(255, 255, 255);
    static const int LIGHT_GRAY = RGB(240, 240, 240);
    appendText(hText, L"ScratchTrack Budget\n\n", BLACK, true, false, 14, WHITE);

    std::vector<std::wstring> curRow;
    curRow.push_back(L"Folder/Envelope");
    curRow.push_back(L"Budgeted In");
    curRow.push_back(L"Budgeted Out");

    static const int COL_SPACE = 26;
    static const int NUM_COLS = 3;

    std::wstring rowText = getRowText(curRow, COL_SPACE, NUM_COLS);

    appendText(hText, rowText.c_str(), BLACK, true, true, 12, WHITE);
    wchar_t szBuff[1024] = { 0 };

    auto groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
    
    for (auto itr = groups.begin(); itr != groups.end(); itr++)
    {
        AccountGroup* pGroup = dynamic_cast<AccountGroup*>(*itr);
        if (!pGroup || pGroup->getHidden() ) {
            continue;
        }

        if (pGroup->getChildren().size() == 1 && pGroup->getChildren()[0]->getId() == ACCOUNT_ID_PAY_CC) {
            continue;
        }

        curRow.clear();
        curRow.push_back(pGroup->getName());
        curRow.push_back(Utils::penniesToDollarsString(abs(pGroup->getExpenseLimit())));
        curRow.push_back(Utils::penniesToDollarsString(abs(pGroup->budgetedToSubAccounts())));
        rowText = getRowText(curRow, COL_SPACE, NUM_COLS);

        appendText(hText, rowText.c_str(), BLACK, true, false, 12, WHITE);

        auto children = pGroup->getChildren();
        int i = 0;
        for (auto env = children.begin(); env != children.end(); env++)
        {
            AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(*env);
            if (pEnvelope && !pEnvelope->getHidden()) {
                curRow.clear();
                curRow.push_back(pEnvelope->getName());
                curRow.push_back(L"");
                curRow.push_back(Utils::penniesToDollarsString(abs(pEnvelope->getBudgeted())));
                rowText = getRowText(curRow, COL_SPACE, NUM_COLS);
                appendText(hText, rowText.c_str(), BLACK, false, false, 12, !(i % 2) ? LIGHT_GRAY : WHITE);
            }
            i++;
        }
        appendText(hText, L"\n", BLACK, true, false, 12, WHITE);
    }


}

void ScratchTrackPrintDlg::appendText(HWND hTextControl, const wchar_t* szText, COLORREF cr, bool bold, bool underline, int sizeInPoints, COLORREF crBackColor)
{
    // EM_SETCHARFORMAT
    CHARFORMAT cf = { 0 };
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE | CFM_UNDERLINE | CFM_COLOR | CFM_FACE;
    wcscpy_s(cf.szFaceName, L"consolas");

    if (bold) {
        cf.dwEffects = CFE_BOLD;
    }

    if (underline) {
        cf.dwEffects |= CFE_UNDERLINE;
    }


    int screenWidth = GetSystemMetrics(SM_CXSCREEN);

    static const LONG FONT_POINT_SCALE = 20;

    cf.yHeight = sizeInPoints * FONT_POINT_SCALE;

    cf.crTextColor = cr;
    

    // Move selection to the end
    SendMessage(hTextControl, EM_SETSEL, ACCOUNT_ID_INVALID, ACCOUNT_ID_INVALID);

    SendMessageW(hTextControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    CHARFORMAT2 cf2 = { 0 };
    cf2.cbSize = sizeof(cf2);
    cf2.crBackColor = crBackColor;
    cf2.dwMask = CFM_BACKCOLOR;
    SendMessageW(hTextControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

    SendMessage(hTextControl, EM_REPLACESEL, 0, (LPARAM)szText);
}

BOOL ScratchTrackPrintDlg::PrintRTF(HWND hwnd, HDC hdc)
{
    DOCINFO di = { sizeof(di) };

    if (!StartDoc(hdc, &di))
    {
        return FALSE;
    }

    int cxPhysOffset = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    int cyPhysOffset = GetDeviceCaps(hdc, PHYSICALOFFSETY);

    int cxPhys = GetDeviceCaps(hdc, PHYSICALWIDTH);
    int cyPhys = GetDeviceCaps(hdc, PHYSICALHEIGHT);

    // Create "print preview". 
    SendMessage(hwnd, EM_SETTARGETDEVICE, (WPARAM)hdc, cxPhys /* /2*/);

    FORMATRANGE fr;

    fr.hdc = hdc;
    fr.hdcTarget = hdc;

    // Set page rect to physical page size in twips.
    fr.rcPage.top = 0;
    fr.rcPage.left = 0;
    fr.rcPage.right = MulDiv(cxPhys, 1440, GetDeviceCaps(hdc, LOGPIXELSX));
    fr.rcPage.bottom = MulDiv(cyPhys, 1440, GetDeviceCaps(hdc, LOGPIXELSY));

    // Set the rendering rectangle to the pintable area of the page.
    cxPhysOffset = MulDiv(cxPhysOffset, 1440, GetDeviceCaps(hdc, LOGPIXELSX));
    cyPhysOffset = MulDiv(cyPhysOffset, 1440, GetDeviceCaps(hdc, LOGPIXELSY));

    fr.rc.left = cxPhysOffset;
    fr.rc.right = fr.rcPage.right - cxPhysOffset;
    fr.rc.top = cyPhysOffset;
    fr.rc.bottom = fr.rcPage.bottom - cyPhysOffset;

    SendMessage(hwnd, EM_SETSEL, 0, (LPARAM)-1);          // Select the entire contents.
    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&fr.chrg);  // Get the selection into a CHARRANGE.

    BOOL fSuccess = TRUE;

    // Use GDI to print successive pages.
    while (fr.chrg.cpMin < fr.chrg.cpMax&& fSuccess)
    {
        fSuccess = StartPage(hdc) > 0;

        if (!fSuccess) break;

        int cpMin = (int)SendMessage(hwnd, EM_FORMATRANGE, TRUE, (LPARAM)&fr);

        if (cpMin <= fr.chrg.cpMin)
        {
            fSuccess = FALSE;
            break;
        }

        fr.chrg.cpMin = cpMin;
        fSuccess = EndPage(hdc) > 0;
    }

    SendMessage(hwnd, EM_FORMATRANGE, FALSE, 0);

    if (fSuccess)
    {
        EndDoc(hdc);
    }

    else

    {
        AbortDoc(hdc);
    }

    return fSuccess;

}
bool ScratchTrackPrintDlg::ShowDialog()
{
    PTHIS = this;

    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_PRINT), g_hTopWindow, ScratchTrackPrintDlgProc);
    PTHIS = nullptr;

    return result == IDOK;
}

INT_PTR CALLBACK ScratchTrackPrintDlg::ScratchTrackPrintDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (PTHIS) {
            PTHIS->populateReport(hDlg);
        }

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                PRINTDLG pd = { 0 };
                pd.lStructSize = sizeof(pd);
                pd.Flags = PD_RETURNDC ;
                pd.hwndOwner = hDlg;
                if (PrintDlg(&pd))
                {
                    PTHIS->PrintRTF(GetDlgItem(hDlg, IDC_RICHEDIT_PRINT), pd.hDC);
                }

            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

ScratchTrackPrintDlg* ScratchTrackPrintDlg::PTHIS = nullptr;
