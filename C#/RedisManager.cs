using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Threading;
using System.Threading.Tasks;
using StackExchange.Redis;
using StackExchange.Redis.MultiplexerPool;
using WebSocketSharp.Server;

using System.Text.Json;
using System.Text.Json.Serialization;
using System.Collections.Concurrent;

namespace ChatServer
{
    class RedisManager
    {
        private static volatile RedisManager _instance;
        private static object _syncRoot = new Object();

        public static RedisManager Instance
        {
            get
            {
                if (_instance == null)
                {
                    lock (_syncRoot)
                    {
                        if (_instance == null)
                            _instance = new RedisManager();
                    }
                }

                return _instance;
            }
        }

        private Action<RedisChannel, RedisValue> m_onRedisMessageHandler = null;
        public Action<RedisChannel, RedisValue> OnRedisMessageHandler
        {
            get
            {
                if (this.m_onRedisMessageHandler == null)
                {
                    this.m_onRedisMessageHandler = new Action<RedisChannel, RedisValue>
                                                ((channel, value) =>
                                                {
                                                    // 채널별로 메시지 전송
                                                    SendChannelUser(channel, value.ToString());
                                                    //SendAsync(value.ToString(), null);
                                                });
                }
                return this.m_onRedisMessageHandler;
            }
        }


        JsonSerializerOptions options = new JsonSerializerOptions
        {
            Converters =
                {
                    new JsonStringEnumConverter()
                }
        };

        private ConfigurationOptions _redisConfigurationOptions;

        private string _env { get; }

        // 구독중인 채널, Client Session
        private ConcurrentDictionary<string, ConcurrentDictionary<string, ChatUserDataEx>> _subChannelDict = new ConcurrentDictionary<string, ConcurrentDictionary<string, ChatUserDataEx>>();

        // SessionID, 구독중인 채널
        private ConcurrentDictionary<string, ConcurrentBag<string>> _subSessionDict = new ConcurrentDictionary<string, ConcurrentBag<string>>();

        // 
        private ConnectionMultiplexer _chatMultiPlexer = null;
        private SessionState _chatState = new SessionState();

        public ConnectionMultiplexer gameServerRedis = null;
        public SessionState gameServerState = new SessionState();

        
        public RedisManager()
        {
            _env = Constance.Env.ChatServerRedisDBServerIP;
            // 채팅서버 레디스
            if (Constance.Env.ChatServerRedisDBServerIP == null)
            {
                Logger.WriteLog("ChatServer Redis Connection Fail..");
                Environment.Exit(0);
            }

            _redisConfigurationOptions = ConfigurationOptions.Parse(_env);

            _redisConfigurationOptions.User = Constance.Env.ChatServerRedisDBUserID;
            _redisConfigurationOptions.Password = Constance.Env.ChatServerRedisDBPassword;

            _chatMultiPlexer = ConnectionMultiplexer.Connect(_redisConfigurationOptions);
            _chatState.ServerSessionID = "Server";
            _chatState.subscriber = _chatMultiPlexer.GetSubscriber();
            _chatState.db = _chatMultiPlexer.GetDatabase();

        }

        public bool ConnectGameServerRedis()
        {
            try
            {
                ConfigurationOptions redisConf;                

                string gameRedisIP = Constance.Env.GameServerRedisDBServerIP;

                // 게임서버 레디스
                if (gameRedisIP == null)
                {
                    Logger.WriteLog("GameServer Redis Unknown Addr or Port..");
                    Environment.Exit(0);
                }


                redisConf = ConfigurationOptions.Parse(gameRedisIP);
                if (redisConf == null)
                {
                    Logger.WriteLog("GameServer Redis Parse Error..");
                    Environment.Exit(0);
                }

                redisConf.AbortOnConnectFail = false;

                this.gameServerRedis = ConnectionMultiplexer.Connect(redisConf);
                this.gameServerState.ServerSessionID = "Game";
                this.gameServerState.db = gameServerRedis.GetDatabase();
                this.gameServerState.subscriber = gameServerRedis.GetSubscriber();

                if (this.gameServerRedis == null)
                {
                    Logger.WriteLog("GameServer Redis Connection Fail..");
                    Environment.Exit(0);
                }
            }
            catch (Exception)
            {
                Logger.WriteLog("GameServer Redis Error");
                Environment.Exit(0);
            }
            return true;
        }

