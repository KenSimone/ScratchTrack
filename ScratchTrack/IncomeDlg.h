#pragma once

#include <windows.h>
#include <string>

class AccountGroup;

class IncomeDlg
{
public:
	IncomeDlg(const wchar_t* szName, int limit) :
		_name(szName), _limit(limit)
	{
	};

	~IncomeDlg() {};

	bool ShowDialog();

	int getLimit() { return _limit; }
	const wchar_t* getName() { return _name.c_str(); }


protected:
	static INT_PTR CALLBACK IncomeDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void enableDisableUi(HWND hDlg);

protected:
	int _limit;
	std::wstring _name;
	static IncomeDlg* PTHIS;

}; // class IncomeDlg
