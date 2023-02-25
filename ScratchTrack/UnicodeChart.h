#pragma once

#include <iostream>
#include <vector>
#include <string>

static const wchar_t SOLID_BLOCK_CHAR = L'\u2588';
//static const wchar_t LIGHT_BLOCK_CHAR = L'\u2592';
static const wchar_t LIGHT_BLOCK_CHAR = L' ';

// If you want to build the text yourself, implement this class and pass an instance to render.
// This is useful for drawing colored bar charts inside a rich edit control (for instance).
struct UnicodeRenderCallback
{
    virtual void addChar(const wchar_t ch, const wchar_t* szItemName, double itemValue, void* pUserData) = 0;
    virtual void addSpace() = 0;
    virtual void addNewLine() = 0;
};

class unicodeSparkline
{
public:
    static std::wstring render(unsigned int val, unsigned int maxVal, unsigned int maxWidthChars)
    {
        std::wstring ret;
        double scale = (double)maxVal / (double)maxWidthChars;
        for (size_t i = 0; i < maxWidthChars; i++)
        {
            bool blocked = i * scale <= val;
            ret += (blocked) ? SOLID_BLOCK_CHAR : LIGHT_BLOCK_CHAR;
        }
        return ret;
    }

protected:
    unicodeSparkline() {};

};

class UnicodeBarChart
{
public:
    UnicodeBarChart() : _maxHeightInChars(40) {}

    void addItem(const wchar_t* szName, double value, void* pUserData = nullptr)
    {
        UnicodeBarItem item;
        item._name = szName;
        item._value = value;
        item._pUserData = pUserData;
        _items.push_back(item);
    }

    // The maximum height of the chart in characters. Think rows in a text edit
    void setMaxHeightInChars(size_t maxHeight) { _maxHeightInChars = maxHeight; }

    // Renders the bar chart to a string, optionally calls a callback for custom draw
    std::wstring render(UnicodeRenderCallback* pCallback = nullptr)
    {
        std::wstring ret;
        if (_items.empty()) {
            return ret;
        }

        // Calc rnage and scale
        size_t range = calcRange();
        double scale = (double)range / (double)_maxHeightInChars;

        // Figur out our width in chars
        static const size_t BARWIDTH = 4, BARSPACE = 1;
        size_t width = _items.size() * (BARWIDTH + BARSPACE);

        for (int y = (int)_maxHeightInChars - 1; y >= 0; y--)
        {
            double curY = y * scale;
            for (size_t x = 0; x < width; x++)
            {
                bool blocked = false;
                size_t barIndex = x / (BARWIDTH + BARSPACE);
                if (x % (BARWIDTH + BARSPACE) && _items.size() > barIndex)
                {
                    UnicodeBarItem item = _items[barIndex];
                    if (curY <= item._value) {
                        blocked = true;
                        if (pCallback) {
                            pCallback->addChar(SOLID_BLOCK_CHAR, _items[barIndex]._name.c_str(), _items[barIndex]._value, _items[barIndex]._pUserData);
                        }
                        ret += SOLID_BLOCK_CHAR;
                    } // if the bar renders at this height
                } // if there's a bar in thos column

                if (!blocked) {
                    ret += L' ';
                    if (pCallback) {
                        pCallback->addSpace();
                    }
                }
            } // for x

            if (y) {
                ret += L"\n";
                if (pCallback) {
                    pCallback->addNewLine();
                }
            }
        } // for y
        return ret;
    }

protected:

    size_t calcRange()
    {
        // Figure out our range
        double max = _items[0]._value;
        double min = _items[0]._value;

        for (size_t i = 1; i < _items.size(); i++)
        {
            if (_items[i]._value < min) {
                min = _items[i]._value;
            }

            if (_items[i]._value > max) {
                max = _items[i]._value;
            }
        } // for each item

        return (size_t)abs(max - min);

    }

    struct UnicodeBarItem
    {
        std::wstring _name;
        double _value;
        void* _pUserData;
    };

    std::vector<UnicodeBarItem> _items;
    size_t _maxHeightInChars;

}; // class UnicodeBarChart
