#ifndef C_PARSER
#define C_PARSER

#include <iostream>
#include <Windows.h>
#include <unordered_map>
#include <set>
#include <string>

// ParseCategory �Լ� wprint ���� ��ũ��
//#define PRINT_CATEGORY 

// ParseMuchData, ParseData �Լ� wprint ���� ��ũ��
//#define PRINT_DATA

#define UP_DIR(path) L"../" path

namespace jh_utility
{
	bool LeftTrim(WCHAR** wstr, DWORD numOfElements);
	bool RightTrim(WCHAR* wstr, DWORD numOfElements);
	bool Trim(WCHAR** wstr, DWORD numOfElements);

	// Text��忡�� 0D0A ���� -> 0A ��ȯ
	inline bool IsWhiteSpaceText(const WCHAR* const word)
	{
		return L'\t' == *word || L' ' == *word || L'\n' == *word;
	}

	class Parser
	{
	private:
		enum class ParseType
		{
			CATEGORY,
			DATA,
		};

		struct Category
		{
		public:
			Category()
			{
			}

			~Category()
			{
				dataMap.clear();
			}

			bool RegisterData(const WCHAR* const key, const WCHAR* const value)
			{
				return dataMap.insert({ key, value }).second;
			}

			const std::wstring FindItem(const WCHAR* key)
			{
				auto iter = dataMap.find(key);

				if (iter != dataMap.end())
					return iter->second;

				return std::wstring();
			}

			std::unordered_map<std::wstring, std::wstring> dataMap;
		};

	public:
		Parser();
		~Parser();

		bool LoadFile(const WCHAR* file_name);
		void CloseFile();

		bool SetReadingCategory(const WCHAR* categoryName);

		template <typename Type>
		bool GetValue(const WCHAR* key, OUT Type& value)
		{
			const std::wstring* pWstr;
			if (false == TryGetCategoryItem(key, pWstr))
				return false;

			try
			{
				if constexpr (true == std::is_same<Type, int>::value)
					value = std::stoi(*pWstr);
				else if constexpr (true == std::is_same<Type, unsigned long>::value || true == std::is_same<Type, DWORD>::value)
					value = std::stoul(*pWstr);
				else if constexpr (true == std::is_same<Type, unsigned short>::value || true == std::is_same<Type, short>::value)
					value = static_cast<Type>(std::stoi(*pWstr));
				else if constexpr (true == std::is_same<Type, double>::value)
					value = std::stod(*pWstr);
				else if constexpr (true == std::is_same<Type, float>::value)
					value = std::stof(*pWstr);
				else if constexpr (true == std::is_same<Type, __int64>::value)
					value = std::stoll(*pWstr);
				else if constexpr (true == std::is_same<Type, unsigned __int64>::value)
					value = std::stoull(*pWstr);
				else
				{
					wprintf(L"�ش�ȵ�.\n");
				}

			}
			catch (const std::exception& exception)
			{

				wprintf(L"Catch Exception - GetValue Key : [%s]\n", key);
				return false;
			}
			return true;
		}

		template <>
		bool GetValue(const WCHAR* key, OUT bool& value)
		{
			const std::wstring* pWstr;
			if (false == TryGetCategoryItem(key, pWstr))
				return false;

			if (0 == wcscmp(L"TRUE", pWstr->c_str()))
				value = true;
			else
				value = false;

			return true;
		}

		// ���ڿ�
		bool GetValueWstr(const WCHAR* key, WCHAR* wchBuff, DWORD buffSize);

		void ShowAll();

	private:
		bool TryGetCategoryItem(const WCHAR* key, OUT const std::wstring*& wstrValue);
		void RegisterCategory();

		void ParseCategory(); // ī�װ� �Ľ�
		void ParseMuchData(); // [key : value] 1���̻�
		void ParseData(); // [key : value] 1��

		void SetParseType(ParseType parseType) { m_eParseType = parseType; }

		// m_szLineBuffer, m_szParsingBuffer ������� ����
		// �ּ� üũ
		inline bool IsComment(const WCHAR* const wchPtr) const
		{
			return L'/' == *wchPtr && L'/' == *(wchPtr + 1);
		}

		// Enter, ���ڿ� �� üũ
		inline bool IsLineEnd(const WCHAR* const wchPtr) const
		{
			return L'\0' == *wchPtr || L'\n' == *wchPtr;
		}

		// �ܾ� üũ
		inline bool IsMatch(const WCHAR* const wchPtr, WCHAR word) const
		{
			return word == *wchPtr;
		}

		// �� �ؽ�Ʈ�� �� �� ���̰� _maxLineLen���� Ŭ ��� �Ľ��� ����� ���� ���� �� ����.
		// �˳��ϰ� ����.
		static constexpr DWORD s_dwMaxLineLen = 256;

		// Data �Ľ� ���� �÷���
		static constexpr WCHAR s_wchCloseSymbol = L'$';

		FILE* m_pFile;

		WCHAR m_szLineBuffer[s_dwMaxLineLen];
		// �� �ٸ��� �Ľ��� ���� ����.
		WCHAR m_szParsingBuffer[s_dwMaxLineLen];

		ParseType m_eParseType;

		Category* m_pCurrentParsingCategory; // Load���� �Ľ��� �� ���
		Category* m_pCurrentReadingCategory; // ����ڰ� ������ ��ȯ�� ���� GetValue�Լ� ����ϱ� �� ����

		std::unordered_map<std::wstring, Category*> m_categories;
	};
}

#endif