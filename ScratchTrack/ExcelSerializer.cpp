#include "ExcelSerializer.h"
#include <windows.h>
#include "AccountInfo.h"
#include "Transaction.h"
#include "Utils.h"
#include "OpenXLSX.hpp"
#include "xlsxwriter.h"
#include "Version.h"
#include "Transaction.h"
#include "TransactionImporter.h"
#include <assert.h>
#include <map>

extern TransactionImporter g_transactionImporter;

using namespace OpenXLSX;

int ExcelSerializer::serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems)
{
	std::map<unsigned int, std::string> accountIdToCellRef;

	int ret = ERROR_SUCCESS;

	lxw_workbook* workbook = workbook_new(szFile);

	lxw_worksheet* pBudgetSheet = serializeFrontSheet(workbook, topLevelItems);
	ret = serializeAccounts(workbook, topLevelItems, accountIdToCellRef);
	if (ret != ERROR_SUCCESS ) {
		goto end;
	}
	ret = serializeFolders(workbook, topLevelItems, accountIdToCellRef);
	if (ret != ERROR_SUCCESS) {
		goto end;
	}
	ret = serializeCategories(workbook, topLevelItems, accountIdToCellRef);
	if (ret != ERROR_SUCCESS) {
		goto end;
	}
	ret = serializeTransactions(workbook, topLevelItems, accountIdToCellRef);
	if (ret != ERROR_SUCCESS) {
		goto end;
	}
	ret = serializeSettings(workbook, topLevelItems, accountIdToCellRef);
	if (ret != ERROR_SUCCESS) {
		goto end;
	}
	ret = writeFrontSheetBudget(workbook, pBudgetSheet, topLevelItems);
	if (ret != ERROR_SUCCESS) {
		goto end;
	}

end:
	if (workbook) {

		Utils::doBackups();
		Utils::monitorDataFileForChanges(true);
		ret = workbook_close(workbook);
		Utils::monitorDataFileForChanges(false);
	}


	return ret;
}

std::string ExcelSerializer::accountTypeToString(AccountBase* pAccount)
{
	std::string ret;
	switch (pAccount->getType())
	{
	case AccountBase::GROUP:
		ret = "folder";
		break;
	case AccountBase::EXPENSE:
		ret = "expense";
		break;
	}

	Account* p = dynamic_cast<Account*>(pAccount);
	if (p)
	{
		if (p->isCCAccount())
			ret = "credit card";
		else
			ret = "account";
	}

	return ret;
}

AccountBase::accountItemType ExcelSerializer::accountTypeFromString(const char* szAccountType, bool& isCC)
{
	isCC = false;

	if (_stricmp(szAccountType, "account") == 0)
		return AccountBase::ACCOUNT;
	if (_stricmp(szAccountType, "folder") == 0)
		return AccountBase::GROUP;
	if (_stricmp(szAccountType, "expense") == 0)
		return AccountBase::EXPENSE;
	if (_stricmp(szAccountType, "credit card") == 0) {
		isCC = true;
		return AccountBase::ACCOUNT;
	}

	assert(false);

	return AccountBase::ACCOUNT;
}

