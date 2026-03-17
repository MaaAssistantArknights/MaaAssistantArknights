using System.Text.Json.Nodes;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using MAAUnified.Application.Services.Localization;

namespace MAAUnified.App.ViewModels.Toolbox;

internal static class ToolboxAssetCatalog
{
    private static readonly object CharacterGate = new();
    private static readonly object ItemGate = new();
    private static readonly object ImageGate = new();
    private static readonly object MiniGameGate = new();
    private static IReadOnlyList<string>? _testBaseDirectories;
    private static IReadOnlyDictionary<string, ToolboxOperatorAsset>? _characters;
    private static readonly Dictionary<string, IReadOnlyDictionary<string, string>> ItemNamesByLanguage = new(StringComparer.OrdinalIgnoreCase);
    private static readonly Dictionary<string, Bitmap?> BitmapCache = new(StringComparer.Ordinal);
    private static IReadOnlyList<ToolboxMiniGameEntry>? _miniGames;
    private static readonly IReadOnlyDictionary<string, string> MiniGameTextByKey = new Dictionary<string, string>(StringComparer.Ordinal)
    {
        ["MiniGameNameEmptyTip"] = "在上方选择小游戏以开始运行。",
        ["MiniGame@SecretFront"] = "隐秘战线",
        ["MiniGame@SecretFrontTip"] = "在选小队界面开始，如有存档须手动删除。\n第一次打自己看完把教程关了。\n推荐勾选游戏内「继承上一支队伍发回的数据」",
        ["MiniGameNameGreenTicketStore"] = "绿票商店",
        ["MiniGameNameGreenTicketStoreTip"] = "1层全买。\n2层买寻访凭证和招聘许可。",
        ["MiniGameNameYellowTicketStore"] = "黄票商店",
        ["MiniGameNameYellowTicketStoreTip"] = "请确保自己至少有258张黄票。",
        ["MiniGameNameSsStore"] = "活动商店",
        ["MiniGameNameSsStoreTip"] = "请在活动商店页面开始。\n不买无限池。",
        ["MiniGameNameRAStore"] = "生息演算商店",
        ["MiniGameNameRAStoreTip"] = "请在活动商店页面开始。",
    };

    public static IReadOnlyDictionary<string, ToolboxOperatorAsset> GetOperators()
    {
        lock (CharacterGate)
        {
            _characters ??= LoadOperators();
            return _characters;
        }
    }

    public static IReadOnlyDictionary<string, string> GetItemNames(string language)
    {
        var normalizedLanguage = UiLanguageCatalog.IsSupported(language)
            ? UiLanguageCatalog.Normalize(language)
            : UiLanguageCatalog.DefaultLanguage;

        lock (ItemGate)
        {
            if (!ItemNamesByLanguage.TryGetValue(normalizedLanguage, out var items))
            {
                items = LoadItemNames(normalizedLanguage);
                ItemNamesByLanguage[normalizedLanguage] = items;
            }

            return items;
        }
    }

    public static IReadOnlyList<ToolboxMiniGameEntry> GetMiniGameEntries()
    {
        lock (MiniGameGate)
        {
            _miniGames ??= LoadMiniGameEntries();
            return _miniGames;
        }
    }

    public static bool IsOperatorAvailableInClient(ToolboxOperatorAsset asset, string? clientType)
    {
        var normalized = NormalizeClientType(clientType);
        return normalized switch
        {
            "txwy" => !asset.NameTwUnavailable,
            "YoStarEN" => !asset.NameEnUnavailable,
            "YoStarJP" => !asset.NameJpUnavailable,
            "YoStarKR" => !asset.NameKrUnavailable,
            _ => true,
        };
    }

    public static string GetLocalizedOperatorName(ToolboxOperatorAsset asset, string? language)
    {
        var normalized = UiLanguageCatalog.IsSupported(language)
            ? UiLanguageCatalog.Normalize(language)
            : UiLanguageCatalog.DefaultLanguage;

        return normalized switch
        {
            "zh-tw" when !string.IsNullOrWhiteSpace(asset.NameTw) => asset.NameTw!,
            "en-us" when !string.IsNullOrWhiteSpace(asset.NameEn) => asset.NameEn!,
            "ja-jp" when !string.IsNullOrWhiteSpace(asset.NameJp) => asset.NameJp!,
            "ko-kr" when !string.IsNullOrWhiteSpace(asset.NameKr) => asset.NameKr!,
            _ => asset.Name,
        };
    }

