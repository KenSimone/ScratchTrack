#pragma once

#include <windows.h>
#include <string>

class AccountCategory;

class FundCategoryDlg
{
public:
	FundCategoryDlg(AccountCategory* pCategory) : _pCategory(pCategory), _newBalance(0)
	{
	};

	~FundCategoryDlg() {};

	bool ShowDialog();

	int getNewBalance() { return _newBalance; }

protected:
	static INT_PTR CALLBACK FundCategoryDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);
	void calcNewBalance(HWND hDlg);

protected:
	AccountCategory* _pCategory;
	int _newBalance;

	static FundCategoryDlg* PTHIS;

}; // class FundCategoryDlg
