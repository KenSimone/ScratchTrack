#pragma once

#include <windows.h>
#include <string>

class AccountGroup;

class FundExpensesDlg
{
public:
	FundExpensesDlg(AccountGroup* pGroups) : _pGroup(pGroups), _resetBalance(false)
	{
	};

	~FundExpensesDlg() {};

	bool ShowDialog();

	// If reset, then the new balance becomes the budgeted amount, otherwise add
	// to any existing balance.
	bool getResetBalance() { return _resetBalance; }

protected:
	static INT_PTR CALLBACK FundExpensesDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);

protected:
	AccountGroup* _pGroup;
	bool _resetBalance;

	static FundExpensesDlg* PTHIS;

}; // class FundExpensesDlg
