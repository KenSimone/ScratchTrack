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
	void setEnvelopeId(unsigned int envelopeId) { _envelopeId = envelopeId; }
	void setAccountFromId(unsigned int accountId) { _accountFromId = accountId; }
	void setAccountToId(unsigned int accountToId) { _accountToId = accountToId; }

	// Only used if transaction amount is zero
	void setIsExpense(bool isExpense) { _isExpense = isExpense; }

	// Post-show functions
	int getAmount() { return _amount; }
	const wchar_t* getPayee() { return _payee.c_str(); }
	const wchar_t* getDescription() { return _description.c_str(); }
	unsigned int getEnvelopeId() { return _envelopeId; }
	unsigned int getAccountFromId() { return _accountFromId; }
	unsigned int getAccountToId() { return _accountToId; }
	bool getIsExpense() { return _isExpense; }
	bool isTransfer() { return _accountToId != MAXINT;  }

protected:
	static void updateUi(HWND hDlg);

protected:
	bool _isExpense;
	int _amount;
	std::wstring _payee;
	std::wstring _description;

	unsigned int _accountFromId;
	unsigned int _accountToId;
	unsigned int _envelopeId;

	static INT_PTR CALLBACK TransactionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static TransactionDialog* THIS_PTR;

};

