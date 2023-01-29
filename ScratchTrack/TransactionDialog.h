#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "AccountInfo.h"

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
	void setBucketId(unsigned int bucketId) { _bucketId = bucketId; }
	void setAccountFromId(unsigned int accountId) { _accountFromId = accountId; }

	// Post-show functions
	int getAmount() { return _amount; }
	const wchar_t* getPayee() { return _payee.c_str(); }
	const wchar_t* getDescription() { return _description.c_str(); }
	unsigned int getBucketId() { return _bucketId; }
	unsigned int getAccountFromId() { return _accountFromId; }

protected:
	static void updateUi(HWND hDlg);

protected:
	int _amount;
	std::wstring _payee;
	std::wstring _description;

	unsigned int _accountFromId;
	unsigned int _bucketId;

	static INT_PTR CALLBACK TransactionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static TransactionDialog* THIS_PTR;

};

