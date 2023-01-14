#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "TinyXml2.h"

class AccountTreeItem;


// Transaction class
class Transaction
{
public:
	static Transaction* addTransaction(int amount, unsigned int accountId, unsigned int categoryId, const wchar_t* szPayee, const wchar_t* szDescription, SYSTEMTIME* pTimeStamp = nullptr);

	static std::vector<Transaction*> getTransactionsForItem(AccountTreeItem* pItem);
	static std::vector<Transaction*> getAllTransactions();

	int getAmount() { return _amount; }
	unsigned int getCategoryId() { return _categoryId; }
	unsigned int getAccountId() { return _accountId; }
	const wchar_t* getPayee() { return _payee.c_str(); }
	const wchar_t* getDescription() { return _description.c_str(); }

	void setDetails(int amount, const wchar_t* szPayee, const wchar_t* szDesc)
	{
		_amount = amount;
		_payee = szPayee;
		_description = szDesc;
	}

	SYSTEMTIME getTimestamp() { return _timestamp; }
	void getLocalTimeStamp(std::wstring& result)
	{
		SYSTEMTIME localTime = { 0 };
		SystemTimeToTzSpecificLocalTime(NULL, &_timestamp, &localTime);

		wchar_t szBuff[1024] = { 0 };
		GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &localTime, nullptr, szBuff, 1024, nullptr);

		result = szBuff;
	}

	virtual int serialize(tinyxml2::XMLElement* pParent);

protected:
	Transaction(int amount, unsigned int accountId, unsigned int categoryId, const wchar_t* szPayee, const wchar_t* szDescription, SYSTEMTIME* pTimeStamp = nullptr)
	{
		if (pTimeStamp) {
			_timestamp = *pTimeStamp;
		}
		else {
			GetSystemTime(&_timestamp);
		}

		_amount = amount;
		_categoryId = categoryId;
		_accountId = accountId;
		if (szPayee) {
			_payee = szPayee;
		}

		if (szDescription) {
			_description = szDescription;
		}
	}

protected:
	int _amount;
	unsigned int _categoryId;
	unsigned int _accountId;
	std::wstring _payee;
	std::wstring _description;
	SYSTEMTIME _timestamp;

}; // class Transaction

