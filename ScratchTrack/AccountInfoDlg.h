#pragma once

#include <windows.h>
#include <string>
#include <vector>

class AccountInfoDlg
{
public:
	AccountInfoDlg();
	~AccountInfoDlg();

	HWND create(HWND hParent);
	operator HWND() { return _hThisDialg; }


	static const UINT NM_ACCOUNT_INFO_DIALOG_FIRST = 0x8000;
	static const UINT NM_ACCOUNT_INFO_DIALOG_ADD_ACCOUNT = NM_ACCOUNT_INFO_DIALOG_FIRST + 0;
	static const UINT NM_ACCOUNT_DIALOG_ADD_INCOME = NM_ACCOUNT_INFO_DIALOG_FIRST + 1;

	void updateReport();

	// Captures the text in the dialog, for later restoration
	// This is so dialogs can show help here.
	void saveText();
	void restoreText();
	void appendText(const wchar_t* szText, COLORREF cr, bool bold, bool underline, int sizeInPoints, bool deleteExisting = false);

	static const int HEADER_SIZE = 12;
	static const int TEXT_SIZE = 10;
	static const COLORREF TEXT_COLOR_NORMAL = RGB(0, 0, 0);
	static const COLORREF TEXT_COLOR_NEGATIVE = RGB(255, 0, 0);


protected:
	static DWORD CALLBACK EditStreamCallbackOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb);
	static DWORD CALLBACK EditStreamCallbackIn(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb);
	static INT_PTR CALLBACK AccountInfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void printMasterSummary();
	void doUnhidePopup();

protected:
	HWND _hParent, _hThisDialg;

	std::vector<unsigned char> _textBuff;

};
