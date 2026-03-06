using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;

namespace MAAUnified.App.ViewModels.Settings;

internal static class ConnectionGameProfileSync
{
    private const string ConnectAddressKey = "ConnectAddress";
    private const string ConnectConfigKey = "ConnectConfig";
    private const string AdbPathKey = "AdbPath";
    private const string ConnectAddressLegacyKey = "Connect.Address";
    private const string ConnectConfigLegacyKey = "Connect.ConnectConfig";
    private const string AdbPathLegacyKey = "Connect.AdbPath";
    private const string ClientTypeKey = "ClientType";
    private const string StartGameKey = "StartGame";
    private const string TouchModeKey = "TouchMode";
    private const string AutoDetectKey = "AutoDetect";

    private const string DefaultConnectAddress = "127.0.0.1:5555";
    private const string DefaultConnectConfig = "General";
    private const string DefaultAdbPath = "";
    private const string DefaultClientType = "Official";
    private const bool DefaultStartGame = true;
    private const string DefaultTouchMode = "minitouch";
    private const bool DefaultAutoDetect = true;

    private static readonly HashSet<string> SharedPropertyNames = new(StringComparer.Ordinal)
    {
        nameof(ConnectionGameSharedStateViewModel.ConnectAddress),
        nameof(ConnectionGameSharedStateViewModel.ConnectConfig),
        nameof(ConnectionGameSharedStateViewModel.AdbPath),
        nameof(ConnectionGameSharedStateViewModel.ClientType),
        nameof(ConnectionGameSharedStateViewModel.StartGameEnabled),
        nameof(ConnectionGameSharedStateViewModel.TouchMode),
        nameof(ConnectionGameSharedStateViewModel.AutoDetect),
    };

    public static bool ShouldSyncProperty(string? propertyName)
    {
        return string.IsNullOrEmpty(propertyName) || SharedPropertyNames.Contains(propertyName);
    }

    public static void WriteToProfile(UnifiedProfile profile, ConnectionGameSharedStateViewModel state)
    {
        profile.Values[ConnectAddressKey] = JsonValue.Create((state.ConnectAddress ?? string.Empty).Trim());
        profile.Values[ConnectConfigKey] = JsonValue.Create((state.ConnectConfig ?? string.Empty).Trim());
        profile.Values[AdbPathKey] = JsonValue.Create((state.AdbPath ?? string.Empty).Trim());
        profile.Values[ClientTypeKey] = JsonValue.Create((state.ClientType ?? string.Empty).Trim());
        profile.Values[StartGameKey] = JsonValue.Create(state.StartGameEnabled);
        profile.Values[TouchModeKey] = JsonValue.Create((state.TouchMode ?? string.Empty).Trim());
        profile.Values[AutoDetectKey] = JsonValue.Create(state.AutoDetect);
    }

    public static void WritePropertyToProfile(
        UnifiedProfile profile,
        ConnectionGameSharedStateViewModel state,
        string? propertyName)
    {
        switch (propertyName)
        {
            case nameof(ConnectionGameSharedStateViewModel.ConnectAddress):
                profile.Values[ConnectAddressKey] = JsonValue.Create((state.ConnectAddress ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.ConnectConfig):
                profile.Values[ConnectConfigKey] = JsonValue.Create((state.ConnectConfig ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.AdbPath):
                profile.Values[AdbPathKey] = JsonValue.Create((state.AdbPath ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.ClientType):
                profile.Values[ClientTypeKey] = JsonValue.Create((state.ClientType ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.StartGameEnabled):
                profile.Values[StartGameKey] = JsonValue.Create(state.StartGameEnabled);
                return;
            case nameof(ConnectionGameSharedStateViewModel.TouchMode):
                profile.Values[TouchModeKey] = JsonValue.Create((state.TouchMode ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.AutoDetect):
                profile.Values[AutoDetectKey] = JsonValue.Create(state.AutoDetect);
                return;
            default:
                WriteToProfile(profile, state);
                return;
        }
    }

    public static void ReadFromProfile(
        UnifiedProfile profile,
        ConnectionGameSharedStateViewModel state,
        bool tolerateMissing = true)
    {
        var fallbackConnectAddress = tolerateMissing ? state.ConnectAddress : DefaultConnectAddress;
        var fallbackConnectConfig = tolerateMissing ? state.ConnectConfig : DefaultConnectConfig;
        var fallbackAdbPath = tolerateMissing ? state.AdbPath : DefaultAdbPath;
        var fallbackClientType = tolerateMissing ? state.ClientType : DefaultClientType;
        var fallbackStartGame = tolerateMissing ? state.StartGameEnabled : DefaultStartGame;
        var fallbackTouchMode = tolerateMissing ? state.TouchMode : DefaultTouchMode;
        var fallbackAutoDetect = tolerateMissing ? state.AutoDetect : DefaultAutoDetect;

        state.ConnectAddress = ReadProfileStringWithAliases(
            profile,
            fallbackConnectAddress,
            ConnectAddressKey,
            ConnectAddressLegacyKey);
        state.ConnectConfig = ReadProfileStringWithAliases(
            profile,
            fallbackConnectConfig,
            ConnectConfigKey,
            ConnectConfigLegacyKey);
        state.AdbPath = ReadProfileStringWithAliases(
            profile,
            fallbackAdbPath,
            AdbPathKey,
            AdbPathLegacyKey);
        state.ClientType = ReadProfileString(profile, ClientTypeKey, fallbackClientType);
        state.StartGameEnabled = ReadProfileBool(profile, StartGameKey, fallbackStartGame);
        state.TouchMode = ReadProfileString(profile, TouchModeKey, fallbackTouchMode);
        state.AutoDetect = ReadProfileBool(profile, AutoDetectKey, fallbackAutoDetect);
    }

    private static string ReadProfileStringWithAliases(UnifiedProfile profile, string fallback, params string[] keys)
    {
        foreach (var key in keys)
        {
            if (TryReadProfileString(profile, key, out var value))
            {
                return value;
            }
        }

        return fallback;
    }

    private static string ReadProfileString(UnifiedProfile profile, string key, string fallback)
    {
        return TryReadProfileString(profile, key, out var value)
            ? value
            : fallback;
    }

    private static bool TryReadProfileString(UnifiedProfile profile, string key, out string value)
    {
        if (profile.Values.TryGetValue(key, out var node)
            && node is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? text)
            && !string.IsNullOrWhiteSpace(text))
        {
            value = text;
            return true;
        }

        if (profile.Values.TryGetValue(key, out node)
            && node is JsonValue valueNode
            && valueNode.TryGetValue(out JsonElement rawElement)
            && rawElement.ValueKind == JsonValueKind.String)
        {
            var rawText = rawElement.GetString();
            if (!string.IsNullOrWhiteSpace(rawText))
            {
                value = rawText;
                return true;
            }
        }

        value = string.Empty;
        return false;
    }

    private static bool ReadProfileBool(UnifiedProfile profile, string key, bool fallback)
    {
        if (!profile.Values.TryGetValue(key, out var node) || node is not JsonValue value)
        {
            return fallback;
        }

        if (value.TryGetValue(out bool parsed))
        {
            return parsed;
        }

        if (value.TryGetValue(out int parsedInt))
        {
            return parsedInt != 0;
        }

        if (value.TryGetValue(out string? text) && bool.TryParse(text, out var parsedText))
        {
            return parsedText;
        }

        return fallback;
    }
}