    public static string? ResolveItemImagePath(string itemId)
    {
        foreach (var root in EnumerateBaseDirectories())
        {
            var path = Path.Combine(root, "resource", "template", "items", $"{itemId}.png");
            if (File.Exists(path))
            {
                return path;
            }
        }

        return null;
    }

    public static Bitmap? ResolveItemBitmap(string? itemId)
    {
        var normalized = string.IsNullOrWhiteSpace(itemId) ? string.Empty : itemId.Trim();
        if (string.IsNullOrWhiteSpace(normalized))
        {
            return null;
        }

        lock (ImageGate)
        {
            if (BitmapCache.TryGetValue($"item:{normalized}", out var cached))
            {
                return cached;
            }

            var bitmap = TryLoadFileBitmap(ResolveItemImagePath(normalized));
            BitmapCache[$"item:{normalized}"] = bitmap;
            return bitmap;
        }
    }

    public static Bitmap? ResolveOperatorEliteBitmap(int elite)
    {
        return ResolveEmbeddedBitmap($"operator:elite:{Math.Clamp(elite, 0, 2)}", $"avares://MAAUnified/Assets/Toolbox/Operator/Elite_{Math.Clamp(elite, 0, 2)}.png");
    }

    public static Bitmap? ResolveOperatorPotentialBitmap(int potential)
    {
        var normalized = potential is >= 1 and <= 6 ? potential : 1;
        return ResolveEmbeddedBitmap($"operator:potential:{normalized}", $"avares://MAAUnified/Assets/Toolbox/Operator/Potential_{normalized}.png");
    }

    internal static string? ResolveOperatorEliteAssetPath(int elite)
    {
        return ResolveAssetFilePath($"avares://MAAUnified/Assets/Toolbox/Operator/Elite_{Math.Clamp(elite, 0, 2)}.png");
    }

    internal static string? ResolveOperatorPotentialAssetPath(int potential)
    {
        var normalized = potential is >= 1 and <= 6 ? potential : 1;
        return ResolveAssetFilePath($"avares://MAAUnified/Assets/Toolbox/Operator/Potential_{normalized}.png");
    }

    internal static IDisposable PushTestBaseDirectoriesForTests(params string[] directories)
    {
        var normalized = directories
            .Where(path => !string.IsNullOrWhiteSpace(path))
            .Select(Path.GetFullPath)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();
        var previous = _testBaseDirectories;
        _testBaseDirectories = normalized;
        ResetCachesForTests();
        return new ToolboxAssetCatalogResetScope(previous);
    }

    internal static void ResetCachesForTests()
    {
        lock (CharacterGate)
        {
            _characters = null;
        }

        lock (ItemGate)
        {
            ItemNamesByLanguage.Clear();
        }

        lock (ImageGate)
        {
            foreach (var bitmap in BitmapCache.Values.Where(bitmap => bitmap is not null))
            {
                bitmap!.Dispose();
            }

            BitmapCache.Clear();
        }

        lock (MiniGameGate)
        {
            _miniGames = null;
        }
    }

    internal static void RestoreTestBaseDirectoriesForTests(IReadOnlyList<string>? directories)
    {
        _testBaseDirectories = directories;
        ResetCachesForTests();
    }

