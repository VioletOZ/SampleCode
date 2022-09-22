using System;
using System.Text.Json;
using System.Text.Json.Serialization;
using WebSocketSharp;
using WebSocketSharp.Server;

using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

using StackExchange.Redis;
using System.Text;
using System.IO;
using System.Diagnostics;

namespace ChatServer
{
    class Chat : WebSocketBehavior
    {
        private string _suffix;
        private BlockingCollection<string> outQueue = new BlockingCollection<string>();
        // Redis 구독메시지올경우 메시지 처리
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
                                                    SendAsync(value.ToString(), null);
                                                });
                }
                return this.m_onRedisMessageHandler;
            }
        }

        public Action<string> OnSendMessage
        {
            get
            {
                return new Action<string>((value) =>
               {
                   SendAsync(value, null);
               });
            }
        }
        

        JsonSerializerOptions options = new JsonSerializerOptions
        {
            Converters =
                {
                    new JsonStringEnumConverter()
                }
        };

        enum SslProtocolHack
        {
            Tls = 192,
            Tls11 = 768,
            Tls12 = 3072
        }

        //private BlockingCollection<string> _blockQueue;
        public MessageQueue m_MessageQueue;
        public ChatPlayer m_ChatPlayer;


        public Chat()
        {
            this._suffix = String.Empty;
            this.m_MessageQueue = new MessageQueue();
            this.m_ChatPlayer = null;

    }

        public string Suffix
        {
            get
            {
                return _suffix;
            }

            set
            {
                _suffix = value ?? String.Empty;
            }
        }

        protected override async void OnMessage(MessageEventArgs args)
        {
            try
            {
                Logger.WriteLog("OnMessage ID : " + ID + "  Msg : " + args.Data);
                if (args.Data == null)
                {
                    Logger.WriteLog("Chat OnMessage Data Null");
                    return;
                }

                MemoryStream stream;
                req_Command command = new req_Command();
                try
                {
                    stream = new MemoryStream(Encoding.UTF8.GetBytes(args.Data));
                    command = JsonSerializer.Deserialize<req_Command>(stream, options);
                }
                catch (Exception e)
                {
                    Logger.WriteLog("Chat OnMessage Json Parse Error : " + e.Message);
                    SendAsync("Send Data Error", null);
                    return;
                }


                if (command == null)
                {
                    Logger.WriteLog("Chat OnMessage Command Null");
                    return;
                }

                stream = new MemoryStream(Encoding.UTF8.GetBytes(args.Data));
                string channel;
                // 

                switch (command.Command)
                {
                    case CHAT_COMMAND.CT_LOGIN:
                        break;

                    case CHAT_COMMAND.CT_RECONNECT:
                        m_ChatPlayer.Reconnect((req_ChatReConnect)EncodingJson.Deserialize<req_ChatReConnect>(stream));

                        res_ChatReConnect reconnect = new res_ChatReConnect();
                        reconnect.Command = CHAT_COMMAND.CT_RECONNECT;
                        reconnect.ReturnCode = RETURN_CODE.RC_OK;

                        switch (this.ConnectionState)
                        {
                            case WebSocketState.Connecting:
                            case WebSocketState.Open:
                                break;

                            case WebSocketState.Closing:
                            case WebSocketState.Closed:
                                reconnect.ReturnCode = RETURN_CODE.RC_CHAT_LOGIN_FAIL;
                                break;
                        }

                        SendAsync(EncodingJson.Serialize(reconnect), null);
                        break;

                    case CHAT_COMMAND.CT_LOGOUT:
                        //m_ChatPlayer.LogOut(JsonSerializer.Deserialize<req_ChatLogout>(stream, new JsonSerializerOptions()));
                        //정상 종료가 있을까?? 종료는 OnClose 로 넘긴다
                        // 필요시에 종료 Code 와 Reason 작성.
                        this.OnClose(null);
                        break;

                    case CHAT_COMMAND.CT_INFO:
                        req_ChatInfo infoMessage = (req_ChatInfo)EncodingJson.Deserialize<req_ChatInfo>(stream);

                        //m_ChatPlayer.ChannelInfo(infoMessage);
                        //
                        if (CHAT_TYPE.CT_NORMAL == infoMessage.ChatType)
                            channel = m_ChatPlayer.GetNormalChannel();
                        else
                            channel = m_ChatPlayer.GetGuildChannel();

                        res_ChatInfo info = new res_ChatInfo();
                        List<ChatUserData> userData = RedisManager.Instance.GetUsersByChannel(channel);

                        info.ReturnCode = RETURN_CODE.RC_OK;
                        if (userData == null)
                            info.ReturnCode = RETURN_CODE.RC_FAIL;
                        info.Command = CHAT_COMMAND.CT_INFO;
                        info.ChatType = infoMessage.ChatType;
                        info.ChannelID = infoMessage.ChannelID;
                        info.ChannelUserDataList = userData;

                        SendAsync(JsonSerializer.Serialize<res_ChatInfo>(info, options), null);
                        break;

                    case CHAT_COMMAND.CT_GUILD_LOG:
                        req_ChatGuildLog logMessage = (req_ChatGuildLog)EncodingJson.Deserialize<req_ChatGuildLog>(stream);                        
                        res_ChatGuildLog guildLog = new res_ChatGuildLog();
                        List<ChatGuildLogData> logs = new List<ChatGuildLogData>();
                        logs = m_ChatPlayer.GetGuildLog();

                        guildLog.Command = CHAT_COMMAND.CT_GUILD_LOG;
                        guildLog.ReturnCode = RETURN_CODE.RC_OK;
                        guildLog.GuildLogDataList = logs;

                        SendAsync(JsonSerializer.Serialize<res_ChatGuildLog>(guildLog, options), null);
                        break;

                    case CHAT_COMMAND.CT_LEADER_CHANGE:             //10
                        req_ChatLeaderChange leaderMessage = JsonSerializer.Deserialize<req_ChatLeaderChange>(stream, new JsonSerializerOptions());
                        res_ChatLeaderChange leaderChange = new res_ChatLeaderChange();
                        leaderChange.ReturnCode = RETURN_CODE.RC_OK;
                        if (!await m_ChatPlayer.LeaderChange(leaderMessage))
                            leaderChange.ReturnCode = RETURN_CODE.RC_FAIL;

                        leaderChange.Command = CHAT_COMMAND.CT_LEADER_CHANGE;

                        SendAsync(JsonSerializer.Serialize<res_ChatLeaderChange>(leaderChange, options), null);
                        break;

                    case CHAT_COMMAND.CT_NORMAL_LOG:
                        break;

                    case CHAT_COMMAND.CT_MESSAGE:
                        await m_ChatPlayer.SendMessage(JsonSerializer.Deserialize<req_ChatMessage>(stream, new JsonSerializerOptions()));
                        break;

                    case CHAT_COMMAND.CT_CHANNEL_CHANGE:            //11
                        string beforeChannel = m_ChatPlayer.GetNormalChannel();
                        req_ChatChangeChannel changeMessage = JsonSerializer.Deserialize<req_ChatChangeChannel>(stream, new JsonSerializerOptions());
                        res_ChatChangeChannel changeChannel = new res_ChatChangeChannel();

                        changeChannel.ReturnCode = RETURN_CODE.RC_OK;
                        if (!await m_ChatPlayer.ChangeChannel(changeMessage, m_ChatPlayer.UserData, OnRedisMessageHandler))
                            changeChannel.ReturnCode = RETURN_CODE.RC_CHAT_DUPLICATE_CHANNEL;

                        changeChannel.Command = command.Command;
                        changeChannel.ChannelID = m_ChatPlayer.NormalChannel;                        
                        changeChannel.ChannelUserDataList = RedisManager.Instance.GetUsersByChannel(m_ChatPlayer.GetNormalChannel());

                        SendAsync(JsonSerializer.Serialize<res_ChatChangeChannel>(changeChannel, options), null);

                        break;

                    case CHAT_COMMAND.CT_CHANNEL_ENTER:             //12
                        req_ChatEnterChannel enterMessage = JsonSerializer.Deserialize<req_ChatEnterChannel>(stream, new JsonSerializerOptions());
                        res_ChatEnterChannel enterChannel = new res_ChatEnterChannel();

                        enterChannel.ReturnCode = RETURN_CODE.RC_OK;
                        if (!await m_ChatPlayer.EnterChannel(enterMessage.ChatType, enterMessage.ChannelID, OnRedisMessageHandler))
                            enterChannel.ReturnCode = RETURN_CODE.RC_FAIL;

                        if (CHAT_TYPE.CT_NORMAL == enterMessage.ChatType)
                            channel = m_ChatPlayer.GetNormalChannel();
                        else
                            channel = m_ChatPlayer.GetGuildChannel();

                        enterChannel.Command = CHAT_COMMAND.CT_CHANNEL_ENTER;
                        enterChannel.ChatType = enterMessage.ChatType;
                        enterChannel.ChannelID = enterMessage.ChannelID;
                        enterChannel.ChannelUserDataList = RedisManager.Instance.GetUsersByChannel(channel);

                        SendAsync(JsonSerializer.Serialize<res_ChatEnterChannel>(enterChannel, options), null);
                        break;

                    case CHAT_COMMAND.CT_CHANNEL_LEAVE:
                        req_ChatLeaveChannel leaveMessage = JsonSerializer.Deserialize<req_ChatLeaveChannel>(stream, new JsonSerializerOptions());
                        res_ChatLeaveChannel leaveChannel = new res_ChatLeaveChannel();

                        leaveChannel.ReturnCode = RETURN_CODE.RC_OK;
                        if (!await m_ChatPlayer.LeaveChannel(leaveMessage.ChatType))
                            leaveChannel.ReturnCode = RETURN_CODE.RC_FAIL;
                        leaveChannel.ChatType = leaveMessage.ChatType;

                        leaveChannel.Command = CHAT_COMMAND.CT_CHANNEL_LEAVE;

                        SendAsync(JsonSerializer.Serialize<res_ChatLeaveChannel>(leaveChannel, options), null);
                        break;

                    case CHAT_COMMAND.CT_CHANNEL_ENTER_USER:
                        //m_ChatPlayer.UserStateChannel(CHAT_ENTER_STATE.CT_ENTER,);
                        break;

                    case CHAT_COMMAND.CT_CHANNEL_LEAVE_USER:
                        //m_ChatPlayer.LeaveUserChannel()
                        break;

                    case CHAT_COMMAND.CT_CHANNEL_RECEIVE_END:
                        res_ChatReceiveEnd resEnd = new res_ChatReceiveEnd();
                        resEnd.Command = CHAT_COMMAND.CT_CHANNEL_RECEIVE_END;
                        resEnd.ReturnCode = RETURN_CODE.RC_OK;
                        if (!m_ChatPlayer.ReceiveEnd())
                            resEnd.ReturnCode = RETURN_CODE.RC_FAIL;

                        SendAsync(EncodingJson.Serialize<res_ChatReceiveEnd>(resEnd), null);
                        break;

                    case CHAT_COMMAND.CT_NOTICE_GACHA:
                        req_ChatGachaNotice gachamessage = JsonSerializer.Deserialize<req_ChatGachaNotice>(stream, new JsonSerializerOptions());
                        m_ChatPlayer.GachaNotice(gachamessage);

                        res_ChatGachaNotice gachaNotice = new res_ChatGachaNotice();
                        gachaNotice.Command = CHAT_COMMAND.CT_NOTICE_GACHA;
                        gachaNotice.ReturnCode = RETURN_CODE.RC_OK;
                        gachaNotice.UserName = m_ChatPlayer.UserData.UserName;
                        gachaNotice.ItemIDList = gachamessage.ItemIDList;
                        gachaNotice.CharIDList = gachamessage.CharIDList;

                        //Sessions.BroadcastAsync(JsonSerializer.Serialize<res_ChatGachaNotice>(gachaNotice, options), null);

                        await RedisManager.Instance.GachaPublish(m_ChatPlayer.GetNormalChannel(), gachaNotice);
                        await RedisManager.Instance.GachaPublish(m_ChatPlayer.GetGuildChannel(), gachaNotice);

                        break;

                    default:
                        Logger.WriteLog("Request Command Error : " + command.Command);
                        break;
                }
            }
            catch (Exception e)
            {
                SendAsync("OnMessgae Error : " + e.Message, null);
            }
        }

        protected override void OnClose(CloseEventArgs args)
        {
            // 채팅서버만 끊길 경우를 대비해 재접속 관련 코드 추가 필요.
            // 레디스에서 세션확인 해야함
            // 지금은 일단 그냥 접속종료
            try
            {
                Logger.WriteLog("OnClose : " + ID);
                CloseAsync();
                // 
                if (m_ChatPlayer != null)
                {
                    m_ChatPlayer.LeaveAllChannel();
                }

                //_ = RedisManager.Instance.UnSubscribe(m_ChatPlayer.GetNormalChannel(), m_ChatPlayer.UserData);
                //_ = RedisManager.Instance.UnSubscribe(m_ChatPlayer.GetGuildChannel(), m_ChatPlayer.UserData);

                //var sslProtocol = (System.Security.Authentication.SslProtocols)(SslProtocolHack.Tls12 | SslProtocolHack.Tls11 | SslProtocolHack.Tls);

                // TLS 핸드세이크 오류 발생시 프로토콜 변경하여 재접속 처리\
                //if (args.Code == 1015 && Context.WebSocket.SslConfiguration.EnabledSslProtocols != sslProtocol)
                //{
                //    Context.WebSocket.SslConfiguration.EnabledSslProtocols = sslProtocol;
                //    Context.WebSocket.Connect();
                //}

                Logger.WriteLog("Session Close Count : " + Sessions.Count);
                Task.Run(() => RedisManager.Instance.RemoveUserDict(ID));
            }
            catch (Exception e)
            {
                // 정상적인 종료가 아닐경우 세션이 없다...
                Logger.WriteLog("OnClose Message : " + e.Message);                
                Task.Run(() => RedisManager.Instance.RemoveUserDict(ID));
            }
        }

        protected override void OnError(WebSocketSharp.ErrorEventArgs e)
        {
            Logger.WriteLog("OnError : " + e.Message);

            if (ID != null)
                Task.Run(() => RedisManager.Instance.RemoveUserDict(ID));
            //base.OnError(e);
        }

        // Socket 연결시 호출
        protected override void OnOpen()
        {
            try
            {
                // Redis 에서 해당유저 UID로 세션 검색.
                string sessionID = this.Headers.Get("SessionID") ?? "";               //SessionID
                long UID = 0;
                Int64.TryParse(this.Headers.Get("UserUID"), out UID);                   //UserUID
                string name = this.Headers.Get("UserName") ?? "";                     //Name - Redis에 없음
                int guildID = 0;
                Int32.TryParse(this.Headers.Get("GuildID"), out guildID);
                int charID = 0;
                Int32.TryParse(this.Headers.Get("FavoriteCharacterID"), out charID);

                if (sessionID == "" || UID == 0 || name == "")
                {
                    Logger.WriteLog("Chat OnOpen Request Header(Session) Error. SessionID : " + sessionID + " UID : " + UID);
                    CloseAsync();
                    return;
                }

                Logger.WriteLog("Connected Client : " + Sessions.Count);
                var result = InitClient(sessionID, UID, name, charID, guildID);

            }
            catch (Exception e)
            {
                Logger.WriteLog("OnOpen Error - " + e.Message + " SessionCount : " + Sessions.Count);
            }
        }

        public async Task<bool> InitClient(string sessionId, long uid, string name, int charId, int guildId = 0)
        {
            bool result = true;

            if (m_ChatPlayer != null)
            {
                Logger.WriteLog("Chat InitClient ChatPlayer not null : " + ID);
                result = false;
            }
            
            // 접속시에 유저정보 세팅
            m_ChatPlayer = new ChatPlayer(ID, sessionId, uid, name, guildId, charId);

            result  = await m_ChatPlayer.AuthVerify();
            if (!result)
            {
                Logger.WriteLog("세션 인증실패 - SessionID : " + sessionId + " " + uid + name);
                Close(CloseStatusCode.ServerError, "InvalidData");
                return false;
            }

            if (OnRedisMessageHandler == null)
            {
                Logger.WriteLog("Client Init Fail RedisHandler Create Fail : " + ID);
                result = false;
            }

            // 시스템, Notice 구독
            if (!await m_ChatPlayer.EnterChannel(CHAT_TYPE.CT_SYSTEM, 0, OnRedisMessageHandler))
            {
                Logger.WriteLog("Chat InitClient System Subscribe Fail : " + ID);
                result = false;
            }
                
            if (!await m_ChatPlayer.EnterChannel(CHAT_TYPE.CT_GM_NOTICE, 0, OnRedisMessageHandler))
            {
                Logger.WriteLog("Chat InitClient GM_Notice Subscribe Fail : " + ID);
                result = false;
            }
            
            // 접속시에 일반 채널 구독            
            int channelNum = 1;            
            result = await m_ChatPlayer.EnterChannel(CHAT_TYPE.CT_NORMAL, channelNum, OnRedisMessageHandler);
            if(!result)
            {
                Logger.WriteLog("Chat InitClient Redis Subscribe Fail : " + ID);
                Close(CloseStatusCode.ServerError, "Sbuscribe");
                return false;
            }

            // 길드가 있을경우 길드도 구독
            if (guildId > 0)
            {
                // 길드UID를 채널번호로 지정 . 
                result = await m_ChatPlayer.EnterChannel(CHAT_TYPE.CT_GUILD, guildId, OnRedisMessageHandler);
                if (!result)
                {
                    Logger.WriteLog("Chat InitClient EnterChannel Fail : " + ID);
                    Close(CloseStatusCode.ServerError, "Sbuscribe");
                    return false;
                }

            }

            res_ChatLogin resLogin = new res_ChatLogin();
            resLogin.ReturnCode = RETURN_CODE.RC_OK;
            resLogin.ChannelID = m_ChatPlayer.NormalChannel;
            resLogin.GuildChannelID = m_ChatPlayer.GuildChannel;

            string json = JsonSerializer.Serialize<res_ChatLogin>(resLogin, options);
            SendAsync(json, null);

            return true;
        }
    
    }
}
