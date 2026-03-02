using System.Text.Json;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

public sealed class GuiJsonConfigImporter : IConfigImporter
{
    public string Name => "gui.json";

    public bool CanImport(LegacyConfigSnapshot snapshot) => snapshot.GuiExists;

    public async Task ImportAsync(
        LegacyConfigSnapshot snapshot,
        UnifiedConfig target,
        ImportReport report,
        bool fillMissingOnly,
        CancellationToken cancellationToken = default)
    {
        if (!snapshot.GuiExists)
        {
            report.DefaultFallbackCount += 1;
            report.Warnings.Add("gui.json not found, skipped");
            return;
        }

        try
        {
            await using var stream = File.OpenRead(snapshot.GuiPath);
            using var doc = await JsonDocument.ParseAsync(stream, cancellationToken: cancellationToken);
            var root = doc.RootElement;

            if (root.TryGetProperty("Current", out var currentProp) && currentProp.ValueKind == JsonValueKind.String)
            {
                if (fillMissingOnly)
                {
                    if (string.Equals(target.CurrentProfile, "Default", StringComparison.OrdinalIgnoreCase))
                    {
                        target.CurrentProfile = currentProp.GetString() ?? target.CurrentProfile;
                        report.MappedFieldCount += 1;
                    }
                }
                else
                {
                    target.CurrentProfile = currentProp.GetString() ?? target.CurrentProfile;
                    report.MappedFieldCount += 1;
                }
            }

            if (root.TryGetProperty("Configurations", out var configsProp) && configsProp.ValueKind == JsonValueKind.Object)
            {
                foreach (var configProp in configsProp.EnumerateObject())
                {
                    if (!target.Profiles.TryGetValue(configProp.Name, out var profile))
                    {
                        profile = new UnifiedProfile();
                        target.Profiles[configProp.Name] = profile;
                    }

                    if (configProp.Value.ValueKind != JsonValueKind.Object)
                    {
                        continue;
                    }

                    foreach (var valueProp in configProp.Value.EnumerateObject())
                    {
                        JsonImportMergeHelper.MergeProfileValue(
                            profile,
                            valueProp.Name,
                            JsonImportMergeHelper.ToJsonNode(valueProp.Value),
                            fillMissingOnly,
                            report);
                    }
                }
            }

            if (root.TryGetProperty("Global", out var globalProp) && globalProp.ValueKind == JsonValueKind.Object)
            {
                foreach (var valueProp in globalProp.EnumerateObject())
                {
                    JsonImportMergeHelper.MergeGlobalValue(
                        target,
                        valueProp.Name,
                        JsonImportMergeHelper.ToJsonNode(valueProp.Value),
                        fillMissingOnly,
                        report);
                }
            }

            report.ImportedGui = true;
        }
        catch (Exception ex)
        {
            report.Errors.Add($"Failed to import gui.json: {ex.Message}");
        }
    }
}
