#include "LibraryPch.h"
#include "Job.h"
#include "Memory.h"


/// <summary>
/// �ϳ��� ���ǿ� ���� ��Ŷ ���� �޽����� �ϼ���Ų��
/// </summary>
/// <param name="sessionId"></param>
/// <param name="packetPtr"></param>
jh_utility::DispatchJob::DispatchJob(ULONGLONG sessionId, PacketPtr& packetPtr)
{
    m_dwSessionCount = 1;
    m_sessionInfo.m_ullSingleSessionId = sessionId;
    m_packet = packetPtr;

}

/// <summary>
/// ���� ���ǿ� ���� ��Ŷ ���� �޽����� ���¸� �����.
/// </summary>
/// <param name="sessionCount"></param>
/// <param name="sessionIdList"></param>
/// <param name="packetPtr"></param>
jh_utility::DispatchJob::DispatchJob(DWORD sessionCount, ULONGLONG* sessionIdList, PacketPtr& packetPtr)
{
    if (0 == sessionCount)
    {
        _LOG(L"DispatchJob", LOG_LEVEL_WARNING, L"-[Multiple DispatchJob()] SesssionCount : [0].");
        return;
    }

    rsize_t copyBytes = sessionCount * sizeof(ULONGLONG);

    ULONGLONG* idList = static_cast<ULONGLONG*>(g_memAllocator->Alloc(copyBytes));

    m_dwSessionCount = sessionCount;
    m_sessionInfo.m_pSessionIdList = idList;
    m_packet = packetPtr;

    memcpy_s(m_sessionInfo.m_pSessionIdList, copyBytes, sessionIdList, copyBytes);
}

jh_utility::DispatchJob::~DispatchJob()
{
    if (1 < m_dwSessionCount)
    {
        g_memAllocator->Free(m_sessionInfo.m_pSessionIdList);
    }

    m_packet.reset();
}