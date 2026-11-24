#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "Room/GameRules.h"
#include "Common/TokenBucket.h"
#include "proto/GamePacket.pb.h"

// 방 게임(코인 줍기) 규칙 — 소켓/세션을 모르는 순수 로직(단위 테스트: ServerTest/RoomGameTest.cpp).
// Room이 하나씩 소유하고 GameLoop 스레드에서만 호출한다. 플레이어 키는 Room이 입장 때 발급하는 player_id.
// 단계 전환은 Update(now) 한 곳에서만 일어난다: 준비/퇴장은 값만 바꾸고, 다음 틱 Update가 조건을 보고 전환.
//   대기 --(GAME_MIN_PLAYERS 이상 전원 준비)--> 카운트다운 --> 진행 --(시간 종료 또는 인원 부족)--> 결과 --> 대기
//   카운트다운 중 준비 취소/퇴장으로 조건이 깨지면 대기로 돌아감
// 이동·코인 판정·점수는 모두 서버가 결정(클라이언트는 목표 위치만 보냄)
class RoomGame {
public:
    using Clock = std::chrono::steady_clock;

    struct MoveResult {
        protobuf::StatusCode status;
        float x;   // 거부(ERR_INVALID_MOVE) 시 서버가 가진 위치 — 클라이언트 보정용
        float y;
    };

    explicit RoomGame(uint32_t seed) : _rng(seed) {}

    protobuf::GamePhase GetPhase() const { return _phase; }
    // 대기/결과 단계에서만 입장 가능(카운트다운·진행 중 입장은 ERR_GAME_IN_PROGRESS)
    bool IsJoinable() const { return _phase == protobuf::GAME_WAITING || _phase == protobuf::GAME_RESULT; }
    size_t GetPlayerCount() const { return _players.size(); }

    void AddPlayer(uint32_t player_id, std::string name)
    {
        _players.try_emplace(player_id, std::move(name));
        _state_changed = true;
    }

    void RemovePlayer(uint32_t player_id)
    {
        if (_players.erase(player_id) == 0) return;
        if (_phase == protobuf::GAME_PLAYING) _removed_players.push_back(player_id);
        _state_changed = true;
    }

    // 대기/카운트다운 단계에서만(그 외 ERR_INVALID_STATE). 같은 값이면 변화 없음
    protobuf::StatusCode SetReady(uint32_t player_id, bool ready)
    {
        auto it = _players.find(player_id);
        if (it == _players.end() || (_phase != protobuf::GAME_WAITING && _phase != protobuf::GAME_COUNTDOWN)) {
            return protobuf::ERR_INVALID_STATE;
        }
        if (it->second.ready != ready) {
            it->second.ready = ready;
            _state_changed = true;
        }
        return protobuf::OK;
    }

    // 목표 위치로 이동. 진행 중에만(그 외 ERR_INVALID_STATE).
    // 맵 밖 / NaN·무한대 / 이동 예산(거리 버킷) 초과 -> ERR_INVALID_MOVE, 위치는 그대로(예산도 쓰지 않음)
    MoveResult TryMove(uint32_t player_id, float x, float y, uint32_t seq, Clock::time_point now)
    {
        auto it = _players.find(player_id);
        if (it == _players.end() || _phase != protobuf::GAME_PLAYING) return { protobuf::ERR_INVALID_STATE, 0.0f, 0.0f };

        Player& player = it->second;
        const bool in_map = std::isfinite(x) && std::isfinite(y) &&
                            x >= 0.0f && x <= GAME_MAP_SIZE && y >= 0.0f && y <= GAME_MAP_SIZE;
        if (!in_map || !player.move_budget.TryTake(now, std::hypot(x - player.x, y - player.y))) {
            return { protobuf::ERR_INVALID_MOVE, player.x, player.y };
        }
        player.x = x;
        player.y = y;
        player.last_seq = seq;
        player.dirty = true;

        // 이동한 위치에서 반경 안의 코인 획득(코인은 최대 GAME_MAX_COINS개라 전부 확인)
        std::erase_if(_coins, [&](const protobuf::Coin& coin) {
            if (std::hypot(coin.x() - x, coin.y() - y) > GAME_COIN_RADIUS) return false;
            ++player.score;
            _removed_coins.push_back(coin.coin_id());
            return true;
        });
        return { protobuf::OK, x, y };
    }

