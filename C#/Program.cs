using ChatServer;
using StackExchange.Redis;

const int MaxSessions = 5000;
var sessions = 0;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<RedisManager>();
var app = builder.Build();

var redis = app.Services.GetRequiredService<RedisManager>();
try
{
    await redis.ConnectAsync();
}
catch (Exception ex) when (ex is RedisException or InvalidOperationException)
{
    app.Logger.LogCritical("Redis connection failed: {Error}", ex.Message);
    return 1;
}

app.UseWebSockets(new WebSocketOptions { KeepAliveInterval = TimeSpan.FromSeconds(30), KeepAliveTimeout = TimeSpan.FromSeconds(30) });

app.MapGet("/health", () => Results.Json(
    new { redis = redis.IsConnected, sessions = Volatile.Read(ref sessions), channels = redis.ChannelCount },
    statusCode: redis.IsConnected ? 200 : 503));

app.Map("/chat", async (HttpContext context, ILogger<Chat> logger, IHostApplicationLifetime lifetime) =>
{
    if (!context.WebSockets.IsWebSocketRequest)
    {
        context.Response.StatusCode = StatusCodes.Status400BadRequest;
        return;
    }
    try
    {
        if (Interlocked.Increment(ref sessions) > MaxSessions)
        {
            context.Response.StatusCode = StatusCodes.Status503ServiceUnavailable;
            return;
        }

        // 인증이 끝난 뒤에만 WebSocket 연결을 받는다
        var sessionId = context.Request.Headers["SessionID"].ToString();
        ChatUserData? user;
        try
        {
            user = sessionId.Length is > 0 and <= 128 ? await redis.AuthVerifyAsync(sessionId) : null;
        }
        catch (Exception ex) when (ex is RedisException or TimeoutException)
        {
            logger.LogError("Auth failed: {Error}", ex.Message);
            context.Response.StatusCode = StatusCodes.Status503ServiceUnavailable;
            return;
        }
        if (user == null)
        {
            context.Response.StatusCode = StatusCodes.Status401Unauthorized;
            return;
        }

        using var socket = await context.WebSockets.AcceptWebSocketAsync();
        await new Chat(socket, user, redis, logger).RunAsync(lifetime.ApplicationStopping);
    }
    finally
    {
        Interlocked.Decrement(ref sessions);
    }
});

await app.RunAsync();
return 0;
