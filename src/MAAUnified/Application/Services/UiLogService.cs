using MAAUnified.Application.Models;

namespace MAAUnified.Application.Services;

public sealed class UiLogService
{
    private readonly List<UiLogMessage> _buffer = [];
    private bool _verboseEnabled;

    public event Action<UiLogMessage>? LogReceived;

    public IReadOnlyList<UiLogMessage> Snapshot => _buffer;

    public bool VerboseEnabled => _verboseEnabled;

    public void SetVerboseEnabled(bool enabled)
    {
        if (_verboseEnabled == enabled)
        {
            return;
        }

        _verboseEnabled = enabled;
        Push("INFO", enabled ? "Developer mode enabled: verbose diagnostics active." : "Developer mode disabled: verbose diagnostics inactive.");
    }

    public void Info(string message) => Push("INFO", message);

    public void Debug(string message)
    {
        if (_verboseEnabled)
        {
            Push("DEBUG", message);
        }
    }

    public void Warn(string message) => Push("WARN", message);

    public void Error(string message) => Push("ERROR", message);

    private void Push(string level, string message)
    {
        var log = new UiLogMessage(DateTimeOffset.UtcNow, level, message);
        _buffer.Add(log);
        LogReceived?.Invoke(log);
    }
}
