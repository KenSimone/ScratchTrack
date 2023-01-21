#pragma once

#include <windows.h>
#include <string>

class AccountGroup;

class FolderDlg
{
public:
	FolderDlg(AccountGroup* pAccount, const wchar_t* szName, int limit) :
		_pAccount(pAccount), _name(szName), _limit(limit)
	{
	};

	~FolderDlg() {};

	bool ShowDialog();

	int getLimit() { return _limit; }
	const wchar_t* getName() { return _name.c_str(); }
	AccountGroup* getGroup() { return _pAccount; }


protected:
	static INT_PTR CALLBACK FolderDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	int _limit;
	AccountGroup* _pAccount;
	std::wstring _name;
	static FolderDlg* PTHIS;

}; // class FolderDlg
