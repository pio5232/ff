#include "pch.h"
#include "IContentClient.h"

jh_content::IContentClient::IContentClient(ContentClientType type, ULONGLONG sessionId) : m_clientType(type), m_ullSessionId(sessionId)
{

}

jh_content::IContentClient::~IContentClient()
{

}

void jh_content::IContentClient::ChangeClientType(ContentClientType to)
{
	// 기존의 작업 처리

	m_clientType = to;

	// 그다음 작업 처리

}
