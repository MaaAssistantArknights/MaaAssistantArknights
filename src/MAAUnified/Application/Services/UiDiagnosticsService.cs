using System.IO.Compression;
using System.Text;
using System.Text.Json;
using MAAUnified.Application.Models;
using MAAUnified.Platform;

namespace MAAUnified.Application.Services;

public sealed class UiDiagnosticsService
{
    private const string StartupLogFileName = "avalonia-ui-startup.log";
    private readonly SemaphoreSlim _writeLock = new(1, 1);
    private readonly string _debugDirectory;

    public UiDiagnosticsService(string baseDirectory, UiLogService uiLogService)
    {
        _debugDirectory = Path.Combine(baseDirectory, "debug");
        StartupLogPath = Path.Combine(_debugDirectory, StartupLogFileName);
        ErrorLogPath = Path.Combine(_debugDirectory, "avalonia-ui-errors.log");
        EventLogPath = Path.Combine(_debugDirectory, "avalonia-ui-events.log");
        PlatformEventLogPath = Path.Combine(_debugDirectory, "avalonia-platform-events.log");
        Directory.CreateDirectory(_debugDirectory);

        uiLogService.LogReceived += log =>
        {
            _ = WriteLineAsync(EventLogPath, $"{log.Timestamp:O} [{log.Level}] {log.Message}");
        };
    }

    public string ErrorLogPath { get; }

    public string EventLogPath { get; }

    public string PlatformEventLogPath { get; }

    public string StartupLogPath { get; }

    public async Task RecordErrorAsync(string scope, string message, Exception? exception = null, CancellationToken cancellationToken = default)
    {
        var builder = new StringBuilder()
            .Append(DateTimeOffset.UtcNow.ToString("O"))
            .Append(" [ERROR] [")
            .Append(scope)
            .Append("] ")
            .Append(message);

        if (exception is not null)
        {
            builder.Append(" | ").Append(exception.GetType().Name).Append(": ").Append(exception.Message);
        }

        await WriteLineAsync(ErrorLogPath, builder.ToString(), cancellationToken);
    }

    public Task RecordFailedResultAsync(string scope, UiOperationResult result, CancellationToken cancellationToken = default)
    {
        var details = result.Error?.Details;
        var code = string.IsNullOrWhiteSpace(result.Error?.Code)
            ? UiErrorCode.UiOperationFailed
            : result.Error!.Code;
        var caseId = code;
        var payload = $"{DateTimeOffset.UtcNow:O} [FAILED] [{scope}] {result.Message} | code={code} | case_id={caseId}";
        if (!string.IsNullOrWhiteSpace(details))
        {
            payload += $" | details={details}";
        }

        return WriteLineAsync(ErrorLogPath, payload, cancellationToken);
    }

    public Task RecordConfigValidationFailureAsync(ConfigValidationIssue? issue, CancellationToken cancellationToken = default)
    {
        var scope = issue?.Scope ?? "ConfigValidation";
        var code = issue?.Code ?? "ConfigValidationBlocked";
        var caseId = code;
        var field = issue?.Field ?? "config";
        var profile = issue?.ProfileName ?? "-";
        var taskIndex = issue?.TaskIndex?.ToString() ?? "-";
        var message = issue?.Message ?? "Execution blocked due to config validation issues.";
        var payload =
            $"{DateTimeOffset.UtcNow:O} [FAILED][{scope}] code={code} case_id={caseId} field={field} profile={profile} taskIndex={taskIndex} message={message}";
        return WriteLineAsync(ErrorLogPath, payload, cancellationToken);
    }

    public Task RecordEventAsync(string scope, string message, CancellationToken cancellationToken = default)
    {
        return WriteLineAsync(EventLogPath, $"{DateTimeOffset.UtcNow:O} [EVENT] [{scope}] {message}", cancellationToken);
    }