int ExcelSerializer::serializeAccounts(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, ACCOUNT_WORKSHEET_NAME);

	lxw_format* formatMoney = workbook_add_format(workbook);
	format_set_num_format(formatMoney, "$#,##0.00");
	formatMoney->text_h_align = LXW_ALIGN_CENTER;
	formatMoney->bg_color = RGB(238, 238, 238);

	uint16_t curRow = 0;

	lxw_format* formatHeader = workbook_add_format(workbook);
	lxw_format* formatLeft = workbook_add_format(workbook);

	formatLeft->text_h_align = LXW_ALIGN_LEFT;
	formatLeft->bg_color = RGB(238, 238, 238);

	lxw_format* formatDate = workbook_add_format(workbook);
	formatDate->text_h_align = LXW_ALIGN_LEFT;
	formatDate->bg_color = RGB(238, 238, 238);
	format_set_num_format(formatDate, "m/d/yyyy");

	formatHeader->bold = true;
	formatHeader->bg_color = RGB(200, 200, 200);
	formatHeader->fg_color = RGB(0, 0, 0);
	formatHeader->text_h_align = LXW_ALIGN_LEFT;

	// Write accounts
	worksheet_write_string(worksheet, curRow, 0, "Account name", formatHeader);
	worksheet_write_string(worksheet, curRow, 1, "Type", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Balance", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "InBudget", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "Hidden", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);
	curRow++;

	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::ACCOUNT) {
			continue;
		}

		Account* p = dynamic_cast<Account*>(topLevelItems[i]);

		worksheet_write_string(worksheet, curRow, 0, Utils::wideToMultibyte(p->getName()).c_str(), formatLeft);
		worksheet_write_string(worksheet, curRow, 1, accountTypeToString(p).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 2, _wtof(Utils::penniesToDollarsString(p->getBalance(), false).c_str()), formatMoney);
		worksheet_write_string(worksheet, curRow, 3, p->getContributesToBudget() ? "yes" : "no", formatLeft);
		worksheet_write_string(worksheet, curRow, 4, p->getHidden() ? "yes" : "no", formatLeft);

		char szBuff[1024] = { 0 };
		sprintf_s(szBuff, "=Accounts!A%d", curRow+1);
		accountIdToCellRef[p->getId()] = szBuff;

		curRow++;
	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeFolders(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, FOLDER_WORKSHEET_NAME);

	uint16_t curRow = 0;

	lxw_format* formatHeader = workbook_add_format(workbook);
	lxw_format* formatLeft = workbook_add_format(workbook);

	formatLeft->text_h_align = LXW_ALIGN_LEFT;
	formatLeft->bg_color = RGB(238, 238, 238);

	lxw_format* formatDate = workbook_add_format(workbook);
	formatDate->text_h_align = LXW_ALIGN_LEFT;
	formatDate->bg_color = RGB(238, 238, 238);
	format_set_num_format(formatDate, "m/d/yyyy");

	formatHeader->bold = true;
	formatHeader->bg_color = RGB(200, 200, 200);
	formatHeader->fg_color = RGB(0, 0, 0);
	formatHeader->text_h_align = LXW_ALIGN_LEFT;

	lxw_format* formatMoney = workbook_add_format(workbook);
	format_set_num_format(formatMoney, "$#,##0.00");
	formatMoney->text_h_align = LXW_ALIGN_CENTER;
	formatMoney->bg_color = RGB(238, 238, 238);

	// Write folders
	worksheet_write_string(worksheet, curRow, 0, "Folder name", formatHeader);
	worksheet_write_string(worksheet, curRow, 1, "Folder", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Hidden", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "Read Only", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);

	curRow++;
	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::GROUP) {
			continue;
		}

		AccountGroup* p = dynamic_cast<AccountGroup*>(topLevelItems[i]);

		worksheet_write_string(worksheet, curRow, 0, Utils::wideToMultibyte(p->getName()).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 1, -_wtof(Utils::penniesToDollarsString(p->getExpenseLimit(), false).c_str()), formatMoney);
		worksheet_write_string(worksheet, curRow, 2, p->getHidden() ? "yes" : "no", formatLeft);
		worksheet_write_string(worksheet, curRow, 3, p->getReadOnly() ? "yes" : "no", formatLeft);

		char szBuff[1024] = { 0 };
		sprintf_s(szBuff, "=Folders!A%d", curRow + 1);
		accountIdToCellRef[p->getId()] = szBuff;

		curRow++;
	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeCategories(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, EXPENSE_WORKSHEET_NAME);
	uint16_t curRow = 0;

	lxw_format* formatHeader = workbook_add_format(workbook);
	lxw_format* formatLeft = workbook_add_format(workbook);

	formatLeft->text_h_align = LXW_ALIGN_LEFT;
	formatLeft->bg_color = RGB(238, 238, 238);

	lxw_format* formatDate = workbook_add_format(workbook);
	formatDate->text_h_align = LXW_ALIGN_LEFT;
	formatDate->bg_color = RGB(238, 238, 238);
	format_set_num_format(formatDate, "m/d/yyyy");

	formatHeader->bold = true;
	formatHeader->bg_color = RGB(200, 200, 200);
	formatHeader->fg_color = RGB(0, 0, 0);
	formatHeader->text_h_align = LXW_ALIGN_LEFT;

	lxw_format* formatMoney = workbook_add_format(workbook);
	format_set_num_format(formatMoney, "$#,##0.00");
	formatMoney->text_h_align = LXW_ALIGN_CENTER;
	formatMoney->bg_color = RGB(238, 238, 238);



	// Write categories
	worksheet_write_string(worksheet, curRow, 0, "Expense name", formatHeader);
	worksheet_write_string(worksheet, curRow, 1, "Folder Id", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Budgeted", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "Balance", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "Hidden", formatHeader);
	worksheet_write_string(worksheet, curRow, 5, "Read Only", formatHeader);
	worksheet_write_string(worksheet, curRow, 6, "default Payee", formatHeader);
	worksheet_write_string(worksheet, curRow, 7, "default Amount", formatHeader);
	worksheet_write_string(worksheet, curRow, 8, "default Payer Id", formatHeader);
	worksheet_write_string(worksheet, curRow, 9, "Flags", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);


	curRow++;
	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::GROUP) {
			continue;
		}

		AccountGroup* pGroup = dynamic_cast<AccountGroup*>(topLevelItems[i]);
		auto children = pGroup->getChildren();
		for (int j = 0; j < (int)children.size(); j++)
		{
			if (children[j]->getType() != AccountBase::EXPENSE) {
				continue;
			}

			AccountExpense* p = dynamic_cast<AccountExpense*>(children[j]);

			worksheet_write_string(worksheet, curRow, 0, Utils::wideToMultibyte(p->getName()).c_str(), formatLeft);

			auto itr = accountIdToCellRef.find(pGroup->getId());
			if (itr != accountIdToCellRef.end()) {
				//worksheet_write_number(worksheet, curRow, 2, pGroup->getId(), formatLeft);
				worksheet_write_formula_str(worksheet, curRow, 1, itr->second.c_str(), formatLeft, "");
			}
			else {
				worksheet_write_string(worksheet, curRow, 1, "", formatLeft);
			}

			worksheet_write_number(worksheet, curRow, 2, _wtof(Utils::penniesToDollarsString(p->getBudgeted(), false).c_str()), formatMoney);
			worksheet_write_number(worksheet, curRow, 3, _wtof(Utils::penniesToDollarsString(p->getBalance(), false).c_str()), formatMoney);

			worksheet_write_string(worksheet, curRow, 4, p->getHidden() ? "yes" : "no", formatLeft);
			worksheet_write_string(worksheet, curRow, 5, p->getReadOnly() ? "yes" : "no", formatLeft);

			worksheet_write_string(worksheet, curRow, 6, Utils::wideToMultibyte(p->getDefaultTransactionPayee()).c_str(), formatLeft);
			worksheet_write_number(worksheet, curRow, 7, _wtof(Utils::penniesToDollarsString(p->getDefaultTransactionAmount(), false).c_str()), formatMoney);

			if (p->getDefaultTransactionPayFromAccount()) {
				itr = accountIdToCellRef.find(p->getDefaultTransactionPayFromAccount()->getId());
				if (itr != accountIdToCellRef.end()) {
					worksheet_write_formula_str(worksheet, curRow, 8, itr->second.c_str(), formatLeft, "");
				}
			}
			else {
				worksheet_write_string(worksheet, curRow, 8, "", formatLeft);
			}

			worksheet_write_number(worksheet, curRow, 9, p->getId() == ACCOUNT_ID_PAY_CC ? FLAG_PAYS_CCS : FLAG_NO_FLAGS, formatLeft);

			char szBuff[1024] = { 0 };
			sprintf_s(szBuff, "=Categories!A%d", curRow + 1);
			accountIdToCellRef[p->getId()] = szBuff;

			curRow++;
		}
	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeTransactions(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, TRANSACTIONS_WORKSHEET_NAME);
	uint16_t curRow = 0;

	lxw_format* formatHeader = workbook_add_format(workbook);
	lxw_format* formatLeft = workbook_add_format(workbook);

	formatLeft->text_h_align = LXW_ALIGN_LEFT;
	formatLeft->bg_color = RGB(238, 238, 238);

	lxw_format* formatDate = workbook_add_format(workbook);
	formatDate->text_h_align = LXW_ALIGN_LEFT;
	formatDate->bg_color = RGB(238, 238, 238);
	format_set_num_format(formatDate, "m/d/yyyy");

	formatHeader->bold = true;
	formatHeader->bg_color = RGB(200, 200, 200);
	formatHeader->fg_color = RGB(0, 0, 0);
	formatHeader->text_h_align = LXW_ALIGN_LEFT;


	lxw_format* formatMoney = workbook_add_format(workbook);
	format_set_num_format(formatMoney, "$#,##0.00");
	formatMoney->text_h_align = LXW_ALIGN_CENTER;
	formatMoney->bg_color = RGB(238, 238, 238);

	// Write transactions
	worksheet_write_string(worksheet, curRow, 0, "Date", formatHeader);
	worksheet_write_string(worksheet, curRow, 1, "Payee", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Amount", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "Expense", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "Account From", formatHeader);
	worksheet_write_string(worksheet, curRow, 5, "Account To", formatHeader);
	worksheet_write_string(worksheet, curRow, 6, "Description", formatHeader);
	worksheet_write_string(worksheet, curRow, 7, "Type", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);

	curRow++;

	auto transactions = Transaction::getAllTransactions();

	for (auto i = transactions.rbegin(); i != transactions.rend(); i++)
	{
		Transaction* pTrans = *i;
		lxw_datetime dt;
		ZeroMemory(&dt, sizeof(dt));
		dt.year = pTrans->getTimestamp().wYear;
		dt.month = pTrans->getTimestamp().wMonth;
		dt.day = pTrans->getTimestamp().wDay;
		worksheet_write_datetime(worksheet, curRow, 0, &dt, formatDate);
		worksheet_write_string(worksheet, curRow, 1, Utils::wideToMultibyte(pTrans->getPayee()).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 2, _wtof(Utils::penniesToDollarsString(pTrans->getAmount(), false).c_str()), formatMoney);

		auto itr = accountIdToCellRef.find(pTrans->getExpenseId());
		if (itr != accountIdToCellRef.end()) {
			//worksheet_write_number(worksheet, curRow, 3, pTrans->getExpenseId(), formatLeft);
			worksheet_write_formula_str(worksheet, curRow, 3, itr->second.c_str(), formatLeft, "");
		}
		else {
			worksheet_write_string(worksheet, curRow, 3, "", formatLeft);
		}

		itr = accountIdToCellRef.find(pTrans->getAccountFromId());
		if (itr != accountIdToCellRef.end()) {
			//worksheet_write_number(worksheet, curRow, 4, pTrans->getAccountFromId(), formatLeft);
			worksheet_write_formula_str(worksheet, curRow, 4, itr->second.c_str(), formatLeft, "");
		}
		else {
			worksheet_write_string(worksheet, curRow, 4, "", formatLeft);
		}

		itr = accountIdToCellRef.find(pTrans->getAccountToId());
		if (itr != accountIdToCellRef.end()) {
			worksheet_write_formula_str(worksheet, curRow, 5, itr->second.c_str(), formatLeft, "");
			//worksheet_write_number(worksheet, curRow, 5, pTrans->getAccountToId(), formatLeft);
		}
		else {
			worksheet_write_string(worksheet, curRow, 5, "", formatLeft);
		}


		worksheet_write_string(worksheet, curRow, 6, Utils::wideToMultibyte(pTrans->getDescription()).c_str(), formatLeft);
		worksheet_write_string(worksheet, curRow, 7, transactionTypeToString(pTrans).c_str(), formatLeft);

		curRow++;

	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeSettings(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems, std::map<unsigned int, std::string>& accountIdToCellRef)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, SETTINGS_WORKSHEET_NAME);

	auto map = g_transactionImporter.map();
	uint16_t curRow = 0;

	lxw_format* formatHeader = workbook_add_format(workbook);
	lxw_format* formatLeft = workbook_add_format(workbook);

	formatLeft->text_h_align = LXW_ALIGN_LEFT;
	formatLeft->bg_color = RGB(238, 238, 238);

	lxw_format* formatDate = workbook_add_format(workbook);
	formatDate->text_h_align = LXW_ALIGN_LEFT;
	formatDate->bg_color = RGB(238, 238, 238);
	format_set_num_format(formatDate, "m/d/yyyy");

	formatHeader->bold = true;
	formatHeader->bg_color = RGB(200, 200, 200);
	formatHeader->fg_color = RGB(0, 0, 0);
	formatHeader->text_h_align = LXW_ALIGN_LEFT;

	// Write transactions
	worksheet_write_string(worksheet, curRow, 0, "Imported payee", formatHeader);
	worksheet_write_string(worksheet, curRow, 1, "Maps to payee", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Maps to expense", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 25, nullptr);
	curRow++;

	for (auto itr = map.begin(); itr != map.end(); itr++)
	{
		std::string impName = Utils::wideToMultibyte(itr->first.c_str());
		std::string mapName = Utils::wideToMultibyte(itr->second._payee.c_str());

		worksheet_write_string(worksheet, curRow, 0, impName.c_str(), formatLeft);
		worksheet_write_string(worksheet, curRow, 1, mapName.c_str(), formatLeft);

		auto itrMap = accountIdToCellRef.find(itr->second._expenseId);
		if (itrMap != accountIdToCellRef.end()) {
			//worksheet_write_number(worksheet, curRow, 2, catId, formatLeft);
			worksheet_write_formula_str(worksheet, curRow, 2, itrMap->second.c_str(), formatLeft, "");
		}

		
		curRow++;

	}

	return ERROR_SUCCESS;
}


lxw_worksheet* ExcelSerializer::serializeFrontSheet(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, FRONT_WORKSHEET_NAME);
	worksheet_set_column(worksheet, 0, 25, 12, nullptr);


	lxw_format* formatST = workbook_add_format(workbook);
	lxw_format* formatSTHP = workbook_add_format(workbook);
	lxw_format* formatSTWrap = workbook_add_format(workbook);
	lxw_format* formatSTHPBold = workbook_add_format(workbook);
	formatSTWrap->font_size = formatSTHP->font_size = formatST->font_size = 11;
	formatSTHPBold->font_size = 12;
	formatSTWrap->bg_color = formatSTHPBold->bg_color = formatST->bg_color = formatSTHP->bg_color = RGB(96, 73, 51);
	formatSTWrap->font_color = formatSTHPBold->font_color = formatST->font_color = formatSTHP->font_color = RGB(242, 224, 204);

	formatSTHPBold->bold = true;
	formatSTHPBold->underline = true;
	formatSTHPBold->italic = true;
	formatSTHP->hyperlink = true;
	formatSTHP->xf_id = 1; 
	formatSTHP->underline = true;

	formatSTWrap->text_v_align = LXW_ALIGN_VERTICAL_TOP;
	formatSTWrap->text_wrap = true;

	for (uint16_t row = 0; row < 6; row++) {
		for (uint16_t col = 0; col < 13; col++) {
			worksheet_write_string(worksheet, row, col, "", formatST);
		}
	}
	//worksheet_set_column(worksheet, 0, 8, 15, nullptr);
	//worksheet_set_row(worksheet, 0, 6, formatST);

	worksheet_merge_range(worksheet, 0, 0, 0, 3, "SCRATCHTRACK BUDGET", formatSTHPBold);
	worksheet_merge_range(worksheet, 0, 7, 0, 12, "THIS BUDGET IS INTENDED TO BE EDITED IN SCRATCHTRACK", formatST);


	worksheet_merge_range(worksheet, 2, 0, 3, 12, "ScratchTrack is a free budget and cash flow tracking application for Windows that saves its data in the Excel file format. Excel is not required.", formatSTWrap);
	worksheet_merge_range(worksheet, 5, 0, 5, 7, "https://scratchtrack.net", formatSTHP);

	worksheet_write_string(worksheet, 5, 11, "version", formatST);
	worksheet_write_string(worksheet, 5, 12, VER_STRING, formatST);

	return worksheet;
}

