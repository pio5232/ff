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
	// ������ �۾� ó��

	m_clientType = to;

	// �״��� �۾� ó��

}
