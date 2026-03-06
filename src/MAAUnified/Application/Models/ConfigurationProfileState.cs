namespace MAAUnified.Application.Models;

public sealed record ConfigurationProfileState(
    string CurrentProfile,
    IReadOnlyList<string> OrderedProfiles);
