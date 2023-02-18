#pragma once
#include <vector>
#include "AccountInfo.h"

struct lxw_workbook;
struct lxw_worksheet;

class ExcelSerializer
{
public:
	static int serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);
	static int deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);

protected:
	static std::string accountTypeToString(AccountBase* pAccount);
	static AccountBase::accountItemType accountTypeFromString(const char* szAccountType, bool& isCC);

	static lxw_worksheet* serializeFrontSheet(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeAccounts(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeIncomes(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeSettings(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeTransactions(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeCategories(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);

	// Do this after serializing other stuff so that locations are captured
	static int writeFrontSheetBudget(lxw_workbook* workbook, lxw_worksheet* worksheet, std::vector<AccountBase*>& topLevelItems);

	inline static const char* ACCOUNT_WORKSHEET_NAME = "Accounts";
	inline static const char* INCOME_WORKSHEET_NAME = "Incomes";
	inline static const char* EXPENSE_WORKSHEET_NAME = "Categories";
	inline static const char* SETTINGS_WORKSHEET_NAME = "Settings";
	inline static const char* FRONT_WORKSHEET_NAME = "Budget";
	inline static const char* TRANSACTIONS_WORKSHEET_NAME = "Transactions";
}; // class ExcelSerializer
