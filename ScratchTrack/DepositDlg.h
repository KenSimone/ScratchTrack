#pragma once

#include <windows.h>
#include <string>

class DepositDlg
{
public:
	DepositDlg();
	~DepositDlg();

	HWND create(HWND hParent);
	operator HWND() { return _hThisDialg; }

	bool ShowDialog();

protected:
	static INT_PTR CALLBACK DepositDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	HWND _hParent, _hThisDialg;
	static DepositDlg* PTHIS;

}; // class DepositDlg
