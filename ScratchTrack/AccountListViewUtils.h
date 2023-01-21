#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "AccountInfo.h"

class Account;

LRESULT accountListCustomDraw(LPNMLVCUSTOMDRAW  pCustomDraw);
void createAccountListControl();
void addAccount(Account* pAccount);
Account::ACTION_FLAG hitTestAccountList(POINT pt, Account** ppAccount = nullptr);
void killAccountListToolTips();
