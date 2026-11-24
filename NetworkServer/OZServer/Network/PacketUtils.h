#pragma once
#include <string>
#include <google/protobuf/message.h>
#include "Network/PacketHeader.h"

class PacketUtils
{
public:
	// protobuf 메시지를 직렬화하여 패킷으로 변환
	static std::string Serialize(protobuf::PacketType type, const google::protobuf::Message& msg)
	{
		const size_t body_size = msg.ByteSizeLong();

		// 헤더 + 바디를 한 번에 담을 버퍼를 미리 할당
		std::string packet;
		packet.resize(HEADER_SIZE + body_size);

		// 헤더 구성
		PacketHeader header;
		header.body_size = htonl(static_cast<uint32_t>(body_size));
		header.type = htons(static_cast<uint16_t>(type));
		memcpy(packet.data(), &header, HEADER_SIZE);

		// protobuf 직렬화: 중간 버퍼 없이 packet의 바디 영역에 바로 기록
		if (!msg.SerializeToArray(packet.data() + HEADER_SIZE, static_cast<int>(body_size)))
		{
			throw std::runtime_error("Failed to serialize message");
		}

		return packet;
	}

	// 응답 메시지에 status_code만 담아 직렬화(요청과 같은 PacketType). 거부/오류 응답 공용
	template<typename Response>
	static std::string SerializeStatus(protobuf::PacketType type, protobuf::StatusCode code)
	{
		Response response;
		response.set_status_code(code);
		return Serialize(type, response);
	}

	// 수신한 헤더의 body_size가 허용 범위인지(0 ~ MAX_PACKET_SIZE). 서버->클라 방향(클라이언트/LoadTest 수신)에 사용
	static bool IsValidBodySize(uint32_t body_size)
	{
		return body_size <= MAX_PACKET_SIZE;
	}

	// 클라->서버 요청 body 상한. 가장 큰 요청(채팅 150바이트)보다 충분히 크게, 헤더만 보내고 멈추는 연결이
	// 큰 수신 버퍼를 미리 잡게 만들지 못하도록 작게 둔다. 초과 시 서버는 연결을 끊음(Session::HandleReadHeader)
	static constexpr uint32_t MAX_REQUEST_SIZE = 4 * 1024;

	static bool IsValidRequestBodySize(uint32_t body_size)
	{
		return body_size <= MAX_REQUEST_SIZE;
	}

	// 헤더 파싱
	static PacketHeader ParseHeader(const char* data)
	{
		PacketHeader header;
		memcpy(&header, data, HEADER_SIZE);
		header.body_size = ntohl(header.body_size);
		header.type = ntohs(header.type);
		return header;
	}
};
