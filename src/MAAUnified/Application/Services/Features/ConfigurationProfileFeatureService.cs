using System.Text.Json.Nodes;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Services.Features;

public sealed class ConfigurationProfileFeatureService : IConfigurationProfileFeatureService
{
    private readonly UnifiedConfigurationService? _configService;

    public ConfigurationProfileFeatureService()
    {
    }

    public ConfigurationProfileFeatureService(UnifiedConfigurationService configService)
    {
        _configService = configService;
    }

    public Task<UiOperationResult<ConfigurationProfileState>> LoadStateAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetConfig(out var config, out var failure))
        {
            return Task.FromResult(failure);
        }

        if (!TryValidateConfig(config, out var invalid))
        {
            return Task.FromResult(invalid);
        }

        return Task.FromResult(UiOperationResult<ConfigurationProfileState>.Ok(
            BuildState(config),
            "Loaded configuration profile state."));
    }

    public async Task<UiOperationResult<ConfigurationProfileState>> AddProfileAsync(
        string profileName,
        string? copyFrom = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetConfig(out var config, out var failure))
        {
            return failure;
        }

        if (!TryValidateConfig(config, out var invalid))
        {
            return invalid;
        }

        if (!TryNormalizeProfileName(profileName, out var normalizedName, out var nameInvalid))
        {
            return nameInvalid;
        }

        if (config.Profiles.ContainsKey(normalizedName))
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileAlreadyExists,
                $"Profile `{normalizedName}` already exists.");
        }

        var sourceName = string.IsNullOrWhiteSpace(copyFrom)
            ? config.CurrentProfile
            : copyFrom.Trim();
        if (!config.Profiles.TryGetValue(sourceName, out var sourceProfile))
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileNotFound,
                $"Source profile `{sourceName}` does not exist.");
        }

        var snapshot = CaptureSnapshot(config);
        config.Profiles[normalizedName] = CloneProfile(sourceProfile);
        return await PersistWithRollbackAsync(
            snapshot,
            $"Profile `{normalizedName}` added.",
            cancellationToken);
    }

    public async Task<UiOperationResult<ConfigurationProfileState>> DeleteProfileAsync(
        string profileName,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetConfig(out var config, out var failure))
        {
            return failure;
        }

        if (!TryValidateConfig(config, out var invalid))
        {
            return invalid;
        }

        if (!TryNormalizeProfileName(profileName, out var normalizedName, out var nameInvalid))
        {
            return nameInvalid;
        }

        if (!config.Profiles.ContainsKey(normalizedName))
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileNotFound,
                $"Profile `{normalizedName}` does not exist.");
        }

        if (config.Profiles.Count <= 1)
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileDeleteLastForbidden,
                "Cannot delete the last remaining profile.");
        }

        if (string.Equals(config.CurrentProfile, normalizedName, StringComparison.OrdinalIgnoreCase))
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileDeleteCurrentForbidden,
                $"Cannot delete current profile `{normalizedName}`.");
        }

        var snapshot = CaptureSnapshot(config);
        config.Profiles.Remove(normalizedName);
        return await PersistWithRollbackAsync(
            snapshot,
            $"Profile `{normalizedName}` deleted.",
            cancellationToken);
    }

    public async Task<UiOperationResult<ConfigurationProfileState>> MoveProfileAsync(
        string profileName,
        int offset,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetConfig(out var config, out var failure))
        {
            return failure;
        }

        if (!TryValidateConfig(config, out var invalid))
        {
            return invalid;
        }

        if (!TryNormalizeProfileName(profileName, out var normalizedName, out var nameInvalid))
        {
            return nameInvalid;
        }

        if (!config.Profiles.ContainsKey(normalizedName))
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileNotFound,
                $"Profile `{normalizedName}` does not exist.");
        }

        var names = config.Profiles.Keys.ToList();
        var index = names.FindIndex(name => string.Equals(name, normalizedName, StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileNotFound,
                $"Profile `{normalizedName}` does not exist.");
        }

        var targetIndex = index + offset;
        if (targetIndex < 0 || targetIndex >= names.Count)
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileMoveOutOfRange,
                $"Profile `{normalizedName}` cannot be moved further.");
        }

        var snapshot = CaptureSnapshot(config);
        (names[index], names[targetIndex]) = (names[targetIndex], names[index]);
        var reordered = new Dictionary<string, UnifiedProfile>(StringComparer.OrdinalIgnoreCase);
        foreach (var name in names)
        {
            reordered[name] = config.Profiles[name];
        }

        config.Profiles = reordered;
        return await PersistWithRollbackAsync(
            snapshot,
            $"Profile `{normalizedName}` moved.",
            cancellationToken);
    }

    public async Task<UiOperationResult<ConfigurationProfileState>> SwitchProfileAsync(
        string profileName,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetConfig(out var config, out var failure))
        {
            return failure;
        }

        if (!TryValidateConfig(config, out var invalid))
        {
            return invalid;
        }

        if (!TryNormalizeProfileName(profileName, out var normalizedName, out var nameInvalid))
        {
            return nameInvalid;
        }

        if (!config.Profiles.ContainsKey(normalizedName))
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileNotFound,
                $"Profile `{normalizedName}` does not exist.");
        }

        if (string.Equals(config.CurrentProfile, normalizedName, StringComparison.OrdinalIgnoreCase))
        {
            return UiOperationResult<ConfigurationProfileState>.Ok(
                BuildState(config),
                $"Profile `{normalizedName}` is already active.");
        }

        var snapshot = CaptureSnapshot(config);
        config.CurrentProfile = normalizedName;
        return await PersistWithRollbackAsync(
            snapshot,
            $"Switched to profile `{normalizedName}`.",
            cancellationToken);
    }

    private async Task<UiOperationResult<ConfigurationProfileState>> PersistWithRollbackAsync(
        ConfigSnapshot snapshot,
        string successMessage,
        CancellationToken cancellationToken)
    {
        if (_configService is null)
        {
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileServiceUnavailable,
                "Configuration profile service is not initialized.");
        }

        try
        {
            await _configService.SaveAsync(cancellationToken);
            return UiOperationResult<ConfigurationProfileState>.Ok(
                BuildState(_configService.CurrentConfig),
                successMessage);
        }
        catch (Exception ex)
        {
            RestoreSnapshot(_configService.CurrentConfig, snapshot);
            _configService.RevalidateCurrentConfig(logIssues: false);
            return UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileSaveFailed,
                $"Failed to save configuration profiles: {ex.Message}",
                ex.Message);
        }
    }

    private static ConfigSnapshot CaptureSnapshot(UnifiedConfig config)
    {
        var profiles = config.Profiles
            .Select(pair => new KeyValuePair<string, UnifiedProfile>(pair.Key, CloneProfile(pair.Value)))
            .ToList();
        return new ConfigSnapshot(config.CurrentProfile, profiles);
    }

    private static void RestoreSnapshot(UnifiedConfig config, ConfigSnapshot snapshot)
    {
        var restored = new Dictionary<string, UnifiedProfile>(StringComparer.OrdinalIgnoreCase);
        foreach (var pair in snapshot.Profiles)
        {
            restored[pair.Key] = pair.Value;
        }

        config.Profiles = restored;
        config.CurrentProfile = snapshot.CurrentProfile;
    }

    private static bool TryNormalizeProfileName(
        string? value,
        out string normalized,
        out UiOperationResult<ConfigurationProfileState> failure)
    {
        normalized = (value ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(normalized))
        {
            failure = UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileInvalidName,
                "Profile name cannot be empty.");
            return false;
        }

        if (normalized.Any(char.IsControl))
        {
            failure = UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileInvalidName,
                "Profile name cannot contain control characters.");
            return false;
        }

        failure = default!;
        return true;
    }

    private static bool TryValidateConfig(
        UnifiedConfig config,
        out UiOperationResult<ConfigurationProfileState> failure)
    {
        if (config.Profiles.Count == 0)
        {
            failure = UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ProfileMissing,
                "No profile was found in config.");
            return false;
        }

        if (!config.Profiles.ContainsKey(config.CurrentProfile))
        {
            failure = UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ProfileMissing,
                $"Current profile `{config.CurrentProfile}` does not exist.");
            return false;
        }

        failure = default!;
        return true;
    }

    private bool TryGetConfig(
        out UnifiedConfig config,
        out UiOperationResult<ConfigurationProfileState> failure)
    {
        if (_configService is null)
        {
            config = null!;
            failure = UiOperationResult<ConfigurationProfileState>.Fail(
                UiErrorCode.ConfigurationProfileServiceUnavailable,
                "Configuration profile service is not initialized.");
            return false;
        }

        config = _configService.CurrentConfig;
        failure = default!;
        return true;
    }

    private static ConfigurationProfileState BuildState(UnifiedConfig config)
    {
        var orderedProfiles = config.Profiles.Keys.ToList();
        var currentProfile = config.CurrentProfile;
        if (!config.Profiles.ContainsKey(currentProfile) && orderedProfiles.Count > 0)
        {
            currentProfile = orderedProfiles[0];
        }

        return new ConfigurationProfileState(currentProfile, orderedProfiles);
    }

    private static UnifiedProfile CloneProfile(UnifiedProfile source)
    {
        var clone = new UnifiedProfile();
        foreach (var (key, value) in source.Values)
        {
            clone.Values[key] = value?.DeepClone();
        }

        foreach (var task in source.TaskQueue)
        {
            clone.TaskQueue.Add(new UnifiedTaskItem
            {
                Type = task.Type,
                Name = task.Name,
                IsEnabled = task.IsEnabled,
                Params = task.Params.DeepClone() as JsonObject ?? new JsonObject(),
                LegacyRawTask = task.LegacyRawTask?.DeepClone() as JsonObject,
            });
        }

        return clone;
    }

    private sealed record ConfigSnapshot(
        string CurrentProfile,
        IReadOnlyList<KeyValuePair<string, UnifiedProfile>> Profiles);
}
