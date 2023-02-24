#include "pch.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include "TransactionImporter.h"
#include "ExcelSerializer.h"
#include "Utils.h"
#include <vector>

TransactionImporter g_transactionImporter;

void deleteAllAccounts()
{
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	for (size_t i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);
	for (size_t i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}

	accounts = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
	for (size_t i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}

	AccountBase::calcNextAccountId();
}

void doCreateAccounts()
{
	// Test create
	Account* pAccount1 = new Account;
	pAccount1->setName(L"acct1");
	pAccount1->addBalance(100);

	Account* pAccount2 = new Account;
	pAccount2->setName(L"acct2");
	pAccount2->addBalance(200);

	Account* pAccount3 = new Account;
	pAccount3->setName(L"acct3");
	pAccount3->addBalance(300);
}

int doSerializeAccounts(bool xlslx)
{
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	if (xlslx) {
		return ExcelSerializer::serialize("test.xlsx", accounts);
	}

	return serialize(accounts, "test.xml");
}

TEST(TestAccounts, TestCreateAccount) {
	doCreateAccounts();

	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_EQ(3, accounts.size());

	Account* pAccount1 = dynamic_cast<Account*>(AccountBase::lookupById(0));
	Account* pAccount2 = dynamic_cast<Account*>(AccountBase::lookupById(1));
	Account* pAccount3 = dynamic_cast<Account*>(AccountBase::lookupById(2));

	EXPECT_TRUE(pAccount1);
	EXPECT_TRUE(pAccount2);
	EXPECT_TRUE(pAccount3);

	deleteAllAccounts();

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());
}

TEST(TestAccounts, TestSerializeAccount) {

	deleteAllAccounts();
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());

	// Set up accounts
	doCreateAccounts();

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_EQ(3, accounts.size());

	// Test serialize
	EXPECT_EQ(ERROR_SUCCESS, doSerializeAccounts(false));
	EXPECT_EQ(ERROR_SUCCESS, doSerializeAccounts(true));

	deleteAllAccounts();
}

void doDeserializeTest(bool xlsx)
{
	// Test deserialize
	deleteAllAccounts();
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());

	std::vector<unsigned int> accountIds;
	doCreateAccounts();
	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);

	for (size_t i = 0; i < accounts.size(); i++) {
		accountIds.push_back(accounts[i]->getId());
	}

	EXPECT_EQ(3, accountIds.size());

	EXPECT_EQ(ERROR_SUCCESS, doSerializeAccounts(xlsx));
	deleteAllAccounts();

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());

	if (xlsx) {
		EXPECT_EQ(ERROR_SUCCESS, ExcelSerializer::deserialize("test.xlsx", accounts));
	}
	else {
		EXPECT_EQ(ERROR_SUCCESS, deserialize("test.xml", accounts));
	}
	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_EQ(3, accounts.size());

	Account* pAccount1 = dynamic_cast<Account*>(AccountBase::lookupById(accountIds[0]));
	Account* pAccount2 = dynamic_cast<Account*>(AccountBase::lookupById(accountIds[1]));
	Account* pAccount3 = dynamic_cast<Account*>(AccountBase::lookupById(accountIds[2]));

	EXPECT_EQ(accountIds[0], pAccount1->getId());
	EXPECT_EQ(accountIds[1], pAccount2->getId());
	EXPECT_EQ(accountIds[2], pAccount3->getId());

	EXPECT_EQ(std::wstring(L"acct1"), pAccount1->getName());
	EXPECT_EQ(std::wstring(L"acct2"), pAccount2->getName());
	EXPECT_EQ(std::wstring(L"acct3"), pAccount3->getName());

	EXPECT_EQ(100, pAccount1->getBalance());
	EXPECT_EQ(200, pAccount2->getBalance());
	EXPECT_EQ(300, pAccount3->getBalance());

	EXPECT_TRUE(pAccount1->getId() < 100);
	EXPECT_TRUE(pAccount2->getId() < 100);
	EXPECT_TRUE(pAccount3->getId() < 100);

	deleteAllAccounts();

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());
}

TEST(TestAccounts, TestDeserializeAccounts) {
	doDeserializeTest(true);
	doDeserializeTest(false);
}

TEST(TestAccounts, TestDeposit) {
	deleteAllAccounts();
	Account* pAccount = new Account;
	Transaction* pTrans = Transaction::addTransaction(Transaction::DEPOSIT, 500, ACCOUNT_ID_INVALID, pAccount->getId(), ACCOUNT_ID_INVALID, nullptr, nullptr);

	EXPECT_EQ(0, pAccount->getBalance());

	pTrans->apply();
	EXPECT_EQ(500, pAccount->getBalance());
	pTrans->undo();
	EXPECT_EQ(0, pAccount->getBalance());
}

TEST(TestAccounts, TestTransfer) {
	deleteAllAccounts();
	Account* pAccount = new Account;
	Account* pAccountB = new Account;
	pAccount->addBalance(500);
	pAccountB->addBalance(100);

	Transaction* pTrans = Transaction::addTransaction(Transaction::TRANSFER, 400, pAccount->getId(), pAccountB->getId(), ACCOUNT_ID_INVALID, nullptr, nullptr);

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(100, pAccountB->getBalance());

	pTrans->apply();
	EXPECT_EQ(100, pAccount->getBalance());
	EXPECT_EQ(500, pAccountB->getBalance());
	pTrans->undo();

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(100, pAccountB->getBalance());
}