int ExcelSerializer::writeFrontSheetBudget(lxw_workbook* workbook, lxw_worksheet* worksheet, std::vector<AccountBase*>& topLevelItems)
{
	lxw_format* formatHeader = workbook_add_format(workbook);
	formatHeader->font_size = 24;
	formatHeader->bold = true;
	formatHeader->font_color = RGB(36,100,245);
	worksheet_merge_range(worksheet, CELL("B8"), CELL("E9"), "Monthly Budget", formatHeader);
	worksheet_merge_range(worksheet, CELL("H8"), CELL("K9"), "Monthly Folder", formatHeader);

	lxw_format* formatHeader2 = workbook_add_format(workbook);
	formatHeader2->font_size = 17;
	formatHeader2->bold = true;
	formatHeader2->font_color = RGB(36, 100, 245); 
	formatHeader2->text_v_align = LXW_ALIGN_VERTICAL_CENTER;
	//format_set_bottom(formatHeader2, LXW_BORDER_THIN);
	//format_set_border_color(formatHeader2, RGB(100, 70, 50));
	worksheet_merge_range(worksheet, CELL("B12"), CELL("F13"), "Expenses", formatHeader2);
	worksheet_merge_range(worksheet, CELL("H12"), CELL("L13"), "Folder", formatHeader2);

	std::vector<AccountBase*> expenses = AccountBase::getAllAccountsOfType(AccountBase::EXPENSE);

	char szBuf[1024] = { 0 };

	lxw_format* formatHeader3 = workbook_add_format(workbook);
	formatHeader3->font_size = 13;
	formatHeader3->bold = true;
	//formatHeader3->font_color = RGB(36, 100, 245);
	formatHeader3->text_v_align = LXW_ALIGN_VERTICAL_CENTER;

	lxw_conditional_format formatMoneyConditional = { 0 };
	lxw_format* formatMoney = workbook_add_format(workbook);
	format_set_num_format(formatMoney, "$#,##0.00");

	lxw_format* formatMoneyNegative = workbook_add_format(workbook);
	format_set_num_format(formatMoneyNegative, "$#,##0.00");
	format_set_font_color(formatMoneyNegative, LXW_COLOR_RED);

	formatMoneyConditional.type = LXW_CONDITIONAL_TYPE_CELL;
	formatMoneyConditional.criteria = LXW_CONDITIONAL_CRITERIA_LESS_THAN;
	formatMoneyConditional.value = 0;
	formatMoneyConditional.format = formatMoneyNegative;

	SYSTEMTIME curTime = { 0 };
	GetLocalTime(&curTime);

	worksheet_write_string(worksheet, CELL("B10"), "Totals for", nullptr);
	worksheet_write_string(worksheet, CELL("C10"), "Month:", nullptr);
	worksheet_write_string(worksheet, CELL("C11"), "Year:", nullptr);

	worksheet_write_number(worksheet, CELL("D10"), curTime.wMonth, nullptr);
	worksheet_write_number(worksheet, CELL("D11"), curTime.wYear, nullptr);

// Expenses
	size_t expenseCount = expenses.size();

	worksheet_merge_range(worksheet, 13, 1, 13, 2, "Category", formatHeader3);
	worksheet_write_string(worksheet, 13, 3, "Budgeted", formatHeader3);
	worksheet_write_string(worksheet, 13, 4, "Actual", formatHeader3);
	worksheet_write_string(worksheet, 13, 5, "Diff.", formatHeader3);

	int i = 0;
	int valuesStart = 14;
	for (i = 0; i < (int)expenseCount; i++)
	{
		// Category name =Categories!A2...
		sprintf_s(szBuf, "=Categories!A%d", i+2);
		worksheet_merge_range(worksheet, valuesStart + i, 1, valuesStart + i, 2, nullptr, nullptr);
		worksheet_write_formula_str(worksheet, valuesStart + i, 1, szBuf, nullptr, "");

		// Category budget =Categories!C2...
		sprintf_s(szBuf, "=-Categories!C%d", i + 2);
		worksheet_write_formula_str(worksheet, valuesStart + i, 3, szBuf, formatMoney, "");

		// Actual
		sprintf_s(szBuf, "=-SUMIFS(Transactions!C:C, Transactions!D:D, Categories!A%d, Transactions!A:A, \">= \" & DATE(D11, D10, 1), Transactions!A:A, \"< \" & EDATE(DATE(D11, D10, 1), 1))", i + 2);
		worksheet_write_formula_str(worksheet, valuesStart + i, 4, szBuf, formatMoney, "");
		worksheet_conditional_format_range(worksheet, valuesStart + i, 4, valuesStart + i, 4, &formatMoneyConditional);

		// Diff
		sprintf_s(szBuf, "=D%d-E%d", 15+i, 15+i);
		worksheet_write_formula_str(worksheet, valuesStart + i, 5, szBuf, formatMoney, "");
		worksheet_conditional_format_range(worksheet, valuesStart + i, 5, valuesStart + i, 5, &formatMoneyConditional);
	}

	// Totals
	int totalsRow = i + valuesStart + 2;
	worksheet_write_string(worksheet, totalsRow, 1, "Totals", formatHeader3);

	sprintf_s(szBuf, "=SUM(D%d:D%d)", valuesStart + 1, valuesStart + i);
	worksheet_write_formula_str(worksheet, totalsRow, 3, szBuf, formatMoney, "");
	worksheet_conditional_format_range(worksheet, totalsRow, 3, totalsRow, 3, &formatMoneyConditional);

	sprintf_s(szBuf, "=SUM(E%d:E%d)", valuesStart + 1, valuesStart + i);
	worksheet_write_formula_str(worksheet, totalsRow, 4, szBuf, formatMoney, "");
	worksheet_conditional_format_range(worksheet, totalsRow, 4, totalsRow, 4, &formatMoneyConditional);

	sprintf_s(szBuf, "=SUM(F%d:F%d)", valuesStart + 1, valuesStart + i);
	worksheet_write_formula_str(worksheet, totalsRow, 5, szBuf, formatMoney, "");
	worksheet_conditional_format_range(worksheet, totalsRow, 5, totalsRow, 5, &formatMoneyConditional);


// Folder
	std::vector<AccountBase*> groups = AccountBase::getAllAccountsOfType(AccountBase::GROUP);

	worksheet_write_string(worksheet, CELL("H15"), "Budgeted", formatHeader3);
	worksheet_write_formula_str(worksheet, CELL("I15"), "=SUM(Folders!B:B)", formatMoney, "");

	worksheet_write_string(worksheet, CELL("H16"), "Actual", formatHeader3);
	sprintf_s(szBuf, "=SUMIFS(Transactions!C:C, Transactions!H:H, \"Deposit\", Transactions!A:A, \">= \" & DATE(D11, D10, 1), Transactions!A:A, \"< \" & EDATE(DATE(D11, D10, 1), 1))");
	worksheet_write_formula_str(worksheet, CELL("I16"), szBuf, formatMoney, "");

	worksheet_write_string(worksheet, CELL("H17"), "Diff.", formatHeader3);
	worksheet_write_formula_str(worksheet, CELL("I17"), "=I16-I15", formatMoney, "");
	worksheet_conditional_format_range(worksheet, CELL("I17"), CELL("I17"), &formatMoneyConditional);


	return ERROR_SUCCESS;
}
	
