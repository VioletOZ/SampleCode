#pragma once
#include "stdafx.h"
#include <optional>
#include <string>
#include "Session/Session.h"
#include "Network/PacketPolicy.h"
#include "Network/PacketUtils.h"
#include "proto/ChatPacket.pb.h"
#include "proto/RoomPacket.pb.h"

// 방(RoomManager)과 로비(LobbyManager)가 같이 쓰는 응답 생성 코드. GameLoop 스레드에서만 호출

// 세션 집합(방 멤버 / 채널 로비)의 닉네임을 응답에 바로 채움(중간 이름 목록 복사 없음)
template<typename Sessions>
void AppendUserNames(const Sessions& sessions, protobuf::S_RoomInfo& response)
{
    response.mutable_user_list()->Reserve(static_cast<int>(sessions.size()));
    for (const auto& session : sessions) {
        response.add_user_list()->set_name(session->GetUserName());
    }
}

// 채팅 1건을 받는 사람 모두에게 보낼 패킷으로 직렬화. 빈 메시지/길이 초과(한글 50자 = 150바이트)면
// 보낸 사람에게 ERR_INVALID_ARGUMENT를 응답하고 nullopt — 큰 메시지 하나가 수신자 전원의 송신 대기열을 채우는 것 방지.
// 빈도 제한은 GameLoop가 핸들러 호출 전에 확인(RequestLimit::Chat)
inline std::optional<std::string> SerializeChatBroadcast(const SessionPtr& session, const protobuf::C_ChatPacket& request,
                                                         protobuf::ChatType chat_type)
{
    if (!IsValidChatMessage(request.message())) {
        SendStatus<protobuf::S_ChatPacket>(session, protobuf::CHAT_MESSAGE, protobuf::ERR_INVALID_ARGUMENT);
        return std::nullopt;
    }
    protobuf::C_ChatPacket response;
    response.mutable_user_info()->set_name(session->GetUserName());  // uuid(로그인 ID)는 다른 유저에게 노출하지 않음
    response.set_message(request.message());
    response.set_chat_type(chat_type);
    return PacketUtils::Serialize(protobuf::CHAT_MESSAGE, response);
}