    // 매 틱(GameLoop): ① 다음 단계 판정 ② 단계가 바뀌었으면 진입 처리 ③ 진행 중 코인 생성.
    // 반환: 방 인원에게 알릴 상태 변화(단계 전환, 인원/준비 수 변화)가 있었는지 -> RoomManager가 S_GameState 브로드캐스트
    bool Update(Clock::time_point now)
    {
        const bool enough_players = _players.size() >= GAME_MIN_PLAYERS;
        const bool everyone_ready = enough_players &&
            std::all_of(_players.begin(), _players.end(), [](const auto& p) { return p.second.ready; });
        const bool time_up = now >= _phase_end;   // 대기 단계에는 쓰지 않음

        protobuf::GamePhase next = _phase;
        switch (_phase) {
        case protobuf::GAME_WAITING:   if (everyone_ready) next = protobuf::GAME_COUNTDOWN; break;
        case protobuf::GAME_COUNTDOWN: if (!everyone_ready) next = protobuf::GAME_WAITING;
                                       else if (time_up) next = protobuf::GAME_PLAYING; break;
        case protobuf::GAME_PLAYING:   if (!enough_players || time_up) next = protobuf::GAME_RESULT; break;
        default:                       if (time_up) next = protobuf::GAME_WAITING; break;   // GAME_RESULT
        }

        if (next != _phase) {
            _phase = next;
            _state_changed = true;
            switch (next) {
            case protobuf::GAME_COUNTDOWN:
                _phase_end = now + GAME_COUNTDOWN_TIME;
                break;
            case protobuf::GAME_PLAYING: {
                // 시작 위치: 맵 중심 원 위에 고르게. 점수·이동 번호·이동 예산·코인 초기화, 다음 스냅샷은 전체
                _phase_end = now + GAME_PLAY_TIME;
                const float center = GAME_MAP_SIZE / 2;
                const float step = 2.0f * 3.14159265f / static_cast<float>(_players.size());
                float angle = 0.0f;
                for (auto& [id, player] : _players) {
                    player.x = center + GAME_START_RADIUS * std::cos(angle);
                    player.y = center + GAME_START_RADIUS * std::sin(angle);
                    player.score = 0;
                    player.last_seq = 0;
                    player.dirty = false;
                    player.move_budget = TokenBucket(GAME_MOVE_BUDGET, now);
                    angle += step;
                }
                _coins.clear();
                _new_coins.clear();
                _removed_coins.clear();
                _removed_players.clear();
                _full_snapshot = true;
                _next_coin_spawn = now;   // 아래 코인 생성에서 바로 채움
                break;
            }
            case protobuf::GAME_RESULT:
                // 점수 높은 순(같으면 이름 순). 게임 중 나간 플레이어는 제외
                _phase_end = now + GAME_RESULT_TIME;
                _results.Clear();
                for (const auto& [id, player] : _players) {
                    protobuf::GameResult* result = _results.Add();
                    result->set_name(player.name);
                    result->set_score(player.score);
                }
                std::sort(_results.begin(), _results.end(), [](const protobuf::GameResult& a, const protobuf::GameResult& b) {
                    return a.score() != b.score() ? a.score() > b.score() : a.name() < b.name();
                });
                break;
            default:   // GAME_WAITING: 다음 판은 다시 준비부터
                for (auto& [id, player] : _players) player.ready = false;
                break;
            }
        }

        // 생성 주기가 됐으면 GAME_MAX_COINS개까지 채움(가장자리 2만큼 안쪽 무작위 위치)
        if (_phase == protobuf::GAME_PLAYING && now >= _next_coin_spawn) {
            _next_coin_spawn = now + GAME_COIN_SPAWN_INTERVAL;
            std::uniform_real_distribution<float> position(2.0f, GAME_MAP_SIZE - 2.0f);
            while (_coins.size() < GAME_MAX_COINS) {
                protobuf::Coin coin;
                coin.set_coin_id(++_next_coin_id);
                coin.set_x(position(_rng));
                coin.set_y(position(_rng));
                _new_coins.push_back(coin);
                _coins.push_back(std::move(coin));
            }
        }
        return std::exchange(_state_changed, false);
    }

