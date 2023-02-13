#pragma once

#include <windows.h>
#include <string>
#include "TransactionImporter.h"

class Account;

class ImportMapDlg
{
public:
	ImportMapDlg(ImportedTransactionInfo* pInfo);
	~ImportMapDlg();

	bool ShowDialog(HWND hParent);

protected:
	static INT_PTR CALLBACK ImportMapDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void enableDisableUi(HWND hDlg);

protected:
	static ImportMapDlg* PTHIS;
	ImportedTransactionInfo* _pTransactionInfo;

}; // class ImportMapDlg
