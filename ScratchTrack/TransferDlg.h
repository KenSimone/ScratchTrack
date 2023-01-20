#pragma once

#include <windows.h>
#include <string>


class TransferDlg
{
public:
	TransferDlg();
	~TransferDlg();

	bool ShowDialog();
	operator HWND() { return _hThisDialg; }


protected:
	static INT_PTR CALLBACK TransferDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	HWND _hParent, _hThisDialg;
	static TransferDlg* PTHIS;
};
