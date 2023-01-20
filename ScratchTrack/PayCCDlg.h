#pragma once

#include <windows.h>
#include <string>

class PayCCDlg
{
public:
	PayCCDlg();
	~PayCCDlg();

	bool ShowDialog();
	operator HWND() { return _hThisDialg; }


protected:
	static INT_PTR CALLBACK PayCCDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	HWND _hParent, _hThisDialg;
	static PayCCDlg* PTHIS;

}; // class PayCCDlg
