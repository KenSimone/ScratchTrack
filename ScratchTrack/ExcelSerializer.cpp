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

extern TransactionImporter g_transactionImporter;

using namespace OpenXLSX;

int ExcelSerializer::serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems)
{
	lxw_workbook* workbook = workbook_new(szFile);

	serializeFrontSheet(workbook, topLevelItems);
	serializeAccounts(workbook, topLevelItems);
	serializeFolders(workbook, topLevelItems);
	serializeSettings(workbook, topLevelItems);
	serializeTransactions(workbook, topLevelItems);
	serializeCategories(workbook, topLevelItems);

	workbook_close(workbook);


	return ERROR_CALL_NOT_IMPLEMENTED;
}

int ExcelSerializer::deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems)
{
	szFile; topLevelItems;
	XLDocument doc;
	DWORD dwAttribs = ::GetFileAttributesA(szFile);

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
		doc.create(szFile);
	}

	if (!doc.workbook().worksheetExists(ACCOUNT_WORKSHEET_NAME)) {

		if (doc.workbook().worksheetExists("Sheet1")) {
			auto ws = doc.workbook().worksheet("Sheet1");
			ws.setName(ACCOUNT_WORKSHEET_NAME);
		}
		else {
			doc.workbook().addWorksheet(ACCOUNT_WORKSHEET_NAME);
		}
	}

	auto worksheet = doc.workbook().worksheet(ACCOUNT_WORKSHEET_NAME);
	std::string name = worksheet.cell(2, 2).value();

	worksheet.cell(1, 1).value() = "Account name";
	worksheet.cell(1, 2).value() = "Id";
	worksheet.cell(1, 3).value() = "Type";
	worksheet.cell(1, 4).value() = "Balance";
	worksheet.cell(1, 5).value() = "InBudget";
	worksheet.cell(1, 6).value() = "Account name";

	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::ACCOUNT) {
			continue;
		}

		AccountBase* pAccount = dynamic_cast<AccountBase*>(topLevelItems[i]);

		worksheet.cell(i + 2, 1).value() = Utils::wideToMultibyte(pAccount->getName());
		worksheet.cell(i + 2, 2).value() = pAccount->getId();
		worksheet.cell(i + 2, 3).value() = accountTypeToString(pAccount);
		worksheet.cell(i + 2, 4).value() = _wtof(Utils::penniesToDollarsString(pAccount->getBalance(), false).c_str());
		worksheet.cell(i + 2, 5).value() = pAccount->getBudgeted() ? "yes" : "no";

	}

	doc.save();
	doc.close();

	return ERROR_CALL_NOT_IMPLEMENTED;
}

