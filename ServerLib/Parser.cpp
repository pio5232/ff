#include "LibraryPch.h"
using namespace jh_utility;


bool jh_utility::LeftTrim(WCHAR** wstr, DWORD numOfElements)
{
	WCHAR* originWstr = *wstr;

	DWORD len = wcsnlen_s(originWstr, numOfElements);

	if (0 == len || numOfElements == len)
	{
		return false;
	}

	DWORD leftPos = 0;

	// LeftTrim�� RightTrim�� �ٸ� ����
	// LeftTrim�� ��� �պκ��� ������ ������ �Ŀ� �������� �Ű�����Ѵ�.

	while (leftPos <= len)
	{
		// �պκ��� ������ ��� (' ', '\t', '\n')
		if (true == IsWhiteSpaceText(originWstr + leftPos))
		{
			*(originWstr + leftPos) = L'\0';
			leftPos++;
		}
		else
			break;
	}

	// ���� ������ ������ ��ŭ N�� ��ġ�� �缳��.
	*wstr = ((WCHAR*)originWstr) + leftPos;

	return true;


}

bool jh_utility::RightTrim(WCHAR* wstr, DWORD numOfElements)
{
	// wcsnlen_s �� nullCheck����. nullptr�̸� 0��ȯ
	// ���� wcsnlen�� ���������� �ִ� ���̸� ����, ������ ���� ������ �ִ� ���̸�ŭ�� ��ȯ

	// wstr�� ������ ������ Ȯ��?
	// 0 �Ǵ� ���� �� �ִ� ������ �� �ʰ����� �� ������ ����ٰ� �Ǵ�.

	DWORD len = wcsnlen_s(wstr, numOfElements);

	if (0 == len || numOfElements == len)
	{
		return false;
	}

	DWORD rightPos = len - 1; // ������ �� �� end ��ġ

	while (0 <= rightPos)
	{
		if (true == IsWhiteSpaceText(wstr + rightPos))
		{
			*(wstr + rightPos) = L'\0';
			rightPos--;
		}
		else
			break;
	}

	return true;
}

bool jh_utility::Trim(WCHAR** wstr, DWORD numOfElements)
{
	bool rightRes = RightTrim(*wstr, numOfElements);

	bool leftRes = LeftTrim(wstr, numOfElements);

	return rightRes && leftRes;
}


Parser::Parser() : m_pFile(nullptr), m_szLineBuffer{}, m_categories(), m_szParsingBuffer{},
m_pCurrentParsingCategory(nullptr), m_pCurrentReadingCategory(nullptr)
{
	SetParseType(ParseType::CATEGORY);

	_wsetlocale(LC_ALL, L"korean");
}

Parser::~Parser()
{
	CloseFile();
}

void Parser::CloseFile()
{
	if (m_pFile)
	{
		for (std::pair<const std::wstring, Category*>& pair : m_categories)
		{
			delete pair.second;
		}

		m_categories.clear();

		fclose(m_pFile);
		m_pFile = nullptr;
	}
}

bool jh_utility::Parser::LoadFile(const WCHAR* file_name)
{
	errno_t ret = _wfopen_s(&m_pFile, file_name, L"rt, ccs=UNICODE");

	// 0�� ���� open
	if (0 != ret || nullptr == m_pFile)
	{
		wprintf(L"FILE OPEN ERROR [%d]\n", GetLastError());
		return false;
	}

	//WCHAR* fgetwsRet;
	//while ((fgetwsRet = fgetws(m_szLineBuffer, s_dwMaxLineLen, m_pFile)) != nullptr)
	// �̶��� ��ȯ�Ǵ� ���ڿ��� _lineBuffer�� ������ ���ڿ� �ּҸ� ���� ����.

	// fgetws
	// BinaryMode, Text �ٸ��� �д´�
	// TextMode - �� �پ� �д´�.
	// BinaryMode - �ѹ��� �о�´�.
	while (nullptr != fgetws(m_szLineBuffer, s_dwMaxLineLen, m_pFile))
	{
		switch (m_eParseType)
		{
		case jh_utility::Parser::ParseType::CATEGORY:
			ParseCategory();
			break;
		case jh_utility::Parser::ParseType::DATA:
			ParseMuchData();
			break;
		default:
			break;
		}
	}
}


