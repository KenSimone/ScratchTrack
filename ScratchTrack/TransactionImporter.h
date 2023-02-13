#pragma once

#include <string>
#include <vector>
#include "TinyXml2.h"

struct ImportedTransactionInfo
{
	ImportedTransactionInfo() : _amount(0) {}
	void reset()
	{
		_payee.clear();
		_date.clear();
		_amount = 0;
	}
	std::wstring _payee;
	std::wstring _date;
	int _amount;
};

class TransactionImporter
{
public:
	TransactionImporter() {};
	~TransactionImporter() {};

	int serialize(tinyxml2::XMLElement* pSettings);
	int deserialize(tinyxml2::XMLElement* pSettings);

	int readTransactions(const wchar_t* szFile, std::vector<ImportedTransactionInfo>& transactions);

	int bestGuessAtPayeeAndCat(const wchar_t* szPayeeIn, std::wstring& payeeOut, std::wstring& catOut);

protected:
}; // class TransactionImporter

