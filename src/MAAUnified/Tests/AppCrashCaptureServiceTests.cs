using MAAUnified.Application.Services;

namespace MAAUnified.Tests;

public sealed class AppCrashCaptureServiceTests
{
    [Fact]
    public async Task TryGetPendingCrashReportAsync_ShouldReturnReport_ForNewCrashLog()
    {
        using var sandbox = new CrashCaptureSandbox();
        var crashLogPath = Path.Combine(sandbox.Root, "crash.log");
        await File.WriteAllTextAsync(crashLogPath, "=== FATAL ERROR ===\nDetail: SIGSEGV");

        var service = new AppCrashCaptureService(sandbox.Root);
        var report = await service.TryGetPendingCrashReportAsync();

        Assert.NotNull(report);
        Assert.Equal(crashLogPath, report!.CrashLogPath);
        Assert.Contains("SIGSEGV", report.Detail, StringComparison.Ordinal);
    }

    [Fact]
    public async Task TryGetPendingCrashReportAsync_ShouldIgnoreCrashLog_AfterItIsMarkedAsSeen()
    {
        using var sandbox = new CrashCaptureSandbox();
        var crashLogPath = Path.Combine(sandbox.Root, "crash.log");
        await File.WriteAllTextAsync(crashLogPath, "=== FATAL ERROR ===\nDetail: SIGSEGV");

        var service = new AppCrashCaptureService(sandbox.Root);
        var report = await service.TryGetPendingCrashReportAsync();

        Assert.NotNull(report);
        await service.MarkCrashReportAsSeenAsync(report!);

        var secondRead = await service.TryGetPendingCrashReportAsync();
        Assert.Null(secondRead);
    }

    [Fact]
    public async Task TryGetPendingCrashReportAsync_ShouldReturnUpdatedCrashLog_AfterSignatureChanges()
    {
        using var sandbox = new CrashCaptureSandbox();
        var crashLogPath = Path.Combine(sandbox.Root, "crash.log");
        await File.WriteAllTextAsync(crashLogPath, "=== FATAL ERROR ===\nDetail: SIGSEGV");

        var service = new AppCrashCaptureService(sandbox.Root);
        var first = await service.TryGetPendingCrashReportAsync();

        Assert.NotNull(first);
        await service.MarkCrashReportAsSeenAsync(first!);

        await Task.Delay(20);
        await File.WriteAllTextAsync(crashLogPath, "=== FATAL ERROR ===\nDetail: SIGABRT");

        var second = await service.TryGetPendingCrashReportAsync();

        Assert.NotNull(second);
        Assert.NotEqual(first.Signature, second!.Signature);
        Assert.Contains("SIGABRT", second.Detail, StringComparison.Ordinal);
    }

    private sealed class CrashCaptureSandbox : IDisposable
    {
        public CrashCaptureSandbox()
        {
            Root = Path.Combine(Path.GetTempPath(), $"maa-crash-capture-{Guid.NewGuid():N}");
            Directory.CreateDirectory(Root);
        }

        public string Root { get; }

        public void Dispose()
        {
            try
            {
                if (Directory.Exists(Root))
                {
                    Directory.Delete(Root, recursive: true);
                }
            }
            catch
            {
                // Ignore best-effort cleanup failures on test teardown.
            }
        }
    }
}