    public Task RecordPlatformEventAsync(
        PlatformCapabilityId capability,
        string action,
        PlatformOperationResult result,
        CancellationToken cancellationToken = default)
    {
        var payload = new PlatformEventLogLine(
            DateTimeOffset.UtcNow,
            capability,
            action,
            result.Success,
            result.ExecutionMode.ToString(),
            result.Provider,
            result.UsedFallback,
            result.ErrorCode,
            result.Message,
            result.OperationId);
        return WriteLineAsync(PlatformEventLogPath, JsonSerializer.Serialize(payload), cancellationToken);
    }

    public Task RecordPlatformEventAsync<T>(
        PlatformCapabilityId capability,
        string action,
        PlatformOperationResult<T> result,
        CancellationToken cancellationToken = default)
    {
        var payload = new PlatformEventLogLine(
            DateTimeOffset.UtcNow,
            capability,
            action,
            result.Success,
            result.ExecutionMode.ToString(),
            result.Provider,
            result.UsedFallback,
            result.ErrorCode,
            result.Message,
            result.OperationId);
        return WriteLineAsync(PlatformEventLogPath, JsonSerializer.Serialize(payload), cancellationToken);
    }

    public async Task<string> BuildIssueReportBundleAsync(string baseDirectory, CancellationToken cancellationToken = default)
    {
        Directory.CreateDirectory(_debugDirectory);
        var outputPath = Path.Combine(_debugDirectory, $"issue-report-{DateTimeOffset.UtcNow:yyyyMMddHHmmss}.zip");
        if (File.Exists(outputPath))
        {
            File.Delete(outputPath);
        }

        using var archive = ZipFile.Open(outputPath, ZipArchiveMode.Create);
        AddFileOrPlaceholder(
            archive,
            Path.Combine(baseDirectory, "config", "avalonia.json"),
            "config/avalonia.json",
            "avalonia.json not found when bundle was generated.");
        AddFileOrPlaceholder(
            archive,
            Path.Combine(baseDirectory, "debug", "config-import-report.json"),
            "debug/config-import-report.json",
            "config-import-report.json not found when bundle was generated.");
        AddFileOrPlaceholder(
            archive,
            StartupLogPath,
            "debug/avalonia-ui-startup.log",
            "UI startup log is empty or missing.");
        AddFileOrPlaceholder(
            archive,
            ErrorLogPath,
            "debug/avalonia-ui-errors.log",
            "UI error log is empty or missing.");
        AddFileOrPlaceholder(
            archive,
            EventLogPath,
            "debug/avalonia-ui-events.log",
            "UI event log is empty or missing.");
        AddFileOrPlaceholder(
            archive,
            PlatformEventLogPath,
            "debug/avalonia-platform-events.log",
            "Platform event log is empty or missing.");

        await RecordEventAsync("IssueReport", $"Support bundle generated: {outputPath}", cancellationToken);
        return outputPath;
    }

    private async Task WriteLineAsync(string path, string line, CancellationToken cancellationToken = default)
    {
        await _writeLock.WaitAsync(cancellationToken);
        try
        {
            await File.AppendAllTextAsync(path, line + Environment.NewLine, cancellationToken);
        }
        finally
        {
            _writeLock.Release();
        }
    }

    private static void AddFileOrPlaceholder(
        ZipArchive archive,
        string filePath,
        string entryName,
        string placeholderMessage)
    {
        if (File.Exists(filePath))
        {
            archive.CreateEntryFromFile(filePath, entryName);
            return;
        }

        var entry = archive.CreateEntry(entryName);
        using var stream = entry.Open();
        using var writer = new StreamWriter(stream, Encoding.UTF8);
        writer.WriteLine(placeholderMessage);
    }

    private sealed record PlatformEventLogLine(
        DateTimeOffset Timestamp,
        PlatformCapabilityId Capability,
        string Action,
        bool Success,
        string ExecutionMode,
        string Provider,
        bool UsedFallback,
        string? ErrorCode,
        string Message,
        string? OperationId);
}
