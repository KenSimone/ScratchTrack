#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AccountTreeItem;

class AccountDlg
{
public:
	AccountDlg();
	~AccountDlg();

	bool ShowDialog();

	// Pre-show functions
	void setBalance(int amount) { _amount = amount; }
	void setName(const wchar_t* szName) { _name = szName; }
	void setBudgeted(bool budgeted) { _budgeted = budgeted; }

	// Post-show functions
	int getBalance() { return _amount; }
	const wchar_t* getName() { return _name.c_str(); }
	bool getBudgeted() { return _budgeted; }

protected:
	int _amount;
	std::wstring _name;
	bool _budgeted;

	static INT_PTR CALLBACK AccountDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static AccountDlg* PTHIS;

};

