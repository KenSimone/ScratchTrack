#include "pch.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include "TransactionImporter.h"
#include "Utils.h"
#include <vector>

TransactionImporter g_transactionImporter;

void deleteAllAccounts()
{
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	for (size_t i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}

	accounts = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);
	for (size_t i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}

	accounts = AccountBase::getAllAccountsOfType(AccountBase::GROUP);
	for (size_t i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}
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

int doSerializeAccounts()
{
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
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
	EXPECT_EQ(ERROR_SUCCESS, doSerializeAccounts());

	deleteAllAccounts();
}

TEST(TestAccounts, TestDeserializeAccounts) {

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

	EXPECT_EQ(ERROR_SUCCESS, doSerializeAccounts());
	deleteAllAccounts();

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());

	EXPECT_EQ(ERROR_SUCCESS, deserialize("test.xml", accounts));
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

	EXPECT_TRUE(pAccount1->getId() < 10);
	EXPECT_TRUE(pAccount2->getId() < 10);
	EXPECT_TRUE(pAccount3->getId() < 10);

	deleteAllAccounts();

	accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
	EXPECT_TRUE(accounts.empty());
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
	AccountCategory* pCategory = new AccountCategory;
	pCategory->addBalance(100);
	pFolder->addChild(pCategory);

	pAccount->addBalance(500);

	Transaction* pTrans = Transaction::addTransaction(Transaction::EXPENSE, -99, pAccount->getId(), ACCOUNT_ID_INVALID, pCategory->getId(), nullptr, nullptr);

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(100, pCategory->getBalance());

	pTrans->apply();
	EXPECT_EQ(401, pAccount->getBalance());
	EXPECT_EQ(1, pCategory->getBalance());
	pTrans->undo();

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(100, pCategory->getBalance());
}

TEST(TestAccounts, TestExpenseCC) {
	deleteAllAccounts();
	Account* pAccount = new Account;
	Account* pAccountCC = new Account;
	AccountGroup* pFolder = new AccountGroup;
	AccountCategory* pCategory = new AccountCategory;
	pCategory->addBalance(100);
	pFolder->addChild(pCategory);

	pAccount->addBalance(500);
	pAccountCC->setIsCCAccount(true);
	pAccountCC->setName(L"CC Account");

	Transaction* pTrans = Transaction::addTransaction(Transaction::EXPENSE, -99, pAccountCC->getId(), ACCOUNT_ID_INVALID, pCategory->getId(), nullptr, nullptr);

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(0, pAccountCC->getBalance());
	EXPECT_EQ(100, pCategory->getBalance());

	pTrans->apply();
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pCategory->getBalance());
	pTrans->undo();

	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(0, pAccountCC->getBalance());
	EXPECT_EQ(100, pCategory->getBalance());

	pTrans->apply();
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pCategory->getBalance());

	// Now test paying the CC
	Transaction* pPayCCTrans = Transaction::addTransaction(Transaction::CCPAYMENT, -99, pAccount->getId(), pAccountCC->getId(), ACCOUNT_ID_PAY_CC, pAccount->getName(), L"Pay CC");
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pCategory->getBalance());

	pPayCCTrans->apply();
	EXPECT_EQ(401, pAccount->getBalance());
	EXPECT_EQ(0, pAccountCC->getBalance());
	EXPECT_EQ(1, pCategory->getBalance());

	pPayCCTrans->undo();
	EXPECT_EQ(500, pAccount->getBalance());
	EXPECT_EQ(-99, pAccountCC->getBalance());
	EXPECT_EQ(1, pCategory->getBalance());

}

TEST(TestAccounts, TestIDManipulations) {
	deleteAllAccounts();

	AccountCategory* pCat = AccountCategory::getOrCreateCCCategory();
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::CATEGORY);

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
