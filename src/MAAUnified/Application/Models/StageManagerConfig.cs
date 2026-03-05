namespace MAAUnified.Application.Models;

public sealed record StageManagerConfig(
    IReadOnlyList<string> StageCodes,
    bool AutoIterate,
    string LastSelectedStage)
{
    public static StageManagerConfig Default { get; } = new(
        StageCodes: Array.Empty<string>(),
        AutoIterate: false,
        LastSelectedStage: string.Empty);

    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            ["Advanced.StageManager.StageCodes"] = string.Join(";", StageCodes),
            ["Advanced.StageManager.AutoIterate"] = AutoIterate.ToString(),
            ["Advanced.StageManager.LastSelectedStage"] = LastSelectedStage,
        };
    }
}