std::string ExcelSerializer::accountTypeToString(AccountBase* pAccount)
{
	std::string ret;
	switch (pAccount->getType())
	{
	case AccountBase::GROUP:
		ret = "folder";
		break;
	case AccountBase::CATEGORY:
		ret = "category";
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
	if (_stricmp(szAccountType, "account") == 0)
		return AccountBase::ACCOUNT;
	if (_stricmp(szAccountType, "folder") == 0)
		return AccountBase::GROUP;
	if (_stricmp(szAccountType, "category") == 0)
		return AccountBase::CATEGORY;
	if (_stricmp(szAccountType, "credit card") == 0)
		isCC = true;

	return AccountBase::ACCOUNT;
}

int ExcelSerializer::serializeAccounts(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
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
	worksheet_write_string(worksheet, curRow, 1, "Id", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Type", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "Balance", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "InBudget", formatHeader);
	worksheet_write_string(worksheet, curRow, 5, "Hidden", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);
	curRow++;

	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::ACCOUNT) {
			continue;
		}

		Account* p = dynamic_cast<Account*>(topLevelItems[i]);

		worksheet_write_string(worksheet, curRow, 0, Utils::wideToMultibyte(p->getName()).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 1, p->getId(), formatLeft);
		worksheet_write_string(worksheet, curRow, 2, accountTypeToString(p).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 3, _wtof(Utils::penniesToDollarsString(p->getBalance(), false).c_str()), formatMoney);
		worksheet_write_string(worksheet, curRow, 4, p->getContributesToBudget() ? "yes" : "no", formatLeft);
		worksheet_write_string(worksheet, curRow, 5, p->getHidden() ? "yes" : "no", formatLeft);

		curRow++;
	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeFolders(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
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
	worksheet_write_string(worksheet, curRow, 1, "Id", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Income", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "Hidden", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "Read Only", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);

	curRow++;
	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::GROUP) {
			continue;
		}

		AccountGroup* p = dynamic_cast<AccountGroup*>(topLevelItems[i]);

		worksheet_write_string(worksheet, curRow, 0, Utils::wideToMultibyte(p->getName()).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 1, p->getId(), formatLeft);
		worksheet_write_number(worksheet, curRow, 2, -_wtof(Utils::penniesToDollarsString(p->getExpenseLimit(), false).c_str()), formatMoney);
		worksheet_write_string(worksheet, curRow, 3, p->getHidden() ? "yes" : "no", formatLeft);
		worksheet_write_string(worksheet, curRow, 4, p->getReadOnly() ? "yes" : "no", formatLeft);

		curRow++;
	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeCategories(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, CATEGORY_WORKSHEET_NAME);
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
	worksheet_write_string(worksheet, curRow, 0, "Category name", formatHeader);
	worksheet_write_string(worksheet, curRow, 1, "Id", formatHeader);
	worksheet_write_string(worksheet, curRow, 2, "Folder Id", formatHeader);
	worksheet_write_string(worksheet, curRow, 3, "Budgeted", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "Hidden", formatHeader);
	worksheet_write_string(worksheet, curRow, 5, "Read Only", formatHeader);
	worksheet_write_string(worksheet, curRow, 6, "default Payee", formatHeader);
	worksheet_write_string(worksheet, curRow, 7, "default Amount", formatHeader);
	worksheet_write_string(worksheet, curRow, 8, "default Payer Id", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);


	curRow++;
	for (uint16_t i = 0; i < topLevelItems.size(); i++)
	{
		if (topLevelItems[i]->getType() != AccountBase::GROUP) {
			continue;
		}

		AccountGroup* pGroup = dynamic_cast<AccountGroup*>(topLevelItems[i]);
		auto children = pGroup->getChildren();
		for (int j = 0; j < children.size(); j++)
		{
			if (children[j]->getType() != AccountBase::CATEGORY) {
				continue;
			}

			AccountCategory* p = dynamic_cast<AccountCategory*>(children[j]);

			worksheet_write_string(worksheet, curRow, 0, Utils::wideToMultibyte(p->getName()).c_str(), formatLeft);
			worksheet_write_number(worksheet, curRow, 1, p->getId(), formatLeft);
			worksheet_write_number(worksheet, curRow, 2, pGroup->getId(), formatLeft);
			worksheet_write_number(worksheet, curRow, 3, _wtof(Utils::penniesToDollarsString(p->getBudgeted(), false).c_str()), formatMoney);
			worksheet_write_string(worksheet, curRow, 4, p->getHidden() ? "yes" : "no", formatLeft);
			worksheet_write_string(worksheet, curRow, 5, p->getReadOnly() ? "yes" : "no", formatLeft);

			worksheet_write_string(worksheet, curRow, 6, Utils::wideToMultibyte(p->getDefaultTransactionPayee()).c_str(), formatLeft);
			worksheet_write_number(worksheet, curRow, 7, _wtof(Utils::penniesToDollarsString(p->getDefaultTransactionAmount(), false).c_str()), formatMoney);

			if (p->getDefaultTransactionPayFromAccount()) {
				worksheet_write_number(worksheet, curRow, 8, p->getDefaultTransactionPayFromAccount()->getId(), formatLeft);
			}

			curRow++;
		}
	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeTransactions(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
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
	worksheet_write_string(worksheet, curRow, 3, "Category", formatHeader);
	worksheet_write_string(worksheet, curRow, 4, "Account From", formatHeader);
	worksheet_write_string(worksheet, curRow, 5, "Account To", formatHeader);
	worksheet_write_string(worksheet, curRow, 6, "Description", formatHeader);
	worksheet_write_string(worksheet, curRow, 7, "Type", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 15, nullptr);

	curRow++;

	auto transactions = Transaction::getAllTransactions();
	for (size_t i = 0; i < transactions.size(); i++)
	{
		Transaction* pTrans = transactions[i];
		lxw_datetime dt;
		ZeroMemory(&dt, sizeof(dt));
		dt.year = pTrans->getTimestamp().wYear;
		dt.month = pTrans->getTimestamp().wMonth;
		dt.day = pTrans->getTimestamp().wDay;
		worksheet_write_datetime(worksheet, curRow, 0, &dt, formatDate);
		worksheet_write_string(worksheet, curRow, 1, Utils::wideToMultibyte(pTrans->getPayee()).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 2, _wtof(Utils::penniesToDollarsString(pTrans->getAmount(), false).c_str()), formatMoney);
		worksheet_write_number(worksheet, curRow, 3, pTrans->getCategoryId(), formatLeft);
		worksheet_write_number(worksheet, curRow, 4, pTrans->getAccountFromId(), formatLeft);
		worksheet_write_number(worksheet, curRow, 5, pTrans->getAccountToId(), formatLeft);
		worksheet_write_string(worksheet, curRow, 6, Utils::wideToMultibyte(pTrans->getDescription()).c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 7, pTrans->getType(), formatLeft);

		curRow++;

	}

	return ERROR_SUCCESS;
}

int ExcelSerializer::serializeSettings(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
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
	worksheet_write_string(worksheet, curRow, 2, "Maps to category ID", formatHeader);
	worksheet_set_column(worksheet, 0, 8, 25, nullptr);
	curRow++;

	for (auto itr = map.begin(); itr != map.end(); itr++)
	{
		std::string impName = Utils::wideToMultibyte(itr->first.c_str());
		std::string mapName = Utils::wideToMultibyte(itr->second._payee.c_str());
		unsigned int catId = itr->second._categoryId;

		worksheet_write_string(worksheet, curRow, 0, impName.c_str(), formatLeft);
		worksheet_write_string(worksheet, curRow, 1, mapName.c_str(), formatLeft);
		worksheet_write_number(worksheet, curRow, 2, catId, formatLeft);
		curRow++;

	}

	return ERROR_SUCCESS;
}


int ExcelSerializer::serializeFrontSheet(lxw_workbook* workbook, std::vector<AccountBase*>& topLevelItems)
{
	lxw_worksheet* worksheet = workbook_add_worksheet(workbook, FRONT_WORKSHEET_NAME);

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



	return ERROR_SUCCESS;
}