std::string ExcelSerializer::transactionTypeToString(Transaction* pTransaction)
{
	switch (pTransaction->getType())
	{
	case Transaction::EXPENSE:
		return "Expense";
	case Transaction::DEPOSIT:
		return "Deposit";
	case Transaction::TRANSFER:
		return "Transfer";
	case Transaction::CCPAYMENT:
		return "CC Payment";
	}

	assert(false);
	return "Unknown";
}

Transaction::TRANSACTION_TYPE ExcelSerializer::transactionTypeFromString(const char* szTransactionType)
{
	if (_stricmp(szTransactionType, "Expense") == 0) {
		return Transaction::EXPENSE;
	}
	if (_stricmp(szTransactionType, "Deposit") == 0) {
		return Transaction::DEPOSIT;
	}
	if (_stricmp(szTransactionType, "Transfer") == 0) {
		return Transaction::TRANSFER;
	}
	if (_stricmp(szTransactionType, "CC Payment") == 0) {
		return Transaction::CCPAYMENT;
	}

	assert(false);
	return Transaction::EXPENSE;
}

int ExcelSerializer::deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems)
{
	szFile; topLevelItems;
	XLDocument doc;
	DWORD dwAttribs = ::GetFileAttributesA(szFile);
	int ret = ERROR_SUCCESS;

	try
	{
		if (dwAttribs != INVALID_FILE_ATTRIBUTES) {
			doc.open(szFile);
		}
	}
	catch (XLException& exception)
	{
		exception;
	}

	if (!doc.isOpen()) {
		return ERROR_FILE_NOT_FOUND;
	}

	std::map<std::string, unsigned int> cellRefToAccountId;

	//AccountBase::startDeserialize();

	OpenXLSX::XLWorkbook wb = doc.workbook();
	int err = deserializeAccounts(wb, topLevelItems, cellRefToAccountId);
	if (err != ERROR_SUCCESS) {
		goto end;
	}

	err = deserializeFolders(wb, topLevelItems, cellRefToAccountId);
	if (err != ERROR_SUCCESS) {
		goto end;
	}

	err = deserializeCategories(wb, topLevelItems, cellRefToAccountId);
	if (err != ERROR_SUCCESS) {
		goto end;
	}

	err = deserializeSettings(wb, topLevelItems, cellRefToAccountId);
	if (err != ERROR_SUCCESS) {
		goto end;
	}

	err = deserializeTransactions(wb, topLevelItems, cellRefToAccountId);
	if (err != ERROR_SUCCESS) {
		goto end;
	}


end:
	//AccountBase::stopDeserialize();
	return ret;
}