void jh_utility::Parser::RegisterCategory()
{
	std::wstring categoryName(m_szParsingBuffer);

	auto findIt = m_categories.find(categoryName);

	if (findIt != m_categories.end())
	{
		wprintf(L"Already Exist Category Name : [%s]\n", m_szParsingBuffer);
	}
	else
	{
		Category* newCategory = new Category();
		m_categories.insert({ categoryName, newCategory });
		wprintf(L"\t\tCategory [%s] Insert \n", categoryName.c_str());

		m_pCurrentParsingCategory = newCategory;
	}

	SetParseType(ParseType::DATA);
}

void jh_utility::Parser::ParseCategory()
{
	// ī�װ��� ó���� �ϳ��� �ܾ üũ�Ѵ�.
	bool isChecking = false;

	DWORD nextParseBufferIndex = 0;

#ifdef  PRINT_CATEGORY
	wprintf(L"+--------------------------------------------------------------+\n");
	wprintf(L"%s\t", _lineBuffer);
#endif //  PRINT_CATEGORY

	if (0x0a == *m_szLineBuffer)
	{
#ifdef  PRINT_CATEGORY
		wprintf(L"Blank.. Enter\n");
		wprintf(L"+--------------------------------------------------------------+\n\n\n\n");
#endif
		return;
	}
	for (int i = 0; i < s_dwMaxLineLen - 1; i++)
	{
		if (true == IsWhiteSpaceText(m_szLineBuffer + i))
		{
			// ���࿡ �ܾ ã�Ҵٸ�. ����
			if (true == isChecking)
			{
				*(m_szParsingBuffer + nextParseBufferIndex++) = L'\0';

				RegisterCategory();

				break;
			}

			// �� ���� "     \n" �� ���� ���� ����
			if (true == IsMatch(m_szLineBuffer + i, L'\n'))
				break;
			// �ƴϸ� ������ �ܾ� ã������.
			continue;
		}

		// �ּ��̸� �����Ľ��� �����Ѵ�.
		if (true == IsComment(m_szLineBuffer + i))
		{
#ifdef  PRINT_CATEGORY
			wprintf(L"Comment.. Go NextLine\n");
#endif
			break;
		}

		if (false == isChecking)
			isChecking = true;

		*(m_szParsingBuffer + nextParseBufferIndex++) = *(m_szLineBuffer + i);
	}
#ifdef  PRINT_CATEGORY
	wprintf(L"+--------------------------------------------------------------+\n\n\n\n");
#endif
}

