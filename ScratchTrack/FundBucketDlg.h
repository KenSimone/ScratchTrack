#pragma once

#include <windows.h>
#include <string>

class AccountBucket;

class FundBucketDlg
{
public:
	FundBucketDlg(AccountBucket* pBucket) : _pBucket(pBucket), _newBalance(0)
	{
	};

	~FundBucketDlg() {};

	bool ShowDialog();

	int getNewBalance() { return _newBalance; }

protected:
	static INT_PTR CALLBACK FundBucketDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);
	void calcNewBalance(HWND hDlg);

protected:
	AccountBucket* _pBucket;
	int _newBalance;

	static FundBucketDlg* PTHIS;

}; // class FundBucketDlg
