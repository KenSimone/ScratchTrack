#pragma once

#include <string>
#include <Windows.h>
#include <cmath>

class Utils
{
public:
	static bool searchText(const wchar_t* szString, const wchar_t* szSearchText);

	static std::wstring penniesToDollarsString(int money)
	{
		// Remember if we're negative, and then flip to positive for easier math
		bool negative = false;
		if (money < 0) {
			negative = true;
			money *= -1;
		}

		// Split into logical parts
		int coins = money % 100;
		int dollars = money / 100;

		// Create a string without any formating
		wchar_t szBuff[255] = { 0 }, szBuff2[255];
		if (negative) {
			swprintf(szBuff, 255, L"-%d.%02d", dollars, coins);
		}
		else {
			swprintf(szBuff, 255, L"%d.%02d", dollars, coins);
		}

		// Now use the handy windows API to print a locale-specific string
		GetCurrencyFormatEx(LOCALE_NAME_USER_DEFAULT, 0, szBuff, nullptr, szBuff2, 255);
		if (szBuff2[0] == L'(')
		{
			szBuff2[0] = L'-';
			size_t len = wcslen(szBuff2);
			szBuff2[len - 1] = 0;
		}

		// And return it
		return std::wstring(szBuff2);
	}

	static int dollarsStringToPennies(const wchar_t* szCurrency)
	{
		std::wstring coins, dollars;
		const wchar_t* szCur = szCurrency;

		// Are we to the right of a decimal?
		bool atCoins = false; 

		// Remember if this is a negative number
		bool negative = szCur && szCur[0] == L'-';

		// Spin through each character
		while (szCur && *szCur && coins.length() < 2)
		{
			if (*szCur == L'.') {
				// If it's a dot, subsequent chars are coins. TODO: Get decimal separator by locale
				atCoins = true;
			}
			else if (iswdigit(*szCur))
			{
				if (atCoins) {
					// This is a digit and we're to the right of the decimal, add to coins
					coins += *szCur;
				}
				else {
					// This is a digit and we're NOT to the right of the decimal, add to dollars
					dollars += *szCur;
				}
			}

			// Advance to the next char. This logic skips junk characters
			szCur++;
		}

		// Our unit is pennies, so multiply by 100
		int ret = _wtoi(dollars.c_str()) * 100;

		// Coins are a little tricky. If we've only got one character, assume tens spots
		int iCoins = _wtoi(coins.c_str());
		if (coins.length() == 1) {
			iCoins *= 10;
		}
		ret += iCoins;

		// Lastly, if it's negative, flip the sign
		if (negative) {
			ret *= -1;
		}
		return ret;
	}
};
