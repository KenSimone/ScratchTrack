#pragma once
#include <vector>

class AccountBase;

class ExcelSerializer
{
public:
	static int serialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);
	static int deserialize(const char* szFile, std::vector<AccountBase*>& topLevelItems);
}; // class ExcelSerializer
