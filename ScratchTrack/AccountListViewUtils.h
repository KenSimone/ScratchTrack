#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

class Account;

LRESULT accountListCustomDraw(LPNMLVCUSTOMDRAW  pCustomDraw);
void createAccountListControl();
void addAccount(Account* pAccount);
