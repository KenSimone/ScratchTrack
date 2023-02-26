#pragma once

#include <windows.h>
#include <string>
#include <map>

class ReportDlg
{
public:
	ReportDlg(): _hDlg(nullptr), _displayed30dayWarn(false)
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

	void populateGrid();

	std::map<unsigned int, int> getEnvelopeIdsToTotalSpentMap(int* pElapsedDays, int* pElapsedMonths);

protected:
	static ReportDlg* PTHIS;
	HWND _hDlg;

	static const unsigned int GRAPH_STYLE_BAR = 1;
	static const unsigned int GRAPH_STYLE_GRID = 2;

	bool _displayed30dayWarn;

}; // class FolderDlg
