#pragma once

#include <windows.h>
#include <string>

class AccountGroup;

class FundCategoriesDlg
{
public:
	FundCategoriesDlg(AccountGroup* pGroups) : _pGroup(pGroups), _resetBalance(false)
	{
	};

	~FundCategoriesDlg() {};

	bool ShowDialog();

	// If reset, then the new balance becomes the budgeted amount, otherwise add
	// to any existing balance.
	bool getResetBalance() { return _resetBalance; }

protected:
	static INT_PTR CALLBACK FundCategoriesDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);

protected:
	AccountGroup* _pGroup;
	bool _resetBalance;

	static FundCategoriesDlg* PTHIS;

}; // class FundCategoriesDlg