void jh_utility::Parser::ParseMuchData()
{
	DWORD nextParseBufferIndex = 0;

	bool isParsing = false;// <�� �����ؼ� >�� ������ ������ �ľ��ϴ� ���ΰ�?
	bool hasColon = false;

#ifdef PRINT_DATA
	wprintf(L"+--------------------------------------------------------------+\n");
	wprintf(L"%s\t", _lineBuffer);
#endif // PRINT_DATA

	// 1. �ּ� üũ
	// 2. < > üũ
	for (int i = 0; i < s_dwMaxLineLen - 1; i++)
	{
		// �ּ��̸� �����Ľ��� ����.
		if (true == IsComment(m_szLineBuffer + i))
		{
#ifdef PRINT_DATA
			wprintf(L"[DATA] Comment.. Go NextLine\n");
#endif
			break;
		}

		if (true == IsMatch(m_szLineBuffer + i, L'<') && false == isParsing)
		{
			isParsing = true;
			continue;
		}

		// <�� ���� ������ �Է���
		if (true == isParsing)
		{
			// >�� ����
			if (true == IsMatch(m_szLineBuffer + i, L'>'))
			{
				if (true == hasColon)
				{
					*(m_szParsingBuffer + nextParseBufferIndex++) = L'\0';
#ifdef PRINT_DATA
					wprintf(L"ParseMuch -> Call ParseData [ %s ] \n", _parseBuffer);
#endif // #ifdef PRINT_DATA

					ParseData();
				}

				nextParseBufferIndex = 0;
				isParsing = false;
				hasColon = false;
			}
			// > �� ������ ���� ��Ȳ
			else
			{
				if (true == IsMatch(m_szLineBuffer + i, L':'))
				{
					hasColon = true;
				}

				*(m_szParsingBuffer + nextParseBufferIndex++) = *(m_szLineBuffer + i);
			}
			continue;
		}

		if (true == IsMatch(m_szLineBuffer + i, s_wchCloseSymbol))
		{
#ifdef PRINT_DATA
			wprintf(L"Match $.. ParseType Data -> Category Change\n");
#endif
			SetParseType(ParseType::CATEGORY);
			break;
		}

		// �����ų� �ٹٲ��̸� ����
		if (true == IsLineEnd(m_szLineBuffer + i))
		{
			break;
		}

	}
#ifdef PRINT_DATA
	wprintf(L"+--------------------------------------------------------------+\n\n\n\n");
#endif // PRINT_DATA
}
void jh_utility::Parser::ParseData()
{
	WCHAR* key;
	WCHAR* value = nullptr;

	// wcstok_s( ���ڿ�, �ڸ� ����, context )
	// context�� �ڸ��� ���� ���ڿ��� ��ġ.

	key = wcstok_s(m_szParsingBuffer, L":", &value);

	Trim(&key, s_dwMaxLineLen);
	Trim(&value, s_dwMaxLineLen);
#ifdef PRINT_DATA
	wprintf(L"\tParseData__ Key : [%s] Value : [%s]\n", key, value);
#endif // PRINT_DATA

	if (false == m_pCurrentParsingCategory->RegisterData(key, value))
	{
		wprintf(L"ParseData - Key Value Register Failed\n");
	}
}

void jh_utility::Parser::ShowAll()
{
	for (auto& category : m_categories)
	{
		wprintf(L"----------------------------------------\n");
		wprintf(L"Category Name : %s\n", category.first.c_str());

		int dataCount = 0;
		for (auto& dataPair : category.second->dataMap)
		{
			wprintf(L"%3u\t([%s] : [%s])\n", ++dataCount, dataPair.first.c_str(), dataPair.second.c_str());
		}
	}
}


bool jh_utility::Parser::TryGetCategoryItem(const WCHAR* key, OUT const std::wstring*& wstrValue)
{
	if (nullptr == m_pCurrentReadingCategory)
	{
		wprintf(L"GetCategoryItem - CurrentReadingCategory is null\n");
		return false;
	}

	std::unordered_map<std::wstring, std::wstring>::iterator findIt =
		m_pCurrentReadingCategory->dataMap.find(key);

	if (findIt == m_pCurrentReadingCategory->dataMap.end())
	{
		wprintf(L"GetCategoryItem [%s] - Item is not exist\n", key);

		return false;
	}
	wstrValue = &(findIt->second);

	return true;
}

bool jh_utility::Parser::SetReadingCategory(const WCHAR* categoryName)
{
	std::unordered_map<std::wstring, Category*>::iterator findIt =
		m_categories.find(std::wstring(categoryName));

	if (findIt == m_categories.end())
		return false;

	m_pCurrentReadingCategory = findIt->second;

	return true;
}

bool jh_utility::Parser::GetValueWstr(const WCHAR* key, WCHAR* wchBuff, DWORD buffSize)
{
	const std::wstring* pWstr;
	if (false == TryGetCategoryItem(key, pWstr))
		return false;

	if (pWstr->size() + 1 > buffSize)
	{
		wprintf(L"GetValue Wstr : BuffSize is too small\n");
		return false;
	}

	wcscpy_s(wchBuff, buffSize, pWstr->c_str());
	return true;
}
