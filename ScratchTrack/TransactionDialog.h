#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AccountBase;

class TransactionDialog
{
public:
	TransactionDialog();
	~TransactionDialog();

	bool ShowDialog();

	// Pre-show functions
	void setAmount(int amount) { _amount = amount; }
	void setPayee(const wchar_t* szPayee) { _payee = szPayee; }
	void setDescription(const wchar_t* szDescription) { _description = szDescription; }
	void setCategoryId(unsigned int categoryId) { _categoryId = categoryId; }
	void setAccountId(unsigned int accountId) { _accountId = accountId; }

	// Only used if transaction amount is zero
	void setIsExpense(bool isExpense) { _isExpense = isExpense; }

	// Post-show functions
	int getAmount() { return _amount; }
	const wchar_t* getPayee() { return _payee.c_str(); }
	const wchar_t* getDescription() { return _description.c_str(); }
	unsigned int getCategoryId() { return _categoryId; }
	unsigned int getAccountId() { return _accountId; }
	bool getIsExpense() { return _isExpense; }

protected:
	bool _isExpense;
	int _amount;
	std::wstring _payee;
	std::wstring _description;

	unsigned int _accountId;
	unsigned int _categoryId;

	static INT_PTR CALLBACK TransactionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static TransactionDialog* THIS_PTR;

};

