#include "Utils.h"

bool Utils::searchText(const wchar_t* szString, const wchar_t* szSearchText)
{
	if (!szString || !szSearchText) {
		return false;
	}

	if (!*szSearchText || !*szSearchText) {
		return false;
	}

	wchar_t* szUpcaseString = new wchar_t[wcslen(szString) + 1];
	size_t i = 0;
	for (i = 0; i < wcslen(szString); i++) {
		szUpcaseString[i] = toupper(szString[i]);
	}
	szUpcaseString[i] = 0;

	wchar_t* szUpcaseSearchText = new wchar_t[wcslen(szSearchText) + 1];
	for (i = 0; i < wcslen(szSearchText); i++) {
		szUpcaseSearchText[i] = toupper(szSearchText[i]);
	}
	szUpcaseSearchText[i] = 0;

	bool found = wcsstr(szUpcaseString, szUpcaseSearchText) != nullptr;

	delete[] szUpcaseString;
	delete[] szUpcaseSearchText;

	return found;
}