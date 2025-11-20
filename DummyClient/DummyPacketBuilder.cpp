#include "pch.h"
#include "DummyPacketBuilder.h"
#include "Memory.h"

PacketBufferRef jh_content::DummyPacketBuilder::BuildLoginRequestPacket()
{
    jh_network::LogInRequestPacket logInReqPkt;
    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(logInReqPkt));

    *buffer << logInReqPkt.size << logInReqPkt.type << (ULONGLONG)1 << (ULONGLONG)2;

    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildHeartbeatPacket(ULONGLONG timeStamp)
{
    jh_network::HeartbeatPacket hbPkt;

    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(hbPkt));

    *buffer << hbPkt.size << hbPkt.type << timeStamp;
    
    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildMakeRoomRequestPacket()
{
    jh_network::MakeRoomRequestPacket makeRoomRequestPkt;

    static LONGLONG roomNumGen = 0;

    LONGLONG roomNum = InterlockedIncrement64(&roomNumGen);
    WCHAR roomName[ROOM_NAME_MAX_LEN]{};

    swprintf_s(roomName, ROOM_NAME_MAX_LEN, L"R_%lld", roomNum);

    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(makeRoomRequestPkt));

    *buffer << makeRoomRequestPkt.size << makeRoomRequestPkt.type;

    buffer->PutData(reinterpret_cast<const char*>(roomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);

    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildEnterRoomRequestPacket(USHORT roomNum,const WCHAR* roomName)
{
    jh_network::EnterRoomRequestPacket enterRoomRequestPkt;
    
    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(enterRoomRequestPkt));

    *buffer << enterRoomRequestPkt.size << enterRoomRequestPkt.type << roomNum;

    buffer->PutData(reinterpret_cast<const char*>(roomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);

    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildLeaveRoomRequestPacket(USHORT roomNum, const WCHAR* roomName)
{
    jh_network::LeaveRoomRequestPacket leaveRoomRequestPkt;

    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(leaveRoomRequestPkt));

    *buffer << leaveRoomRequestPkt.size << leaveRoomRequestPkt.type << roomNum;

    buffer->PutData(reinterpret_cast<const char*>(roomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);

    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildChatRequestPacket(USHORT roomNum)
{
    static const WCHAR* dummyChattingMsg[] =
    {
        L"안녕하세요",
        L"반갑습니다.",
        L"ABNCMXNMFNDMSF",
        L"스핏츠의 노래가 좋아요",
        L"아브라카다브라",
        L"Southside johnny",
        L"I was a ghost, I was alone (hah)",
        L"어두워진 (hah), 앞길 속에 (ah)",
        L"Given the throne, I didn't know how to believe",
        L"I was the queen that I'm meant to be (oh)",
        L"I lived two lives, tried to play both sides",
        L"But I couldn't find my own place (oh, oh)",
        L"Called a problem child,'cause I got too wild",
        L"But now that's how I'm",
        L"gettin'paid, 끝없이 on stage",
        L"I'm done hidin', now I'm shinin'",
        L"Like I'm born to be",
        L"We dreamin'hard, we came so far",
        L"Now I believe",
        L"We're goin'up,up,up, it's our moment",
        L"You know together we're glowin'",
        L"Gonna be, gonna be golden",
        L"Oh, I'm done hidin' now I'm shinin'",
        L"미치도록 사랑했던",
        L"지겹도록 다투었던 네가 먼저 떠나고",
        L"여긴 온종일 비가 왔어",
        L"금세 턱 끝까지 차올랐고 숨이 막혀와",
        L"Oh, 내 맘이란 추는 나를 더 깊게",
        L"더 깊게 붙잡아",
        L"Oh, I'm Drowning",
        L"It's raining all day, yeah - yeah (yeah)",
        L"I can't (yeah) breathe",
        L"동해물과 백두산이 마르고 닳도록",
        L"오빤 강남스타일",
        L"가나다라마바사아자차카타파하",
        L"가지마 가지마 가지마"
    };

    int cnt = sizeof(dummyChattingMsg) / sizeof(dummyChattingMsg[0]);

    int idx = rand() % cnt;

    USHORT msgLen = wcslen(dummyChattingMsg[idx]) * 2;

    // ROOMNUM, MSGLEN, MSG
    USHORT pktSize = sizeof(USHORT) * 2 + msgLen;

    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(jh_network::PacketHeader) + pktSize);

    *buffer << pktSize << static_cast<USHORT>(jh_network::CHAT_TO_ROOM_REQUEST_PACKET) << roomNum << msgLen;

    buffer->PutData(reinterpret_cast<const char*>(dummyChattingMsg[idx]), msgLen);

    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildRoomListRequestPacket()
{
    jh_network::RoomListRequestPacket roomListRequestPkt;

    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(roomListRequestPkt));

    *buffer << roomListRequestPkt.size << roomListRequestPkt.type;

    return buffer;
}

PacketBufferRef jh_content::DummyPacketBuilder::BuildEchoPacket()
{
    static ULONGLONG data = 0;

    ULONGLONG incData = InterlockedIncrement64((LONGLONG*)&data);

    jh_network::EchoPacket echoPacket;

    PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(echoPacket));

    *buffer << echoPacket.size << echoPacket.type << incData;

    return buffer;
}
