using System.Text.Json;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Settings;

internal static class ConfigurationImportSelectionAnalyzer
{
    public static ConfigurationImportSelectionAnalysis Analyze(IEnumerable<string> filePaths)
    {
        var normalized = filePaths
            .Where(static path => !string.IsNullOrWhiteSpace(path))
            .Select(static path => Path.GetFullPath(path.Trim()))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();

        if (normalized.Length == 0)
        {
            return ConfigurationImportSelectionAnalysis.Invalid("未选择任何文件。");
        }

        if (normalized.Length > 2)
        {
            return ConfigurationImportSelectionAnalysis.Invalid("旧配置导入最多选择两个文件；统一配置 json 只支持单文件导入。");
        }

        if (normalized.Length == 1)
        {
            var singlePath = normalized[0];
            var fileName = Path.GetFileName(singlePath);
            if (IsGuiNewFile(fileName))
            {
                return ConfigurationImportSelectionAnalysis.Legacy(singlePath, null, hasInvalidFiles: false);
            }

            if (IsGuiFile(fileName))
            {
                return ConfigurationImportSelectionAnalysis.Legacy(null, singlePath, hasInvalidFiles: false);
            }

            return InspectSingleFile(singlePath);
        }

        var guiNewPath = normalized.FirstOrDefault(path => IsGuiNewFile(Path.GetFileName(path)));
        var guiPath = normalized.FirstOrDefault(path => IsGuiFile(Path.GetFileName(path)));
        if (!string.IsNullOrWhiteSpace(guiNewPath) || !string.IsNullOrWhiteSpace(guiPath))
        {
            var invalidCount = normalized.Length
                               - (string.IsNullOrWhiteSpace(guiNewPath) ? 0 : 1)
                               - (string.IsNullOrWhiteSpace(guiPath) ? 0 : 1);
            return ConfigurationImportSelectionAnalysis.Legacy(guiNewPath, guiPath, invalidCount > 0);
        }

        if (normalized.Length == 2 && normalized.Any(path => InspectJsonShape(path) == ConfigurationImportJsonShape.UnifiedConfig))
        {
            return ConfigurationImportSelectionAnalysis.Invalid("统一配置 json 只支持单文件导入。");
        }

        return ConfigurationImportSelectionAnalysis.Invalid("无法识别导入文件，请选择导出的配置 json，或 gui.json / gui.new.json。");
    }

    private static ConfigurationImportSelectionAnalysis InspectSingleFile(string path)
    {
        return InspectJsonShape(path) switch
        {
            ConfigurationImportJsonShape.UnifiedConfig => ConfigurationImportSelectionAnalysis.Unified(path),
            ConfigurationImportJsonShape.LegacyConfig => ConfigurationImportSelectionAnalysis.Invalid(
                "旧配置文件名不正确，请选择 gui.json 或 gui.new.json。"),
            _ => ConfigurationImportSelectionAnalysis.Invalid(
                "无法识别导入文件，请选择导出的配置 json，或 gui.json / gui.new.json。"),
        };
    }

    private static ConfigurationImportJsonShape InspectJsonShape(string path)
    {
        try
        {
            using var stream = File.OpenRead(path);
            using var document = JsonDocument.Parse(stream);
            var root = document.RootElement;
            if (root.ValueKind != JsonValueKind.Object)
            {
                return ConfigurationImportJsonShape.Unknown;
            }

            var looksUnified =
                root.TryGetProperty("Profiles", out _)
                && root.TryGetProperty("CurrentProfile", out _);
            if (looksUnified)
            {
                return ConfigurationImportJsonShape.UnifiedConfig;
            }

            var looksLegacy =
                root.TryGetProperty("Configurations", out _)
                || root.TryGetProperty("Current", out _)
                || root.TryGetProperty("GUI", out _)
                || root.TryGetProperty("Global", out _);
            return looksLegacy
                ? ConfigurationImportJsonShape.LegacyConfig
                : ConfigurationImportJsonShape.Unknown;
        }
        catch
        {
            return ConfigurationImportJsonShape.Unknown;
        }
    }

    private static bool IsGuiNewFile(string? fileName)
        => string.Equals(fileName, "gui.new.json", StringComparison.OrdinalIgnoreCase);

    private static bool IsGuiFile(string? fileName)
        => string.Equals(fileName, "gui.json", StringComparison.OrdinalIgnoreCase);
}

internal sealed record ConfigurationImportSelectionAnalysis(
    ConfigurationImportSelectionKind Kind,
    string? UnifiedConfigPath,
    string? GuiNewPath,
    string? GuiPath,
    bool HasInvalidFiles,
    string Message)
{
    public bool HasUsableLegacyFile => !string.IsNullOrWhiteSpace(GuiNewPath) || !string.IsNullOrWhiteSpace(GuiPath);

    public ImportSource LegacyImportSource => (!string.IsNullOrWhiteSpace(GuiNewPath), !string.IsNullOrWhiteSpace(GuiPath)) switch
    {
        (true, true) => ImportSource.Auto,
        (true, false) => ImportSource.GuiNewOnly,
        (false, true) => ImportSource.GuiOnly,
        _ => ImportSource.Auto,
    };

    public static ConfigurationImportSelectionAnalysis Unified(string filePath)
        => new(
            ConfigurationImportSelectionKind.UnifiedConfig,
            filePath,
            null,
            null,
            false,
            string.Empty);

    public static ConfigurationImportSelectionAnalysis Legacy(string? guiNewPath, string? guiPath, bool hasInvalidFiles)
    {
        var missingParts = new List<string>();
        if (string.IsNullOrWhiteSpace(guiNewPath))
        {
            missingParts.Add("gui.new.json");
        }

        if (string.IsNullOrWhiteSpace(guiPath))
        {
            missingParts.Add("gui.json");
        }

        var message = missingParts.Count > 0
            ? $"请同时选择 {string.Join(" 和 ", missingParts)}。"
            : string.Empty;
        if (hasInvalidFiles)
        {
            message = string.IsNullOrWhiteSpace(message)
                ? "选择中存在文件名不正确的旧配置文件。"
                : $"{message} 另外存在文件名不正确的旧配置文件。";
        }

        return new ConfigurationImportSelectionAnalysis(
            missingParts.Count == 0 && !hasInvalidFiles
                ? ConfigurationImportSelectionKind.LegacyReady
                : ConfigurationImportSelectionKind.LegacyPartial,
            null,
            guiNewPath,
            guiPath,
            hasInvalidFiles,
            message);
    }

    public static ConfigurationImportSelectionAnalysis Invalid(string message)
        => new(
            ConfigurationImportSelectionKind.Invalid,
            null,
            null,
            null,
            false,
            message);
}

internal enum ConfigurationImportSelectionKind
{
    UnifiedConfig = 0,
    LegacyReady = 1,
    LegacyPartial = 2,
    Invalid = 3,
}

internal enum ConfigurationImportJsonShape
{
    Unknown = 0,
    UnifiedConfig = 1,
    LegacyConfig = 2,
}
