#include "Utils.h"
#include <Windows.h>
#include <windowsx.h>
#include <process.h>
#include "Transaction.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"
#include "AutoCombo.h"

extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

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
bool Utils::populateAccountCombo(HWND hParent, int resourceId, int defaultAcctId /*= INVALID_ACCOUNT_ID*/, bool noCCAccount /* = false */, int avoidAccount /*= INVALID_ACCOUNT_ID*/)
{
	HWND hCombo = GetDlgItem(hParent, resourceId);
	bool selSet = false;
	int selBackupChoice = -1;
	if (hCombo) {
		auto accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);

		for (auto itr = accounts.begin(); itr != accounts.end(); itr++) {
			Account* pAccount = dynamic_cast<Account*>(*itr);

			// Skip hidden and read only accounts
			if (pAccount->getReadOnly() || pAccount->getHidden()) {
				continue;
			}

			// Skip credit card accounts
			if (!pAccount || (noCCAccount && pAccount->isCCAccount())) {
				continue;
			}

			int pos = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)pAccount->getName());
			SendMessage(hCombo, CB_SETITEMDATA, pos, (LPARAM)pAccount);
			if (pAccount->getId() == defaultAcctId) {
				selSet = true;
				ComboBox_SetCurSel(hCombo, pos);
			}
			if (!selSet && pAccount->getId() != avoidAccount) {
				selBackupChoice = pos;
			}
		}

		if ( !selSet )
		{
			selBackupChoice = selBackupChoice >= 0 ? selBackupChoice : 0;
			ComboBox_SetCurSel(hCombo, selBackupChoice);
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

void Utils::setDlgItemDollarText(HWND hDlg, int resourceId, int amount)
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

LRESULT Utils::customDrawListViewAlternatingColors(NMLVCUSTOMDRAW* pCustomDrawItem)
{
	static const int darkerAmt = 25;
	switch (pCustomDrawItem->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		if (pCustomDrawItem->nmcd.dwItemSpec % 2)
		{
			COLORREF crOrig = pCustomDrawItem->clrTextBk;
			pCustomDrawItem->clrTextBk = RGB(GetRValue(crOrig) - darkerAmt, GetGValue(crOrig) - darkerAmt, GetBValue(crOrig) - darkerAmt);
		}
		return CDRF_DODEFAULT;
	default:
		return CDRF_DODEFAULT;
	}
	return 0;
}

bool Utils::confirmHide(AccountBase* pAccount)
{
	wchar_t szBuff[1024] = { 0 };
	swprintf(szBuff, 1024, L"Are you sure you want to hide '%ls'?", pAccount->getName());

	g_accountInfoDlg.saveText();
	g_accountInfoDlg.appendText(L"About hiding things\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

	g_accountInfoDlg.appendText(L"In ScratchTrack there is no delete, at least not yet. If you want something gone from your view you "
		L"can hide it. As of now, hidden items are simply not displayed. No money moves, nothing is assigned or unassigned. If you hide an "
		L"envelope with money in it, those funds will be hidden as well. You can always unhide things with the unhide button.",
		RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);

	if (IDYES == MessageBox(g_hTopWindow, szBuff, L"Confirm Hide", MB_YESNO)) {
		return true;
	}

	g_accountInfoDlg.restoreText();

	return false;
}

bool Utils::isSingleInstance()
{
	bool ret = true;
	wchar_t szFilename[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, szFilename, MAX_PATH);
	for (size_t i = 0; i < wcslen(szFilename); i++)
	{
		if (szFilename[i] == L'\\' || szFilename[i] == L':') {
			szFilename[i] = L'_';
		}
	}

	HANDLE hMutex = CreateMutex(nullptr, false, szFilename);
	if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
		return false;
	}

	return true;

}

unsigned __stdcall Utils::monitorFileChangeThreadFunc(void* parg)
{
	static const int WAIT_FILE_CHANGE = 0;
	static const int WAIT_ABORT = 1;

	const char* szPersistFile = getPersistFile();

	char szPersistPath[MAX_PATH] = { 0 };
	strcpy_s(szPersistPath, szPersistFile);
	char* szSlashSpot = strrchr(szPersistPath, '\\');
	if (!szSlashSpot) {
		return 0;
	}

	*szSlashSpot = 0;

	bool shuttingDown = false;
	do
	{
		FILETIME lastModifiedBefore = { 0 }, lastModifieAfter = { 0 };
		HANDLE hFile = CreateFileA(szPersistFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE) {
			GetFileTime(hFile, nullptr, nullptr, &lastModifiedBefore);
			CloseHandle(hFile);
		}

		HANDLE waitHandles[2] = { 0 };
		waitHandles[WAIT_FILE_CHANGE] = FindFirstChangeNotificationA(szPersistPath, false, FILE_NOTIFY_CHANGE_LAST_WRITE);
		waitHandles[WAIT_ABORT] = (HANDLE)parg;

		DWORD dwWaitResult = WaitForMultipleObjects(2, waitHandles, false, INFINITE);
		FindCloseChangeNotification(waitHandles[WAIT_FILE_CHANGE]);
		waitHandles[WAIT_FILE_CHANGE] = nullptr;

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0 + WAIT_FILE_CHANGE:
			hFile = CreateFileA(szPersistFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE) {
				GetFileTime(hFile, nullptr, nullptr, &lastModifieAfter);
				CloseHandle(hFile);
			}

			if (memcmp(&lastModifieAfter, &lastModifiedBefore, sizeof(FILETIME)) != 0) {
				// The file changed
				MessageBox(g_hTopWindow, L"The budget file has been modified outside of this instance of ScratchTrack. This instance of ScratchTrack will exit.", L"External file modification", MB_OK);
				exit(0);
			}

			break;
		case WAIT_OBJECT_0 + WAIT_ABORT:
			shuttingDown = true;
			break;
		};
	} while (!shuttingDown);

	return 0;
}

