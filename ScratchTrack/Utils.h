#pragma once

#include <string>
#include <Windows.h>
#include <cmath>
#include <commctrl.h>
#include "AccountInfo.h"


class Utils
{
public:
	static bool searchText(const wchar_t* szString, const wchar_t* szSearchText);

	static std::wstring penniesToDollarsString(int money);
	static int dollarsStringToPennies(const wchar_t* szCurrency);

	static bool populatePayeeCombo(HWND hParent, int resourceId, const wchar_t* szDefaultPayee = nullptr);

	// Returns true if default account ID was found
	static bool populateAccountCombo(HWND hParent, int resourceId, int defaultAcctId = INVALID_ACCOUNT_ID, bool noCCAccount = false, int avoidAccount = INVALID_ACCOUNT_ID);
	static int getAccountIdFromAccountCombo(HWND hParent, int resourceId, int defaultAcctId = INVALID_ACCOUNT_ID);

	static std::wstring accountGuiName(Account* pAccount);
	static std::wstring getDlgItemText(HWND hDlg, int resourceId);

	static void setDlgItemDollarText(HWND hDlg, int resourceId, int amount);
	static int getDlgItemDollarText(HWND hDlg, int resourceId);

	static LRESULT customDrawListViewAlternatingColors(NMLVCUSTOMDRAW* pCustomDrawItem);

	static bool confirmHide(AccountBase* pAccount);

	static bool isSingleInstance();

	static void monitorDataFileForChanges(bool stop);

	static const char* getPersistFile();

protected:

	static unsigned __stdcall monitorFileChangeThreadFunc(void* parg);

	static const int WAIT_FILE_CHANGE = 0;
	static const int WAIT_ABORT = 1;

};
