#include "Utils.h"
#include <Windows.h>
#include <windowsx.h>
#include <process.h>
#include <shldisp.h>
#include "Transaction.h"
#include "AccountInfo.h"
#include "AccountInfoDlg.h"
#include <objidl.h>
#include <shlobj.h>
#include <algorithm>

extern HWND g_hTopWindow;
extern AccountInfoDlg g_accountInfoDlg;

class EnumString : public IEnumString
{
public:
	EnumString(std::vector<std::wstring> strings) : _refCount(1), _pos(0), _strings(strings) { };
	~EnumString() {};

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
	{
		HRESULT hr = S_OK;
		wchar_t szBuff[1024];
		StringFromGUID2(riid, szBuff, 1024);
		if (memcmp(&IID_IUnknown, &riid, sizeof(riid)) == 0)
		{
			AddRef();
			*ppvObject = (IUnknown*)this;
		}
		else if (memcmp(&IID_IEnumString, &riid, sizeof(riid)) == 0)
		{
			AddRef();
			*ppvObject = (IEnumString*)this;
		}
		else
		{
			hr = E_NOINTERFACE;
		}
		return hr;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return InterlockedIncrement(&_refCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		ULONG ref = InterlockedDecrement(&_refCount);
		if (!ref) {
			delete this;
		}
		return ref;
	}


	virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched)
	{
		if (!celt) {
			celt = 1;
		}

		size_t end = min(_strings.size(), celt + _pos);

		size_t i = 0;
		for (_pos; _pos < end; _pos++) {
			size_t toAlloc = (_strings[_pos].length() + 1) * sizeof(wchar_t);
			rgelt[i] = (LPOLESTR)CoTaskMemAlloc(toAlloc);
			wcscpy_s(rgelt[i], _strings[_pos].length() + 1, _strings[_pos].c_str());
			i++;
		}

		*pceltFetched = (ULONG)i;

		return i == celt ? S_OK : S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
	{
		_pos += celt;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Reset(void)
	{
		_pos = 0;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumString** ppenum)
	{
		*ppenum = new EnumString(_strings);
		return S_OK;
	}

protected:
	ULONG _refCount;
	std::vector<std::wstring> _strings;
	size_t _pos;
};

SYSTEMTIME Utils::dateStringToSysTime(const wchar_t* szDate)
{
	// The MS function VarDateFromStr seems to weed out crap fairly well
	// The only thing I've found that's trouble is an apostrophe
	std::wstring toParse;
	const wchar_t* szCur = szDate;

	while (szCur && *szCur) {
		if (*szCur != L'\'') {
			toParse += *szCur;
		}
		szCur++;
	}
	// Assume month day year
	DATE date = { 0 };
	VarDateFromStr(toParse.c_str(), LOCALE_SYSTEM_DEFAULT, VAR_DATEVALUEONLY, &date);
	SYSTEMTIME ret = { 0 };
	VariantTimeToSystemTime(date, &ret);

	return ret;
}


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

std::wstring Utils::penniesToDollarsString(int money, bool formatted /*= true*/)
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
	if (formatted) {
		GetCurrencyFormatEx(LOCALE_NAME_USER_DEFAULT, 0, szBuff, nullptr, szBuff2, 255);
		if (szBuff2[0] == L'(')
		{
			szBuff2[0] = L'-';
			size_t len = wcslen(szBuff2);
			szBuff2[len - 1] = 0;
		}
		return std::wstring(szBuff2);
	}

	// And return it
	return std::wstring(szBuff);
}

size_t Utils::looksSimilar(std::wstring s1, std::wstring s2)
{
	if (s1.empty() || s2.empty()) {
		return false;
	}

	std::transform(s1.begin(), s1.end(), s1.begin(), towupper);
	std::transform(s2.begin(), s2.end(), s2.begin(), towupper);
	size_t matchesCount = 0;
	const wchar_t* szCur = &s1[0];
	const wchar_t* szEnd = szCur + s1.length();

	while (szCur < szEnd) {
		size_t step = min(3, szEnd - szCur);
		if (s2.find(szCur, 0, step) != std::string::npos)
		{
			matchesCount++;
		}
		szCur++;
	}
	return matchesCount;
}

size_t Utils::stringDistance(std::wstring s1, std::wstring s2)
{
	const std::size_t len1 = s1.length(), len2 = s2.length();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

	d[0][0] = 0;
	for (unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for (unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

	for (unsigned int i = 1; i <= len1; ++i)
		for (unsigned int j = 1; j <= len2; ++j)
		{
			// note that std::min({arg1, arg2, arg3}) works only in C++11,
			// for C++98 use std::min(std::min(arg1, arg2), arg3)

			unsigned int a = d[i - 1][j] + 1;
			unsigned int b = d[i][j - 1] + 1;
			unsigned int c = d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1);
			d[i][j] = min(min(a, b), c);

			//		std::min( std::min(d[i - 1][j] + 1, 
			//			d[i][j - 1] + 1), 
			//			d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) ;

			//		d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) });
		}

	return d[len1][len2];
}
std::string Utils::wideToMultibyte(const wchar_t* szString)
{
	char szBuff[1024] = { 0 };
	WideCharToMultiByte(CP_UTF8, 0, szString, -1, szBuff, 1024, nullptr, nullptr);

	return std::string(szBuff);
}


std::wstring Utils::multiByteToWide(const char* szString)
{
	std::wstring ret;
	wchar_t szWide[1024] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, szString, -1, szWide, 1024);

	return szWide;

}

int Utils::dollarsStringToPennies(const char* szCurrency)
{
	wchar_t szWide[1024] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, szCurrency, -1, szWide, 1024);
	return dollarsStringToPennies(szWide);
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
bool Utils::populateAccountCombo(HWND hParent, int resourceId, int defaultAcctId /*= ACCOUNT_ID_INVALID*/, bool noCCAccount /* = false */, int avoidAccount /*= ACCOUNT_ID_INVALID*/)
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

int Utils::getAccountIdFromAccountCombo(HWND hParent, int resourceId, int defaultAcctId /*= ACCOUNT_ID_INVALID*/)
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
	std::wstring name = (!pAccount->isCCAccount()) ? szBank : szCreditCard;
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

bool Utils::isDlgItemTextEmpty(HWND hDlg, int resourceId)
{
	return getDlgItemText(hDlg, resourceId).empty();
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

	std::vector<std::wstring> enumStrings;
	bool selSet = false;
	for (auto itr = uniquePayees.begin(); itr != uniquePayees.end(); itr++) {
		if (!itr->empty()) {
			enumStrings.push_back(*itr);
			int pos = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)itr->c_str());
			if (szDefaultPayee && _wcsicmp(itr->c_str(), szDefaultPayee) == 0)
			{
				selSet = true;
				ComboBox_SetCurSel(hCombo, pos);
			}
		}
	}

	IAutoComplete* pAutoComplete = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_AutoComplete, nullptr, CLSCTX_INPROC_SERVER, IID_IAutoComplete, (void**)&pAutoComplete);
	if (SUCCEEDED(hr) && pAutoComplete)
	{
		EnumString* pEnumString = new EnumString(enumStrings);

		HWND hEdit = FindWindowEx(hCombo, NULL, WC_EDIT, NULL);
		pAutoComplete->Init(hEdit, (IUnknown*)pEnumString, nullptr, nullptr);
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

#ifndef SCRATCHTRACK_TESTS
	g_accountInfoDlg.saveText();
	g_accountInfoDlg.appendText(L"About hiding things\n", RGB(0, 0, 0), false, true, AccountInfoDlg::HEADER_SIZE, true);

	g_accountInfoDlg.appendText(L"In ScratchTrack there is no delete, at least not yet. If you want something gone from your view you "
		L"can hide it. As of now, hidden items are simply not displayed. No money moves, nothing is assigned or unassigned. If you hide an "
		L"category with money in it, those funds will be hidden as well. You can always unhide things with the unhide button.",
		RGB(0, 0, 0), false, false, AccountInfoDlg::TEXT_SIZE, false);
	if (IDYES == MessageBox(g_hTopWindow, szBuff, L"Confirm Hide", MB_YESNO)) {
		return true;
	}
#endif

#ifndef SCRATCHTRACK_TESTS
	g_accountInfoDlg.restoreText();
#endif
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

#ifndef SCRATCHTRACK_TESTS
				MessageBox(g_hTopWindow, L"The budget file has been modified outside of this instance of ScratchTrack. This instance of ScratchTrack will exit.", L"External file modification", MB_OK);
#endif
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

const char* Utils::getPersistFileXlsx()
{
	static char szBuff[1024] = { 0 };
	GetModuleFileNameA(NULL, szBuff, 1024);

	char* slashSpot = strrchr(szBuff, '.');
	*slashSpot = 0;

	strcat_s(szBuff, 1024, ".xlsx");

	return szBuff;
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
	char szPersistPath[MAX_PATH] = { 0 };
	strcpy_s(szPersistPath, szPersistFile);
	char* pSlashSpot = strrchr(szPersistPath, '\\');
	char* pFilename = pSlashSpot + 1;
	*pSlashSpot = 0;

	std::string backupPath(szPersistPath);
	backupPath += "\\ScratchTrackBackup";

	if (!CreateDirectoryA(backupPath.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS){
		backupPath = szPersistPath;
	}
	backupPath += "\\";

	static const size_t BACKUP_DEPTH = 10;
	for (size_t i = BACKUP_DEPTH; i > 0; i--)
	{
		char curIndex[4] = { 0 }, prevIndex[4] = { 0 };
		_itoa_s((int)i, curIndex, 10);
		_itoa_s((int)(i - 1), prevIndex, 10);

		std::string cur(backupPath.c_str());
		cur += pFilename;
		cur += ".bak.";
		cur += curIndex;

		std::string prev(backupPath.c_str());
		prev += pFilename;
		prev += ".bak.";
		prev += prevIndex;

		// Delete the cur file
		DeleteFileA(cur.c_str());

		// Move prev to cur
		if (i == 1) {
			CopyFileA(szPersistFile, cur.c_str(), false);
		}
		else
		{
			MoveFileA(prev.c_str(), cur.c_str());
		}
	}

}

unsigned int Utils::daysDiff(const SYSTEMTIME& t1, const SYSTEMTIME& t2)
{
	double d1, d2;
	SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&t1), &d1);
	SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&t2), &d2);

	return (unsigned int)abs(d2 - d1);
}
