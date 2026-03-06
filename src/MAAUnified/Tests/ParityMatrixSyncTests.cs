namespace MAAUnified.Tests;

public sealed class ParityMatrixSyncTests
{
    private static readonly string[] AllowedMatrixStatus =
    [
        "Implemented",
        "InProgress",
        "Pending",
    ];

    [Fact]
    public void ParityMatrix_ShouldDeclareBaselineFreezeAsSyncSource()
    {
        var doc = BaselineTestSupport.ReadDoc("avalonia-parity-matrix.md");
        Assert.Contains("Docs/baseline.freeze.v1.md", doc, StringComparison.Ordinal);
    }

    [Fact]
    public void ParityMatrix_DopTargets_ShouldMatchBaselineParity()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var matrixStatus = ParseMatrixStatus(BaselineTestSupport.ReadDoc("avalonia-parity-matrix.md"));

        var targets = new (string ItemId, string MatrixModule)[]
        {
            ("Advanced.Toolbox", "Toolbox"),
            ("Dialog.Announcement", "AnnouncementDialog"),
            ("Dialog.VersionUpdate", "VersionUpdateDialog"),
            ("Dialog.ProcessPicker", "ProcessPickerDialog"),
            ("Dialog.EmulatorPath", "EmulatorPathSelectionDialog"),
            ("Dialog.Error", "ErrorDialog"),
            ("Dialog.AchievementList", "AchievementListDialog"),
            ("Dialog.TextDialog", "TextDialog"),
        };

        foreach (var (itemId, matrixModule) in targets)
        {
            var item = Assert.Single(
                baseline.Items,
                i => string.Equals(i.ItemId, itemId, StringComparison.Ordinal));
            var expectedStatus = ToMatrixStatus(item.ParityStatus);

            Assert.True(
                matrixStatus.TryGetValue(matrixModule, out var actualStatus),
                $"Parity matrix row `{matrixModule}` is missing.");
            Assert.Contains(actualStatus, AllowedMatrixStatus);
            Assert.Equal(
                expectedStatus,
                actualStatus);
        }
    }

    private static string ToMatrixStatus(string parityStatus)
    {
        return parityStatus switch
        {
            "Aligned" => "Implemented",
            "Gap" => "InProgress",
            "Waived" => "Pending",
            _ => throw new InvalidOperationException($"Unsupported baseline parity status `{parityStatus}`."),
        };
    }

    private static IReadOnlyDictionary<string, string> ParseMatrixStatus(string markdown)
    {
        var rows = new Dictionary<string, string>(StringComparer.Ordinal);
        var lines = BaselineTestSupport.NormalizeLineEndings(markdown).Split('\n', StringSplitOptions.RemoveEmptyEntries);
        foreach (var rawLine in lines)
        {
            var line = rawLine.Trim();
            if (!line.StartsWith("|", StringComparison.Ordinal) || !line.EndsWith("|", StringComparison.Ordinal))
            {
                continue;
            }

            var cells = line.Split('|', StringSplitOptions.TrimEntries);
            if (cells.Length < 4)
            {
                continue;
            }

            var module = cells[1];
            var status = cells[2];
            if (string.IsNullOrWhiteSpace(module) || string.IsNullOrWhiteSpace(status))
            {
                continue;
            }

            if (string.Equals(module, "模块", StringComparison.Ordinal) || string.Equals(module, "---", StringComparison.Ordinal))
            {
                continue;
            }

            rows[module] = status;
        }

        return rows;
    }
}
