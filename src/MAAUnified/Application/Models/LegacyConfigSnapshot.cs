namespace MAAUnified.Application.Models;

public sealed class LegacyConfigSnapshot
{
    public required string GuiNewPath { get; init; }

    public required string GuiPath { get; init; }

    public bool GuiNewExists { get; init; }

    public bool GuiExists { get; init; }

    public static LegacyConfigSnapshot FromBaseDirectory(string baseDirectory)
    {
        var configDir = Path.Combine(baseDirectory, "config");
        var guiNew = Path.Combine(configDir, "gui.new.json");
        var gui = Path.Combine(configDir, "gui.json");

        return FromPaths(guiNew, gui);
    }

    public static LegacyConfigSnapshot FromPaths(string? guiNewPath, string? guiPath)
    {
        var normalizedGuiNew = NormalizePath(guiNewPath);
        var normalizedGui = NormalizePath(guiPath);

        return new LegacyConfigSnapshot
        {
            GuiNewPath = normalizedGuiNew,
            GuiPath = normalizedGui,
            GuiNewExists = !string.IsNullOrWhiteSpace(normalizedGuiNew) && File.Exists(normalizedGuiNew),
            GuiExists = !string.IsNullOrWhiteSpace(normalizedGui) && File.Exists(normalizedGui),
        };
    }

    private static string NormalizePath(string? path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return string.Empty;
        }

        return Path.GetFullPath(path.Trim());
    }
}
