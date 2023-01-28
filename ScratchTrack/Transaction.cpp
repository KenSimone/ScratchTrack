#include "Transaction.h"
#include "AccountInfo.h"

#include <vector>
#include <set>


std::vector<Transaction*> _Transactions;


Transaction* Transaction::addTransaction(TRANSACTION_TYPE type, int amount, unsigned int payFromAccountId, unsigned int payToAccountId, unsigned int envelopeId, const wchar_t* szPayee, const wchar_t* szDescription, SYSTEMTIME* pTimeStamp /*= nullptr*/)
{
	Transaction* pTransaction = new Transaction(type, amount, payFromAccountId, payToAccountId, envelopeId, szPayee, szDescription, pTimeStamp);

	_Transactions.push_back(pTransaction);

	return pTransaction;
}

std::vector<Transaction*> Transaction::getTransactionsForItem(AccountBase* pItem)
{
	std::vector<Transaction*> ret;
	std::set<unsigned int> ids;

	if (pItem) {
		pItem->getIdsForSelfAndChildren(ids);
	}

	for (size_t i = 0; i < _Transactions.size(); i++)
	{
		if (ids.find(_Transactions[i]->getEnvelopeId()) != ids.end())
		{
			ret.push_back(_Transactions[i]);
		}
	}
	return ret;
}

std::vector<Transaction*> Transaction::getAllTransactions()
{
	return _Transactions;
}

int Transaction::serialize(tinyxml2::XMLElement* pParent)
{
	tinyxml2::XMLElement* pElement = pParent->GetDocument()->NewElement("transaction");

	pElement->SetAttribute("envelope", _envelopeId);
	pElement->SetAttribute("accountFrom", _payFromAccountId);
	pElement->SetAttribute("accountTo", _payToAccountId);

	char szBuff[1024] = { 0 };
	WideCharToMultiByte(CP_UTF8, 0, _payee.c_str(), -1, szBuff, 1024, nullptr, nullptr);
	pElement->SetAttribute("payee", szBuff);

	WideCharToMultiByte(CP_UTF8, 0, _description.c_str(), -1, szBuff, 1024, nullptr, nullptr);
	pElement->SetAttribute("description", szBuff);

	char szTimestamp[1024] = { 0 };
	snprintf(szTimestamp, 1024, "%02d-%02d-%04d %02d:%02d:%02d", _timestamp.wMonth, _timestamp.wDay, _timestamp.wYear, _timestamp.wHour, _timestamp.wMinute, _timestamp.wSecond);
	pElement->SetAttribute("transactionTime", szTimestamp);

	pElement->SetAttribute("amount", _amount);
	pElement->SetAttribute("type", (int)_type);

	pParent->InsertEndChild(pElement);

	return ERROR_SUCCESS;

}

int Transaction::deserialize(tinyxml2::XMLElement* pRoot)
{
	_Transactions.clear();
	tinyxml2::XMLElement* pTransactions = pRoot->FirstChildElement("transactions");
	if (pTransactions) {
		tinyxml2::XMLElement* pTransaction = pTransactions->FirstChildElement("transaction");
		while (pTransaction) {
			// deserialize
			int envelopeId = 0, fromAccountId = 0, toAccountId = INVALID_ACCOUNT_ID;
			pTransaction->QueryIntAttribute("envelope", &envelopeId);
			pTransaction->QueryIntAttribute("accountFrom", &fromAccountId);
			pTransaction->QueryIntAttribute("accountTo", &toAccountId);


			const char* szPayee, * szDescription;
			wchar_t szPayeeW[1024] = { 0 }, szDescriptionW[1024] = { 0 };
			pTransaction->QueryStringAttribute("payee", &szPayee);
			pTransaction->QueryStringAttribute("description", &szDescription);

			MultiByteToWideChar(CP_UTF8, 0, szPayee, -1, szPayeeW, 1024);
			MultiByteToWideChar(CP_UTF8, 0, szDescription, -1, szDescriptionW, 1024);

			uint64_t timestamp = 0;
			SYSTEMTIME st = { 0 };
			GetSystemTime(&st);

			pTransaction->QueryUnsigned64Attribute("timestamp", &timestamp);
			if (timestamp) {
				FileTimeToSystemTime((FILETIME*)&timestamp, &st);
			}
			else
			{
				const char* szTimestamp = nullptr;
				pTransaction->QueryStringAttribute("transactionTime", &szTimestamp);
				if (szTimestamp) {
					st.wMonth = atoi(szTimestamp);

					szTimestamp = strchr(szTimestamp, '-');
					if (szTimestamp) {
						st.wDay = atoi(++szTimestamp);
					}

					szTimestamp = szTimestamp ? strchr(szTimestamp, '-') : nullptr;
					if (szTimestamp) {
						st.wYear = atoi(++szTimestamp);
					}

					szTimestamp = szTimestamp ? strchr(szTimestamp, ' ') : nullptr;
					if (szTimestamp) {
						st.wHour = atoi(++szTimestamp);
					}

					szTimestamp = szTimestamp ? strchr(szTimestamp, ':') : nullptr;
					if (szTimestamp) {
						st.wMinute = atoi(++szTimestamp);
					}

					szTimestamp = szTimestamp ? strchr(szTimestamp, ':') : nullptr;
					if (szTimestamp) {
						st.wSecond = atoi(++szTimestamp);
					}
				}
			}

			int amount = 0;
			pTransaction->QueryIntAttribute("amount", &amount);

			int type = 0;
			pTransaction->QueryIntAttribute("type", &type);
			Transaction::addTransaction((Transaction::TRANSACTION_TYPE)type, amount, fromAccountId, toAccountId, envelopeId, szPayeeW, szDescriptionW, &st);

			// next
			pTransaction = pTransaction->NextSiblingElement("transaction");
		}
	}

	return ERROR_SUCCESS;

}