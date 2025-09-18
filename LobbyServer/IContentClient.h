#pragma once

namespace jh_content
{
	// 네트워크 정보는 세션
	// 컨텐츠에서 사용되는 타입의 계층을 나눴을 때 복합적으로 사용하기 위해서 정의

	enum class ContentClientType : int
	{
		GUEST, 
		USER,
	};

	class IContentClient
	{
	public:
		IContentClient(ContentClientType type, ULONGLONG sessionId);
		virtual ~IContentClient() = 0;

		IContentClient(const IContentClient& other) = default;
		IContentClient(IContentClient&& other) = default;

		IContentClient& operator=(const IContentClient& other) = default;
		IContentClient& operator=(IContentClient&& other) = default;
		
		// 내 타입이 입력 타입 이상의 값인가? 를 체크
		bool IsAtLeast(ContentClientType inputType) const { return static_cast<byte>(m_clientType) >= static_cast<byte>(inputType); }

		void ChangeClientType(ContentClientType to);

	private:

		ContentClientType m_clientType;
		ULONGLONG m_ullSessionId;
	};
}
