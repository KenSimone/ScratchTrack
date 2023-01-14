#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AccountTreeItem;
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
	void setDefaultAmount(int defTransAmt) { _amount = defTransAmt; }
	void setDefaultAccount(Account* pDefaultAccount) { _pDefaultAccount = pDefaultAccount; }
	void setDefaultPayee(const wchar_t* szPayee) { _payee = szPayee; }
	void setIsIncome(bool isIncome) { _isIncome = isIncome;  }
	void setIsVirtualAccount(bool isVirtual) { _isVirtual = isVirtual; }

	// Post-show functions
	const wchar_t* getCategoryName() { return _name.c_str(); }
	int getBudget() { return _budget; }
	int getDefaultTransactionAmount() { return _amount; }
	Account* getDefaultAccount() { return _pDefaultAccount; }
	const wchar_t* getDefaultPayee() { return _payee.c_str(); }
	bool getIsIncome() { return _isIncome; }
	bool getIsVirtual() { return _isVirtual; }


protected:
	std::wstring _name;
	int _amount;
	Account* _pDefaultAccount;
	std::wstring _payee;
	std::wstring _category;
	int _budget;
	bool _isIncome;
	bool _isVirtual;

	static INT_PTR CALLBACK CategoryDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static CategoryDialog* PTHIS;
};

