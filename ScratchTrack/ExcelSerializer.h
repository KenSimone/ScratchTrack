#pragma once
#include <vector>
#include "AccountInfo.h"

struct lxw_workbook;

class ExcelSerializer
{
public:
	static int serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);
	static int deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);

protected:
	static std::string accountTypeToString(AccountBase* pAccount);
	static AccountBase::accountItemType accountTypeFromString(const char* szAccountType, bool& isCC);

	static int serializeFrontSheet(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeAccounts(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeIncomes(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeSettings(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeTransactions(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeCategories(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);

	inline static const char* ACCOUNT_WORKSHEET_NAME = "Accounts";
	inline static const char* INCOME_WORKSHEET_NAME = "Incomes";
	inline static const char* EXPENSE_WORKSHEET_NAME = "Categories";
	inline static const char* SETTINGS_WORKSHEET_NAME = "Settings";
	inline static const char* FRONT_WORKSHEET_NAME = "Budget";
	inline static const char* TRANSACTIONS_WORKSHEET_NAME = "Transactions";
}; // class ExcelSerializer
