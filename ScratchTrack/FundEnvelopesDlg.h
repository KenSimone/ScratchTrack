#pragma once

#include <windows.h>
#include <string>

class AccountGroup;

class FundEnvelopesDlg
{
public:
	FundEnvelopesDlg(AccountGroup* pGroups) : _pGroup(pGroups)
	{
	};

	~FundEnvelopesDlg() {};

	bool ShowDialog();

protected:
	static INT_PTR CALLBACK FundEnvelopesDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void updateUi(HWND hDlg);

protected:
	AccountGroup* _pGroup;

	static FundEnvelopesDlg* PTHIS;

}; // class FundEnvelopesDlg
