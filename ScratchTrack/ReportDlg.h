#pragma once

#include <windows.h>
#include <string>

class ReportDlg
{
public:
	ReportDlg(): _hDlg(nullptr)
	{
	};

	~ReportDlg() {};

	void ShowDialog();

protected:
	static INT_PTR CALLBACK ReportDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK StaticReportProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void paintExpenseByCategoryReport(HWND hWnd);
	void initUi();

	void positionConrtols();

protected:
	static ReportDlg* PTHIS;
	HWND _hDlg;

}; // class FolderDlg
