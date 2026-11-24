using System.Net.WebSockets;
using System.Text;
using System.Text.Json;
using System.Threading.Channels;
using StackExchange.Redis;

namespace ChatServer;

public sealed class Chat
{
    private const int MaxRequestBytes = 4096;
    private const int MaxPendingSends = 256;
    private const int GuildLogCount = 100;
    private const int MaxGachaItems = 10;
    private const int MaxViolations = 50;
    private static readonly TimeSpan SendTimeout = TimeSpan.FromSeconds(10);
    private static readonly TimeSpan CloseTimeout = TimeSpan.FromSeconds(3);

    private readonly string _id = Guid.NewGuid().ToString("N");
    private readonly WebSocket _socket;
    private readonly RedisManager _redis;
    private readonly ILogger<Chat> _logger;
    private readonly ChatPlayer _player;
    private readonly Channel<string> _sends = Channel.CreateBounded<string>(new BoundedChannelOptions(MaxPendingSends) { SingleReader = true });
    private readonly CancellationTokenSource _receive = new();
    private readonly RateLimit _commandLimit = new(20, 10);
    private readonly RateLimit _messageLimit = new(5, 2);
    private readonly RateLimit _gachaLimit = new(3, 1);
    private int _closeStatus;
    private int _violations;

    public Chat(WebSocket socket, ChatUserData user, RedisManager redis, ILogger<Chat> logger)
    {
        _socket = socket;
        _redis = redis;
        _logger = logger;
        _player = new ChatPlayer(_id, user, redis, Enqueue);
    }

    public async Task RunAsync(CancellationToken stopping)
    {
        var sending = SendLoopAsync();
        using var registration = stopping.Register(() => BeginClose(WebSocketCloseStatus.EndpointUnavailable));
        try
        {
            await _player.InitAsync();
            Send(new res_ChatLogin { Command = CHAT_COMMAND.CT_LOGIN, ChannelID = _player.NormalChannel, GuildChannelID = _player.GuildChannel });
            _logger.LogInformation("Connected {Id} uid={Uid}", _id, _player.UserData.UserUID);
            BeginClose(await ReceiveLoopAsync());
        }
        catch (Exception ex) when (ex is WebSocketException or OperationCanceledException)
        {
            _socket.Abort();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Session {Id} failed", _id);
            BeginClose(WebSocketCloseStatus.InternalServerError);
        }
        finally
        {
            await _player.LeaveAllChannelAsync();
            BeginClose(WebSocketCloseStatus.NormalClosure);
            await sending;
            _receive.Dispose();
            _logger.LogInformation("Disconnected {Id}", _id);
        }
    }

    // 남은 메시지를 보낸 뒤 닫는다(처음 호출한 이유만 유효)
    private void BeginClose(WebSocketCloseStatus status)
    {
        Interlocked.CompareExchange(ref _closeStatus, (int)status, 0);
        _sends.Writer.TryComplete();
    }

    private async Task<WebSocketCloseStatus> ReceiveLoopAsync()
    {
        var buffer = new byte[MaxRequestBytes];
        var length = 0;
        while (true)
        {
            var result = await _socket.ReceiveAsync(buffer.AsMemory(length), _receive.Token);
            if (result.MessageType == WebSocketMessageType.Close) return WebSocketCloseStatus.NormalClosure;
            if (result.MessageType != WebSocketMessageType.Text) return WebSocketCloseStatus.InvalidMessageType;

            length += result.Count;
            if (!result.EndOfMessage)
            {
                if (length == buffer.Length) return WebSocketCloseStatus.MessageTooBig;
                continue;
            }
            if (!await HandleAsync(buffer.AsMemory(0, length))) return WebSocketCloseStatus.NormalClosure;
            if (_violations > MaxViolations) return WebSocketCloseStatus.PolicyViolation;
            length = 0;
        }
    }

    // false면 연결 종료
    private async Task<bool> HandleAsync(ReadOnlyMemory<byte> data)
    {
        var command = CHAT_COMMAND.CT_LOGIN;
        try
        {
            command = EncodingJson.Deserialize<req_Command>(data.Span).Command;
            if (!_commandLimit.TryTake())
            {
                ++_violations;
                SendCode(command, RETURN_CODE.RC_CHAT_RATE_LIMITED);
                return true;
            }
            if (command == CHAT_COMMAND.CT_LOGOUT) return false;
            await HandleCommandAsync(command, data);
        }
        catch (JsonException)
        {
            ++_violations;
            SendCode(command, RETURN_CODE.RC_CHAT_INVALID_REQUEST);
        }
        catch (Exception ex) when (ex is RedisException or TimeoutException)
        {
            _logger.LogError("Command {Command} failed: {Error}", command, ex.Message);
            SendCode(command, RETURN_CODE.RC_CHAT_SERVER_ERROR);
        }
        return true;
    }

