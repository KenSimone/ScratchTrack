#pragma once

#include <windows.h>
#include <string>
#include "TransactionImporter.h"

class Account;

class TransactionImportDlg
{
public:
	TransactionImportDlg(Account* pAccount, std::vector<ImportedTransactionInfo>& transactions);
	~TransactionImportDlg();

	bool ShowDialog();

protected:
	static INT_PTR CALLBACK TransactionImportDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void enableDisableUi(HWND hDlg);

protected:
	Account* _pAccount;
	static TransactionImportDlg* PTHIS;
	std::vector<ImportedTransactionInfo>& _transactions;

}; // class TransactionImportDlg
