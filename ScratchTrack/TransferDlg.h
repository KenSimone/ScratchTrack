#pragma once

#include <windows.h>
#include <string>


class TransferDlg
{
public:
	TransferDlg(int accountToId, int accountFromId, int amount, const wchar_t* szDescription = nullptr);
	~TransferDlg();

	bool ShowDialog();

	int getAmount() { return _amount; }
	int getAccountToId() { return _accountToId; }
	int getAccountFromId() { return _accountFromId; }
	const wchar_t* getDescription() { return _description.c_str(); }


protected:
	static INT_PTR CALLBACK TransferDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

protected:
	static TransferDlg* PTHIS;
	int _accountToId;
	int _accountFromId;
	int _amount;
	std::wstring _description;
};
