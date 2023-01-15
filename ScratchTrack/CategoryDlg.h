#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AccountBase;
class Account;

class CategoryDialog
{
public:
	CategoryDialog();
	~CategoryDialog();

	bool ShowDialog();

	// Pre-show functions
	void setCategoryName(const wchar_t* szCategory) { _name = szCategory;  }
	void setBudget(int budget) { _budget = budget; }
	void setDefaultTransactionAmount(int defTransAmt) { _amount = defTransAmt; }
	void setDefaultTransactionPayFromAccount(Account* pDefaultAccount) { _pDefaultTransactionPayFromAccount = pDefaultAccount; }
	void setDefaultTransactionPayToAccount(Account* pDefaultAccount) { _pDefaultTransactionPayToAccount = pDefaultAccount; }
	void setDefaultTransactionPayee(const wchar_t* szPayee) { _payee = szPayee; }
	void setIsIncome(bool isIncome) { _isIncome = isIncome;  }
	void setIsVirtualAccount(bool isVirtual) { _isVirtual = isVirtual; }

	// Post-show functions
	const wchar_t* getCategoryName() { return _name.c_str(); }
	int getBudget() { return _budget; }
	int getDefaultTransactionAmount() { return _amount; }
	Account* getDefaultTransactionPayFromAccount() { return _pDefaultTransactionPayFromAccount; }
	Account* getDefaultTransactionPayToAccount() { return _pDefaultTransactionPayToAccount; }
	const wchar_t* getDefaultTransactionPayee() { return _payee.c_str(); }
	bool getIsIncome() { return _isIncome; }
	bool getIsVirtual() { return _isVirtual; }


protected:
	std::wstring _name;
	Account* _pDefaultTransactionPayFromAccount;
	Account* _pDefaultTransactionPayToAccount;
	std::wstring _payee;
	std::wstring _category;
	int _amount;
	int _budget;
	bool _isIncome;
	bool _isVirtual;

	static INT_PTR CALLBACK CategoryDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static CategoryDialog* PTHIS;
};

