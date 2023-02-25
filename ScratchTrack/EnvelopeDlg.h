#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AccountBase;
class Account;

class EnvelopeDialog
{
public:
	EnvelopeDialog();
	~EnvelopeDialog();

	bool ShowDialog();

	// Pre-show functions
	void setEnvelopeName(const wchar_t* szEnvelope) { _name = szEnvelope;  }
	void setBudget(int budget) { _budget = budget; }
	void setDefaultTransactionAmount(int defTransAmt) { _amount = defTransAmt; }
	void setDefaultTransactionPayFromAccount(Account* pDefaultAccount) { _pDefaultTransactionPayFromAccount = pDefaultAccount; }
	void setDefaultTransactionPayee(const wchar_t* szPayee) { _payee = szPayee; }
	void setSavingsGoal(int savingsGoal) { _savingsGoal = savingsGoal; }

	// Post-show functions
	const wchar_t* getEnvelopeName() { return _name.c_str(); }
	int getBudget() { return _budget; }
	int getDefaultTransactionAmount() { return _amount; }
	Account* getDefaultTransactionPayFromAccount() { return _pDefaultTransactionPayFromAccount; }
	const wchar_t* getDefaultTransactionPayee() { return _payee.c_str(); }
	int getSavingsGoal() { return _savingsGoal; }

	void setIsPayCCEnvelope(bool isPayCC) { _isPayCCEnvelope = isPayCC;  }

protected:
	std::wstring _name;
	Account* _pDefaultTransactionPayFromAccount;
	std::wstring _payee;
	std::wstring _expense;
	int _amount;
	int _budget;
	int _savingsGoal;
	bool _isPayCCEnvelope;

	static INT_PTR CALLBACK EnvelopeDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void enableDisableUi(HWND hDlg);

	static EnvelopeDialog* PTHIS;
};

