#pragma once

namespace jh_content
{
	// ��Ʈ��ũ ������ ����
	// ���������� ���Ǵ� Ÿ���� ������ ������ �� ���������� ����ϱ� ���ؼ� ����

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
		
		// �� Ÿ���� �Է� Ÿ�� �̻��� ���ΰ�? �� üũ
		bool IsAtLeast(ContentClientType inputType) const { return static_cast<byte>(m_clientType) >= static_cast<byte>(inputType); }

		void ChangeClientType(ContentClientType to);

	private:

		ContentClientType m_clientType;
		ULONGLONG m_ullSessionId;
	};
}
