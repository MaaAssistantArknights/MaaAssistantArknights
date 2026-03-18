using System.Diagnostics;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Services;

public interface IAppLifecycleService
{
    bool SupportsExit => false;

    Task<UiOperationResult> RestartAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> ExitAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(UiOperationResult.Fail(UiErrorCode.AppExitUnsupported, "Exit lifecycle service is unavailable."));
    }
}

public sealed class ProcessAppLifecycleService : IAppLifecycleService
{
    public Task<UiOperationResult> RestartAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        try
        {
            var executable = Environment.ProcessPath;
            if (string.IsNullOrWhiteSpace(executable))
            {
                return Task.FromResult(UiOperationResult.Fail(UiErrorCode.AppRestartExecutableMissing, "Current executable path is unavailable."));
            }

            var arguments = Environment.GetCommandLineArgs().Skip(1).Select(Quote).ToArray();
            var startInfo = new ProcessStartInfo
            {
                FileName = executable,
                Arguments = string.Join(' ', arguments),
                UseShellExecute = false,
            };

            _ = Process.Start(startInfo);
            return Task.FromResult(UiOperationResult.Ok("Restart process launched."));
        }
        catch (Exception ex)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.AppRestartFailed, $"Failed to restart process: {ex.Message}"));
        }
    }

    private static string Quote(string value)
    {
        return string.IsNullOrEmpty(value)
            ? "\"\""
            : value.Contains(' ', StringComparison.Ordinal)
                ? $"\"{value.Replace("\"", "\\\"", StringComparison.Ordinal)}\""
                : value;
    }
}

public sealed class NoOpAppLifecycleService : IAppLifecycleService
{
    public Task<UiOperationResult> RestartAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(UiOperationResult.Fail(UiErrorCode.AppRestartUnsupported, "Restart lifecycle service is unavailable."));
    }
}
