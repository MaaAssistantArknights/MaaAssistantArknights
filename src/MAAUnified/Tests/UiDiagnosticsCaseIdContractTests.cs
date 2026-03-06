using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.Tests;

public sealed class UiDiagnosticsCaseIdContractTests
{
    [Fact]
    public async Task RecordFailedResultAsync_ShouldWriteCaseIdField()
    {
        var root = CreateTempRoot();
        try
        {
            var diagnostics = new UiDiagnosticsService(root, new UiLogService());

            await diagnostics.RecordFailedResultAsync(
                "Diagnostics.Contract.Failed",
                UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, "failed", "details"));

            var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
            Assert.Contains("[FAILED] [Diagnostics.Contract.Failed]", content, StringComparison.Ordinal);
            Assert.Contains($"code={UiErrorCode.TaskValidationFailed}", content, StringComparison.Ordinal);
            Assert.Contains($"case_id={UiErrorCode.TaskValidationFailed}", content, StringComparison.Ordinal);
        }
        finally
        {
            TryDeleteRoot(root);
        }
    }

    [Fact]
    public async Task RecordConfigValidationFailureAsync_ShouldWriteCaseIdField()
    {
        var root = CreateTempRoot();
        try
        {
            var diagnostics = new UiDiagnosticsService(root, new UiLogService());
            var issue = new ConfigValidationIssue
            {
                Scope = "Config.Scope",
                Code = "ConfigValidationBlocked",
                Field = "task",
                Message = "blocked",
                Blocking = true,
            };

            await diagnostics.RecordConfigValidationFailureAsync(issue);

            var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
            Assert.Contains("[FAILED][Config.Scope]", content, StringComparison.Ordinal);
            Assert.Contains("code=ConfigValidationBlocked", content, StringComparison.Ordinal);
            Assert.Contains("case_id=ConfigValidationBlocked", content, StringComparison.Ordinal);
        }
        finally
        {
            TryDeleteRoot(root);
        }
    }

    private static string CreateTempRoot()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(Path.Combine(root, "config"));
        return root;
    }

    private static void TryDeleteRoot(string root)
    {
        try
        {
            Directory.Delete(root, recursive: true);
        }
        catch
        {
            // ignore temp cleanup failures
        }
    }
}
