#include "Utils.h"
#include <Windows.h>
#include <windowsx.h>
#include "Transaction.h"
#include "AccountInfo.h"
#include "AutoCombo.h"

bool Utils::searchText(const wchar_t* szString, const wchar_t* szSearchText)
{
	if (!szString || !szSearchText) {
		return false;
	}

	if (!*szSearchText || !*szSearchText) {
		return false;
	}

	wchar_t* szUpcaseString = new wchar_t[wcslen(szString) + 1];
	size_t i = 0;
	for (i = 0; i < wcslen(szString); i++) {
		szUpcaseString[i] = toupper(szString[i]);
	}
	szUpcaseString[i] = 0;

	wchar_t* szUpcaseSearchText = new wchar_t[wcslen(szSearchText) + 1];
	for (i = 0; i < wcslen(szSearchText); i++) {
		szUpcaseSearchText[i] = toupper(szSearchText[i]);
	}
	szUpcaseSearchText[i] = 0;

	bool found = wcsstr(szUpcaseString, szUpcaseSearchText) != nullptr;

	delete[] szUpcaseString;
	delete[] szUpcaseSearchText;

	return found;
}

std::wstring Utils::penniesToDollarsString(int money)
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

int Utils::dollarsStringToPennies(const wchar_t* szCurrency)
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

// Returns true if default account ID was found
bool Utils::populateAccountCombo(HWND hParent, int resourceId, int defaultAcctId /*= INVALID_ACCOUNT_ID*/)
{
	HWND hCombo = GetDlgItem(hParent, resourceId);
	bool selSet = false;
	if (hCombo) {
		auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);

		for (auto itr = accounts.begin(); itr != accounts.end(); itr++) {
			AccountBase* pAccount = *itr;

			int pos = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)pAccount->getName());
			SendMessage(hCombo, CB_SETITEMDATA, pos, (LPARAM)pAccount);
			if (pAccount->getId() == defaultAcctId) {
				selSet = true;
				ComboBox_SetCurSel(hCombo, pos);
			}
		}

		if (!selSet) {
			ComboBox_SetCurSel(hCombo, 0);
		}
	}

	return selSet;
}

int Utils::getAccountIdFromAccountCombo(HWND hParent, int resourceId, int defaultAcctId /*= INVALID_ACCOUNT_ID*/)
{
	HWND hCombo = GetDlgItem(hParent, resourceId);
	int iRet = defaultAcctId;
	if (hCombo)
	{
		int iSel = ComboBox_GetCurSel(hCombo);
		if (iSel >= 0) {
			Account* pAcct = (Account*)ComboBox_GetItemData(hCombo, iSel);
			if (pAcct) {
				iRet = pAcct->getId();
			}
		}
	}
	return iRet;
}

std::wstring Utils::accountGuiName(Account* pAccount)
{
	static const wchar_t* szCreditCard = (wchar_t*)(u"\U0001F4B3 ");
	static const wchar_t* szBank = (wchar_t*)u"\U0001F3E6 ";
	std::wstring name = (pAccount->paidFromEnvelopeId() == INVALID_ACCOUNT_ID) ? szBank : szCreditCard;
	name += pAccount->getName();

	return name;
}

std::wstring Utils::getDlgItemText(HWND hDlg, int resourceId)
{
	wchar_t szBuffer[1024] = { 0 };
	GetDlgItemText(hDlg, resourceId, szBuffer, 1024);
	std::wstring ret(szBuffer);
	return ret;
}

void Utils::setDlgItemDollarTet(HWND hDlg, int resourceId, int amount)
{
	std::wstring theText =  Utils::penniesToDollarsString(amount);
	SetDlgItemText(hDlg, resourceId, theText.c_str());
}

int Utils::getDlgItemDollarText(HWND hDlg, int resourceId)
{
	std::wstring theText = getDlgItemText(hDlg, resourceId);
	return Utils::dollarsStringToPennies(theText.c_str());
}

bool Utils::populatePayeeCombo(HWND hParent, int resourceId, const wchar_t* szDefaultPayee /*= nullptr*/)
{
	std::vector<Transaction*> transactions = Transaction::getAllTransactions();
	std::set<std::wstring> uniquePayees;
	for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
		uniquePayees.insert((*itr)->getPayee());
	}

	HWND hCombo = GetDlgItem(hParent, resourceId);
	MakeAutocompleteCombo(hCombo);

	bool selSet = false;
	for (auto itr = uniquePayees.begin(); itr != uniquePayees.end(); itr++) {
		if (!itr->empty()) {
			int pos = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)itr->c_str());
			if (szDefaultPayee && _wcsicmp(itr->c_str(), szDefaultPayee) == 0)
			{
				selSet = true;
				ComboBox_SetCurSel(hCombo, pos);
			}
		}
	}

	return selSet;
}