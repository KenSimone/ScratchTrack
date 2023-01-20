#pragma once

#include <string>
#include <Windows.h>
#include <cmath>
#include "AccountInfo.h"

class Utils
{
public:
	static bool searchText(const wchar_t* szString, const wchar_t* szSearchText);

	static std::wstring penniesToDollarsString(int money);
	static int dollarsStringToPennies(const wchar_t* szCurrency);

	// Returns true if default account ID was found
	static bool populateAccountCombo(HWND hParent, int resourceId, int defaultAcctId = INVALID_ACCOUNT_ID);
	static int getAccountIdFromAccountCombo(HWND hParent, int resourceId, int defaultAcctId = INVALID_ACCOUNT_ID);

	static std::wstring accountGuiName(Account* pAccount);
};
