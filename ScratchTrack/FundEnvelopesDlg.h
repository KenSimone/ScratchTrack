#pragma once

#include <windows.h>
#include <string>

class AccountGroup;

class FundEnvelopesDlg
{
public:
	FundEnvelopesDlg(AccountGroup* pGroups) : _pGroup(pGroups), _resetBalance(false)
	{
	};

	~FundEnvelopesDlg() {};

	bool ShowDialog();

	// If reset, then the new balance becomes the budgeted amount, otherwise add
	// to any existing balance.
	bool getResetBalance() { return _resetBalance; }

protected:
	static INT_PTR CALLBACK FundEnvelopesDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);

protected:
	AccountGroup* _pGroup;
	bool _resetBalance;

	static FundEnvelopesDlg* PTHIS;

}; // class FundEnvelopesDlg
