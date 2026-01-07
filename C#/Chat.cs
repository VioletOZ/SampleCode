using System;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Text;
using System.IO;
using WebSocketSharp;
using WebSocketSharp.Server;
using StackExchange.Redis;

namespace ChatServer
{
    public class Chat : WebSocketBehavior
    {
        private string _suffix = string.Empty;
        private Action<RedisChannel, RedisValue> _redisHandler;
        
        // JSON 옵션 캐싱
        private static readonly JsonSerializerOptions _jsonOptions = new JsonSerializerOptions
        {
            Converters = { new JsonStringEnumConverter() },
            PropertyNameCaseInsensitive = true
        };

        public ChatPlayer Player { get; private set; }

        public Chat()
        {
            // Redis 메시지 수신 시 클라이언트로 즉시 전송하는 핸들러 초기화
            _redisHandler = (channel, value) => SendAsync(value.ToString(), null);
        }

        #region Properties
        public string Suffix
        {
            get => _suffix;
            set => _suffix = value ?? string.Empty;
        }
        #endregion

        #region WebSocket Events
        protected override void OnOpen()
        {
            try
            {
                var headers = Context.Headers;
				string sessionId = headers["SessionID"];
				string name = headers["UserName"];
				string uidStr = headers["UserUID"];
                
				if (string.IsNullOrWhiteSpace(sessionId) || string.IsNullOrWhiteSpace(name) || 
					!long.TryParse(uidStr, out long uid)) 
				{
					Log($"[Auth Error] Invalid Headers. Session: {sessionId}, Name: {name}, UID: {uidStr}");
					CloseAsync();
					return;
				}

                int.TryParse(headers["GuildID"], out int guildId);
                int.TryParse(headers["FavoriteCharacterID"], out int charId);

                Log($"Connected Client: {Sessions.Count} (UID: {uid})");
                _ = InitClientAsync(sessionId, uid, name, charId, guildId);
            }
            catch (Exception ex)
            {
                Log($"OnOpen Error: {ex.Message}");
            }
        }

        protected override async void OnMessage(MessageEventArgs args)
        {
            if (string.IsNullOrEmpty(args.Data)) return;

            try
            {
                Log($"Msg from {ID}: {args.Data}");
                var commandBase = JsonSerializer.Deserialize<req_Command>(args.Data, _jsonOptions);
                if (commandBase == null) return;

                await HandleCommandAsync(commandBase.Command, args.Data);
            }
            catch (Exception ex)
            {
                Log($"OnMessage Error: {ex.Message}");
                SendAsync($"Error: {ex.Message}", null);
            }
        }

        protected override void OnClose(CloseEventArgs args)
        {
            try
            {
                Log($"OnClose ID: {ID}, Reason: {args.Reason}");
                Player?.LeaveAllChannel();
                
                Task.Run(() => RedisManager.Instance.RemoveUserDict(ID));
                Log($"Remaining Sessions: {Sessions.Count}");
            }
            catch (Exception ex)
            {
                Log($"OnClose Error: {ex.Message}");
            }
        }

        protected override void OnError(WebSocketSharp.ErrorEventArgs e)
        {
            Log($"OnError {ID}: {e.Message}");
            if (ID != null) Task.Run(() => RedisManager.Instance.RemoveUserDict(ID));
        }
        #endregion

        #region Command Handlers
        private async Task HandleCommandAsync(CHAT_COMMAND command, string rawData)
        {
            switch (command)
            {
                case CHAT_COMMAND.CT_RECONNECT:
                    await ProcessReconnect(rawData);
                    break;
                case CHAT_COMMAND.CT_INFO:
                    ProcessGetChannelInfo(rawData);
                    break;
                case CHAT_COMMAND.CT_GUILD_LOG:
                    ProcessGetGuildLog();
                    break;
                case CHAT_COMMAND.CT_MESSAGE:
                    var msg = JsonSerializer.Deserialize<req_ChatMessage>(rawData, _jsonOptions);
                    await Player.SendMessage(msg);
                    break;
                case CHAT_COMMAND.CT_CHANNEL_CHANGE:
                    await ProcessChannelChange(rawData);
                    break;
                case CHAT_COMMAND.CT_CHANNEL_ENTER:
                    await ProcessChannelEnter(rawData);
                    break;
                case CHAT_COMMAND.CT_CHANNEL_LEAVE:
                    await ProcessChannelLeave(rawData);
                    break;
                case CHAT_COMMAND.CT_NOTICE_GACHA:
                    await ProcessGachaNotice(rawData);
                    break;
                case CHAT_COMMAND.CT_LOGOUT:
                    OnClose(null);
                    break;
                default:
                    Log($"Unknown Command: {command}");
                    break;
            }
        }

