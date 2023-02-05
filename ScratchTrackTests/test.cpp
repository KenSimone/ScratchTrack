#include "pch.h"
#include "AccountInfo.h"
#include "Transaction.h"
#include <vector>

void deleteAllAccounts()
{
	std::vector<AccountBase*> accounts = AccountBase::getAllAccountsOfType(AccountBase::ACCOUNT);
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