void Utils::monitorDataFileForChanges(bool stop)
{
	static HANDLE hThread = nullptr;
	static HANDLE hAbortEvent = nullptr;

	if (stop)
	{
		if (hAbortEvent) {
			SetEvent(hAbortEvent);
		}

		if (hThread) {
			WaitForSingleObject(hThread, 5000);
			CloseHandle(hThread);
			hThread = nullptr;
		}
		return;
	}

	if (!hAbortEvent) {
		hAbortEvent = CreateEvent(nullptr, true, false, nullptr);
	}
	unsigned int threadId = 0;
	hThread = (HANDLE)_beginthreadex(NULL, 0, &monitorFileChangeThreadFunc, hAbortEvent, 0, &threadId);

}

const char* Utils::getPersistFile()
{
	static char szBuff[1024] = { 0 };
	GetModuleFileNameA(NULL, szBuff, 1024);

	char* slashSpot = strrchr(szBuff, '.');
	*slashSpot = 0;

	strcat_s(szBuff, 1024, ".xml");

	return szBuff;
}

void Utils::doBackups()
{
	// Backup strategy
	// ScratchTrack.xml <-- Current version
	// ScratchTrack.xml.bak.1 <-- Current version -1
	// ScratchTrack.xml.bak.2 <-- Current version -2
	// ScratchTrack.xml.bak.3 <-- Current version -3
	// ScratchTrack.xml.bak.4 <-- Current version -4

	// Right before save:
	// delete 4
	// rename 3->4
	// rename 2->3
	// rename 1->2
	// copy current -> 1 (save op will overwrite)

	const char* szPersistFile = getPersistFile();
	std::string temp(szPersistFile);

	// Delete 4
	temp += ".bak.4";
	DeleteFileA(temp.c_str());

	// rename 3->4 (temp2 is .3, and temp is .4)
	std::string temp2(szPersistFile);
	temp2 += ".bak.3";
	MoveFileA(temp2.c_str(), temp.c_str());

	// Rename 2->3 (temp is .2 and temp2 is still  .3)
	temp = szPersistFile;
	temp += ".bak.2";
	MoveFileA(temp.c_str(), temp2.c_str());

	// Rename 1->2 (temp2 is .1 and temp is still  .2)
	temp2 = szPersistFile;
	temp2 += ".bak.1";
	MoveFileA(temp2.c_str(), temp.c_str());

	// Copy live->1 (temp2 is still .1, and temp is live)
	temp = szPersistFile;
	CopyFileA(temp.c_str(), temp2.c_str(), false);

}
