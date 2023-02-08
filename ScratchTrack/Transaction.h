#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "TinyXml2.h"

class AccountBase;


// Transaction class
class Transaction
{
public:
	enum TRANSACTION_TYPE
	{
		EXPENSE,
		DEPOSIT,
		TRANSFER,
		CCPAYMENT
	};

	void apply();
	void undo();	

	static Transaction* addTransaction(TRANSACTION_TYPE type, int amount, unsigned int payFromAccountId, unsigned int payToAccountId, unsigned int categoryId, const wchar_t* szPayee, const wchar_t* szDescription, SYSTEMTIME* pTimeStamp = nullptr);
	static int deserialize(tinyxml2::XMLElement* pRoot);

	static std::vector<Transaction*> getTransactionsForItem(AccountBase* pItem);
	static std::vector<Transaction*> getAllTransactions();

	int getAmount() { return _amount; }
	unsigned int getCategoryId() { return _categoryId; }
	unsigned int getAccountFromId() { return _payFromAccountId; }
	unsigned int getAccountToId() { return _payToAccountId; }
	const wchar_t* getPayee() { return _payee.c_str(); }
	const wchar_t* getDescription() { return _description.c_str(); }
	TRANSACTION_TYPE getType() { return _type; }

	void setDetails(int amount, const wchar_t* szPayee, const wchar_t* szDesc, int categoryId, int accountFromId, int accountToId)
	{
		_amount = amount;
		_payee = szPayee;
		_description = szDesc;
		_categoryId = categoryId;
		_payToAccountId = accountToId;
		_payFromAccountId = accountFromId;
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
	Transaction(TRANSACTION_TYPE type, int amount, unsigned int payFromAccountId, unsigned int payToAccountId, unsigned int categoryId, const wchar_t* szPayee, const wchar_t* szDescription, SYSTEMTIME* pTimeStamp = nullptr)
	{
		if (pTimeStamp) {
			_timestamp = *pTimeStamp;
		}
		else {
			GetSystemTime(&_timestamp);
		}

		_amount = amount;
		_categoryId = categoryId;
		_payFromAccountId = payFromAccountId;
		_payToAccountId = payToAccountId;
		_type = type;
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
	unsigned int _payFromAccountId;
	unsigned int _payToAccountId;
	std::wstring _payee;
	std::wstring _description;
	SYSTEMTIME _timestamp;
	TRANSACTION_TYPE _type;

}; // class Transaction

