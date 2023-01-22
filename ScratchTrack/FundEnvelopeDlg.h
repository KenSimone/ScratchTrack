#pragma once

#include <windows.h>
#include <string>

class AccountEnvelope;

class FundEnvelopeDlg
{
public:
	FundEnvelopeDlg(AccountEnvelope* pEnvelope) : _pEnvelope(pEnvelope), _newBalance(0)
	{
	};

	~FundEnvelopeDlg() {};

	bool ShowDialog();
	int getNewBalance() { return _newBalance; }

protected:
	static INT_PTR CALLBACK FundEnvelopeDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);
	void calcNewBalance(HWND hDlg);

protected:
	AccountEnvelope* _pEnvelope;
	int _newBalance;

	static FundEnvelopeDlg* PTHIS;

}; // class FundEnvelopeDlg
#pragma once
