#include "GdiChart.h"
#include "Utils.h"

void GdiBarChart::addItem(const wchar_t* szName, int value, void* pUserData)
{
    GdiBarItem item;
    item._value = value;
    item._pUserData = pUserData;
    item._name = szName;

    _items.push_back(item);
}

void GdiBarChart::render(HWND hWnd, HDC hdc)
{
    RECT r = { 0 };
    GetClientRect(hWnd, &r);

    Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    if (_items.empty()) {
        return;
    }

    // Figure out space per item (includes padding on both sides)
    int spacePerItem = (r.right - r.left) / (int)_items.size();
    //spacePerItem = max(spacePerItem, maxWidth + 10);

    // Figure out space per bar
    int spacePerBar = (int)(spacePerItem * .8);

    // Figure out padding
    int padding = (spacePerItem - spacePerBar) / 2;

    // Figure out height scale
    int minv = (int)_items[0]._value, maxv = (int)_items[0]._value;
    for (size_t i = 1; i < _items.size(); i++) {
        minv = min((int)_items[i]._value, minv);
        maxv = max((int)_items[i]._value, maxv);
    }

    int height = maxv - minv;
    minv = min(minv, 0);

    int offset = 0;
    if (minv < 0) {
        offset = abs(minv);
    }

    double scale = r.bottom / (double)height;

    LOGFONT lf = { 0 };
    wcscpy_s(lf.lfFaceName, L"Arial");
    lf.lfEscapement = 900;
    lf.lfWeight = FW_NORMAL;

    int iDc = SaveDC(hdc);
    HFONT hFont = CreateFontIndirect(&lf);
    SelectObject(hdc, hFont);

    HBRUSH barBrush = CreateSolidBrush(RGB(0, 0, 255));
    HBRUSH labelRectBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH shadowBrush = CreateSolidBrush(RGB(100, 100, 100));

    // Now render
    int curSpot = 0;
    for (size_t i = 0; i < _items.size(); i++) {
        RECT rBar = { 0 };
        rBar.left = curSpot + padding;
        rBar.bottom = r.bottom;// (int)(_items[i]._value * scale);
        rBar.top = max(0,r.bottom - (int)(_items[i]._value * scale));
        rBar.right = rBar.left + spacePerBar;
        //rBar.bottom = 0;

        curSpot += spacePerItem;
        SelectObject(hdc, barBrush);
        Rectangle(hdc, rBar.left, rBar.top, rBar.right, rBar.bottom);
        SetBkMode(hdc, TRANSPARENT);

        std::wstring text(_items[i]._name);
        text += L"     ";
        text += Utils::penniesToDollarsString((int)_items[i]._value);
        text += L" ";

        SIZE textSize = { 0 };
        GetTextExtentPoint32(hdc, text.c_str(), (int)text.length(),  &textSize);

        int textLeft = (rBar.left + rBar.right) / 2;
        textLeft -= textSize.cy / 2;

        rBar.bottom -= 10; rBar.top -= 10;
        RECT rText = rBar;

        // Drop shadow
        SelectObject(hdc, shadowBrush);
        static const int SHADOW_OFFSET = 3;
        HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, textLeft + SHADOW_OFFSET, rBar.bottom - textSize.cx - 3 - SHADOW_OFFSET, textLeft + textSize.cy + SHADOW_OFFSET, rBar.bottom + 3 - SHADOW_OFFSET);
        SelectObject(hdc, hOldPen);

        // Label rect
        SelectObject(hdc, labelRectBrush);
        Rectangle(hdc, textLeft, rBar.bottom - textSize.cx - 3, textLeft + textSize.cy, rBar.bottom+3);

        TextOut(hdc, textLeft, rText.bottom, text.c_str(), (int)text.length());
    }

    RestoreDC(hdc, iDc);
    DeleteObject(barBrush);
    DeleteObject(labelRectBrush);
    DeleteObject(shadowBrush);
    
    DeleteObject(hFont);

}

