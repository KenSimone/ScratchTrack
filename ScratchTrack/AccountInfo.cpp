#include "AccountInfo.h"
#include "Transaction.h"
#include <map>
#include <limits.h>

static unsigned int _curId = 0;
std::map<unsigned int, AccountBase*> _idToItem;
extern std::vector<Transaction*> _Transactions;

using namespace tinyxml2;

AccountBase* AccountItemFromXmlElement(tinyxml2::XMLElement* pAccount);


AccountBase::AccountBase(accountItemType type) : _balance(0), _budgeted(0)
{
	if (_curId != UINT_MAX) {
		_id = _curId;
		_curId++;

		_idToItem[_id] = this;
	}
};

// Look up the tree item by ID
AccountBase* AccountBase::lookupById(unsigned int id)
{
	auto itr = _idToItem.find(id);
	if (itr != _idToItem.end()) {
		return itr->second;
	}
	return nullptr;
}

void AccountBase::setId(unsigned int id) 
{ 
	_id = id;
	_idToItem[_id] = this;
}


// Subclasses should always call base first thing and then add to ppSerializedElement as needed
int AccountBase::serialize(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement** ppSerializedElement)
{
	int ret = ERROR_SUCCESS;

	// Create our element
	tinyxml2::XMLElement* pElement = pParent->GetDocument()->NewElement("accountItem");

	// Serialize our members
	char szBuff[1024] = { 0 };
	WideCharToMultiByte(CP_UTF8, 0, _name.c_str(), -1, szBuff, 1024, nullptr, nullptr);

	pElement->SetAttribute("name", szBuff);
	pElement->SetAttribute("type", getType());
	pElement->SetAttribute("id", _id);
	pElement->SetAttribute("balance", _balance);
	pElement->SetAttribute("budgeted", _budgeted);

	// Serialize children
	AccountGroup* pGroup = nullptr;
	if (getType() == GROUP) {
		pGroup = (AccountGroup * )this;
	}

	for (size_t i = 0; pGroup && i < pGroup->getChildren().size(); i++)
	{
		pGroup->getChildren()[i]->serialize(pElement, nullptr);
	}

	pParent->InsertEndChild(pElement);

	if (ppSerializedElement) {
		*ppSerializedElement = pElement;
	}

	return ret;
}

int serialize(std::vector<AccountBase*>& topLevelItems, const char* szFilename)
{
	tinyxml2::XMLDocument* pDoc = new tinyxml2::XMLDocument;
	tinyxml2::XMLElement* pRoot = pDoc->NewElement("root");
	tinyxml2::XMLElement* pAccounts = pDoc->NewElement("accounts");
	tinyxml2::XMLElement* pTransactions = pDoc->NewElement("transactions");

	pDoc->InsertFirstChild(pRoot);
	pRoot->InsertFirstChild(pAccounts);
	pRoot->InsertFirstChild(pTransactions);

	// Serialize accounts
	for (size_t i = 0; i < topLevelItems.size(); i++){
		topLevelItems[i]->serialize(pAccounts, nullptr);
	}

	// Serialize Transactions
	std::vector<Transaction*> Transactions = Transaction::getAllTransactions();
	for (size_t i = 0; i < Transactions.size(); i++) {
		Transactions[i]->serialize(pTransactions);
	}

	pDoc->SaveFile(szFilename);

	return ERROR_SUCCESS;
}

int AccountBase::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	// deserialize
	unsigned int id = 0;
	int balance = 0, budgeted = 0;
	pAccount->QueryIntAttribute("budgeted", &budgeted);
	pAccount->QueryIntAttribute("balance", &balance);
	pAccount->QueryUnsignedAttribute("id", &id);

	const char* szName;
	pAccount->QueryStringAttribute("name", &szName);
	wchar_t szNameW[1024] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, szName, -1, szNameW, 1024);

	setName(szNameW);
	addBalance(balance);
	setBudgeted(budgeted);
	setId(id);

	return ERROR_SUCCESS;
}

AccountBase* AccountItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	AccountBase* pItem = nullptr;

	// deserialize
	unsigned int type = 999;
	pAccount->QueryUnsignedAttribute("type", &type);
	switch (type)
	{
	case AccountBase::ACCOUNT:
		pItem = new Account;
		break;
	case AccountBase::GROUP:
		pItem = new AccountGroup;
		break;
	case AccountBase::CATEGORY:
		pItem = new AccountCategory;
		break;
	}

	pItem->initItemFromXmlElement(pAccount);

	return pItem;

}

int deserialize(const char* szFilename, std::vector<AccountBase*>& topLevelItems)
{
	tinyxml2::XMLDocument doc;
	if (XML_SUCCESS != doc.LoadFile(szFilename)) {
		return -1;
	}

	tinyxml2::XMLElement* pRoot = doc.FirstChildElement();
	if (!pRoot) {
		return -1;
	}

	_curId = UINT_MAX;

	Transaction::deserialize(pRoot);

	topLevelItems.clear();
	tinyxml2::XMLElement* pAccounts = pRoot->FirstChildElement("accounts");
	if (pAccounts) {
		tinyxml2::XMLElement* pAccount = pAccounts->FirstChildElement("accountItem");
		while (pAccount) {

			auto pItem = AccountItemFromXmlElement(pAccount);

			if (pItem) {
				topLevelItems.push_back(pItem);
			}
			
			// next
			pAccount = pAccount->NextSiblingElement("accountItem");
		}
	}

	// Find our biggest ID and set the seed to that +1
	unsigned int biggestId = 0;
	for (auto itr = _idToItem.begin(); itr != _idToItem.end(); itr++) {
		biggestId = max(biggestId, itr->second->getId());
	}

	_curId = biggestId + 1;

	return ERROR_SUCCESS;
}

