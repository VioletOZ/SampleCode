#include "stdafx.h"
#include "Lobby/LobbyManager.h"
#include "Common/Log.h"
#include "Network/ResponseUtils.h"

uint32_t LobbyManager::Enter(const SessionPtr& session)
{
    const uint32_t channel = _allocator.Assign();
    if (_lobbies.size() < channel) _lobbies.resize(channel);
    _lobbies[channel - 1].insert(session);
    session->SetLobbyChannel(channel);
    LogDebug("Lobby: ", session->GetUserName(), " -> channel ", channel);
    return channel;
}

void LobbyManager::EnterRoom(const SessionPtr& session)
{
    if (LobbySet* lobby = FindMutableLobby(session->GetLobbyChannel())) lobby->erase(session);
}

void LobbyManager::ReturnFromRoom(const SessionPtr& session)
{
    if (LobbySet* lobby = FindMutableLobby(session->GetLobbyChannel())) lobby->insert(session);
}

void LobbyManager::Exit(const SessionPtr& session)
{
    const uint32_t channel = session->GetLobbyChannel();
    LobbySet* lobby = FindMutableLobby(channel);
    if (!lobby) return;   // 채널 없음(로그인 전 / 이미 처리됨)

    lobby->erase(session);
    session->SetLobbyChannel(0);
    _allocator.Release(channel);
    _lobbies.resize(_allocator.GetChannelCount());   // 맨 뒤 빈 채널 삭제(로비 집합은 소속 인원의 부분집합이라 비어 있음)
}

protobuf::StatusCode LobbyManager::MoveChannel(const SessionPtr& session, uint32_t channel)
{
    const uint32_t current = session->GetLobbyChannel();
    if (channel == current) return protobuf::ERR_SAME_CHANNEL;
    switch (_allocator.TryAssign(channel)) {
    case ChannelAssignResult::NotFound: return protobuf::ERR_CHANNEL_NOT_FOUND;
    case ChannelAssignResult::Full:     return protobuf::ERR_CHANNEL_FULL;
    case ChannelAssignResult::Ok:       break;
    }

    // 대상 채널을 먼저 +1 했으므로, 이전 채널 반환으로 맨 뒤 빈 채널이 지워져도 대상 채널(번호)은 남는다
    if (LobbySet* lobby = FindMutableLobby(current)) lobby->erase(session);
    _allocator.Release(current);
    _lobbies[channel - 1].insert(session);
    _lobbies.resize(_allocator.GetChannelCount());
    session->SetLobbyChannel(channel);
    return protobuf::OK;
}

void LobbyManager::HandleChannelMove(const SessionPtr& session, const protobuf::C_ChannelMove& request)
{
    const uint32_t from = session->GetLobbyChannel();
    protobuf::S_ChannelMove response;
    response.set_status_code(MoveChannel(session, request.channel()));
    response.set_channel(session->GetLobbyChannel());   // 성공: 새 채널 / 실패: 그대로
    response.set_channel_count(static_cast<uint32_t>(GetChannelCount()));
    LogDebug("Lobby: ", session->GetUserName(), " channel move ", from, " -> ", request.channel(),
             " (status ", static_cast<int>(response.status_code()), ")");
    session->SendPacket(PacketUtils::Serialize(protobuf::LOBBY_CHANNEL_MOVE, response));
}

void LobbyManager::HandleChatMessage(const SessionPtr& session, const protobuf::C_ChatPacket& request)
{
    std::optional<std::string> packet = SerializeChatBroadcast(session, request, protobuf::CHAT_LOBBY);
    if (packet) Broadcast(session->GetLobbyChannel(), std::move(*packet));
}

void LobbyManager::HandleUserList(const SessionPtr& session)
{
    protobuf::S_RoomInfo response;
    if (const LobbySet* lobby = FindLobby(session->GetLobbyChannel())) {
        AppendUserNames(*lobby, response);
    }
    response.set_channel(session->GetLobbyChannel());
    response.set_status_code(protobuf::OK);
    session->SendPacket(PacketUtils::Serialize(protobuf::ROOM_USER_LIST, response));
}

void LobbyManager::Broadcast(uint32_t channel, std::string message) const
{
    const LobbySet* lobby = FindLobby(channel);
    if (!lobby) return;
    auto shared_msg = std::make_shared<const std::string>(std::move(message));
    for (const auto& session : *lobby) {
        session->SendPacket(shared_msg);
    }
}

const LobbyManager::LobbySet* LobbyManager::FindLobby(uint32_t channel) const
{
    return (channel == 0 || channel > _lobbies.size()) ? nullptr : &_lobbies[channel - 1];
}
