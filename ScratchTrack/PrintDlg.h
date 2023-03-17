#pragma once

#include <windows.h>
#include <string>


class ScratchTrackPrintDlg
{
public:
	ScratchTrackPrintDlg(){};

	~ScratchTrackPrintDlg();

	bool ShowDialog();

protected:
	void populateReport(HWND hDlg);
	static INT_PTR CALLBACK ScratchTrackPrintDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL PrintRTF(HWND hwnd, HDC hdc);
	void appendText(HWND hTextControl, const wchar_t* szText, COLORREF cr, bool bold, bool underline, int sizeInPoints, COLORREF crBackColor);

protected:

	static ScratchTrackPrintDlg* PTHIS;

}; // class ScratchTrackPrintDlg
