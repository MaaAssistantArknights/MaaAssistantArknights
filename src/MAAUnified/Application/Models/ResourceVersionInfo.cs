namespace MAAUnified.Application.Models;

public sealed record ResourceVersionInfo(
    string VersionName,
    DateTime LastUpdatedAt)
{
    public static ResourceVersionInfo Empty { get; } = new(
        VersionName: string.Empty,
        LastUpdatedAt: DateTime.MinValue);
}
