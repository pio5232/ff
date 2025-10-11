#include "pch.h"
#include "DummyPacketBuilder.h"
#include "Memory.h"

PacketPtr jh_content::DummyPacketBuilder::BuildLoginRequestPacket()
{
    jh_network::LogInRequestPacket logInReqPkt;
    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(logInReqPkt));

    *buffer << logInReqPkt.size << logInReqPkt.type << (ULONGLONG)1 << (ULONGLONG)2;

    return buffer;
}

PacketPtr jh_content::DummyPacketBuilder::BuildHeartbeatPacket(ULONGLONG timeStamp)
{
    jh_network::HeartbeatPacket hbPkt;

    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(hbPkt));

    *buffer << hbPkt.size << hbPkt.type << timeStamp;
    
    return buffer;
}

PacketPtr jh_content::DummyPacketBuilder::BuildMakeRoomRequestPacket()
{
    jh_network::MakeRoomRequestPacket makeRoomRequestPkt;

    static LONGLONG roomNumGen = 0;

    LONGLONG roomNum = InterlockedIncrement64(&roomNumGen);
    WCHAR roomName[ROOM_NAME_MAX_LEN]{};

    swprintf_s(roomName, ROOM_NAME_MAX_LEN, L"R_%lld", roomNum);

    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(makeRoomRequestPkt));

    *buffer << makeRoomRequestPkt.size << makeRoomRequestPkt.type;

    buffer->PutData(reinterpret_cast<const char*>(roomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);

    return buffer;
}

PacketPtr jh_content::DummyPacketBuilder::BuildEnterRoomRequestPacket(USHORT roomNum,const WCHAR* roomName)
{
    jh_network::EnterRoomRequestPacket enterRoomRequestPkt;
    
    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(enterRoomRequestPkt));

    *buffer << enterRoomRequestPkt.size << enterRoomRequestPkt.type << roomNum;

    buffer->PutData(reinterpret_cast<const char*>(roomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);

    return buffer;
}

PacketPtr jh_content::DummyPacketBuilder::BuildLeaveRoomRequestPacket(USHORT roomNum, const WCHAR* roomName)
{
    jh_network::LeaveRoomRequestPacket leaveRoomRequestPkt;

    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(leaveRoomRequestPkt));

    *buffer << leaveRoomRequestPkt.size << leaveRoomRequestPkt.type << roomNum;

    buffer->PutData(reinterpret_cast<const char*>(roomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);

    return buffer;
}

PacketPtr jh_content::DummyPacketBuilder::BuildChatRequestPacket(USHORT roomNum)
{
    static const WCHAR* dummyChattingMsg[] =
    {
        L"�ȳ��ϼ���",
        L"�ݰ����ϴ�.",
        L"ABNCMXNMFNDMSF",
        L"�������� �뷡�� ���ƿ�",
        L"�ƺ��ī�ٺ��",
        L"Southside johnny",
        L"I was a ghost, I was alone (hah)",
        L"��ο��� (hah), �ձ� �ӿ� (ah)",
        L"Given the throne, I didn't know how to believe",
        L"I was the queen that I'm meant to be (oh)",
        L"I lived two lives, tried to play both sides",
        L"But I couldn't find my own place (oh, oh)",
        L"Called a problem child,'cause I got too wild",
        L"But now that's how I'm",
        L"gettin'paid, ������ on stage",
        L"I'm done hidin', now I'm shinin'",
        L"Like I'm born to be",
        L"We dreamin'hard, we came so far",
        L"Now I believe",
        L"We're goin'up,up,up, it's our moment",
        L"You know together we're glowin'",
        L"Gonna be, gonna be golden",
        L"Oh, I'm done hidin' now I'm shinin'",
        L"��ġ���� ����ߴ�",
        L"���㵵�� �������� �װ� ���� ������",
        L"���� ������ �� �Ծ�",
        L"�ݼ� �� ������ ���ö��� ���� ������",
        L"Oh, �� ���̶� �ߴ� ���� �� ���",
        L"�� ��� �����",
        L"Oh, I'm Drowning",
        L"It's raining all day, yeah - yeah (yeah)",
        L"I can't (yeah) breathe",
        L"���ع��� ��λ��� ������ �⵵��",
        L"���� ������Ÿ��",
        L"�����ٶ󸶹ٻ������īŸ����",
        L"������ ������ ������"
    };

    int cnt = sizeof(dummyChattingMsg) / sizeof(dummyChattingMsg[0]);

    int idx = rand() % cnt;

    USHORT msgLen = wcslen(dummyChattingMsg[idx]) * 2;

    // ROOMNUM, MSGLEN, MSG
    USHORT pktSize = sizeof(USHORT) * 2 + msgLen;

    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(jh_network::PacketHeader) + pktSize);

    *buffer << pktSize << static_cast<USHORT>(jh_network::CHAT_TO_ROOM_REQUEST_PACKET) << roomNum << msgLen;

    buffer->PutData(reinterpret_cast<const char*>(dummyChattingMsg[idx]), msgLen);

    return buffer;
}

PacketPtr jh_content::DummyPacketBuilder::BuildRoomListRequestPacket()
{
    jh_network::RoomListRequestPacket roomListRequestPkt;

    PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(roomListRequestPkt));

    *buffer << roomListRequestPkt.size << roomListRequestPkt.type;

    return buffer;
}
