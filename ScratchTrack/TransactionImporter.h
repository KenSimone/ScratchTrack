#pragma once

#include <string>
#include <vector>
#include <map>
#include <string>
#include "TinyXml2.h"
#include "AccountInfo.h"

class Account;

struct ImportedTransactionInfo
{
	ImportedTransactionInfo() : _amount(0), _envelopeId(ACCOUNT_ID_INVALID) {}
	void reset()
	{
		_payee.clear();
		memset(&_date, 0, sizeof(_date));
		_amount = 0;
	}
	std::wstring _payee;
	std::wstring _payeeMapped;
	SYSTEMTIME _date;
	int _amount;
	unsigned int _envelopeId;
};

class Transaction;

class TransactionImporter
{
public:
	TransactionImporter() {};
	~TransactionImporter() {};

	int serialize(tinyxml2::XMLElement* pSettings);
	int deserialize(tinyxml2::XMLElement* pSettings);

	int readTransactions(const wchar_t* szFile, std::vector<ImportedTransactionInfo>& transactions);

	int bestGuessAtPayeeAndCat(const wchar_t* szPayeeIn, std::wstring& payeeOut, unsigned int& catIdOut);

	Transaction* findDuplicate(Account* pAccount, const wchar_t* szPayee, int amount, SYSTEMTIME date, unsigned int maxDateDiff);

	void addMapping(const wchar_t* szItemNameIn, std::wstring itemNameOut, unsigned int envelopeId)
	{
		MAPPED_IMPORT_ITEM mii;
		mii._envelopeId = envelopeId;
		mii._payee = itemNameOut;
		_importMap[szItemNameIn] = mii;
	}

	struct MAPPED_IMPORT_ITEM
	{
		std::wstring _payee;
		unsigned int _envelopeId;
	};

	const std::map<std::wstring, MAPPED_IMPORT_ITEM>& map() { return _importMap; }

protected:

	std::map<std::wstring, MAPPED_IMPORT_ITEM> _importMap;

}; // class TransactionImporter

