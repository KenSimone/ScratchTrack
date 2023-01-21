#pragma once

#include <windows.h>
#include <string>

class Account;

class PayCCDlg
{
public:
	PayCCDlg(Account* pAccount, int amount, int payFromAcctId, const wchar_t* szDescription = nullptr) : 
		_pAccount(pAccount), _amount(amount), _payFromAcctId(payFromAcctId)
	{
		if (szDescription) {
			_description = szDescription;
		}
	};

	~PayCCDlg();

	bool ShowDialog();

	int getAmount() { return _amount; }
	const wchar_t* getDescription() { return _description.c_str(); }

	int getPaidFromId() { return _payFromAcctId; }

protected:
	static INT_PTR CALLBACK PayCCDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	int _amount;
	Account* _pAccount;

	std::wstring _description;
	static PayCCDlg* PTHIS;
	int _payFromAcctId;

}; // class PayCCDlg
