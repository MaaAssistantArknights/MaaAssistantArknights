namespace MAAUnified.Tests;

public sealed class AcceptanceExecutionSyncTests
{
    private static readonly HashSet<string> AllowedResults =
    [
        "Pass",
        "Fail",
        "Blocked",
    ];

    [Fact]
    public void AcceptanceExecutionReport_ShouldCoverAllCases_WithRequiredEvidenceFields()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var acceptance = BaselineTestSupport.LoadAcceptanceTemplate();
        var report = BaselineTestSupport.ReadDoc("acceptance.execution.v1.md");

        var rows = ParseRows(report);
        Assert.Equal(acceptance.Cases.Count, rows.Count);

        var duplicateCaseIds = rows.GroupBy(row => row.CaseId, StringComparer.Ordinal)
            .Where(group => group.Count() > 1)
            .Select(group => group.Key)
            .ToList();
        Assert.Empty(duplicateCaseIds);

        var acceptanceByCaseId = acceptance.Cases.ToDictionary(testCase => testCase.CaseId, StringComparer.Ordinal);
        var baselineByItemId = baseline.Items.ToDictionary(item => item.ItemId, StringComparer.Ordinal);

        var reportCaseIds = rows.Select(row => row.CaseId).ToHashSet(StringComparer.Ordinal);
        var expectedCaseIds = acceptanceByCaseId.Keys.ToHashSet(StringComparer.Ordinal);
        Assert.True(expectedCaseIds.SetEquals(reportCaseIds),
            $"Execution case coverage mismatch. Missing: {string.Join(", ", expectedCaseIds.Except(reportCaseIds))}; Extra: {string.Join(", ", reportCaseIds.Except(expectedCaseIds))}");

        foreach (var row in rows)
        {
            Assert.Contains(row.Result, AllowedResults);
            Assert.True(DateTimeOffset.TryParse(row.ExecutedAtUtc, out _), $"Invalid executed_at_utc => {row.CaseId}");
            Assert.False(string.IsNullOrWhiteSpace(row.Verifier));
            Assert.False(string.IsNullOrWhiteSpace(row.Note));

            Assert.False(string.IsNullOrWhiteSpace(row.EvidenceUiPath));
            Assert.False(string.IsNullOrWhiteSpace(row.EvidenceLogPath));
            Assert.False(string.IsNullOrWhiteSpace(row.EvidenceScope));
            Assert.False(string.IsNullOrWhiteSpace(row.BaselineCaseId));

            var testCase = Assert.Contains(row.CaseId, acceptanceByCaseId);
            Assert.Equal(testCase.ItemId, row.ItemId);

            var item = Assert.Contains(row.ItemId, baselineByItemId);
            Assert.Equal(item.Evidence.UiPath, row.EvidenceUiPath);
            Assert.Equal(item.Evidence.LogPath, row.EvidenceLogPath);
            Assert.Equal(item.Evidence.Scope, row.EvidenceScope);
            Assert.Equal(item.Evidence.CaseId, row.BaselineCaseId);
        }
    }

    private static List<ExecutionRow> ParseRows(string markdown)
    {
        var rows = new List<ExecutionRow>();
        var lines = BaselineTestSupport.NormalizeLineEndings(markdown)
            .Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

        foreach (var line in lines)
        {
            if (!line.StartsWith("| `ACC-", StringComparison.Ordinal))
            {
                continue;
            }

            var cells = line.Trim('|').Split('|', StringSplitOptions.TrimEntries);
            Assert.Equal(10, cells.Length);

            rows.Add(new ExecutionRow(
                NormalizeCell(cells[0]),
                NormalizeCell(cells[1]),
                NormalizeCell(cells[2]),
                NormalizeCell(cells[3]),
                NormalizeCell(cells[4]),
                NormalizeCell(cells[5]),
                NormalizeCell(cells[6]),
                NormalizeCell(cells[7]),
                NormalizeCell(cells[8]),
                NormalizeCell(cells[9])));
        }

        return rows;
    }

    private static string NormalizeCell(string cell)
    {
        var trimmed = cell.Trim();
        if (trimmed.StartsWith('`') && trimmed.EndsWith('`') && trimmed.Length >= 2)
        {
            return trimmed[1..^1];
        }

        return trimmed;
    }

    private sealed record ExecutionRow(
        string CaseId,
        string ItemId,
        string Result,
        string ExecutedAtUtc,
        string EvidenceUiPath,
        string EvidenceLogPath,
        string EvidenceScope,
        string BaselineCaseId,
        string Verifier,
        string Note);
}
