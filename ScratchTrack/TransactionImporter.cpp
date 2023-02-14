#include <Windows.h>

#include "TransactionImporter.h"
#include "AccountInfo.h"
#include "Utils.h"
#include "Transaction.h"
#include <algorithm>
#include <set>
#include <vector>

int TransactionImporter::readTransactions(const wchar_t* szFile, std::vector<ImportedTransactionInfo>& transactions)
{
	int ret = ERROR_SUCCESS;
	DWORD dwRead = 0, dwSize = 0;
	char* pCur = nullptr, * pBuff = nullptr;
	bool newLine = false;
	std::string line;
	ImportedTransactionInfo t;

	transactions.clear();

	HANDLE hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		goto end;
	}

	dwSize = ::GetFileSize(hFile, nullptr);
	pBuff = pCur = (char*)malloc(dwSize + 1);
	if (!pBuff) {
		ret = ERROR_NOT_ENOUGH_MEMORY;
		goto end;
	}

	memset(pBuff, 0, dwSize + 1);

	if (!ReadFile(hFile, pBuff, dwSize, &dwRead, nullptr)) {
		ret = GetLastError();
		goto end;
	}

	// Parse the QIF file
	while (*pCur) {
		char cur = *pCur++;
		if (cur == '\r' || cur == '\n') {
			newLine = true;
			// Process line
			if (!line.empty()) {
				switch (line[0])
				{
				case 'D':
				case 'd':
					t._date = Utils::dateStringToSysTime(Utils::multiByteToWide(line.substr(1).c_str()).c_str());
					break;
				case 'P':
				case 'p':
					t._payee = Utils::multiByteToWide(line.substr(1).c_str());
					break;
				case 'T':
				case 't':
					t._amount = Utils::dollarsStringToPennies(line.substr(1).c_str());
					break;
				}

				if (t._amount && t._date.wYear && !t._payee.empty()) {
					transactions.push_back(t);
					t.reset();
				}
			}
			line.clear();
			continue;
		}

		line += cur;
	}

end:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}

	if (pBuff) {
		free(pBuff);
	}

	return ret;
}


int TransactionImporter::serialize(tinyxml2::XMLElement* pSettings)
{
	int ret = ERROR_SUCCESS;
	tinyxml2::XMLDocument* pDoc = pSettings->GetDocument();
	tinyxml2::XMLElement* pImporterSettings = pDoc->NewElement("importer");
	pSettings->InsertFirstChild(pImporterSettings);

	tinyxml2::XMLElement* pPayeeMap = pDoc->NewElement("payeeMap");
	pImporterSettings->InsertFirstChild(pPayeeMap);

	for (auto itr = _impoortMap.begin(); itr != _impoortMap.end(); itr++)
	{
		tinyxml2::XMLElement* pPayeeMapping = pDoc->NewElement("payeeMapping");
		std::wstring nameIn = itr->first;
		pPayeeMapping->SetAttribute("from", Utils::wideToMultibyte(nameIn.c_str()).c_str());

		std::wstring nameOut = itr->second._payee.c_str();
		pPayeeMapping->SetAttribute("to", Utils::wideToMultibyte(nameOut.c_str()).c_str());
		pPayeeMapping->SetAttribute("categoryId", itr->second._categoryId);

		pPayeeMap->InsertFirstChild(pPayeeMapping);
	}

	return ret;
}

int TransactionImporter::deserialize(tinyxml2::XMLElement* pSettings)
{
	int ret = ERROR_SUCCESS;
	tinyxml2::XMLElement* pImporterSettings = pSettings->FirstChildElement("importer");
	if (pImporterSettings) {
		tinyxml2::XMLElement* pPayeeMap = pImporterSettings->FirstChildElement("payeeMap");

		tinyxml2::XMLElement* pMapping = pPayeeMap ? pPayeeMap->FirstChildElement("payeeMapping") : nullptr;
		while (pMapping)
		{
			const char* pFrom, * pTo;
			pMapping->QueryStringAttribute("from", &pFrom);
			pMapping->QueryStringAttribute("to", &pTo);

			unsigned int catId = 0;
			pMapping->QueryUnsignedAttribute("categoryId", &catId);

			if (pFrom && pTo) {
				std::wstring from = Utils::multiByteToWide(pFrom);
				std::wstring to = Utils::multiByteToWide(pTo);
				addMapping(from.c_str(), to.c_str(), catId);
			}
			pMapping = pMapping->NextSiblingElement("payeeMapping");
		}
	}

	return ret;
}

int TransactionImporter::bestGuessAtPayeeAndCat(const wchar_t* szPayeeIn, std::wstring& payeeOut, unsigned int& catIdOut)
{
	catIdOut = ACCOUNT_ID_INVALID;

	// First see if we already know about this mapping
	auto itr = _impoortMap.find(szPayeeIn);
	if (itr != _impoortMap.end()) {
		payeeOut = itr->second._payee;
		catIdOut = itr->second._categoryId;
		return ERROR_SUCCESS;
	}


	std::wstring origPayee(szPayeeIn);

	std::vector<Transaction*> transactions = Transaction::getAllTransactions();
	unsigned int similarity = 0;

	Transaction* pCur = nullptr;
	for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
		std::wstring curPayee((*itr)->getPayee());

		//unsigned int curDistance = Utils::stringDistance(origPayee, curPayee);
		unsigned int curSimilarity = (unsigned int)Utils::looksSimilar(origPayee, curPayee);
		if (curSimilarity > similarity) {
			pCur = *itr;
			similarity = curSimilarity;
		}
	}

	if (pCur && similarity >= 3) {
		payeeOut = pCur->getPayee();
		AccountBase* pCat = AccountBase::lookupById(pCur->getCategoryId());
		if (pCat) {
			catIdOut = pCat->getId();
		}
	}

	return ERROR_SUCCESS;
}

Transaction* TransactionImporter::findDuplicate(Account* pAccount, const wchar_t* szPayee, int amount, SYSTEMTIME date, unsigned int maxDateDiff)
{
	Transaction* pTrans = nullptr;
	std::vector<Transaction*> trans = Transaction::getAllTransactions();
	for (size_t i = 0; i < trans.size(); i++)
	{
		if (pAccount->getId() != trans[i]->getAccountFromId() && pAccount->getId() != trans[i]->getAccountToId()) {
			continue;
		}

		if (pAccount->getId() == trans[i]->getAccountToId()) {
			if (trans[i]->getAmount() != -amount) {
				continue;
			}
		}
		// Amount has to be an exact match
		else if (trans[i]->getAmount() != amount) {
			continue;
		}

		unsigned int dateDiff = Utils::daysDiff(date, trans[i]->getTimestamp());
		if (dateDiff > maxDateDiff) {
			continue;
		}

		// If this is a unique enough number, don't bother trying to match on name
		bool skipNameCheck = false;
		wchar_t szBuff[MAX_PATH] = { 0 };
		_itow_s(amount, szBuff, 10);
		size_t len = wcslen(szBuff);
		if (len > 2)
		{
			wchar_t one = szBuff[len - 1];
			wchar_t ten = szBuff[len - 2];

			if (one != L'0' || ten != L'0')
			{
				skipNameCheck = true;
			}
		}

		// See if the names are similar
		if ( !skipNameCheck ) {
			size_t iPayDelta = Utils::looksSimilar(szPayee, trans[i]->getPayee());
			if (iPayDelta < 2) {
				continue;

			}
		}

		pTrans = trans[i];
		break;
	}

	return pTrans;
}
