#pragma once
#include <queue>
#include <mutex>
#include "Common/ServerCommon.h"
#include "Common/Common.h"

struct IncomingMessage {
    SessionPtr session;
    RawPacket  packet;
    bool       is_disconnect = false;

    IncomingMessage(SessionPtr s, RawPacket p) : session(std::move(s)), packet(std::move(p)) {}
    explicit IncomingMessage(SessionPtr s) : session(std::move(s)), is_disconnect(true) {}
};

class MessageQueue {
public:
    void Push(const SessionPtr& session, RawPacket&& packet) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push({ session, std::move(packet) });
    }

    // 세션 종료 시 룸/세션 정리를 GameLoop 스레드로 넘기기 위한 이벤트
    void PushDisconnect(const SessionPtr& session) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(IncomingMessage(session));
    }

    std::queue<IncomingMessage> PopAll() {
        std::queue<IncomingMessage> tmp;
        std::lock_guard<std::mutex> lock(_mutex);
        std::swap(tmp, _queue);
        return tmp;
    }

private:
    std::queue<IncomingMessage> _queue;
    std::mutex _mutex;
};
