#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <google/protobuf/message.h>

#include "Network/PacketHeader.h"
#include "Common/ServerCommon.h"
#include "Network/PacketUtils.h"
#include "Network/PacketPolicy.h"
#include "Room/Room.h"


// 핸들러 함수 타입
// session: 세션 포인터, message: 역직렬화된 메시지
using HandleFunc = std::function<void(SessionPtr, const google::protobuf::Message&)>;

// 요청 타입에 맞는 응답 메시지로 status_code만 담은 패킷 생성(PacketUtils::SerializeStatus<Response>)
using StatusSerializer = std::string(*)(protobuf::PacketType, protobuf::StatusCode);

// 타입별 핸들러 저장
struct HandlerEntry {
	// 빈 메시지 객체 생성 (역직렬화용)
	std::function<std::unique_ptr<google::protobuf::Message>()> factory;
	HandleFunc handler;
	StatusSerializer serialize_status;   // 거부 응답(ERR_INVALID_STATE, ERR_RATE_LIMITED 등)
	AllowedStates allowed;               // 이 패킷을 받는 세션 상태(FSM 가드)
	RequestLimit limit;                  // 요청 빈도 제한 종류(None = 세션 전체 패킷 제한만)
};

// Dispatch 결과. Ok가 아니면 프로토콜 위반(정상 클라이언트라면 발생하지 않음) — 처리/로그는 호출 측(GameLoop)에서
enum class DispatchResult {
	Ok,
	UnknownType,  // 등록되지 않은 PacketType
	ParseError,   // body 역직렬화 실패
};

// handlers[type] = { factory, handler }
class PacketDispatcher
{
public:
	// 패킷 1종 등록(이 한 줄이 그 패킷의 전부): 요청/응답 타입, 받는 세션 상태, 빈도 제한, 핸들러.
	// Response = 이 요청의 응답 메시지(S_*) — 거부 응답(상태 위반, 빈도 초과, 종료 중)을 등록 정보로 만듦
	template<typename ProtoMsg, typename Response>
	void Register(protobuf::PacketType type, AllowedStates allowed, std::function<void(SessionPtr, const ProtoMsg&)> handler,
	              RequestLimit limit = RequestLimit::None) {
		_handlers[type] = {
			// factory: 빈 메시지 생성
			[]() { return std::make_unique<ProtoMsg>(); },

			// handler: 다운 캐스트 후 호출
			[handler](SessionPtr session, const google::protobuf::Message& msg) {
				handler(session, static_cast<const ProtoMsg&>(msg));
			},

			&PacketUtils::SerializeStatus<Response>,
			allowed,
			limit
		};
	}

	bool IsRegistered(protobuf::PacketType type) const { return _handlers.count(type) > 0; }

	// 이 상태에서 받을 수 있는 패킷인지. 미등록 타입은 모든 상태에서 false(기본 거부)
	bool IsAllowed(SessionState state, protobuf::PacketType type) const {
		auto it = _handlers.find(type);
		return it != _handlers.end() && it->second.allowed.Has(state);
	}

	// 등록된 빈도 제한 종류. 미등록이면 None
	RequestLimit GetRequestLimit(protobuf::PacketType type) const {
		auto it = _handlers.find(type);
		return (it != _handlers.end()) ? it->second.limit : RequestLimit::None;
	}

	// 등록된 요청 타입에 맞는 응답 메시지로 status_code만 담은 패킷. 미등록 타입이면 빈 문자열
	std::string SerializeStatus(protobuf::PacketType type, protobuf::StatusCode code) const {
		auto it = _handlers.find(type);
		return (it != _handlers.end()) ? it->second.serialize_status(type, code) : std::string();
	}

	// 수신된 패킷 디스패치
	// type: 라우팅할 패킷 타입, data: 직렬화된 body
	DispatchResult Dispatch(SessionPtr session, protobuf::PacketType type, const std::string& data) {
		auto it = _handlers.find(type);
		if (it == _handlers.end()) {
			return DispatchResult::UnknownType;
		}

		auto& entry = it->second;

		// 빈 메시지 생성
		auto msg = entry.factory();

		// 역직렬화
		if (!msg->ParseFromString(data)) {
			return DispatchResult::ParseError;
		}

		// 핸들러 호출
		entry.handler(session, *msg);
		return DispatchResult::Ok;
	}

private:
	std::unordered_map<protobuf::PacketType, HandlerEntry> _handlers;
};
