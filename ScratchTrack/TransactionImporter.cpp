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
					t._date = Utils::multiByteToWide(line.substr(1).c_str());
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

				if (t._amount && !t._date.empty() && !t._payee.empty()) {
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

	return ret;
}

int TransactionImporter::deserialize(tinyxml2::XMLElement* pSettings)
{
	int ret = ERROR_SUCCESS;
	tinyxml2::XMLElement* pImporterSettings = pSettings->FirstChildElement("importer");
	if (pImporterSettings) {

	}

	return ret;
}

int TransactionImporter::bestGuessAtPayeeAndCat(const wchar_t* szPayeeIn, std::wstring& payeeOut, std::wstring& catOut)
{
	std::wstring origPayee(szPayeeIn);
	std::transform(origPayee.begin(), origPayee.end(), origPayee.begin(), towupper);

	
	std::vector<Transaction*> transactions = Transaction::getAllTransactions();
	unsigned int distance = MAXUINT32;

	Transaction* pCur = nullptr;
	for (auto itr = transactions.begin(); itr != transactions.end(); itr++) {
		std::wstring curPayee((*itr)->getPayee());
		std::transform(curPayee.begin(), curPayee.end(), curPayee.begin(), towupper);

		unsigned int curDistance = Utils::stringDistance(origPayee, curPayee);
		if (curDistance < distance) {
			pCur = *itr;
			distance = curDistance;
		}
	}

	payeeOut = pCur->getPayee();
	AccountBase* pCat = AccountBase::lookupById(pCur->getCategoryId());
	if (pCat) {
		catOut = pCat->getName();
	}
	return ERROR_SUCCESS;
}
