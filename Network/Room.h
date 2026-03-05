#pragma once
#include "stdafx.h"
#include "Session.h"

typedef boost::shared_ptr<Session> SessionPtr;

class Room {
public:
    Room(uint32_t id, const std::string& name);

    void Create(SessionPtr session);
    void Join(SessionPtr session);
    void Leave(SessionPtr session);
    void Broadcast(const google::protobuf::Message& message);

    uint32_t GetId() const                  { return _id; }
    const std::string& GetName() const      { return _name; }
    size_t GetMemberCount() const           { return _members.size(); }

    std::vector<uint32_t> GetMemberIds() const;

private:
    uint32_t _id;
    std::string _name;
    std::set<SessionPtr> _members;
};
