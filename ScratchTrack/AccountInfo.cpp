#include "AccountInfo.h"
#include "Transaction.h"
#include "Utils.h"
#include "Version.h"
#include "TransactionImporter.h"
#include <map>
#include <limits.h>

std::map<unsigned int, AccountBase*> _idToItem;
extern std::vector<Transaction*> _Transactions;
extern TransactionImporter g_transactionImporter;

using namespace tinyxml2;

AccountBase* AccountItemFromXmlElement(tinyxml2::XMLElement* pAccount);


AccountBase::AccountBase(accountItemType type) : _balance(0), _budgeted(0), _hidden(false), _readOnly(false), _id(ACCOUNT_ID_INVALID)
{
	if (_curId != ACCOUNT_ID_INVALID) {
		_id = _curId;
		_curId++;

		_idToItem[_id] = this;
	}
};

AccountBase::~AccountBase()
{
	auto itr = _idToItem.find(getId());
	if (itr != _idToItem.end()) {
		_idToItem.erase(itr);
	}
}

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
	_idToItem.erase(_id);
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
	pElement->SetAttribute("hidden", _hidden);
	pElement->SetAttribute("readOnly", _readOnly);

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
	tinyxml2::XMLElement* pSettings = pDoc->NewElement("settings");
	tinyxml2::XMLElement* pAccounts = pDoc->NewElement("accounts");
	tinyxml2::XMLElement* pTransactions = pDoc->NewElement("transactions");

	pDoc->InsertFirstChild(pRoot);
	pRoot->InsertFirstChild(pAccounts);
	pRoot->InsertFirstChild(pTransactions);
	pRoot->InsertFirstChild(pSettings);

	pRoot->SetAttribute("version", VER_STRING);

	g_transactionImporter.serialize(pSettings);

	// Serialize accounts
	for (size_t i = 0; i < topLevelItems.size(); i++){
		topLevelItems[i]->serialize(pAccounts, nullptr);
	}

	// Serialize Transactions
	std::vector<Transaction*> Transactions = Transaction::getAllTransactions();
	for (size_t i = 0; i < Transactions.size(); i++) {
		Transactions[i]->serialize(pTransactions);
	}

	Utils::doBackups();
	Utils::monitorDataFileForChanges(true);
	pDoc->SaveFile(szFilename);
	Utils::monitorDataFileForChanges(false);

	return ERROR_SUCCESS;
}

int AccountBase::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	// deserialize
	unsigned int id = 0;
	int balance = 0, budgeted = 0;
	bool hidden = false, readOnly = false;
	pAccount->QueryIntAttribute("budgeted", &budgeted);
	pAccount->QueryIntAttribute("balance", &balance);
	pAccount->QueryUnsignedAttribute("id", &id);
	pAccount->QueryBoolAttribute("hidden", &hidden);
	pAccount->QueryBoolAttribute("readOnly", &readOnly);


	const char* szName;
	pAccount->QueryStringAttribute("name", &szName);
	wchar_t szNameW[1024] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, szName, -1, szNameW, 1024);

	setName(szNameW);
	addBalance(balance);
	setBudgeted(budgeted);
	setId(id);
	setHidden(hidden);
	setReadOnly(readOnly);

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
	case AccountBase::ENVELOPE:
		pItem = new AccountEnvelope;
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

	AccountBase::startDeserialize();

	Transaction::deserialize(pRoot);

	tinyxml2::XMLElement* pSettings = pRoot->FirstChildElement("settings");
	g_transactionImporter.deserialize(pSettings);

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

	AccountBase::stopDeserialize();

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

