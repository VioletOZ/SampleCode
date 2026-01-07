using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading.Tasks;
using System.Text.Json;
using System.Text.Json.Serialization;
using StackExchange.Redis;

namespace ChatServer
{
    public class RedisManager
    {
        // 1. Lazy를 이용한 Thread-safe 싱글톤
        private static readonly Lazy<RedisManager> _lazy = new Lazy<RedisManager>(() => new RedisManager());
        public static RedisManager Instance => _lazy.Value;

        // JSON 옵션 공통 사용
        private static readonly JsonSerializerOptions _jsonOptions = new JsonSerializerOptions
        {
            Converters = { new JsonStringEnumConverter() }
        };

        // 2. 관리용 딕셔너리 (ConcurrentDict 활용)
        // 채널명 -> { SessionID -> 유저데이터 }
        private readonly ConcurrentDictionary<string, ConcurrentDictionary<string, ChatUserDataEx>> _subChannelDict = new ConcurrentDictionary<string, ConcurrentDictionary<string, ChatUserDataEx>>();
        // SessionID -> [구독중인 채널들]
        private readonly ConcurrentDictionary<string, ConcurrentBag<string>> _subSessionDict = new ConcurrentDictionary<string, ConcurrentBag<string>>();

        // 3. Redis 연결 상태
        private ConnectionMultiplexer _chatRedis;
        private ISubscriber _chatSubscriber;
        private IDatabase _chatDb;

        private ConnectionMultiplexer _gameRedis;
        private IDatabase _gameDb;

        private RedisManager()
        {
            InitializeChatRedis();
        }

        #region Initialization
        private void InitializeChatRedis()
        {
            try
            {
                string ip = Constance.Env.ChatServerRedisDBServerIP;
                if (string.IsNullOrEmpty(ip)) throw new Exception("Chat Redis IP is null");

                var config = ConfigurationOptions.Parse(ip);
                config.User = Constance.Env.ChatServerRedisDBUserID;
                config.Password = Constance.Env.ChatServerRedisDBPassword;
                config.AbortOnConnectFail = false;

                _chatRedis = ConnectionMultiplexer.Connect(config);
                _chatSubscriber = _chatRedis.GetSubscriber();
                _chatDb = _chatRedis.GetDatabase();

                Logger.WriteLog("ChatServer Redis Connected.");
            }
            catch (Exception ex)
            {
                Logger.WriteLog($"ChatServer Redis Connection Fail: {ex.Message}");
                Environment.Exit(0);
            }
        }

        public bool ConnectGameServerRedis()
        {
            try
            {
                string ip = Constance.Env.GameServerRedisDBServerIP;
                if (string.IsNullOrEmpty(ip)) return false;

                var config = ConfigurationOptions.Parse(ip);
                config.AbortOnConnectFail = false;

                _gameRedis = ConnectionMultiplexer.Connect(config);
                _gameDb = _gameRedis.GetDatabase();

                Logger.WriteLog("GameServer Redis Connected.");
                return true;
            }
            catch (Exception ex)
            {
                Logger.WriteLog($"GameServer Redis Error: {ex.Message}");
                return false;
            }
        }
        #endregion

        #region Auth & Pub/Sub
        public async Task<bool> AuthVerify(string sessionId)
        {
            if (_gameDb == null) return false;
            try
            {
                return await _gameDb.KeyExistsAsync($"Session:{sessionId}");
            }
            catch (Exception ex)
            {
                Logger.WriteLog($"AuthVerify Exception: {ex.Message}");
                return false;
            }
        }

        public async Task<bool> SubscribeAction(string channel, ChatUserData user, Action<RedisChannel, RedisValue> handler)
        {
            // 채널 관리 딕셔너리 확보
            var usersInChannel = _subChannelDict.GetOrAdd(channel, _ =>
            {
                // 새 채널이면 Redis 구독 시작
                _chatSubscriber.Subscribe(channel, (ch, val) => SendToChannelLocal(ch, val.ToString()));
                return new ConcurrentDictionary<string, ChatUserDataEx>();
            });

            var userEx = new ChatUserDataEx { UserData = user, Handler = handler };
            usersInChannel.TryAdd(user.ID, userEx);

            // 세션별 구독 채널 리스트 관리
            _subSessionDict.GetOrAdd(user.ID, _ => new ConcurrentBag<string>()).Add(channel);

            return true;
        }

        public async Task PublishAsync(string channel, req_ChatMessage message)
        {
            var resMessage = new res_ChatMessage
            {
                Command = CHAT_COMMAND.CT_MESSAGE,
                ReturnCode = RETURN_CODE.RC_OK,
                ChatType = message.ChatType,
                ChannelID = message.ChannelID,
                LogData = message.LogData
            };

            string payload = EncodingJson.Serialize(resMessage);
            await _chatSubscriber.PublishAsync(channel, payload);

            // 길드 채팅 로그 저장
            if (message.ChatType == CHAT_TYPE.CT_GUILD)
            {
                string timeKey = DateTime.Now.AddHours(9).ToString("yyyyMMddHHmmss");
                await _chatDb.HashSetAsync($"{Constance.LOG}{channel}", timeKey, payload);
                await _chatDb.KeyExpireAsync($"{Constance.LOG}{channel}", TimeSpan.FromDays(7));
            }
        }

        public async Task GachaPublish(string channel, res_ChatGachaNotice notice)
        {
            await _chatSubscriber.PublishAsync(channel, EncodingJson.Serialize(notice));
        }
        #endregion

        #region Local Memory Management (User/Channel)
        private void SendToChannelLocal(string channel, string message)
        {
            if (!_subChannelDict.TryGetValue(channel, out var users)) return;

            foreach (var kvp in users)
            {
                var userEx = kvp.Value;
                if (userEx.Handler != null)
                {
                    userEx.Handler(channel, message);
                }
                else
                {
                    RemoveUserFromMemory(userEx.UserData.ID);
                }
            }
        }

        public void LeaveUser(string userId)
        {
            if (_subSessionDict.TryRemove(userId, out var channels))
            {
                foreach (var channel in channels)
                {
                    if (_subChannelDict.TryGetValue(channel, out var users))
                    {
                        users.TryRemove(userId, out _);
                    }
                }
            }
        }

        public void RemoveUserFromMemory(string userId)
        {
            _subSessionDict.TryRemove(userId, out _);
        }

        public List<ChatUserData> GetUsersByChannel(string channel)
        {
            if (_subChannelDict.TryGetValue(channel, out var users))
            {
                return users.Values.Select(v => v.UserData).ToList();
            }
            return new List<ChatUserData>();
        }
        #endregion

        #region Utils
        public List<ChatGuildLogData> GetGuildLogData(string channel, DateTime loginTime)
        {
            string pattern = loginTime.ToString("yyyyMMdd") + "*";
            var entries = _chatDb.HashScan($"{Constance.LOG}{channel}", pattern);

            var logs = new List<ChatGuildLogData>();
            foreach (var entry in entries)
            {
                var log = JsonSerializer.Deserialize<ChatGuildLogData>(entry.Value.ToString(), _jsonOptions);
                if (log != null)
                {
                    log.Time = DateTime.ParseExact(entry.Name, "yyyyMMddHHmmss", null).ToUniversalTime();
                    logs.Add(log);
                }
            }
            return logs;
        }
        #endregion
    }
}