    private static IReadOnlyDictionary<string, ToolboxOperatorAsset> LoadOperators()
    {
        var path = ResolveBattleDataPath();
        if (path is null)
        {
            return new Dictionary<string, ToolboxOperatorAsset>(StringComparer.Ordinal);
        }

        try
        {
            var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
            if (root?["chars"] is not JsonObject chars)
            {
                return new Dictionary<string, ToolboxOperatorAsset>(StringComparer.Ordinal);
            }

            var result = new Dictionary<string, ToolboxOperatorAsset>(StringComparer.Ordinal);
            foreach (var pair in chars)
            {
                if (pair.Value is not JsonObject node || string.IsNullOrWhiteSpace(pair.Key))
                {
                    continue;
                }

                if (!TryReadString(node["name"], out var name) || string.IsNullOrWhiteSpace(name))
                {
                    continue;
                }

                var isOperator = pair.Key.StartsWith("char_", StringComparison.Ordinal);
                if (!isOperator)
                {
                    continue;
                }

                _ = TryReadString(node["name_tw"], out var nameTw);
                _ = TryReadString(node["name_en"], out var nameEn);
                _ = TryReadString(node["name_jp"], out var nameJp);
                _ = TryReadString(node["name_kr"], out var nameKr);
                _ = TryReadBool(node["name_tw_unavailable"], out var nameTwUnavailable);
                _ = TryReadBool(node["name_en_unavailable"], out var nameEnUnavailable);
                _ = TryReadBool(node["name_jp_unavailable"], out var nameJpUnavailable);
                _ = TryReadBool(node["name_kr_unavailable"], out var nameKrUnavailable);
                _ = TryReadInt(node["rarity"], out var rarity);

                result[pair.Key] = new ToolboxOperatorAsset(
                    pair.Key,
                    name,
                    string.IsNullOrWhiteSpace(nameTw) ? null : nameTw,
                    string.IsNullOrWhiteSpace(nameEn) ? null : nameEn,
                    string.IsNullOrWhiteSpace(nameJp) ? null : nameJp,
                    string.IsNullOrWhiteSpace(nameKr) ? null : nameKr,
                    rarity,
                    nameTwUnavailable,
                    nameEnUnavailable,
                    nameJpUnavailable,
                    nameKrUnavailable);
            }

            return result;
        }
        catch
        {
            return new Dictionary<string, ToolboxOperatorAsset>(StringComparer.Ordinal);
        }
    }

