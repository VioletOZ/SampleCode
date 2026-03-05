#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN             


#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <sstream>
#include <set>
#include <ctime>
#include <chrono>
#include <functional>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>


using namespace boost;

class Session;
class Room;
class RoomManager;

typedef boost::shared_ptr<Session> SessionPtr;
typedef boost::shared_ptr<Room> RoomPtr;