    void BuildState(Clock::time_point now, protobuf::S_GameState& out) const
    {
        out.set_phase(_phase);
        if (_phase != protobuf::GAME_WAITING && now < _phase_end) {
            out.set_remaining_ms(static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(_phase_end - now).count()));
        }
        out.set_player_count(static_cast<uint32_t>(_players.size()));
        out.set_ready_count(static_cast<uint32_t>(std::count_if(_players.begin(), _players.end(),
                                                                [](const auto& p) { return p.second.ready; })));
        if (_phase == protobuf::GAME_RESULT) *out.mutable_results() = _results;
    }

    // 진행 중 스냅샷: 시작 후 첫 번째는 전체(이름 포함), 이후는 지난 스냅샷 뒤로 바뀐 것만(델타).
    // 바뀐 것이 없으면 false(보내지 않음)
    bool BuildSnapshot(protobuf::S_GameSnapshot& out)
    {
        if (_phase != protobuf::GAME_PLAYING) return false;
        const bool full = std::exchange(_full_snapshot, false);
        for (auto& [id, player] : _players) {
            if (!full && !player.dirty) continue;
            player.dirty = false;
            protobuf::PlayerState* state = out.add_players();
            state->set_player_id(id);
            if (full) state->set_name(player.name);
            state->set_x(player.x);
            state->set_y(player.y);
            state->set_score(player.score);
            state->set_last_seq(player.last_seq);
        }
        const std::vector<protobuf::Coin>& coins = full ? _coins : _new_coins;
        out.mutable_coins()->Add(coins.begin(), coins.end());
        out.mutable_removed_coins()->Add(_removed_coins.begin(), _removed_coins.end());
        out.mutable_removed_players()->Add(_removed_players.begin(), _removed_players.end());
        _new_coins.clear();
        _removed_coins.clear();
        _removed_players.clear();

        if (out.players_size() == 0 && out.coins_size() == 0 && out.removed_coins_size() == 0 && out.removed_players_size() == 0) {
            return false;
        }
        out.set_snapshot_seq(++_snapshot_seq);
        out.set_full(full);
        return true;
    }

private:
    struct Player {
        explicit Player(std::string player_name) : name(std::move(player_name)) {}

        std::string name;
        float       x = 0.0f;
        float       y = 0.0f;
        uint32_t    score = 0;
        uint32_t    last_seq = 0;
        bool        ready = false;
        bool        dirty = false;   // 지난 스냅샷 뒤로 위치/점수가 바뀜
        TokenBucket move_budget{ GAME_MOVE_BUDGET };
    };

    protobuf::GamePhase           _phase = protobuf::GAME_WAITING;
    Clock::time_point             _phase_end{};
    std::map<uint32_t, Player>    _players;          // player_id 순(시작 위치 배치 순서 고정)
    bool                          _state_changed = false;
    bool                          _full_snapshot = false;
    uint32_t                      _snapshot_seq = 0;

    std::vector<protobuf::Coin>   _coins;            // 맵에 있는 코인(최대 GAME_MAX_COINS)
    std::vector<protobuf::Coin>   _new_coins;        // 지난 스냅샷 뒤로 생긴 코인
    std::vector<uint32_t>         _removed_coins;    // 지난 스냅샷 뒤로 주워진 코인
    std::vector<uint32_t>         _removed_players;  // 지난 스냅샷 뒤로 게임 중 나간 플레이어
    uint32_t                      _next_coin_id = 0;
    Clock::time_point             _next_coin_spawn{};
    google::protobuf::RepeatedPtrField<protobuf::GameResult> _results;   // 결과 단계 순위
    std::mt19937                  _rng;              // 코인 위치(방마다 시드 — 테스트에서 고정 가능)
};
