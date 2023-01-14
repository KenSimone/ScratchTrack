#include "Transaction.h"
#include "AccountInfo.h"

#include <vector>
#include <set>


std::vector<Transaction*> _Transactions;


Transaction* Transaction::addTransaction(int amount, unsigned int accountId, unsigned int categoryId, const wchar_t* szPayee, const wchar_t* szDescription, SYSTEMTIME* pTimeStamp /*= nullptr*/)
{
	Transaction* pTransaction = new Transaction(amount, accountId, categoryId, szPayee, szDescription, pTimeStamp);

	_Transactions.push_back(pTransaction);

	return pTransaction;
}

std::vector<Transaction*> Transaction::getTransactionsForItem(AccountBase* pItem)
{
	std::vector<Transaction*> ret;
	std::set<unsigned int> ids;
	pItem->getIdsForSelfAndChildren(ids);

	for (size_t i = 0; i < _Transactions.size(); i++)
	{
		if (ids.find(_Transactions[i]->getCategoryId()) != ids.end())
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

	pElement->SetAttribute("category", _categoryId);
	pElement->SetAttribute("account", _accountId);

	char szBuff[1024] = { 0 };
	WideCharToMultiByte(CP_UTF8, 0, _payee.c_str(), -1, szBuff, 1024, nullptr, nullptr);
	pElement->SetAttribute("payee", szBuff);

	WideCharToMultiByte(CP_UTF8, 0, _description.c_str(), -1, szBuff, 1024, nullptr, nullptr);
	pElement->SetAttribute("description", szBuff);

	uint64_t ft = 0;
	SystemTimeToFileTime(&_timestamp, (FILETIME*)&ft);
	pElement->SetAttribute("timestamp", ft);

	pElement->SetAttribute("amount", _amount);

	pParent->InsertEndChild(pElement);

	return ERROR_SUCCESS;

}