int ExcelSerializer::deserializeAccounts(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId)
{
	if (!workbook.worksheetExists(ACCOUNT_WORKSHEET_NAME)) {
		return ERROR_INVALID_DATA;
	}

	OpenXLSX::XLWorksheet worksheet = workbook.worksheet(ACCOUNT_WORKSHEET_NAME);

	int row = 2;
	while (true) {

		// Name
		std::string name = worksheet.cell(row, 1).value();
		if (name.empty()) {
			break;
		}

		// Type
		std::string typeS = worksheet.cell(row, 2).value();
		bool isCC = false;
		AccountBase::accountItemType type = accountTypeFromString(typeS.c_str(), isCC);

		// Balance
		int pennies = pennyValueFromCell(row, 3, worksheet);

		// in budget
		std::string inBudget = worksheet.cell(row, 4).value();

		// Hidden
		std::string hidden = worksheet.cell(row, 5).value();

		
		Account* pAccount = new Account;
		pAccount->setName(Utils::multiByteToWide(name.c_str()).c_str());
		pAccount->setIsCCAccount(isCC);
		//pAccount->setId(id);
		pAccount->addBalance(pennies);
		pAccount->setContributesToBudget(Utils::stringToBool(inBudget.c_str()));
		pAccount->setHidden(Utils::stringToBool(hidden.c_str()));
		addAccountLocationToMap(row, 1, worksheet, pAccount, cellRefToAccountId);

		topLevelItems.push_back(pAccount);

		row++;
	}

	//auto expenses = workbook.worksheet(EXPENSE_WORKSHEET_NAME);
	//XLFormula formula = expenses.cell(2, 9).formula();
	//std::string formulaText = formula.get();

	return ERROR_SUCCESS;
}

