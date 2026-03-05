namespace MAAUnified.Tests;

public sealed class BaselineAlignedNotesConsistencyTests
{
    private static readonly string[] ForbiddenAlignedNoteTokens =
    [
        "Not yet bound",
        "gap",
    ];

    [Fact]
    public void AlignedConfigKeyMappings_ShouldNotContainUnimplementedSemanticsInNotes()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var conflicts = baseline.ConfigKeyMappings
            .Where(mapping => string.Equals(mapping.ParityStatus, "Aligned", StringComparison.Ordinal))
            .Where(mapping => ContainsForbiddenToken(mapping.Notes))
            .Select(mapping => $"{mapping.Key}: {mapping.Notes}")
            .ToList();

        Assert.True(conflicts.Count == 0, $"Aligned mappings with conflicting notes: {string.Join(" | ", conflicts)}");
    }

    private static bool ContainsForbiddenToken(string? note)
    {
        if (string.IsNullOrWhiteSpace(note))
        {
            return false;
        }

        foreach (var token in ForbiddenAlignedNoteTokens)
        {
            if (note.IndexOf(token, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                return true;
            }
        }

        return false;
    }
}