    private static IReadOnlyDictionary<string, string> LoadItemNames(string language)
    {
        var path = ResolveItemIndexPath(language);
        if (path is null)
        {
            return new Dictionary<string, string>(StringComparer.Ordinal);
        }

        try
        {
            var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
            if (root is null)
            {
                return new Dictionary<string, string>(StringComparer.Ordinal);
            }

            var result = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var pair in root)
            {
                if (pair.Value is not JsonObject node || string.IsNullOrWhiteSpace(pair.Key))
                {
                    continue;
                }

                if (TryReadString(node["name"], out var name) && !string.IsNullOrWhiteSpace(name))
                {
                    result[pair.Key] = name;
                }
            }

            return result;
        }
        catch
        {
            return new Dictionary<string, string>(StringComparer.Ordinal);
        }
    }

    private static IReadOnlyList<ToolboxMiniGameEntry> LoadMiniGameEntries()
    {
        var entries = new List<ToolboxMiniGameEntry>
        {
            new("活动商店", "SS@Store@Begin", ResolveMiniGameText("MiniGameNameSsStoreTip") ?? string.Empty),
            new("绿票商店", "GreenTicket@Store@Begin", ResolveMiniGameText("MiniGameNameGreenTicketStoreTip") ?? string.Empty),
            new("黄票商店", "YellowTicket@Store@Begin", ResolveMiniGameText("MiniGameNameYellowTicketStoreTip") ?? string.Empty),
            new("生息演算商店", "RA@Store@Begin", ResolveMiniGameText("MiniGameNameRAStoreTip") ?? string.Empty),
            new("隐秘战线", "MiniGame@SecretFront", ResolveMiniGameText("MiniGame@SecretFrontTip") ?? string.Empty),
        };

        var path = ResolveMiniGameStagePath();
        if (path is null)
        {
            return entries;
        }

        try
        {
            var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
            if (root is null)
            {
                return entries;
            }

            foreach (var clientEntry in root)
            {
                if (clientEntry.Value is not JsonObject clientNode || clientNode["miniGame"] is null)
                {
                    continue;
                }

                AppendMiniGameEntries(entries, clientNode["miniGame"]);
            }
        }
        catch
        {
            // Ignore optional stage activity parse failures.
        }

        return entries
            .GroupBy(entry => entry.Value, StringComparer.Ordinal)
            .Select(group => group.First())
            .ToArray();
    }

    private static void AppendMiniGameEntries(ICollection<ToolboxMiniGameEntry> target, JsonNode? miniGameNode)
    {
        if (miniGameNode is JsonArray array)
        {
            foreach (var item in array)
            {
                TryAppendMiniGameEntry(target, item);
            }

            return;
        }

        TryAppendMiniGameEntry(target, miniGameNode);
    }

    private static void TryAppendMiniGameEntry(ICollection<ToolboxMiniGameEntry> target, JsonNode? node)
    {
        if (node is null)
        {
            return;
        }

        if (node is JsonValue value && value.TryGetValue(out string? simpleValue) && !string.IsNullOrWhiteSpace(simpleValue))
        {
            var trimmed = simpleValue.Trim();
            target.Add(new ToolboxMiniGameEntry(
                ResolveMiniGameText(trimmed) ?? trimmed,
                trimmed,
                ResolveMiniGameText(trimmed + "Tip") ?? string.Empty));
            return;
        }

        if (node is not JsonObject obj)
        {
            return;
        }

        _ = TryReadString(obj["Display"], out var display);
        _ = TryReadString(obj["DisplayKey"], out var displayKey);
        _ = TryReadString(obj["Value"], out var explicitValue);
        _ = TryReadString(obj["value"], out var lowerValue);
        _ = TryReadString(obj["Tip"], out var tip);
        _ = TryReadString(obj["TipKey"], out var tipKey);

        var finalValue = FirstNonEmpty(explicitValue, lowerValue, display, displayKey);
        if (string.IsNullOrWhiteSpace(finalValue))
        {
            return;
        }

        var finalDisplay = FirstNonEmpty(display, ResolveMiniGameText(displayKey), finalValue);
        var finalTip = FirstNonEmpty(ResolveMiniGameText(tipKey), tip, ResolveMiniGameText(displayKey + "Tip"), string.Empty);
        target.Add(new ToolboxMiniGameEntry(finalDisplay!, finalValue!, finalTip!));
    }

    private static Bitmap? ResolveEmbeddedBitmap(string cacheKey, string assetUri)
    {
        lock (ImageGate)
        {
            if (BitmapCache.TryGetValue(cacheKey, out var cached))
            {
                return cached;
            }

            var bitmap = TryLoadAssetBitmap(assetUri);
            BitmapCache[cacheKey] = bitmap;
            return bitmap;
        }
    }

    private static Bitmap? TryLoadAssetBitmap(string assetUri)
    {
        try
        {
            using var stream = AssetLoader.Open(new Uri(assetUri));
            return new Bitmap(stream);
        }
        catch
        {
            return TryLoadFileBitmap(ResolveAssetFilePath(assetUri));
        }
    }

    private static Bitmap? TryLoadFileBitmap(string? path)
    {
        if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
        {
            return null;
        }

        try
        {
            return new Bitmap(path);
        }
        catch
        {
            return null;
        }
    }

    private static string? ResolveAssetFilePath(string assetUri)
    {
        if (!Uri.TryCreate(assetUri, UriKind.Absolute, out var uri))
        {
            return null;
        }

        var relativePath = uri.AbsolutePath.TrimStart('/')
            .Replace('/', Path.DirectorySeparatorChar);
        if (string.IsNullOrWhiteSpace(relativePath))
        {
            return null;
        }

        foreach (var root in EnumerateBaseDirectories())
        {
            var direct = Path.Combine(root, relativePath);
            if (File.Exists(direct))
            {
                return direct;
            }

            var appRelative = Path.Combine(root, "App", relativePath);
            if (File.Exists(appRelative))
            {
                return appRelative;
            }

            var sourceTree = Path.Combine(root, "src", "MAAUnified", "App", relativePath);
            if (File.Exists(sourceTree))
            {
                return sourceTree;
            }
        }

        return null;
    }

    private static string? ResolveBattleDataPath()
    {
        foreach (var root in EnumerateBaseDirectories())
        {
            var candidate = Path.Combine(root, "resource", "battle_data.json");
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }

    private static string? ResolveItemIndexPath(string language)
    {
        var normalized = UiLanguageCatalog.IsSupported(language)
            ? UiLanguageCatalog.Normalize(language)
            : UiLanguageCatalog.DefaultLanguage;
        var clientDirectory = normalized switch
        {
            "zh-tw" => "txwy",
            "en-us" => "YoStarEN",
            "ja-jp" => "YoStarJP",
            "ko-kr" => "YoStarKR",
            _ => null,
        };

        foreach (var root in EnumerateBaseDirectories())
        {
            if (!string.IsNullOrWhiteSpace(clientDirectory))
            {
                var localized = Path.Combine(root, "resource", "global", clientDirectory, "resource", "item_index.json");
                if (File.Exists(localized))
                {
                    return localized;
                }
            }

            var fallback = Path.Combine(root, "resource", "item_index.json");
            if (File.Exists(fallback))
            {
                return fallback;
            }
        }

        return null;
    }

    private static string? ResolveMiniGameStagePath()
    {
        foreach (var root in EnumerateBaseDirectories())
        {
            var direct = Path.Combine(root, "gui", "StageActivityV2.json");
            if (File.Exists(direct))
            {
                return direct;
            }

            var underResource = Path.Combine(root, "resource", "gui", "StageActivityV2.json");
            if (File.Exists(underResource))
            {
                return underResource;
            }
        }

        return null;
    }

    private static IEnumerable<string> EnumerateBaseDirectories()
    {
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        if (_testBaseDirectories is not null)
        {
            foreach (var candidate in _testBaseDirectories)
            {
                foreach (var directory in EnumerateDirectoryAndParents(candidate))
                {
                    if (seen.Add(directory))
                    {
                        yield return directory;
                    }
                }
            }
        }

        var candidates = new[]
        {
            AppContext.BaseDirectory,
            Environment.CurrentDirectory,
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..")),
        };

        foreach (var candidate in candidates)
        {
            foreach (var directory in EnumerateDirectoryAndParents(candidate))
            {
                if (seen.Add(directory))
                {
                    yield return directory;
                }
            }
        }
    }

    private static IEnumerable<string> EnumerateDirectoryAndParents(string? candidate)
    {
        if (string.IsNullOrWhiteSpace(candidate))
        {
            yield break;
        }

        var current = new DirectoryInfo(Path.GetFullPath(candidate));
        while (current is not null)
        {
            yield return current.FullName;
            current = current.Parent;
        }
    }

    private static string NormalizeClientType(string? clientType)
    {
        var normalized = (clientType ?? string.Empty).Trim();
        if (string.Equals(normalized, "Txwy", StringComparison.OrdinalIgnoreCase))
        {
            return "txwy";
        }

        return string.IsNullOrWhiteSpace(normalized) ? "Official" : normalized;
    }

    private static string? FirstNonEmpty(params string?[] values)
    {
        foreach (var value in values)
        {
            if (!string.IsNullOrWhiteSpace(value))
            {
                return value.Trim();
            }
        }

        return null;
    }

    private static string? ResolveMiniGameText(string? key)
    {
        if (string.IsNullOrWhiteSpace(key))
        {
            return null;
        }

        return MiniGameTextByKey.TryGetValue(key.Trim(), out var text)
            ? text
            : null;
    }

    private static bool TryReadString(JsonNode? node, out string value)
    {
        value = string.Empty;
        if (node is not JsonValue jsonValue || !jsonValue.TryGetValue(out string? parsed) || string.IsNullOrWhiteSpace(parsed))
        {
            return false;
        }

        value = parsed.Trim();
        return true;
    }

    private static bool TryReadBool(JsonNode? node, out bool value)
    {
        value = false;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out bool parsedBool))
        {
            value = parsedBool;
            return true;
        }

        if (jsonValue.TryGetValue(out string? parsedText) && bool.TryParse(parsedText, out parsedBool))
        {
            value = parsedBool;
            return true;
        }

        return false;
    }

    private static bool TryReadInt(JsonNode? node, out int value)
    {
        value = 0;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            value = parsedInt;
            return true;
        }

        if (jsonValue.TryGetValue(out string? parsedText) && int.TryParse(parsedText, out parsedInt))
        {
            value = parsedInt;
            return true;
        }

        return false;
    }
}

internal sealed class ToolboxAssetCatalogResetScope : IDisposable
{
    private readonly IReadOnlyList<string>? _previous;
    private bool _disposed;

    public ToolboxAssetCatalogResetScope(IReadOnlyList<string>? previous)
    {
        _previous = previous;
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        ToolboxAssetCatalog.RestoreTestBaseDirectoriesForTests(_previous);
    }
}

internal sealed record ToolboxOperatorAsset(
    string Id,
    string Name,
    string? NameTw,
    string? NameEn,
    string? NameJp,
    string? NameKr,
    int Rarity,
    bool NameTwUnavailable,
    bool NameEnUnavailable,
    bool NameJpUnavailable,
    bool NameKrUnavailable);

public sealed record ToolboxMiniGameEntry(string Display, string Value, string Tip);
