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
	const std::vector<ImportedTransactionInfo> transactionsOut() { return _transactionsOut; }

	SYSTEMTIME maxImportedDate() { return _maxImportDate;  }

protected:
	static INT_PTR CALLBACK TransactionImportDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void enableDisableUi(HWND hDlg);
	void populateImports(HWND hDlg);
	void initRuleMapper(HWND hDlg);
	void populateRuleMapper(HWND hDlg);

	void editMapRule(HWND hDlg);
	void addMapRule(HWND hDlg);
	void deleteMapRule(HWND hDlg);
	bool haveUnmappedItems(HWND hDlg);
	void updateImports(HWND hDlg);

protected:
	Account* _pAccount;
	static TransactionImportDlg* PTHIS;
	std::vector<ImportedTransactionInfo>& _transactions;
	std::vector<ImportedTransactionInfo> _transactionsOut;
	SYSTEMTIME _maxImportDate;

}; // class TransactionImportDlg

