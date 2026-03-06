using System.Text.RegularExpressions;

namespace MAAUnified.Tests;

public sealed class ReleaseDocsContractTests
{
    [Fact]
    public void ReleaseNotes_ShouldContainRequiredQ3Sections()
    {
        var doc = BaselineTestSupport.ReadDoc("avalonia-release-notes.v1.md");

        Assert.Contains("# MAAUnified 发布说明 v1", doc, StringComparison.Ordinal);
        Assert.Contains("## 发布范围", doc, StringComparison.Ordinal);
        Assert.Contains("## 证据生成记录", doc, StringComparison.Ordinal);
        Assert.Contains("## Q1/Q2 Completed Evidence", doc, StringComparison.Ordinal);
        Assert.Contains("## 构建产物清单", doc, StringComparison.Ordinal);
        Assert.Contains("## 配置兼容清单", doc, StringComparison.Ordinal);
        Assert.Contains("## fallback 验证清单", doc, StringComparison.Ordinal);
        Assert.Contains("## 日志验证清单", doc, StringComparison.Ordinal);
        Assert.Contains("## Baseline/Acceptance Sync Audit", doc, StringComparison.Ordinal);
        Assert.Contains("## 提交回填规则", doc, StringComparison.Ordinal);
        Assert.Contains("## 发布阻断条件", doc, StringComparison.Ordinal);
        Assert.Contains("## 固定发布清单（并入发布说明）", doc, StringComparison.Ordinal);
    }

    [Fact]
    public void ReleaseNotes_ShouldRecordQ1Q2EvidenceAndGatePolicy()
    {
        var doc = BaselineTestSupport.ReadDoc("avalonia-release-notes.v1.md");

        Assert.Contains("Passed 65, Failed 0", doc, StringComparison.Ordinal);
        Assert.Contains("Passed 12, Failed 0", doc, StringComparison.Ordinal);
        Assert.Contains("No banned markers in ci-avalonia workflows", doc, StringComparison.Ordinal);
        Assert.Contains("Linux baseline consistency gate fails", doc, StringComparison.Ordinal);
        Assert.Contains("Linux full `MAAUnified.Tests` gate fails", doc, StringComparison.Ordinal);
        Assert.Contains("Windows platform capability contract gate fails", doc, StringComparison.Ordinal);
    }

    [Fact]
    public void ReleaseAndExecutionDocs_ShouldUseSameCommitHash()
    {
        var releaseDoc = BaselineTestSupport.ReadDoc("avalonia-release-notes.v1.md");
        var executionDoc = BaselineTestSupport.ReadDoc("acceptance.execution.v1.md");

        var releaseCommit = ExtractCommit(releaseDoc);
        var executionCommit = ExtractCommit(executionDoc);
        Assert.Equal(releaseCommit, executionCommit);
    }

    [Fact]
    public void RollbackDoc_ShouldContainRequiredRunbookSections()
    {
        var doc = BaselineTestSupport.ReadDoc("avalonia-rollback.md");

        Assert.Contains("# MAAUnified 回滚手册 v1", doc, StringComparison.Ordinal);
        Assert.Contains("## 触发条件", doc, StringComparison.Ordinal);
        Assert.Contains("## 回滚步骤", doc, StringComparison.Ordinal);
        Assert.Contains("## 回滚后验收", doc, StringComparison.Ordinal);
        Assert.Contains("## 数据保留策略", doc, StringComparison.Ordinal);
        Assert.Contains("## 责任人与响应时限", doc, StringComparison.Ordinal);
    }

    private static string ExtractCommit(string markdown)
    {
        var match = Regex.Match(markdown, @"^- Commit:\s*`([0-9a-f]{40})`", RegexOptions.Multiline);
        Assert.True(match.Success, "Commit line with 40-char SHA was not found.");
        return match.Groups[1].Value;
    }
}
