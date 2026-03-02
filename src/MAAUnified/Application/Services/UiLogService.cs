using MAAUnified.Application.Models;

namespace MAAUnified.Application.Services;

public sealed class UiLogService
{
    private readonly List<UiLogMessage> _buffer = [];

    public event Action<UiLogMessage>? LogReceived;

    public IReadOnlyList<UiLogMessage> Snapshot => _buffer;

    public void Info(string message) => Push("INFO", message);

    public void Warn(string message) => Push("WARN", message);

    public void Error(string message) => Push("ERROR", message);

    private void Push(string level, string message)
    {
        var log = new UiLogMessage(DateTimeOffset.UtcNow, level, message);
        _buffer.Add(log);
        LogReceived?.Invoke(log);
    }
}
