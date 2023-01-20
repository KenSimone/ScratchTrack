#pragma once

#include <windows.h>
#include <string>

class Account;

class DepositDlg
{
public:
	DepositDlg(Account* pAccount, int amount, const wchar_t* szPayee, const wchar_t* szDescription);
	~DepositDlg();

	bool ShowDialog();

	int getAmount() { return _amount; }
	const wchar_t* getPayee() { return _payee.c_str(); }
	const wchar_t* getDescription() { return _description.c_str(); }
	Account* getAccount() { return _pAccount;  }


protected:
	static INT_PTR CALLBACK DepositDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	Account* _pAccount;
	static DepositDlg* PTHIS;
	int _amount;
	std::wstring _payee;
	std::wstring _description;

}; // class DepositDlg