std::vector<AccountBase*> AccountBase::getAllAccountsOfType(accountItemType type)
{
	std::vector<AccountBase*> results;
	for (auto itr = _idToItem.begin(); itr != _idToItem.end(); itr++) {
		if (itr->second->getType() == type) {
			results.push_back(itr->second);
		}
	}

	return results;
}

AccountBase* AccountGroup::getParent()
{
	std::vector<AccountBase*> results;
	for (auto itr = _idToItem.begin(); itr != _idToItem.end(); itr++) {
		if (itr->second->getType() == AccountBase::GROUP)
		{
			AccountGroup* pGroup = (AccountGroup*)itr->second;
			for (auto child = pGroup->_children.begin(); child != pGroup->_children.end(); child++) {
				unsigned int id = (*child)->getId();
				if (id == getId())
				{
					return itr->second;
				}
			}
		}
	}
	return nullptr;
}

// Transaction for self and sub accounts
int AccountBase::getCurrentMonthTransacationTotal()
{
	int ret = 0;

	// Get time as local time
	SYSTEMTIME currentTime = { 0 };
	GetLocalTime(&currentTime);

	// Get all account IDs of interest
	std::set<unsigned int> accountIds;
	getIdsForSelfAndChildren(accountIds);

	// Spin through all transactions
	for (auto itr = _Transactions.begin(); itr != _Transactions.end(); itr++)
	{
		SYSTEMTIME transactionTime = (*itr)->getTimestamp(), localTransactionTime = { 0 };
		SystemTimeToTzSpecificLocalTime(NULL, &transactionTime, &localTransactionTime);

		if ((transactionTime.wYear == currentTime.wYear) &&
			(transactionTime.wMonth == currentTime.wMonth))
		{
			if (accountIds.find((*itr)->getCategoryId()) != accountIds.end())
			{
				ret += (*itr)->getAmount();
			}
		}
	}

	return ret;
}

int AccountGroup::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	AccountBase::initItemFromXmlElement(pAccount);

	tinyxml2::XMLElement* pChild = pAccount->FirstChildElement("accountItem");
	while (pChild)
	{
		AccountBase* pChildItem = AccountItemFromXmlElement(pChild);
		addChild(pChildItem);
		pChild = pChild->NextSiblingElement();
	}
	return ERROR_SUCCESS;
}

int AccountCategory::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	AccountBase::initItemFromXmlElement(pAccount);

	unsigned int accountId = MAXINT, payToAccountId = MAXINT;

	pAccount->QueryUnsignedAttribute("defaultPayFromAccount", &accountId);
	pAccount->QueryUnsignedAttribute("defaultPayToAccount", &payToAccountId);
	pAccount->QueryIntAttribute("defaultAmount", &_defaultTransactionAmount);
	pAccount->QueryBoolAttribute("isExpense", &_isExpense);
	pAccount->QueryBoolAttribute("virtualAccount", &_isVirtualAccount);

	AccountBase* pDefAcct = AccountBase::lookupById(accountId);
	if (pDefAcct && pDefAcct->getType() == ACCOUNT) {
		setDefaultTransactionPayFromAccount((Account*)pDefAcct);
	}

	AccountBase* pDefAcctTo = AccountBase::lookupById(payToAccountId);
	if (pDefAcctTo && pDefAcctTo->getType() == ACCOUNT) {
		setDefaultTransactionPayToAccount((Account*)pDefAcctTo);
	}


	const char* szName;
	pAccount->QueryStringAttribute("defaultPayee", &szName);
	wchar_t szNameW[1024] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, szName, -1, szNameW, 1024);
	
	_defaultTransactionPayee = szNameW;
	return ERROR_SUCCESS;
}

int AccountCategory::serialize(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement** ppSerializedElement)
{
	tinyxml2::XMLElement* pNewElement = nullptr;
	if (!ppSerializedElement) {
		ppSerializedElement = &pNewElement;
	}

	// Add base attributes
	AccountBase::serialize(pParent, ppSerializedElement);

	// Add our own attributes
	char szBuff[1024] = { 0 };
	WideCharToMultiByte(CP_UTF8, 0, _defaultTransactionPayee.c_str(), -1, szBuff, 1024, nullptr, nullptr);

	pNewElement->SetAttribute("defaultPayee", szBuff);
	pNewElement->SetAttribute("defaultAmount", _defaultTransactionAmount);
	pNewElement->SetAttribute("isExpense", _isExpense);
	pNewElement->SetAttribute("virtualAccount", _isVirtualAccount);
	if (_pDefaultTransactionPayFromAccount) {
		pNewElement->SetAttribute("defaultPayFromAccount", _pDefaultTransactionPayFromAccount->getId());
	}
	if (_pDefaultTransactionPayToAccount) {
		pNewElement->SetAttribute("defaultPayToAccount", _pDefaultTransactionPayToAccount->getId());
	}

	return ERROR_SUCCESS;

}

int Account::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	AccountBase::initItemFromXmlElement(pAccount);
	pAccount->QueryBoolAttribute("isBudgeted", &_isBudgeted);
	return ERROR_SUCCESS;
}

int Account::serialize(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement** ppSerializedElement)
{
	tinyxml2::XMLElement* pNewElement = nullptr;
	if (!ppSerializedElement) {
		ppSerializedElement = &pNewElement;
	}

	// Add base attributes
	AccountBase::serialize(pParent, ppSerializedElement);

	// Add our own attributes
	pNewElement->SetAttribute("isBudgeted", _isBudgeted);
	return ERROR_SUCCESS;

}
