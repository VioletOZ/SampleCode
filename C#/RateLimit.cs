using System.Diagnostics;

namespace ChatServer;

public sealed class RateLimit(double burst, double perSecond)
{
    private readonly double _burst = burst;
    private double _tokens = burst;
    private long _last = Stopwatch.GetTimestamp();

    public bool TryTake()
    {
        var now = Stopwatch.GetTimestamp();
        _tokens = Math.Min(_burst, _tokens + Stopwatch.GetElapsedTime(_last, now).TotalSeconds * perSecond);
        _last = now;
        if (_tokens < 1) return false;
        _tokens -= 1;
        return true;
    }
}
