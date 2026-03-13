namespace MAAUnified.Application.Services;

public sealed class AppCrashCaptureService
{
    private const int MaxCrashDetailLength = 16000;
    private readonly string _crashLogPath;
    private readonly string _seenMarkerPath;

    public AppCrashCaptureService(string baseDirectory)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(baseDirectory);

        _crashLogPath = Path.Combine(baseDirectory, "crash.log");
        _seenMarkerPath = Path.Combine(baseDirectory, "debug", "last-seen-crash.signature");
    }

    public async Task<PendingCrashReport?> TryGetPendingCrashReportAsync(CancellationToken cancellationToken = default)
    {
        if (!File.Exists(_crashLogPath))
        {
            return null;
        }

        var info = new FileInfo(_crashLogPath);
        if (info.Length <= 0)
        {
            return null;
        }

        var signature = BuildSignature(info.LastWriteTimeUtc, info.Length);
        var previousSignature = await TryReadSeenSignatureAsync(cancellationToken);
        if (string.Equals(previousSignature, signature, StringComparison.Ordinal))
        {
            return null;
        }

        var detail = await File.ReadAllTextAsync(_crashLogPath, cancellationToken);
        if (string.IsNullOrWhiteSpace(detail))
        {
            return null;
        }

        return new PendingCrashReport(
            _crashLogPath,
            signature,
            info.LastWriteTimeUtc,
            NormalizeDetail(detail));
    }

    public async Task MarkCrashReportAsSeenAsync(PendingCrashReport report, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(report);

        var directory = Path.GetDirectoryName(_seenMarkerPath);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        await File.WriteAllTextAsync(_seenMarkerPath, report.Signature, cancellationToken);
    }

    internal static string BuildSignature(DateTimeOffset lastWriteTimeUtc, long length)
    {
        return $"{lastWriteTimeUtc.UtcDateTime.Ticks}:{length}";
    }

    internal static string NormalizeDetail(string detail)
    {
        var normalized = (detail ?? string.Empty).Replace("\0", string.Empty).Trim();
        if (normalized.Length <= MaxCrashDetailLength)
        {
            return normalized;
        }

        var keep = normalized[..MaxCrashDetailLength];
        return keep + Environment.NewLine + "... <truncated>";
    }

    private async Task<string?> TryReadSeenSignatureAsync(CancellationToken cancellationToken)
    {
        if (!File.Exists(_seenMarkerPath))
        {
            return null;
        }

        var signature = await File.ReadAllTextAsync(_seenMarkerPath, cancellationToken);
        return string.IsNullOrWhiteSpace(signature) ? null : signature.Trim();
    }
}

public sealed record PendingCrashReport(
    string CrashLogPath,
    string Signature,
    DateTimeOffset LastWriteTimeUtc,
    string Detail);