        private async Task ProcessReconnect(string data)
        {
            var req = JsonSerializer.Deserialize<req_ChatReConnect>(data, _jsonOptions);
            Player.Reconnect(req);

            var res = new res_ChatReConnect 
            { 
                Command = CHAT_COMMAND.CT_RECONNECT,
                ReturnCode = (ConnectionState == WebSocketState.Open) ? RETURN_CODE.RC_OK : RETURN_CODE.RC_CHAT_LOGIN_FAIL 
            };
            SendResponse(res);
        }

        private void ProcessGetChannelInfo(string data)
        {
            var req = JsonSerializer.Deserialize<req_ChatInfo>(data, _jsonOptions);
            string channel = (req.ChatType == CHAT_TYPE.CT_NORMAL) ? Player.GetNormalChannel() : Player.GetGuildChannel();
            
            var res = new res_ChatInfo
            {
                Command = CHAT_COMMAND.CT_INFO,
                ReturnCode = RETURN_CODE.RC_OK,
                ChatType = req.ChatType,
                ChannelID = req.ChannelID,
                ChannelUserDataList = RedisManager.Instance.GetUsersByChannel(channel)
            };
            SendResponse(res);
        }

        private async Task ProcessChannelChange(string data)
        {
            var req = JsonSerializer.Deserialize<req_ChatChangeChannel>(data, _jsonOptions);
            var success = await Player.ChangeChannel(req, Player.UserData, _redisHandler);

            var res = new res_ChatChangeChannel
            {
                Command = CHAT_COMMAND.CT_CHANNEL_CHANGE,
                ReturnCode = success ? RETURN_CODE.RC_OK : RETURN_CODE.RC_CHAT_DUPLICATE_CHANNEL,
                ChannelID = Player.NormalChannel,
                ChannelUserDataList = RedisManager.Instance.GetUsersByChannel(Player.GetNormalChannel())
            };
            SendResponse(res);
        }

        private async Task ProcessGachaNotice(string data)
        {
            var req = JsonSerializer.Deserialize<req_ChatGachaNotice>(data, _jsonOptions);
            Player.GachaNotice(req);

            var res = new res_ChatGachaNotice
            {
                Command = CHAT_COMMAND.CT_NOTICE_GACHA,
                ReturnCode = RETURN_CODE.RC_OK,
                UserName = Player.UserData.UserName,
                ItemIDList = req.ItemIDList,
                CharIDList = req.CharIDList
            };

            await RedisManager.Instance.GachaPublish(Player.GetNormalChannel(), res);
            await RedisManager.Instance.GachaPublish(Player.GetGuildChannel(), res);
        }
        #endregion

        #region Helpers
        public async Task<bool> InitClientAsync(string sessionId, long uid, string name, int charId, int guildId)
        {
            if (Player != null) return false;

            Player = new ChatPlayer(ID, sessionId, uid, name, guildId, charId);
            
            if (!await Player.AuthVerify())
            {
                Log($"Auth Verify Failed: {uid}");
                Close(CloseStatusCode.ServerError, "AuthFail");
                return false;
            }

            // 기본 채널 구독 (System, Notice, Normal)
            await Player.EnterChannel(CHAT_TYPE.CT_SYSTEM, 0, _redisHandler);
            await Player.EnterChannel(CHAT_TYPE.CT_GM_NOTICE, 0, _redisHandler);
            await Player.EnterChannel(CHAT_TYPE.CT_NORMAL, 1, _redisHandler);

            if (guildId > 0)
                await Player.EnterChannel(CHAT_TYPE.CT_GUILD, guildId, _redisHandler);

            SendResponse(new res_ChatLogin
            {
                ReturnCode = RETURN_CODE.RC_OK,
                ChannelID = Player.NormalChannel,
                GuildChannelID = Player.GuildChannel
            });

            return true;
        }

        private void SendResponse<T>(T response)
        {
            string json = JsonSerializer.Serialize(response, _jsonOptions);
            SendAsync(json, null);
        }

        private void Log(string message) => Logger.WriteLog(message);
        #endregion
    }
}
