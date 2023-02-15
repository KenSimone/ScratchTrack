#include "ExcelSerializer.h"
#include <windows.h>
#include "AccountInfo.h"
#include "Transaction.h"
#include "Utils.h"
#include "OpenXLSX.hpp"

using namespace OpenXLSX;

int ExcelSerializer::serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems)
{
	// New documents contain a single worksheet named 'Sheet1'
	XLDocument doc;
	doc.create(szFile);

	return ERROR_CALL_NOT_IMPLEMENTED;
}

int ExcelSerializer::deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems)
{
	szFile; topLevelItems;
	return ERROR_CALL_NOT_IMPLEMENTED;
}