int ExcelSerializer::deserializeFolders(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId)
{
	if (!workbook.worksheetExists(FOLDER_WORKSHEET_NAME)) {
		return ERROR_INVALID_DATA;
	}

	OpenXLSX::XLWorksheet worksheet = workbook.worksheet(FOLDER_WORKSHEET_NAME);

	int row = 2;
	while (true) {

		// Name
		std::string name = worksheet.cell(row, 1).value();
		if (name.empty()) {
			break;
		}

		// Balance
		int pennies = pennyValueFromCell(row, 2, worksheet);

		// Hidden
		std::string hidden = worksheet.cell(row, 3).value();

		// readonly
		std::string readOnly = worksheet.cell(row, 4).value();

		AccountGroup* pGroup = new AccountGroup;
		pGroup->setName(Utils::multiByteToWide(name.c_str()).c_str());
		pGroup->setExpenseLimit(-pennies);
		pGroup->setHidden(Utils::stringToBool(hidden.c_str()));
		pGroup->setReadOnly(Utils::stringToBool(readOnly.c_str()));

		addAccountLocationToMap(row, 1, worksheet, pGroup, cellRefToAccountId);

		topLevelItems.push_back(pGroup);

		row++;
	}

	auto expenses = workbook.worksheet(EXPENSE_WORKSHEET_NAME);
	XLFormula formula = expenses.cell(2, 9).formula();
	std::string formulaText = formula.get();

	return ERROR_SUCCESS;
}