AccountBase* AccountBase::getParent()
{
	std::vector<AccountBase*> results;
	for (auto itr = _idToItem.begin(); itr != _idToItem.end(); itr++) {
		AccountGroup* pGroup = dynamic_cast<AccountGroup*>(itr->second);
		if (pGroup)
		{
			std::vector<AccountBase*> children = pGroup->getChildren();
			for (auto child = children.begin(); child != children.end(); child++) {
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
		SYSTEMTIME transactionTime = (*itr)->getTimestamp();

		if ((transactionTime.wYear == currentTime.wYear) &&
			(transactionTime.wMonth == currentTime.wMonth))
		{
			if (accountIds.find((*itr)->getEnvelopeId()) != accountIds.end())
			{
				ret += (*itr)->getAmount();
			}
		}
	}

	return ret;
}

int AccountGroup::serialize(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement** ppSerializedElement)
{
	// Serialize base
	tinyxml2::XMLElement* pGroupElement = nullptr;
	AccountBase::serialize(pParent, &pGroupElement);

	// Envelope limit
	pGroupElement->SetAttribute("expenseLimit", getExpenseLimit());

	// Serialize children
	for (size_t i = 0; i < getChildren().size(); i++) {
		getChildren()[i]->serialize(pGroupElement, nullptr);
	}

	if (ppSerializedElement) {
		*ppSerializedElement = pGroupElement;
	}

	return ERROR_SUCCESS;
}

int AccountGroup::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	AccountBase::initItemFromXmlElement(pAccount);

	int limit = 0;
	pAccount->QueryIntAttribute("expenseLimit", &limit);
	setExpenseLimit(limit);


	tinyxml2::XMLElement* pChild = pAccount->FirstChildElement("accountItem");
	while (pChild)
	{
		AccountBase* pChildItem = AccountItemFromXmlElement(pChild);
		addChild(pChildItem);
		pChild = pChild->NextSiblingElement();
	}
	return ERROR_SUCCESS;
}

int AccountEnvelope::initItemFromXmlElement(tinyxml2::XMLElement* pAccount)
{
	AccountBase::initItemFromXmlElement(pAccount);

	unsigned int accountId = ACCOUNT_ID_INVALID, payToAccountId = ACCOUNT_ID_INVALID;

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

int AccountEnvelope::serialize(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement** ppSerializedElement)
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
	pAccount->QueryBoolAttribute("getContributesToBudget", &_isBudgeted);
	pAccount->QueryBoolAttribute("isCCAccount", &_isCCAccount);

	unsigned int temp = ACCOUNT_ID_INVALID;
	pAccount->QueryUnsignedAttribute("paidFromenvelopeId", &temp);
	if (temp != ACCOUNT_ID_INVALID) {
		_isCCAccount = true;
	}

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
	pNewElement->SetAttribute("getContributesToBudget", _isBudgeted);
	pNewElement->SetAttribute("isCCAccount", _isCCAccount);
	return ERROR_SUCCESS;
}

int AccountEnvelope::getAverageDailySpend(int& totalDaysRecorded, int& averageMonthly)
{
	auto transactions = Transaction::getAllTransactions();

	SYSTEMTIME oldest = { 0 }, newest = { 0 };

	// This is debatable, but newest is the last day of last month, which is bit of a hassle to get

	// Get the current date/time
	GetLocalTime(&newest);

	// Clear all but year and month and set day to one
	WORD year = newest.wYear; WORD month = newest.wMonth;
	memset(&newest, 0, sizeof(newest));
	newest.wMonth = month; newest.wYear = year; newest.wDay = 1;

	// And subtract one day from that
	Utils::subtractDays(newest, 1);

	// Now spin through each tranbsaction and accumulate the amount
	int total = 0;
	for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
		Transaction* pTrans = *itr;

		if (pTrans->getEnvelopeId() == getId() && 
			pTrans->getType() == Transaction::EXPENSE ) 
		{
			SYSTEMTIME ts = pTrans->getTimestamp();

			// Don't include the current month in totals
			if (Utils::compareDates(ts, newest) > 0) {
				continue; 
			}

			if (oldest.wYear == 0) {
				oldest = ts;
			}
			else if (Utils::compareDates(oldest, ts) > 0) {				
				oldest = ts;
			}

			total += pTrans->getAmount();
		}
	} // for each transaction

	// Figure out how many months we saw
	int monthsSeen = 0;
	for (int y = oldest.wYear; y < newest.wYear; y++) {
		for (int m = oldest.wMonth; m <= 12; m++) {
			monthsSeen++;
		}
	}
	monthsSeen += newest.wMonth;


	averageMonthly = (int)(total / (double)monthsSeen);
	if (oldest.wYear && newest.wYear) {
		totalDaysRecorded = Utils::daysDiff(oldest, newest);
	}

	if (totalDaysRecorded) {
		total /= totalDaysRecorded;
	}

	return total;
}

AccountEnvelope* AccountEnvelope::getOrCreateCCEnvelope()
{
	AccountEnvelope* pEnvelope = dynamic_cast<AccountEnvelope*>(lookupById(ACCOUNT_ID_PAY_CC));
	if (!pEnvelope) {
		pEnvelope = new AccountEnvelope;
		pEnvelope->setId(ACCOUNT_ID_PAY_CC);
	}
	return pEnvelope;
}

unsigned int AccountBase::_curId = 0;


void AccountBase::calcNextAccountId()
{
	// Find our biggest ID and set the seed to that +1
	unsigned int biggestId = 0;
	for (auto itr = _idToItem.begin(); itr != _idToItem.end(); itr++) {
		int itemId = itr->second->getId();
		if (itemId != ACCOUNT_ID_PAY_CC && itemId != ACCOUNT_ID_INVALID) {
			biggestId = itemId;
		}
	}

	_curId = biggestId + 1;
}

void AccountBase::stopDeserialize()
{
	if (_curId != ACCOUNT_ID_INVALID){
		return;
	}

	calcNextAccountId();
}