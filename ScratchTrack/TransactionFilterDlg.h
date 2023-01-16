#pragma once

#include <windows.h>
#include <string>


class TransactionFilterDlg
{
public:
	TransactionFilterDlg();
	~TransactionFilterDlg();

	HWND create(HWND hParent);
	operator HWND() { return _hThisDialg;  }

	bool filterToCategorySelection();
	std::wstring filterText();

	static const UINT NM_TRANS_DIALOG_FIRST = 0x8000;
	static const UINT NM_TRANS_DIALOG_FILTER_CHANGED = 0x8000;


protected:
	static INT_PTR CALLBACK TransactionFilterDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	HWND _hParent, _hThisDialg;

};