int ExcelSerializer::deserializeSettings(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId)
{
	if (!workbook.worksheetExists(SETTINGS_WORKSHEET_NAME)) {
		return ERROR_INVALID_DATA;
	}

	OpenXLSX::XLWorksheet worksheet = workbook.worksheet(SETTINGS_WORKSHEET_NAME);

	int row = 2;
	while (true) {

		// Name
		std::string importName = worksheet.cell(row, 1).value();
		if (importName.empty()) {
			break;
		}

		// Payee
		std::string payee = worksheet.cell(row, 2).value();

		// Expense ID
		XLFormula formula = worksheet.cell(row, 3).formula();
		std::string formulaText = formula.get();
		auto itr = cellRefToAccountId.find(formulaText);
		if (itr != cellRefToAccountId.end()) {
			g_transactionImporter.addMapping(Utils::multiByteToWide(importName.c_str()).c_str(), Utils::multiByteToWide(payee.c_str()).c_str(), itr->second);
		}

		row++;
	}
	return ERROR_SUCCESS;

}

int ExcelSerializer::deserializeCategories(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId)
{
	if (!workbook.worksheetExists(EXPENSE_WORKSHEET_NAME)) {
		return ERROR_INVALID_DATA;
	}

	OpenXLSX::XLWorksheet worksheet = workbook.worksheet(EXPENSE_WORKSHEET_NAME);

	int row = 2;
	while (true) {

		// Name
		std::string name = worksheet.cell(row, 1).value();
		if (name.empty()) {
			break;
		}

		// Parent folder
		int parentId = ACCOUNT_ID_INVALID;
		XLFormula formula = worksheet.cell(row, 2).formula();
		std::string formulaText = formula.get();
		auto itr = cellRefToAccountId.find(formulaText);
		if (itr != cellRefToAccountId.end()) {
			parentId = itr->second;
		}

		// Budgeted
		int budgeted = pennyValueFromCell(row, 3, worksheet);
		int balance = pennyValueFromCell(row, 4, worksheet);

		// Hidden
		std::string hidden = worksheet.cell(row, 5).value();

		// readonly
		std::string readOnly = worksheet.cell(row, 6).value();

		// Default payee
		std::string defaultPayee;
		try {
			std::string defaultPayeei = worksheet.cell(row, 7).value();
			defaultPayee = defaultPayeei;
		}
		catch (...) {}

		// Default amount
		int defaultAmount = pennyValueFromCell(row, 8, worksheet);

		// default payer ID
		unsigned int defPayerId = ACCOUNT_ID_PAY_CC;
		try {
			formula = worksheet.cell(row, 9).formula();
			formulaText = formula.get();
		}
		catch (...) {
		}

		// flags
		int flags = worksheet.cell(row, 10).value();
		//int flags = atoi(flagsString.c_str());

		itr = cellRefToAccountId.find(formulaText);
		if (itr != cellRefToAccountId.end()) {
			defPayerId = itr->second;
		}

		AccountExpense* pExpense = new AccountExpense;
		if (flags & FLAG_PAYS_CCS) {
			pExpense->setId(ACCOUNT_ID_PAY_CC);
		}

		pExpense->setName(Utils::multiByteToWide(name.c_str()).c_str());

		AccountGroup* pParent = dynamic_cast<AccountGroup*>(AccountBase::lookupById(parentId));
		pParent->addChild(pExpense);

		pExpense->setBudgeted(budgeted);
		pExpense->setHidden(Utils::stringToBool(hidden.c_str()));
		pExpense->setReadOnly(Utils::stringToBool(readOnly.c_str()));

		pExpense->setDefaultTransactionPayee(Utils::multiByteToWide(defaultPayee.c_str()).c_str());
		pExpense->setDefaultTransactionAmount(defaultAmount);
		pExpense->addBalance(balance);
		pExpense->setIsVirtualAccount(true);

		Account* pAccount = dynamic_cast<Account*>(AccountBase::lookupById(defPayerId));
		if (pAccount) {
			pExpense->setDefaultTransactionPayFromAccount(pAccount);
		}

		addAccountLocationToMap(row, 1, worksheet, pExpense, cellRefToAccountId);

		row++;
	}

	auto expenses = workbook.worksheet(EXPENSE_WORKSHEET_NAME);
	XLFormula formula = expenses.cell(2, 9).formula();
	std::string formulaText = formula.get();

	return ERROR_SUCCESS;
}

