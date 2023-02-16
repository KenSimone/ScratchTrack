#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AccountBase;
class Account;

class ExpenseDialog
{
public:
	ExpenseDialog();
	~ExpenseDialog();

	bool ShowDialog();

	// Pre-show functions
	void setExpenseName(const wchar_t* szExpense) { _name = szExpense;  }
	void setBudget(int budget) { _budget = budget; }
	void setDefaultTransactionAmount(int defTransAmt) { _amount = defTransAmt; }
	void setDefaultTransactionPayFromAccount(Account* pDefaultAccount) { _pDefaultTransactionPayFromAccount = pDefaultAccount; }
	void setDefaultTransactionPayee(const wchar_t* szPayee) { _payee = szPayee; }

	// Post-show functions
	const wchar_t* getExpenseName() { return _name.c_str(); }
	int getBudget() { return _budget; }
	int getDefaultTransactionAmount() { return _amount; }
	Account* getDefaultTransactionPayFromAccount() { return _pDefaultTransactionPayFromAccount; }
	const wchar_t* getDefaultTransactionPayee() { return _payee.c_str(); }

protected:
	std::wstring _name;
	Account* _pDefaultTransactionPayFromAccount;
	std::wstring _payee;
	std::wstring _expense;
	int _amount;
	int _budget;

	static INT_PTR CALLBACK ExpenseDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void enableDisableUi(HWND hDlg);

	static ExpenseDialog* PTHIS;
};

