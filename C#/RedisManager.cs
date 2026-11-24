using System.Collections.Concurrent;
using System.Text.Json;
using StackExchange.Redis;

namespace ChatServer;

public sealed class RedisManager(IConfiguration config, ILogger<RedisManager> logger) : IAsyncDisposable
{
    private const string LogKeyPrefix = "ChatLog:";
    private const int MaxGuildLogs = 1000;
    private static readonly TimeSpan GuildLogLifetime = TimeSpan.FromDays(7);

    private sealed class ChannelState
    {
        public readonly SemaphoreSlim Gate = new(1, 1);
        public readonly ConcurrentDictionary<string, Member> Members = new();
        public ChannelMessageQueue? Queue;
        public bool Closed;
    }

    private sealed record Member(ChatUserData User, Action<string> Send);

    private readonly ConcurrentDictionary<string, ChannelState> _channels = new();
    private ConnectionMultiplexer? _chat;
    private ConnectionMultiplexer? _game;

    public bool IsConnected => _chat is { IsConnected: true } && _game is { IsConnected: true };
    public int ChannelCount => _channels.Count;

    private ISubscriber Subscriber => _chat!.GetSubscriber();

    public async Task ConnectAsync()
    {
        _chat = await ConnectAsync("ChatRedis");
        _game = await ConnectAsync("GameRedis");
    }

    private async Task<ConnectionMultiplexer> ConnectAsync(string name)
    {
        var options = ConfigurationOptions.Parse(config.GetConnectionString(name)
            ?? throw new InvalidOperationException($"ConnectionStrings:{name} is not set"));
        options.BacklogPolicy = BacklogPolicy.FailFast;
        var connection = await ConnectionMultiplexer.ConnectAsync(options);
        connection.ConnectionFailed += (_, e) => logger.LogError("{Name} disconnected: {Type}", name, e.FailureType);
        connection.ConnectionRestored += (_, _) => logger.LogInformation("{Name} restored", name);
        return connection;
    }

    // 유저 정보는 헤더가 아니라 게임 서버가 기록한 세션에서만 읽는다
    public async Task<ChatUserData?> AuthVerifyAsync(string sessionId)
    {
        var values = await _game!.GetDatabase().HashGetAsync($"Session:{sessionId}",
            ["UserUID", "UserName", "GuildID", "FavoriteCharacterID"]);
        if (!values[0].TryParse(out long uid) || uid <= 0 || values[1].IsNullOrEmpty) return null;

        values[2].TryParse(out int guildId);
        values[3].TryParse(out int charId);
        return new ChatUserData(uid, values[1].ToString(), guildId, charId);
    }

    public async Task SubscribeAsync(string channel, string id, ChatUserData user, Action<string> send)
    {
        while (true)
        {
            var state = _channels.GetOrAdd(channel, _ => new ChannelState());
            await state.Gate.WaitAsync();
            try
            {
                if (state.Closed) continue;
                if (state.Queue == null)
                {
                    try
                    {
                        state.Queue = await Subscriber.SubscribeAsync(RedisChannel.Literal(channel));
                    }
                    catch
                    {
                        Remove(channel, state);
                        throw;
                    }
                    // 큐 방식 구독은 채널 안의 메시지 순서를 보장
                    state.Queue.OnMessage(message => Deliver(state, message.Message.ToString()));
                }
                state.Members[id] = new Member(user, send);
                return;
            }
            finally
            {
                state.Gate.Release();
            }
        }
    }

    public async Task UnsubscribeAsync(string channel, string id)
    {
        if (!_channels.TryGetValue(channel, out var state)) return;

        await state.Gate.WaitAsync();
        try
        {
            if (!state.Members.TryRemove(id, out _) || !state.Members.IsEmpty) return;
            try
            {
                if (state.Queue != null) await state.Queue.UnsubscribeAsync();
            }
            catch (Exception ex) when (ex is RedisException or TimeoutException)
            {
                logger.LogWarning("Unsubscribe {Channel} failed: {Error}", channel, ex.Message);
            }
            Remove(channel, state);
        }
        finally
        {
            state.Gate.Release();
        }
    }

    private void Remove(string channel, ChannelState state)
    {
        state.Closed = true;
        _channels.TryRemove(new KeyValuePair<string, ChannelState>(channel, state));
    }

    private static void Deliver(ChannelState state, string message)
    {
        foreach (var member in state.Members.Values) member.Send(message);
    }

    public Task PublishAsync<T>(string channel, T message) =>
        Subscriber.PublishAsync(RedisChannel.Literal(channel), EncodingJson.Serialize(message));

    // 현재 서버에 접속한 유저만 포함
    public List<ChatUserData> GetUsersByChannel(string channel) =>
        _channels.TryGetValue(channel, out var state) ? state.Members.Values.Select(m => m.User).ToList() : [];

    public async Task AddGuildLogAsync(string channel, ChatLogData log)
    {
        try
        {
            var db = _chat!.GetDatabase();
            var key = LogKeyPrefix + channel;
            await db.StreamAddAsync(key, "data", EncodingJson.Serialize(log), maxLength: MaxGuildLogs, useApproximateMaxLength: true);
            await db.KeyExpireAsync(key, GuildLogLifetime);
        }
        catch (Exception ex) when (ex is RedisException or TimeoutException)
        {
            logger.LogWarning("Guild log write failed: {Error}", ex.Message);
        }
    }

    public async Task<List<ChatLogData>> GetGuildLogAsync(string channel, int count)
    {
        var minId = DateTimeOffset.UtcNow.Subtract(GuildLogLifetime).ToUnixTimeMilliseconds();
        var entries = await _chat!.GetDatabase().StreamRangeAsync(LogKeyPrefix + channel, minId, "+", count, Order.Descending);

        var logs = new List<ChatLogData>(entries.Length);
        foreach (var entry in entries.Reverse())
        {
            try
            {
                if (EncodingJson.Deserialize<ChatLogData>(entry["data"].ToString()) is { } log) logs.Add(log);
            }
            catch (JsonException ex)
            {
                logger.LogWarning("Invalid guild log {Id}: {Error}", entry.Id, ex.Message);
            }
        }
        return logs;
    }

    public async ValueTask DisposeAsync()
    {
        if (_chat != null) await _chat.DisposeAsync();
        if (_game != null) await _game.DisposeAsync();
    }
}