int ExcelSerializer::deserializeTransactions(OpenXLSX::XLWorkbook& workbook, std::vector<AccountBase*>& topLevelItems, std::map<std::string, unsigned int>& cellRefToAccountId)
{
	if (!workbook.worksheetExists(TRANSACTIONS_WORKSHEET_NAME)) {
		return ERROR_INVALID_DATA;
	}

	OpenXLSX::XLWorksheet worksheet = workbook.worksheet(TRANSACTIONS_WORKSHEET_NAME);

	int row = 2;
	while (true) {

		// Date
		SYSTEMTIME ts = { 0 };
		double time = 0;
		try {
			XLCellValue valRaw = worksheet.cell(row, 1).value();
			switch (valRaw.type())
			{
			case XLValueType::Integer:
				time = (double)valRaw.get<int>();
				break;
			case XLValueType::Float:
				time = valRaw.get<double>();
				break;
			}

			if (!time) {
				break;
			}

			int day = 0, month = 0, year = 0;
			excelSerialDateToDMY((int)time, day, month, year);
			ts.wYear = year;
			ts.wMonth = month;
			ts.wDay = day;
		}
		catch (...) { break; }
		
		// Name
		std::string payee = worksheet.cell(row, 2).value();

		// Amount
		int amount = pennyValueFromCell(row, 3, worksheet);

		// Category
		int categoryId = ACCOUNT_ID_INVALID;
		XLFormula formula = worksheet.cell(row, 4).formula();
		std::string formulaText = formula.get();
		auto itr = cellRefToAccountId.find(formulaText);
		if (itr != cellRefToAccountId.end()) {
			categoryId = itr->second;
		}

		// Account from
		int accountFrom = ACCOUNT_ID_INVALID;
		formula = worksheet.cell(row, 5).formula();
		formulaText = formula.get();
		itr = cellRefToAccountId.find(formulaText);
		if (itr != cellRefToAccountId.end()) {
			accountFrom = itr->second;
		}

		// Account to
		int accountTo = ACCOUNT_ID_INVALID;
		formula = worksheet.cell(row, 6).formula();
		formulaText = formula.get();
		itr = cellRefToAccountId.find(formulaText);
		if (itr != cellRefToAccountId.end()) {
			accountTo = itr->second;
		}

		// Description
		std::string description = worksheet.cell(row, 7).value();

		std::string type = worksheet.cell(row, 8).value();
		Transaction::TRANSACTION_TYPE t = transactionTypeFromString(type.c_str());

		Transaction::addTransaction(t, amount, accountFrom, accountTo, categoryId, Utils::multiByteToWide(payee.c_str()).c_str(), 
			Utils::multiByteToWide(description.c_str()).c_str(), &ts);

		row++;
	}

	Transaction::sortAllTransactions(); 


	return ERROR_SUCCESS;
}

void ExcelSerializer::addAccountLocationToMap(int row, int col, OpenXLSX::XLWorksheet& worksheet, AccountBase* pAccount, std::map<std::string, unsigned int>& cellRefToAccountId)
{

	XLCellReference ref(row, col);
	std::string textRef = worksheet.name();
	textRef += "!";
	textRef += ref.address();

	cellRefToAccountId[textRef] = pAccount->getId();
}

void ExcelSerializer::excelSerialDateToDMY(int nSerialDate, int& nDay, int& nMonth, int& nYear)
{
	// Modified Julian to DMY calculation with an addition of 2415019
	int l = nSerialDate + 68569 + 2415019;
	int n = int((4 * l) / 146097);
	l = l - int((146097 * n + 3) / 4);
	int i = int((4000 * (l + 1)) / 1461001);
	l = l - int((1461 * i) / 4) + 31;
	int j = int((80 * l) / 2447);
	nDay = l - int((2447 * j) / 80);
	l = int(j / 11);
	nMonth = j + 2 - (12 * l);
	nYear = 100 * (n - 49) + i + l;
}

int ExcelSerializer::pennyValueFromCell(int row, int col, OpenXLSX::XLWorksheet& worksheet)
{
	XLCellValue valRaw = worksheet.cell(row, col).value();
	double valueIn = 0;
	switch (valRaw.type())
	{
	case XLValueType::Integer:
		valueIn = (double)valRaw.get<int>();
		break;
	case XLValueType::Float:
		valueIn = valRaw.get<double>();
		break;
	}

	return Utils::excelMoneyToPennies(valueIn);
}

const char* ExcelSerializer::libxslErrorToText(int error)
{
	return lxw_strerror((lxw_error)error);
}