TEST(TestAccounts, TestExpense) {
	deleteAllAccounts();
	Account* pAccount = new Account;
	AccountGroup* pFolder = new AccountGroup;
	AccountEnvelope* pExpense = new AccountEnvelope;
	pExpense->addBalance(100);
	pFolder->addChild(pExpense);

	pAccount->addBalance(500);

	Transaction* pTrans = Transaction::addTransaction(Transaction::ENVELOPE, -99, pAccount->getId(), ACCOUNT_ID_INVALID, pExpense->getId(), nullptr, nullptr);

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(100, pExpense->getBalance());

	pTrans->apply();
	EXPECT_EQ(401, pAccount->getBalance());
	EXPECT_EQ(1, pExpense->getBalance());
	pTrans->undo();

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(100, pExpense->getBalance());
}

TEST(TestAccounts, TestExpenseCC) {
	deleteAllAccounts();
	Account* pAccount = new Account;
	Account* pAccountCC = new Account;
	AccountGroup* pFolder = new AccountGroup;
	AccountEnvelope* pExpense = new AccountEnvelope;
	pExpense->addBalance(100);
	pFolder->addChild(pExpense);

	pAccount->addBalance(500);
	pAccountCC->setIsCCAccount(true);
	pAccountCC->setName(L"CC Account");

	Transaction* pTrans = Transaction::addTransaction(Transaction::ENVELOPE, -99, pAccountCC->getId(), ACCOUNT_ID_INVALID, pExpense->getId(), nullptr, nullptr);

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(0, pAccountCC->getBalance());
	EXPECT_EQ(100, pExpense->getBalance());

	pTrans->apply();
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pExpense->getBalance());
	pTrans->undo();

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(0, pAccountCC->getBalance());
	EXPECT_EQ(100, pExpense->getBalance());

	pTrans->apply();
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pExpense->getBalance());

	// Now test paying the CC
	Transaction* pPayCCTrans = Transaction::addTransaction(Transaction::CCPAYMENT, -99, pAccount->getId(), pAccountCC->getId(), ACCOUNT_ID_PAY_CC, pAccount->getName(), L"Pay CC");
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pExpense->getBalance());

	pPayCCTrans->apply();
	EXPECT_EQ(401, pAccount->getBalance());
	EXPECT_EQ(0, pAccountCC->getBalance());
	EXPECT_EQ(1, pExpense->getBalance());

	pPayCCTrans->undo();
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pExpense->getBalance());

}

TEST(TestAccounts, TestIDManipulations) {
	deleteAllAccounts();

	AccountEnvelope* pCat = AccountEnvelope::getOrCreateCCEnvelope();
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ENVELOPE);

	EXPECT_EQ(1, accounts.size());
}

TEST(TestUtils, similarStrings)
{
	size_t similarity = Utils::looksSimilar(L"TST*Killa pie-prosper", L"Kill-A-Pie");
	EXPECT_EQ(3, similarity);

}

TEST(TestUtils, TestDateParse) {

	SYSTEMTIME sys= Utils::dateStringToSysTime(L"December 1 2022");
	EXPECT_EQ(2022, sys.wYear);
	EXPECT_EQ(1, sys.wDay);
	EXPECT_EQ(12, sys.wMonth);

	sys = Utils::dateStringToSysTime(L"5/6/23");
	EXPECT_EQ(2023, sys.wYear);
	EXPECT_EQ(6, sys.wDay);
	EXPECT_EQ(5, sys.wMonth);

	sys = Utils::dateStringToSysTime(L"6 7 2023");
	EXPECT_EQ(6, sys.wMonth);
	EXPECT_EQ(7, sys.wDay);
	EXPECT_EQ(2023, sys.wYear);

	sys = Utils::dateStringToSysTime(L"7 8 24");
	EXPECT_EQ(7, sys.wMonth);
	EXPECT_EQ(8, sys.wDay);
	EXPECT_EQ(2024, sys.wYear);

	sys = Utils::dateStringToSysTime(L"8-9-25");
	EXPECT_EQ(8, sys.wMonth);
	EXPECT_EQ(9, sys.wDay);
	EXPECT_EQ(2025, sys.wYear);

	sys = Utils::dateStringToSysTime(L"10 11 '25");
	EXPECT_EQ(10, sys.wMonth);
	EXPECT_EQ(11, sys.wDay);
	EXPECT_EQ(2025, sys.wYear);

}

TEST(TestUtils, TestDateCompare) {
	SYSTEMTIME a = { 0 }, b = { 0 };

	// Diff by one year
	a.wYear = 2023;
	a.wMonth = 1;
	a.wDay = 1;

	b.wYear = 2022;
	b.wMonth = 1;
	b.wDay = 1;

	EXPECT_EQ(1, Utils::compareDates(a, b));
	EXPECT_EQ(-1, Utils::compareDates(b, a));
	EXPECT_EQ(0, Utils::compareDates(a, a));
	EXPECT_EQ(0, Utils::compareDates(b, b));

	EXPECT_EQ(365, Utils::daysDiff(a, b));

	// Off by one day
	a.wYear = 2022;
	a.wDay = 2;

	EXPECT_EQ(1, Utils::compareDates(a, b));
	EXPECT_EQ(-1, Utils::compareDates(b, a));
	EXPECT_EQ(0, Utils::compareDates(a, a));
	EXPECT_EQ(0, Utils::compareDates(b, b));

	EXPECT_EQ(1, Utils::daysDiff(a, b));

	// Off by one hour
	a.wYear = 2022;
	a.wDay = 1;
	a.wHour = 1;

	EXPECT_EQ(1, Utils::compareDates(a, b));
	EXPECT_EQ(-1, Utils::compareDates(b, a));
	EXPECT_EQ(0, Utils::compareDates(a, a));
	EXPECT_EQ(0, Utils::compareDates(b, b));

	EXPECT_EQ(0, Utils::daysDiff(a, b));
}
