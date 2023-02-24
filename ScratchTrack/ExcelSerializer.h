#pragma once
#include <vector>
#include <map>
#include <string>
#include "AccountInfo.h"
#include "Transaction.h"

struct lxw_workbook;
struct lxw_worksheet;
struct lxw_format;

namespace OpenXLSX
{
	class XLWorkbook;
	class XLWorksheet;
};

class ExcelSerializer
{
public:
	static int serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);
	static int deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);

	static const char* libxslErrorToText(int error);

protected:
	static std::string accountTypeToString(AccountBase* pAccount);
	static AccountBase::accountItemType accountTypeFromString(const char* szAccountType, bool& isCC);

	static std::string transactionTypeToString(Transaction* pTransaction);
	static Transaction::TRANSACTION_TYPE transactionTypeFromString(const char* szTransactionType);

	static lxw_worksheet* serializeFrontSheet(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems);
	static int serializeAccounts(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef);
	static int serializeFolders(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef);
	static int serializeSettings(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef);
	static int serializeTransactions(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef);
	static int serializeCategories(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef);

	// Do this after serializing other stuff so that locations are captured
	static int writeFrontSheetBudget(lxw_workbook* workbook, lxw_worksheet* worksheet, std::vector<AccountBase*>& topLevelItems);

	// Deserialize stuff
	static int deserializeAccounts(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId);
	static int deserializeFolders(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId);

	static int deserializeSettings(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId);
	static int deserializeTransactions(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId);
	static int deserializeCategories(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId);

	// Handy helpers
	static void writeDate(SYSTEMTIME date, lxw_worksheet* worksheet, int row, int col, lxw_format* pFormat);
	static SYSTEMTIME readDate(OpenXLSX::XLWorksheet& worksheet, int row, int col);

	inline static const char* ACCOUNT_WORKSHEET_NAME = "Accounts";
	inline static const char* FOLDER_WORKSHEET_NAME = "Folders";
	inline static const char* ENVELOPE_WORKSHEET_NAME = "Categories";
	inline static const char* SETTINGS_WORKSHEET_NAME = "Settings";
	inline static const char* FRONT_WORKSHEET_NAME = "Budget";
	inline static const char* TRANSACTIONS_WORKSHEET_NAME = "Transactions";

protected:
	static void addAccountLocationToMap(int row, int col, OpenXLSX::XLWorksheet& worksheet, AccountBase* pAccount, std::map<std::string, unsigned int>& cellRefToAccountId);

	// Functions to deal with bugs / deficiencies in OpenXLSX
	static void excelSerialDateToDMY(int nSerialDate, int& nDay, int& nMonth, int& nYear);
	static int pennyValueFromCell(int row, int col, OpenXLSX::XLWorksheet& worksheet);

protected:
	static const unsigned int FLAG_NO_FLAGS = 0;
	static const unsigned int FLAG_PAYS_CCS = 1;
}; // class ExcelSerializer
