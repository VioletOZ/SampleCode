#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include "Common/TokenBucket.h"

// 방 게임(코인 줍기) 규칙 수치. 흐름·검증 로직은 Room/RoomGame.h, 패킷은 proto/GamePacket.proto

// 방 정원(초과 입장 ERR_ROOM_FULL). 스냅샷 1개의 크기 상한도 이 값으로 정해진다
constexpr size_t ROOM_CAPACITY = 10;
// 이 인원 이상이 모두 준비하면 카운트다운 시작. 진행 중 이 인원 미만이 되면 바로 결과
constexpr size_t GAME_MIN_PLAYERS = 2;

constexpr auto GAME_COUNTDOWN_TIME = std::chrono::seconds(3);
constexpr auto GAME_PLAY_TIME      = std::chrono::seconds(60);
constexpr auto GAME_RESULT_TIME    = std::chrono::seconds(5);

// 맵: 0 ~ GAME_MAP_SIZE 정사각형. 시작 위치는 중심에서 GAME_START_RADIUS 원 위에 고르게
constexpr float GAME_MAP_SIZE     = 100.0f;
constexpr float GAME_START_RADIUS = 30.0f;

// 이동 검증: 이동 거리를 토큰으로 쓰는 버킷. 초당 최대 속도(+10% 여유)만큼 차고, 최대 0.5초분까지 쌓임.
// 네트워크에서 이동 패킷이 몰려 한 틱에 여러 개 들어와도 지나간 시간만큼 쌓인 거리로 통과한다
constexpr float     GAME_MAX_SPEED   = 10.0f;   // 거리/초
constexpr RateLimit GAME_MOVE_BUDGET{ GAME_MAX_SPEED * 0.5, GAME_MAX_SPEED * 1.1 };

// 스냅샷: 진행 중 이 틱 수마다 바뀐 것만 방 인원에게(20 TPS / 2 = 10Hz)
constexpr uint32_t GAME_SNAPSHOT_EVERY_TICKS = 2;

// 코인: 맵에 최대 GAME_MAX_COINS개, GAME_COIN_SPAWN_INTERVAL마다 모자란 만큼 생성. 이동한 위치에서 반경 안이면 획득(+1점)
constexpr size_t GAME_MAX_COINS           = 5;
constexpr auto   GAME_COIN_SPAWN_INTERVAL = std::chrono::seconds(2);
constexpr float  GAME_COIN_RADIUS         = 1.5f;
