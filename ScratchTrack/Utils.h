#pragma once

#include <string>
#include <Windows.h>
#include <cmath>
#include <commctrl.h>
#include "AccountInfo.h"


class Utils
{
public:

// String stuff
	static bool searchText(const wchar_t* szString, const wchar_t* szSearchText);
	static std::wstring multiByteToWide(const char* szString);

// Money stuff
	static std::wstring penniesToDollarsString(int money);
	static int dollarsStringToPennies(const char* szCurrency);
	static int dollarsStringToPennies(const wchar_t* szCurrency);

// GUI stuff
	static bool populatePayeeCombo(HWND hParent, int resourceId, const wchar_t* szDefaultPayee = nullptr);
	static bool populateAccountCombo(HWND hParent, int resourceId, int defaultAcctId = ACCOUNT_ID_INVALID, bool noCCAccount = false, int avoidAccount = ACCOUNT_ID_INVALID);
	static int getAccountIdFromAccountCombo(HWND hParent, int resourceId, int defaultAcctId = ACCOUNT_ID_INVALID);
	static std::wstring accountGuiName(Account* pAccount); // Account name pre-pended w/ CC or bank icon
	static std::wstring getDlgItemText(HWND hDlg, int resourceId); // Same as native API, but returns a std::string
	static bool isDlgItemTextEmpty(HWND hDlg, int resourceId); // Returns true if dialog text is unset

	static void setDlgItemDollarText(HWND hDlg, int resourceId, int amount); // Pennies to text
	static int getDlgItemDollarText(HWND hDlg, int resourceId); // Text to pennies
	static LRESULT customDrawListViewAlternatingColors(NMLVCUSTOMDRAW* pCustomDrawItem);

// Validation stuff
	static bool confirmHide(AccountBase* pAccount); // Confirmation message box to confirm "hide" operation
	static bool isSingleInstance(); // Make sure there's no other instance of the appo running

// File stuff
	static void monitorDataFileForChanges(bool stop); // Watch for external file modification, exit if it happens
	static const char* getPersistFile(); // Gets the name of our data file
	static void doBackups(); // ScratchTrack has rolling backup files (.xml, .xml.1, .xml2 and so on). This does the file roll

protected:
	static unsigned __stdcall monitorFileChangeThreadFunc(void* parg); // File monitoring happens on a seprate thread, this is the thread proc
};