        // 레디스 Session 검증 - 서버에서 접속시 등록된 Session으로 허용된 접근인지 확인
        public async Task<bool> AuthVerify(string SessionID)
        {
            try
            {
                var result = await gameServerState.db.KeyExistsAsync("Session:" + SessionID);
                if (!result)
                    return false;
            }
            catch (Exception e)
            {
                Logger.WriteLog("RedisManager AuthVerify Exception : " + e.Message);
                return false;
            }

            return true;
        }

        public async Task<bool> SubscribeAction(string channel, ChatUserData user, Action<RedisChannel, RedisValue> ac)
        {
            if (_chatState == null)
                return false;

            // 채널 기준으로 만든 Dict
            if (!_subChannelDict.ContainsKey(channel))
            {
                _subChannelDict.TryAdd(channel, new ConcurrentDictionary<string, ChatUserDataEx>());
                await _chatState.subscriber.SubscribeAsync(channel, OnRedisMessageHandler);
            }

            dynamic userEx = new ChatUserDataEx();
            userEx.UserData = user;
            userEx.Handler = ac;

            _subChannelDict[channel].TryAdd(user.ID, userEx);

            // 세션 기준으로 만든 DIct
            if (!_subSessionDict.ContainsKey(user.ID))
                _subSessionDict.TryAdd(user.ID, new ConcurrentBag<string>());

            _subSessionDict[user.ID].Add(channel);

            //await _chatState.subscriber.SubscribeAsync(channel, ac);

            return true;
        }

        // Redis Pub
        public async Task Publish(string channel, req_ChatMessage message)
        {
            Logger.WriteLog("Publish Message Channel : " + channel);

            // TODO: 보내기전에 Connect 확인
            //if (!isConnected)
            //    return;

            res_ChatMessage resMessage = new res_ChatMessage();
            resMessage.Command = CHAT_COMMAND.CT_MESSAGE;
            resMessage.ReturnCode = RETURN_CODE.RC_OK;
            resMessage.ChatType = message.ChatType;
            resMessage.ChannelID = message.ChannelID;
            resMessage.LogData = message.LogData;

            var publish = await _chatState.subscriber.PublishAsync(channel, EncodingJson.Serialize(resMessage));

            // 길드 채팅의 경우 내용저장
            // 길드채널
            //      ㄴ 채팅시간
            //              ㄴ 유저네임 : 내용
            if (CHAT_TYPE.CT_GUILD == message.ChatType)
            {
                string log = EncodingJson.Serialize(message.LogData);
                HashEntry[] hash =
                {
                    // 한국시간으로 변경.
                    new HashEntry(DateTime.Now.AddHours(9).ToString(format: "yyyyMMddHHmmss"), log)
                    //new HashEntry("data", message.LogData.UserName + message.LogData.Text)
                };

                // 길드 채팅 저장
                _ = _chatState.db.HashSetAsync(Constance.LOG + channel, hash);
                _ = _chatState.db.KeyExpireAsync(Constance.LOG, DateTime.Now.AddDays(7));
            }
            
        }

        // 서버에서 특정채널에 메시지만 보내도록 쓸려고 만든것
        public async Task ForcePublish(string channel, string message)
        {
            await _chatState.subscriber.PublishAsync(channel, message);
        }

        public async Task GachaPublish(string channel, res_ChatGachaNotice notiMessage)
        {
            //SendChannelUser(channel, EncodingJson.Serialize(notiMessage));
            await _chatState.subscriber.PublishAsync(channel, EncodingJson.Serialize(notiMessage));
        }

        public bool UnSubscribe(string channel, string ID)
        {
            try
            {
                // 채널 기준으로 먼저
                //if (_subChannelDict.ContainsKey(channel))
                //{
                //    int index = _subChannelDict[channel].FindIndex((ChatUserDataEx p) => p.UserData.ID == ID);
                //    _subChannelDict[channel].RemoveAt(index);

                //    // 해당 채널에 아무도없으면 삭제
                //    if (_subChannelDict[channel].Count <= 0)
                //        _subChannelDict.Remove(channel);
                //}

                // 세션 기준으로 삭제
                if (_subSessionDict.ContainsKey(ID))
                {
                    _subSessionDict[ID].TryTake(out channel);
                }

                // 구독 취소 는 하지않고 서버에서 해당 채널 DIct 에서만 제거
                //await _chatState.subscriber.UnsubscribeAsync(channel);
            }
            catch
            {
                // 세션 기준으로 삭제
                if (_subSessionDict.ContainsKey(ID))
                {
                    _subSessionDict[ID].TryTake(out channel);                    
                }
            }
            

            return true;
        }


