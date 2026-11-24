namespace ChatServer;

// 연결 하나의 요청은 순서대로 처리되므로 락이 필요 없다
public sealed class ChatPlayer(string id, ChatUserData userData, RedisManager redis, Action<string> send)
{
    public const int MaxNormalChannel = 100;
    public const int MaxMessageLength = 200;
    private const string SystemChannel = "Chat:System";
    private const string NoticeChannel = "Chat:Notice";

    private readonly HashSet<string> _channels = [];

    public ChatUserData UserData => userData;
    public int NormalChannel { get; private set; }
    public int GuildChannel { get; private set; }

    public string? GetChannel(CHAT_TYPE type) => type switch
    {
        CHAT_TYPE.CT_NORMAL when NormalChannel > 0 => $"Chat:Normal:{NormalChannel}",
        CHAT_TYPE.CT_GUILD when GuildChannel > 0 => $"Chat:Guild:{GuildChannel}",
        _ => null
    };

    public int GetChannelId(CHAT_TYPE type) => type == CHAT_TYPE.CT_GUILD ? GuildChannel : NormalChannel;

    public async Task InitAsync()
    {
        await JoinAsync(SystemChannel);
        await JoinAsync(NoticeChannel);
        await EnterChannelAsync(CHAT_TYPE.CT_NORMAL, 1);
        if (userData.GuildID > 0) await EnterChannelAsync(CHAT_TYPE.CT_GUILD, userData.GuildID);
    }

    public async Task<RETURN_CODE> EnterChannelAsync(CHAT_TYPE type, int channelId)
    {
        if (type == CHAT_TYPE.CT_GUILD)
        {
            if (userData.GuildID <= 0 || channelId != userData.GuildID) return RETURN_CODE.RC_CHAT_INVALID_REQUEST;
            if (GuildChannel > 0) return RETURN_CODE.RC_CHAT_DUPLICATE_CHANNEL;
            await JoinAsync($"Chat:Guild:{channelId}");
            GuildChannel = channelId;
            return RETURN_CODE.RC_OK;
        }

        if (type != CHAT_TYPE.CT_NORMAL || NormalChannel > 0 && channelId != NormalChannel) return RETURN_CODE.RC_CHAT_INVALID_REQUEST;
        return await ChangeChannelAsync(channelId);
    }

    public async Task<RETURN_CODE> ChangeChannelAsync(int channelId)
    {
        if (channelId is < 1 or > MaxNormalChannel) return RETURN_CODE.RC_CHAT_INVALID_REQUEST;
        if (channelId == NormalChannel) return RETURN_CODE.RC_CHAT_DUPLICATE_CHANNEL;

        // 새 채널에 들어간 뒤 이전 채널에서 나온다(실패하면 이전 채널 유지)
        var previous = GetChannel(CHAT_TYPE.CT_NORMAL);
        await JoinAsync($"Chat:Normal:{channelId}");
        NormalChannel = channelId;
        if (previous != null) await LeaveAsync(previous);
        return RETURN_CODE.RC_OK;
    }

    public async Task<RETURN_CODE> LeaveChannelAsync(CHAT_TYPE type)
    {
        if (GetChannel(type) is not { } channel) return RETURN_CODE.RC_CHAT_NOT_IN_CHANNEL;

        if (type == CHAT_TYPE.CT_GUILD) GuildChannel = 0;
        else NormalChannel = 0;
        await LeaveAsync(channel);
        return RETURN_CODE.RC_OK;
    }

    public async Task<RETURN_CODE> SendMessageAsync(CHAT_TYPE type, string? text)
    {
        if (type is not (CHAT_TYPE.CT_NORMAL or CHAT_TYPE.CT_GUILD)) return RETURN_CODE.RC_CHAT_INVALID_REQUEST;
        if (string.IsNullOrWhiteSpace(text) || text.Length > MaxMessageLength) return RETURN_CODE.RC_CHAT_INVALID_REQUEST;
        if (GetChannel(type) is not { } channel) return RETURN_CODE.RC_CHAT_NOT_IN_CHANNEL;

        var log = new ChatLogData { UserUID = userData.UserUID, UserName = userData.UserName, Message = text, Time = DateTime.UtcNow };
        await redis.PublishAsync(channel, new res_ChatMessage
        {
            Command = CHAT_COMMAND.CT_MESSAGE, ChatType = type, ChannelID = GetChannelId(type), LogData = log
        });
        if (type == CHAT_TYPE.CT_GUILD) await redis.AddGuildLogAsync(channel, log);
        return RETURN_CODE.RC_OK;
    }

    public async Task LeaveAllChannelAsync()
    {
        foreach (var channel in _channels.ToArray()) await LeaveAsync(channel);
    }

    private async Task JoinAsync(string channel)
    {
        await redis.SubscribeAsync(channel, id, userData, send);
        _channels.Add(channel);
    }

    private async Task LeaveAsync(string channel)
    {
        await redis.UnsubscribeAsync(channel, id);
        _channels.Remove(channel);
    }
}