    private async Task HandleCommandAsync(CHAT_COMMAND command, ReadOnlyMemory<byte> data)
    {
        switch (command)
        {
            case CHAT_COMMAND.CT_RECONNECT:
            {
                var channelId = EncodingJson.Deserialize<req_ChatReConnect>(data.Span).ChannelID;
                var code = channelId == 0 || channelId == _player.NormalChannel ? RETURN_CODE.RC_OK : await _player.ChangeChannelAsync(channelId);
                SendCode(command, code);
                break;
            }
            case CHAT_COMMAND.CT_INFO:
            {
                var type = EncodingJson.Deserialize<req_ChatInfo>(data.Span).ChatType;
                SendChannel(command, type, _player.GetChannel(type) != null ? RETURN_CODE.RC_OK : RETURN_CODE.RC_CHAT_NOT_IN_CHANNEL);
                break;
            }
            case CHAT_COMMAND.CT_GUILD_LOG:
                await SendGuildLogAsync();
                break;
            case CHAT_COMMAND.CT_MESSAGE:
            {
                var request = EncodingJson.Deserialize<req_ChatMessage>(data.Span);
                var code = _messageLimit.TryTake()
                    ? await _player.SendMessageAsync(request.ChatType, request.LogData?.Message)
                    : RETURN_CODE.RC_CHAT_RATE_LIMITED;
                if (code != RETURN_CODE.RC_OK) SendCode(command, code);
                break;
            }
            case CHAT_COMMAND.CT_CHANNEL_CHANGE:
            {
                var request = EncodingJson.Deserialize<req_ChatChannel>(data.Span);
                SendChannel(command, CHAT_TYPE.CT_NORMAL, await _player.ChangeChannelAsync(request.ChannelID));
                break;
            }
            case CHAT_COMMAND.CT_CHANNEL_ENTER:
            {
                var request = EncodingJson.Deserialize<req_ChatChannel>(data.Span);
                SendChannel(command, request.ChatType, await _player.EnterChannelAsync(request.ChatType, request.ChannelID));
                break;
            }
            case CHAT_COMMAND.CT_CHANNEL_LEAVE:
            {
                var type = EncodingJson.Deserialize<req_ChatChannel>(data.Span).ChatType;
                SendChannel(command, type, await _player.LeaveChannelAsync(type));
                break;
            }
            case CHAT_COMMAND.CT_NOTICE_GACHA:
                await PublishGachaAsync(EncodingJson.Deserialize<req_ChatGachaNotice>(data.Span));
                break;
            default:
                SendCode(command, RETURN_CODE.RC_CHAT_INVALID_REQUEST);
                break;
        }
    }

    private async Task SendGuildLogAsync()
    {
        if (_player.GetChannel(CHAT_TYPE.CT_GUILD) is not { } channel)
        {
            SendCode(CHAT_COMMAND.CT_GUILD_LOG, RETURN_CODE.RC_CHAT_NOT_IN_CHANNEL);
            return;
        }
        Send(new res_ChatGuildLog { Command = CHAT_COMMAND.CT_GUILD_LOG, LogDataList = await _redis.GetGuildLogAsync(channel, GuildLogCount) });
    }

    private async Task PublishGachaAsync(req_ChatGachaNotice request)
    {
        var items = request.ItemIDList ?? [];
        var chars = request.CharIDList ?? [];
        var count = items.Count + chars.Count;
        var code = count is 0 or > MaxGachaItems ? RETURN_CODE.RC_CHAT_INVALID_REQUEST
            : !_gachaLimit.TryTake() ? RETURN_CODE.RC_CHAT_RATE_LIMITED
            : RETURN_CODE.RC_OK;
        if (code != RETURN_CODE.RC_OK)
        {
            SendCode(CHAT_COMMAND.CT_NOTICE_GACHA, code);
            return;
        }

        var notice = new res_ChatGachaNotice
        {
            Command = CHAT_COMMAND.CT_NOTICE_GACHA, UserName = _player.UserData.UserName, ItemIDList = items, CharIDList = chars
        };
        foreach (var type in new[] { CHAT_TYPE.CT_NORMAL, CHAT_TYPE.CT_GUILD })
        {
            if (_player.GetChannel(type) is { } channel) await _redis.PublishAsync(channel, notice);
        }
    }

    private void SendChannel(CHAT_COMMAND command, CHAT_TYPE type, RETURN_CODE code)
    {
        var response = new res_ChatChannel { Command = command, ReturnCode = code, ChatType = type, ChannelID = _player.GetChannelId(type) };
        if (code == RETURN_CODE.RC_OK && _player.GetChannel(type) is { } channel) response.ChannelUserDataList = _redis.GetUsersByChannel(channel);
        Send(response);
    }

    private void SendCode(CHAT_COMMAND command, RETURN_CODE code) => Send(new res_Command { Command = command, ReturnCode = code });

    private void Send<T>(T response) => Enqueue(EncodingJson.Serialize(response));

    // 수신이 밀린 클라이언트는 끊어 서버 메모리를 보호
    private void Enqueue(string message)
    {
        if (!_sends.Writer.TryWrite(message) && _sends.Writer.TryComplete()) _socket.Abort();
    }

    private async Task SendLoopAsync()
    {
        try
        {
            await foreach (var message in _sends.Reader.ReadAllAsync())
            {
                using var timeout = new CancellationTokenSource(SendTimeout);
                await _socket.SendAsync(Encoding.UTF8.GetBytes(message), WebSocketMessageType.Text, true, timeout.Token);
            }
            if (_socket.State is WebSocketState.Open or WebSocketState.CloseReceived)
            {
                using var timeout = new CancellationTokenSource(CloseTimeout);
                await _socket.CloseOutputAsync((WebSocketCloseStatus)_closeStatus, null, timeout.Token);
            }
            _receive.CancelAfter(CloseTimeout);
        }
        catch (Exception ex) when (ex is WebSocketException or OperationCanceledException or ObjectDisposedException or InvalidOperationException)
        {
            _sends.Writer.TryComplete();
            _socket.Abort();
        }
    }
}
