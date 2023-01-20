#pragma once

#include <windows.h>
#include <string>

class Account;

class PayCCDlg
{
public:
	PayCCDlg(Account* pAccount, int amount, const wchar_t* szDescription = nullptr) : 
		_pAccount(pAccount), _amount(amount), _hParent(nullptr), _hThisDialg(nullptr) 
	{
		if (szDescription) {
			_description = szDescription;
		}
	};

	~PayCCDlg();

	bool ShowDialog();
	operator HWND() { return _hThisDialg; }

	int getAmount() { return _amount; }
	const wchar_t* getDescription() { return _description.c_str(); }

protected:
	static INT_PTR CALLBACK PayCCDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	int _amount;
	Account* _pAccount;

	HWND _hParent, _hThisDialg;
	std::wstring _description;
	static PayCCDlg* PTHIS;

}; // class PayCCDlg
