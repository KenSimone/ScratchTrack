#pragma once

#include <windows.h>
#include <string>


class AccountInfoDlg
{
public:
	AccountInfoDlg();
	~AccountInfoDlg();

	HWND create(HWND hParent);
	operator HWND() { return _hThisDialg; }


	static const UINT NM_ACCOUNT_INFO_DIALOG_FIRST = 0x8000;
	static const UINT NM_ACCOUNT_INFO_DIALOG_ADD_ACCOUNT = NM_ACCOUNT_INFO_DIALOG_FIRST + 0;
	static const UINT NM_ACCOUNT_DIALOG_ADD_FOLDER = NM_ACCOUNT_INFO_DIALOG_FIRST + 1;


protected:
	static INT_PTR CALLBACK AccountInfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	HWND _hParent, _hThisDialg;

};
