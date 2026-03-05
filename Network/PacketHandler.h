#pragma once
#include "stdafx.h"

#include "proto/UserPacket.pb.h"
#include "proto/CardPacket.pb.h"
#include "proto/RoomPacket.pb.h"
//using MessageFactory = std::function < std::unique_ptr<google::protobuf::Message>()>;
//
//class PacketHandler 
//{
//public:
//    PacketHandler();
//
//    // 메시지 패킷에 따라 등록
//    void RegisterMessageType(const std::string& type, MessageFactory factory);
//    std::string SerializePacket(const std::string& type, const google::protobuf::Message& message);
//    std::unique_ptr<google::protobuf::Message> DeserializePacket(const std::string& type, const std::string& data);
//
//private:
//    std::unordered_map < std::string, MessageFactory> _message_factories;
//};

using namespace protobuf;

using DynamicHandler = std::function<void(std::shared_ptr<Session>, std::unique_ptr<google::protobuf::Message>)>;

class PacketHandler
{
public:
    /*virtual std::string GetTypeName() = 0;
    virtual void Clear() = 0;
    virtual bool IsInitialized() = 0;*/

    std::string SerializePacket(const std::string& type, const google::protobuf::Message& message);
    std::unique_ptr<google::protobuf::Message> DeserializePacket(const std::string& type, const std::string& data);
    std::unique_ptr<google::protobuf::Message> CreateMessage(const std::string& type) const;

    static PacketHandler& instance() {
        static PacketHandler instance;
        return instance;
    }

    void register_handler(uint32_t type, const std::string& proto_name, DynamicHandler handler)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _type_to_name[type] = proto_name;
        _handlers[type] = std::move(handler);
    }

    bool handle(uint32_t type, std::shared_ptr<Session> session, const google::protobuf::Message& message) const
    {
        std::lock_guard<std::mutex> lock(_mtx);

        auto name_it = _type_to_name.find(type);
        if (name_it == _type_to_name.end()) {
            std::cerr << "Unknown type : " << type << std::endl;
            return false;
        }

        const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(name_it->second);
        if (!desc) {
            std::cerr << "Descriptor not found for type: " << name_it->second << std::endl;
            return false;
        }

        const google::protobuf::Message* proto_type = google::protobuf::MessageFactory::generated_factory()->GetPrototype(desc);
        if (!proto_type) {
            std::cerr << "Prototype not found for type: " << name_it->second << std::endl;
            return false;
        }

        std::unique_ptr<google::protobuf::Message> message_ptr(proto_type->New());
        if (!message_ptr->ParseFromString(message.DebugString()))
        {
            std::cerr << "Failed to parse message for type: " << name_it->second << std::endl;
            return false;
        }

        auto handler_it = _handlers.find(type);
        if (handler_it == _handlers.end())
        {
            std::cerr << "Handler not found for type: " << type << std::endl;
            return false;
        }

        handler_it->second(std::move(session), std::move(message_ptr));

        return true;
    }
    

private:
    PacketHandler() = default;
    mutable std::mutex _mtx;
    std::unordered_map<uint32_t, std::string> _type_to_name;
    std::unordered_map<uint32_t, DynamicHandler> _handlers;


};

void register_dynamic_handlers()
{
    
    PacketHandler::instance().register_handler(1, "game.UserLogin", [](std::shared_ptr<Session> session, std::unique_ptr<google::protobuf::Message> message)
    {
        // Handle UserLogin message
        //auto* login = dynamic_cast<UserPacket*>(message.get());

        //std::cout << login->DebugString() << std::endl;

        //if (login) {
        //    std::cout << "UserLogin received: " << login->type() << std::endl;
        //    // Process login logic here
        //} else {
        //    std::cerr << "Failed to cast message to UserPacket" << std::endl;
        //}

    });
        
}


class MessageWrapper {
public:
    using CreatorFunc = std::function<std::unique_ptr<google::protobuf::Message>()>;

    static MessageWrapper& Instance() {
        static MessageWrapper instance;
        return instance;
    }

    template<typename T>
    void Register(const std::string& type_name) {
        _creators[type_name] = []() {
            return std::make_unique<T>();
            };
    }

    std::unique_ptr<google::protobuf::Message> Create(const std::string& type_name) const {
        auto it = _creators.find(type_name);
        if (it != _creators.end()) {
            return it->second();
        }
        return nullptr; // or throw an exception
    }

private:
    std::unordered_map<std::string, CreatorFunc> _creators;
};

void Foo()
{
    //MessageWrapper::Instance().Register<UserInfo>(UserInfo::descriptor()->full_name());    

    ////
    //auto user_msg = MessageWrapper::Instance().Create(UserInfo::descriptor()->full_name());
    //auto* user = dynamic_cast<UserInfo*>(user_msg.get());
    //user->set_id(1);
    //user->set_name("Test");

    //// 직렬화
    //std::string user_data;
    //user->SerializeToString(&user_data);

    //// 역직렬화
    //auto parsed_user_msg = MessageWrapper::Instance().Create(UserInfo::descriptor()->full_name());
    //parsed_user_msg->ParseFromString(user_data);
    //auto* parsed_user = dynamic_cast<UserInfo*>(parsed_user_msg.get());
    //std::cout << "Parsed User ID: " << parsed_user->id() << ", Name: " << parsed_user->name() << std::endl;

    //
    return;
}