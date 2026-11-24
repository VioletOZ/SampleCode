using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.Unicode;

namespace ChatServer;

public enum CHAT_COMMAND
{
    CT_LOGIN, CT_RECONNECT, CT_INFO, CT_GUILD_LOG, CT_MESSAGE,
    CT_CHANNEL_CHANGE, CT_CHANNEL_ENTER, CT_CHANNEL_LEAVE, CT_NOTICE_GACHA, CT_LOGOUT
}

public enum CHAT_TYPE { CT_SYSTEM, CT_GM_NOTICE, CT_NORMAL, CT_GUILD }

public enum RETURN_CODE
{
    RC_OK, RC_CHAT_LOGIN_FAIL, RC_CHAT_DUPLICATE_CHANNEL,
    RC_CHAT_INVALID_REQUEST, RC_CHAT_NOT_IN_CHANNEL, RC_CHAT_RATE_LIMITED, RC_CHAT_SERVER_ERROR
}

public sealed record ChatUserData(long UserUID, string UserName, int GuildID, int FavoriteCharacterID);

public sealed class ChatLogData
{
    public long UserUID { get; set; }
    public string UserName { get; set; } = "";
    public string Message { get; set; } = "";
    public DateTime Time { get; set; }
}

public class req_Command { public CHAT_COMMAND Command { get; set; } }
public sealed class req_ChatReConnect : req_Command { public int ChannelID { get; set; } }
public sealed class req_ChatInfo : req_Command { public CHAT_TYPE ChatType { get; set; } }
public sealed class req_ChatChannel : req_Command { public CHAT_TYPE ChatType { get; set; } public int ChannelID { get; set; } }
public sealed class req_ChatMessage : req_Command { public CHAT_TYPE ChatType { get; set; } public ChatLogData? LogData { get; set; } }
public sealed class req_ChatGachaNotice : req_Command { public List<int>? ItemIDList { get; set; } public List<int>? CharIDList { get; set; } }

public class res_Command
{
    public CHAT_COMMAND Command { get; set; }
    public RETURN_CODE ReturnCode { get; set; }
}
public sealed class res_ChatLogin : res_Command { public int ChannelID { get; set; } public int GuildChannelID { get; set; } }
public sealed class res_ChatChannel : res_Command
{
    public CHAT_TYPE ChatType { get; set; }
    public int ChannelID { get; set; }
    public List<ChatUserData> ChannelUserDataList { get; set; } = [];
}
public sealed class res_ChatMessage : res_Command { public CHAT_TYPE ChatType { get; set; } public int ChannelID { get; set; } public ChatLogData? LogData { get; set; } }
public sealed class res_ChatGuildLog : res_Command { public List<ChatLogData> LogDataList { get; set; } = []; }
public sealed class res_ChatGachaNotice : res_Command
{
    public string UserName { get; set; } = "";
    public List<int> ItemIDList { get; set; } = [];
    public List<int> CharIDList { get; set; } = [];
}

public static class EncodingJson
{
    private static readonly JsonSerializerOptions Options = new()
    {
        Converters = { new JsonStringEnumConverter() },
        PropertyNameCaseInsensitive = true,
        Encoder = JavaScriptEncoder.Create(UnicodeRanges.All)
    };

    public static string Serialize<T>(T value) => JsonSerializer.Serialize(value, Options);
    public static T Deserialize<T>(ReadOnlySpan<byte> json) => JsonSerializer.Deserialize<T>(json, Options) ?? throw new JsonException("null body");
    public static T? Deserialize<T>(string json) => JsonSerializer.Deserialize<T>(json, Options);
}