        public void LeaveUser(string ID)
        {
            try
            {
                if (_subSessionDict.ContainsKey(ID))
                {
                    ConcurrentBag<string> temp = new ConcurrentBag<string>();
                    //foreach (var ch in _subSessionDict[ID])
                    //{
                        //int index = _subChannelDict[ch].FindIndex((ChatUserDataEx p) => p.UserData.ID == ID);
                        //_subChannelDict[ch].RemoveAt(index);
                    //}                    
                    _subSessionDict.TryRemove(ID, out temp);
                    foreach (string ch in temp)
                    {
                        ChatUserDataEx user = new ChatUserDataEx();
                        _subChannelDict[ch].TryRemove(ID, out user);
                    }
                }
            }
            catch
            {
            }
            
        }
        public async Task UnSubscribeAll()
        {
            await _chatState.subscriber.UnsubscribeAllAsync();
        }

        public List<ChatUserData> GetUsersByChannel(string channel)
        {
            try
            {
                List<ChatUserData> userList = new List<ChatUserData>();
                foreach (var ex in _subChannelDict[channel])
                {
                    userList.Add(ex.Value.UserData);
                }
                return userList;
            }
            catch (Exception e)
            {
                Logger.WriteLog("RedisManager GetUserByChannel subChannelDict Error :" + _chatState.ServerSessionID + ":" + e.Message);
                return null;
            }

        }

        public int GetPossibleChannel()
        {
            int count = 1;
            string channel = Constance.NORMAL + count.ToString();
            while(Constance.CHANNEL_PLAYER_MAX > _subChannelDict[channel].Count())
            {
                channel = Constance.NORMAL + Constance.POSSIBLE_CHANNEL_NUMBER.ToString();
                Constance.POSSIBLE_CHANNEL_NUMBER++;
                if (Constance.POSSIBLE_CHANNEL_NUMBER >= Constance.CHANNEL_MAX)
                    Constance.POSSIBLE_CHANNEL_NUMBER = 1;
            }
            return count;
        }

        public void SendChannelUser(string ch, string value)
        {
            
            foreach (var ex in _subChannelDict[ch])
            {
                if (ex.Value.Handler == null)
                {
                    RemoveUserDict(ex.Value.UserData.ID);
                }
                else
                {
                    ex.Value.Handler(ch, value);
                }
            }
        }

        public List<ChatGuildLogData> GetGuildLogData(string channel, DateTime loginTime)
        {
            string pattern = loginTime.ToString(format:"yyyyMMdd") + "*";
            var result = _chatState.db.HashScan(Constance.LOG + channel, pattern, Constance.PAGE_SIZE, Constance.LOG_COUNT, 0);

            List<ChatGuildLogData> logs = new List<ChatGuildLogData>();
            ChatGuildLogData log = new ChatGuildLogData();
            foreach (HashEntry entry in result)
            {
                log = JsonSerializer.Deserialize<ChatGuildLogData>(entry.Value.ToString());
                log.Time = DateTime.ParseExact(entry.Name, "yyyyMMddHHmmss", null).ToUniversalTime();
                logs.Add(log);
            }

            return logs;
        }

        public async Task GetUserHash(string ID)
        {
            var val = await _chatState.db.HashGetAsync("User:" + ID, "User");
            Logger.WriteLog("GetHash" + val);

        }

        public async Task GetString(string key)
        {
            var val = await _chatState.db.StringGetAsync(key);
            Logger.WriteLog("GetString : " + val);

        }

        public void RemoveUserDict(string ID)
        {
            if (_subSessionDict.ContainsKey(ID))
            {
                // 해당 세션의 채널들 삭제
                //if (_subSessionDict[ID].Count <= 0)
                //{
                //    foreach (string ch in _subSessionDict[ID])
                //    {
                //        int index = _subChannelDict[ch].FindIndex((ChatUserDataEx ex) => ex.UserData.ID == ID);
                //        _subChannelDict[ch].RemoveAt(index);
                //    }
                //}
                ConcurrentBag<string> temp = new ConcurrentBag<string>();
                _subSessionDict.TryRemove(ID, out temp);
            }

            return ;
        }
    }

}
