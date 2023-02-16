#pragma once

#include <windows.h>
#include <string>

class AccountExpense;

class FundExpenseDlg
{
public:
	FundExpenseDlg(AccountExpense* pExpense) : _pExpense(pExpense), _newBalance(0)
	{
	};

	~FundExpenseDlg() {};

	bool ShowDialog();

	int getNewBalance() { return _newBalance; }

protected:
	static INT_PTR CALLBACK FundExpenseDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);
	void calcNewBalance(HWND hDlg);

protected:
	AccountExpense* _pExpense;
	int _newBalance;

	static FundExpenseDlg* PTHIS;

}; // class FundExpenseDlg
