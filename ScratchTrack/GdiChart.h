#pragma once

#include <windows.h>
#include <vector>
#include <string>

class GdiBarChart
{
public:
    void addItem(const wchar_t* szName, int value, void* pUserData = nullptr);
    void render(HWND hWnd, HDC hdc);


protected:

    struct GdiBarItem
    {
        std::wstring _name;
        int _value;
        void* _pUserData;
    };

    std::vector<GdiBarItem> _items;
}; // class GdiBarChart
