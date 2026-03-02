namespace MAAUnified.App.ViewModels;

public sealed record FeatureModule(
    string Key,
    string Group,
    string Title,
    string Description,
    string ViewTypeName,
    string ParityScope);
