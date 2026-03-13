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
    private const string AlwaysAutoDetectKey = "AlwaysAutoDetect";
    private const string RetryOnDisconnectedKey = "RetryOnDisconnected";
    private const string AllowAdbRestartKey = "AllowAdbRestart";
    private const string AllowAdbHardRestartKey = "AllowAdbHardRestart";
    private const string AdbLiteEnabledKey = "AdbLiteEnabled";
    private const string KillAdbOnExitKey = "KillAdbOnExit";
    private const string AdbReplacedKey = "AdbReplaced";
    private const string MuMu12ExtrasEnabledKey = "MuMu12ExtrasEnabled";
    private const string MuMu12EmulatorPathKey = "MuMu12EmulatorPath";
    private const string MuMuBridgeConnectionKey = "MuMuBridgeConnection";
    private const string MuMu12IndexKey = "MuMu12Index";
    private const string LdPlayerExtrasEnabledKey = "LdPlayerExtrasEnabled";
    private const string LdPlayerEmulatorPathKey = "LdPlayerEmulatorPath";
    private const string LdPlayerManualSetIndexKey = "LdPlayerManualSetIndex";
    private const string LdPlayerIndexKey = "LdPlayerIndex";
    private const string AttachWindowScreencapMethodKey = "AttachWindowScreencapMethod";
    private const string AttachWindowMouseMethodKey = "AttachWindowMouseMethod";
    private const string AttachWindowKeyboardMethodKey = "AttachWindowKeyboardMethod";
    private const string AlwaysAutoDetectLegacyKey = "Connect.AlwaysAutoDetect";
    private const string RetryOnDisconnectedLegacyKey = "Connect.RetryOnDisconnected";
    private const string AllowAdbRestartLegacyKey = "Connect.AllowADBRestart";
    private const string AllowAdbHardRestartLegacyKey = "Connect.AllowADBHardRestart";
    private const string AdbLiteEnabledLegacyKey = "Connect.AdbLiteEnabled";
    private const string KillAdbOnExitLegacyKey = "Connect.KillAdbOnExit";
    private const string AdbReplacedLegacyKey = "Connect.AdbReplaced";
    private const string MuMu12ExtrasEnabledLegacyKey = "Connect.MuMu12Extras.Enabled";
    private const string MuMu12EmulatorPathLegacyKey = "Connect.MuMu12EmulatorPath";
    private const string MuMuBridgeConnectionLegacyKey = "Connect.MumuBridgeConnection";
    private const string MuMu12IndexLegacyKey = "Connect.MuMu12Index";
    private const string LdPlayerExtrasEnabledLegacyKey = "Connect.LdPlayerExtras.Enabled";
    private const string LdPlayerEmulatorPathLegacyKey = "Connect.LdPlayerEmulatorPath";
    private const string LdPlayerManualSetIndexLegacyKey = "Connect.LdPlayerManualSetIndex";
    private const string LdPlayerIndexLegacyKey = "Connect.LdPlayerIndex";
    private const string AttachWindowScreencapMethodLegacyKey = "Connect.AttachWindow.ScreencapMethod";
    private const string AttachWindowMouseMethodLegacyKey = "Connect.AttachWindow.MouseMethod";
    private const string AttachWindowKeyboardMethodLegacyKey = "Connect.AttachWindow.KeyboardMethod";

    private const string DefaultConnectAddress = "127.0.0.1:5555";
    private const string DefaultConnectConfig = "General";
    private const string DefaultAdbPath = "";
    private const string DefaultClientType = "Official";
    private const bool DefaultStartGame = true;
    private const string DefaultTouchMode = "minitouch";
    private const bool DefaultAutoDetect = true;
    private const bool DefaultAlwaysAutoDetect = false;
    private const bool DefaultRetryOnDisconnected = false;
    private const bool DefaultAllowAdbRestart = true;
    private const bool DefaultAllowAdbHardRestart = true;
    private const bool DefaultAdbLiteEnabled = false;
    private const bool DefaultKillAdbOnExit = false;
    private const bool DefaultAdbReplaced = false;
    private const bool DefaultMuMu12ExtrasEnabled = false;
    private const string DefaultMuMu12EmulatorPath = "";
    private const bool DefaultMuMuBridgeConnection = false;
    private const string DefaultMuMu12Index = "0";
    private const bool DefaultLdPlayerExtrasEnabled = false;
    private const string DefaultLdPlayerEmulatorPath = "";
    private const bool DefaultLdPlayerManualSetIndex = false;
    private const string DefaultLdPlayerIndex = "0";
    private const string DefaultAttachWindowScreencapMethod = "2";
    private const string DefaultAttachWindowMouseMethod = "64";
    private const string DefaultAttachWindowKeyboardMethod = "64";

    private static readonly HashSet<string> SharedPropertyNames = new(StringComparer.Ordinal)
    {
        nameof(ConnectionGameSharedStateViewModel.ConnectAddress),
        nameof(ConnectionGameSharedStateViewModel.ConnectConfig),
        nameof(ConnectionGameSharedStateViewModel.AdbPath),
        nameof(ConnectionGameSharedStateViewModel.ClientType),
        nameof(ConnectionGameSharedStateViewModel.StartGameEnabled),
        nameof(ConnectionGameSharedStateViewModel.TouchMode),
        nameof(ConnectionGameSharedStateViewModel.AutoDetect),
        nameof(ConnectionGameSharedStateViewModel.AlwaysAutoDetect),
        nameof(ConnectionGameSharedStateViewModel.RetryOnDisconnected),
        nameof(ConnectionGameSharedStateViewModel.AllowAdbRestart),
        nameof(ConnectionGameSharedStateViewModel.AllowAdbHardRestart),
        nameof(ConnectionGameSharedStateViewModel.AdbLiteEnabled),
        nameof(ConnectionGameSharedStateViewModel.KillAdbOnExit),
        nameof(ConnectionGameSharedStateViewModel.AdbReplaced),
        nameof(ConnectionGameSharedStateViewModel.MuMu12ExtrasEnabled),
        nameof(ConnectionGameSharedStateViewModel.MuMu12EmulatorPath),
        nameof(ConnectionGameSharedStateViewModel.MuMuBridgeConnection),
        nameof(ConnectionGameSharedStateViewModel.MuMu12Index),
        nameof(ConnectionGameSharedStateViewModel.LdPlayerExtrasEnabled),
        nameof(ConnectionGameSharedStateViewModel.LdPlayerEmulatorPath),
        nameof(ConnectionGameSharedStateViewModel.LdPlayerManualSetIndex),
        nameof(ConnectionGameSharedStateViewModel.LdPlayerIndex),
        nameof(ConnectionGameSharedStateViewModel.AttachWindowScreencapMethod),
        nameof(ConnectionGameSharedStateViewModel.AttachWindowMouseMethod),
        nameof(ConnectionGameSharedStateViewModel.AttachWindowKeyboardMethod),
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
        profile.Values[AlwaysAutoDetectKey] = JsonValue.Create(state.AlwaysAutoDetect);
        profile.Values[RetryOnDisconnectedKey] = JsonValue.Create(state.RetryOnDisconnected);
        profile.Values[AllowAdbRestartKey] = JsonValue.Create(state.AllowAdbRestart);
        profile.Values[AllowAdbHardRestartKey] = JsonValue.Create(state.AllowAdbHardRestart);
        profile.Values[AdbLiteEnabledKey] = JsonValue.Create(state.AdbLiteEnabled);
        profile.Values[KillAdbOnExitKey] = JsonValue.Create(state.KillAdbOnExit);
        profile.Values[AdbReplacedKey] = JsonValue.Create(state.AdbReplaced);
        profile.Values[MuMu12ExtrasEnabledKey] = JsonValue.Create(state.MuMu12ExtrasEnabled);
        profile.Values[MuMu12EmulatorPathKey] = JsonValue.Create((state.MuMu12EmulatorPath ?? string.Empty).Trim());
        profile.Values[MuMuBridgeConnectionKey] = JsonValue.Create(state.MuMuBridgeConnection);
        profile.Values[MuMu12IndexKey] = JsonValue.Create((state.MuMu12Index ?? string.Empty).Trim());
        profile.Values[LdPlayerExtrasEnabledKey] = JsonValue.Create(state.LdPlayerExtrasEnabled);
        profile.Values[LdPlayerEmulatorPathKey] = JsonValue.Create((state.LdPlayerEmulatorPath ?? string.Empty).Trim());
        profile.Values[LdPlayerManualSetIndexKey] = JsonValue.Create(state.LdPlayerManualSetIndex);
        profile.Values[LdPlayerIndexKey] = JsonValue.Create((state.LdPlayerIndex ?? string.Empty).Trim());
        profile.Values[AttachWindowScreencapMethodKey] = JsonValue.Create((state.AttachWindowScreencapMethod ?? string.Empty).Trim());
        profile.Values[AttachWindowMouseMethodKey] = JsonValue.Create((state.AttachWindowMouseMethod ?? string.Empty).Trim());
        profile.Values[AttachWindowKeyboardMethodKey] = JsonValue.Create((state.AttachWindowKeyboardMethod ?? string.Empty).Trim());
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
            case nameof(ConnectionGameSharedStateViewModel.AlwaysAutoDetect):
                profile.Values[AlwaysAutoDetectKey] = JsonValue.Create(state.AlwaysAutoDetect);
                return;
            case nameof(ConnectionGameSharedStateViewModel.RetryOnDisconnected):
                profile.Values[RetryOnDisconnectedKey] = JsonValue.Create(state.RetryOnDisconnected);
                return;
            case nameof(ConnectionGameSharedStateViewModel.AllowAdbRestart):
                profile.Values[AllowAdbRestartKey] = JsonValue.Create(state.AllowAdbRestart);
                return;
            case nameof(ConnectionGameSharedStateViewModel.AllowAdbHardRestart):
                profile.Values[AllowAdbHardRestartKey] = JsonValue.Create(state.AllowAdbHardRestart);
                return;
            case nameof(ConnectionGameSharedStateViewModel.AdbLiteEnabled):
                profile.Values[AdbLiteEnabledKey] = JsonValue.Create(state.AdbLiteEnabled);
                return;
            case nameof(ConnectionGameSharedStateViewModel.KillAdbOnExit):
                profile.Values[KillAdbOnExitKey] = JsonValue.Create(state.KillAdbOnExit);
                return;
            case nameof(ConnectionGameSharedStateViewModel.AdbReplaced):
                profile.Values[AdbReplacedKey] = JsonValue.Create(state.AdbReplaced);
                return;
            case nameof(ConnectionGameSharedStateViewModel.MuMu12ExtrasEnabled):
                profile.Values[MuMu12ExtrasEnabledKey] = JsonValue.Create(state.MuMu12ExtrasEnabled);
                return;
            case nameof(ConnectionGameSharedStateViewModel.MuMu12EmulatorPath):
                profile.Values[MuMu12EmulatorPathKey] = JsonValue.Create((state.MuMu12EmulatorPath ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.MuMuBridgeConnection):
                profile.Values[MuMuBridgeConnectionKey] = JsonValue.Create(state.MuMuBridgeConnection);
                return;
            case nameof(ConnectionGameSharedStateViewModel.MuMu12Index):
                profile.Values[MuMu12IndexKey] = JsonValue.Create((state.MuMu12Index ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.LdPlayerExtrasEnabled):
                profile.Values[LdPlayerExtrasEnabledKey] = JsonValue.Create(state.LdPlayerExtrasEnabled);
                return;
            case nameof(ConnectionGameSharedStateViewModel.LdPlayerEmulatorPath):
                profile.Values[LdPlayerEmulatorPathKey] = JsonValue.Create((state.LdPlayerEmulatorPath ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.LdPlayerManualSetIndex):
                profile.Values[LdPlayerManualSetIndexKey] = JsonValue.Create(state.LdPlayerManualSetIndex);
                return;
            case nameof(ConnectionGameSharedStateViewModel.LdPlayerIndex):
                profile.Values[LdPlayerIndexKey] = JsonValue.Create((state.LdPlayerIndex ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.AttachWindowScreencapMethod):
                profile.Values[AttachWindowScreencapMethodKey] = JsonValue.Create((state.AttachWindowScreencapMethod ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.AttachWindowMouseMethod):
                profile.Values[AttachWindowMouseMethodKey] = JsonValue.Create((state.AttachWindowMouseMethod ?? string.Empty).Trim());
                return;
            case nameof(ConnectionGameSharedStateViewModel.AttachWindowKeyboardMethod):
                profile.Values[AttachWindowKeyboardMethodKey] = JsonValue.Create((state.AttachWindowKeyboardMethod ?? string.Empty).Trim());
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
        var fallbackAlwaysAutoDetect = tolerateMissing ? state.AlwaysAutoDetect : DefaultAlwaysAutoDetect;
        var fallbackRetryOnDisconnected = tolerateMissing ? state.RetryOnDisconnected : DefaultRetryOnDisconnected;
        var fallbackAllowAdbRestart = tolerateMissing ? state.AllowAdbRestart : DefaultAllowAdbRestart;
        var fallbackAllowAdbHardRestart = tolerateMissing ? state.AllowAdbHardRestart : DefaultAllowAdbHardRestart;
        var fallbackAdbLiteEnabled = tolerateMissing ? state.AdbLiteEnabled : DefaultAdbLiteEnabled;
        var fallbackKillAdbOnExit = tolerateMissing ? state.KillAdbOnExit : DefaultKillAdbOnExit;
        var fallbackAdbReplaced = tolerateMissing ? state.AdbReplaced : DefaultAdbReplaced;
        var fallbackMuMu12ExtrasEnabled = tolerateMissing ? state.MuMu12ExtrasEnabled : DefaultMuMu12ExtrasEnabled;
        var fallbackMuMu12EmulatorPath = tolerateMissing ? state.MuMu12EmulatorPath : DefaultMuMu12EmulatorPath;
        var fallbackMuMuBridgeConnection = tolerateMissing ? state.MuMuBridgeConnection : DefaultMuMuBridgeConnection;
        var fallbackMuMu12Index = tolerateMissing ? state.MuMu12Index : DefaultMuMu12Index;
        var fallbackLdPlayerExtrasEnabled = tolerateMissing ? state.LdPlayerExtrasEnabled : DefaultLdPlayerExtrasEnabled;
        var fallbackLdPlayerEmulatorPath = tolerateMissing ? state.LdPlayerEmulatorPath : DefaultLdPlayerEmulatorPath;
        var fallbackLdPlayerManualSetIndex = tolerateMissing ? state.LdPlayerManualSetIndex : DefaultLdPlayerManualSetIndex;
        var fallbackLdPlayerIndex = tolerateMissing ? state.LdPlayerIndex : DefaultLdPlayerIndex;
        var fallbackAttachWindowScreencapMethod = tolerateMissing ? state.AttachWindowScreencapMethod : DefaultAttachWindowScreencapMethod;
        var fallbackAttachWindowMouseMethod = tolerateMissing ? state.AttachWindowMouseMethod : DefaultAttachWindowMouseMethod;
        var fallbackAttachWindowKeyboardMethod = tolerateMissing ? state.AttachWindowKeyboardMethod : DefaultAttachWindowKeyboardMethod;

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
        state.AlwaysAutoDetect = ReadProfileBoolWithAliases(
            profile,
            fallbackAlwaysAutoDetect,
            AlwaysAutoDetectKey,
            AlwaysAutoDetectLegacyKey);
        state.RetryOnDisconnected = ReadProfileBoolWithAliases(
            profile,
            fallbackRetryOnDisconnected,
            RetryOnDisconnectedKey,
            RetryOnDisconnectedLegacyKey);
        state.AllowAdbRestart = ReadProfileBoolWithAliases(
            profile,
            fallbackAllowAdbRestart,
            AllowAdbRestartKey,
            AllowAdbRestartLegacyKey);
        state.AllowAdbHardRestart = ReadProfileBoolWithAliases(
            profile,
            fallbackAllowAdbHardRestart,
            AllowAdbHardRestartKey,
            AllowAdbHardRestartLegacyKey);
        state.AdbLiteEnabled = ReadProfileBoolWithAliases(
            profile,
            fallbackAdbLiteEnabled,
            AdbLiteEnabledKey,
            AdbLiteEnabledLegacyKey);
        state.KillAdbOnExit = ReadProfileBoolWithAliases(
            profile,
            fallbackKillAdbOnExit,
            KillAdbOnExitKey,
            KillAdbOnExitLegacyKey);
        state.AdbReplaced = ReadProfileBoolWithAliases(
            profile,
            fallbackAdbReplaced,
            AdbReplacedKey,
            AdbReplacedLegacyKey);
        state.MuMu12ExtrasEnabled = ReadProfileBoolWithAliases(
            profile,
            fallbackMuMu12ExtrasEnabled,
            MuMu12ExtrasEnabledKey,
            MuMu12ExtrasEnabledLegacyKey);
        state.MuMu12EmulatorPath = ReadProfileStringWithAliases(
            profile,
            fallbackMuMu12EmulatorPath,
            MuMu12EmulatorPathKey,
            MuMu12EmulatorPathLegacyKey);
        state.MuMuBridgeConnection = ReadProfileBoolWithAliases(
            profile,
            fallbackMuMuBridgeConnection,
            MuMuBridgeConnectionKey,
            MuMuBridgeConnectionLegacyKey);
        state.MuMu12Index = ReadProfileStringWithAliases(
            profile,
            fallbackMuMu12Index,
            MuMu12IndexKey,
            MuMu12IndexLegacyKey);
        state.LdPlayerExtrasEnabled = ReadProfileBoolWithAliases(
            profile,
            fallbackLdPlayerExtrasEnabled,
            LdPlayerExtrasEnabledKey,
            LdPlayerExtrasEnabledLegacyKey);
        state.LdPlayerEmulatorPath = ReadProfileStringWithAliases(
            profile,
            fallbackLdPlayerEmulatorPath,
            LdPlayerEmulatorPathKey,
            LdPlayerEmulatorPathLegacyKey);
        state.LdPlayerManualSetIndex = ReadProfileBoolWithAliases(
            profile,
            fallbackLdPlayerManualSetIndex,
            LdPlayerManualSetIndexKey,
            LdPlayerManualSetIndexLegacyKey);
        state.LdPlayerIndex = ReadProfileStringWithAliases(
            profile,
            fallbackLdPlayerIndex,
            LdPlayerIndexKey,
            LdPlayerIndexLegacyKey);
        state.AttachWindowScreencapMethod = ReadProfileStringWithAliases(
            profile,
            fallbackAttachWindowScreencapMethod,
            AttachWindowScreencapMethodKey,
            AttachWindowScreencapMethodLegacyKey);
        state.AttachWindowMouseMethod = ReadProfileStringWithAliases(
            profile,
            fallbackAttachWindowMouseMethod,
            AttachWindowMouseMethodKey,
            AttachWindowMouseMethodLegacyKey);
        state.AttachWindowKeyboardMethod = ReadProfileStringWithAliases(
            profile,
            fallbackAttachWindowKeyboardMethod,
            AttachWindowKeyboardMethodKey,
            AttachWindowKeyboardMethodLegacyKey);
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

    private static bool ReadProfileBoolWithAliases(UnifiedProfile profile, bool fallback, params string[] keys)
    {
        foreach (var key in keys)
        {
            if (TryReadProfileBool(profile, key, out var parsed))
            {
                return parsed;
            }
        }

        return fallback;
    }

    private static bool TryReadProfileBool(UnifiedProfile profile, string key, out bool value)
    {
        if (!profile.Values.TryGetValue(key, out var node) || node is not JsonValue jsonValue)
        {
            value = false;
            return false;
        }

        if (jsonValue.TryGetValue(out bool parsedBool))
        {
            value = parsedBool;
            return true;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            value = parsedInt != 0;
            return true;
        }

        if (jsonValue.TryGetValue(out string? text))
        {
            if (bool.TryParse(text, out var parsedText))
            {
                value = parsedText;
                return true;
            }

            if (int.TryParse(text, out parsedInt))
            {
                value = parsedInt != 0;
                return true;
            }
        }

        value = false;
        return false;
    }
}
