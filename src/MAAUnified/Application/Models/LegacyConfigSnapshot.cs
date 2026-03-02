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

        return new LegacyConfigSnapshot {
            GuiNewPath = guiNew,
            GuiPath = gui,
            GuiNewExists = File.Exists(guiNew),
            GuiExists = File.Exists(gui),
        };
    }
}
