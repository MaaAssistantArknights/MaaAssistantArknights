using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

public sealed class GuiNewJsonConfigImporter : IConfigImporter
{
    public string Name => "gui.new.json";

    public bool CanImport(LegacyConfigSnapshot snapshot) => snapshot.GuiNewExists;

    public async Task ImportAsync(
        LegacyConfigSnapshot snapshot,
        UnifiedConfig target,
        ImportReport report,
        bool fillMissingOnly,
        CancellationToken cancellationToken = default)
    {
        if (!snapshot.GuiNewExists)
        {
            report.DefaultFallbackCount += 1;
            report.Warnings.Add("gui.new.json not found, skipped");
            return;
        }

        try
        {
            await using var stream = File.OpenRead(snapshot.GuiNewPath);
            using var doc = await JsonDocument.ParseAsync(stream, cancellationToken: cancellationToken);
            var root = doc.RootElement;

            if (root.TryGetProperty("Current", out var currentProp) && currentProp.ValueKind == JsonValueKind.String)
            {
                target.CurrentProfile = currentProp.GetString() ?? target.CurrentProfile;
                report.MappedFieldCount += 1;
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

                    JsonElement? taskQueueProp = null;
                    foreach (var valueProp in configProp.Value.EnumerateObject())
                    {
                        if (string.Equals(valueProp.Name, "TaskQueue", StringComparison.OrdinalIgnoreCase)
                            && valueProp.Value.ValueKind == JsonValueKind.Array)
                        {
                            taskQueueProp = valueProp.Value;
                            continue;
                        }

                        JsonImportMergeHelper.MergeProfileValue(
                            profile,
                            valueProp.Name,
                            JsonImportMergeHelper.ToJsonNode(valueProp.Value),
                            fillMissingOnly,
                            report);
                    }

                    if (taskQueueProp is JsonElement queueElement)
                    {
                        MergeTaskQueue(profile, target, queueElement, fillMissingOnly, report);
                    }
                }
            }

            MergeObjectAsGlobal(root, "GUI", target, fillMissingOnly, report);
            MergeObjectAsGlobal(root, "VersionUpdate", target, fillMissingOnly, report);
            MergeObjectAsGlobal(root, "AnnouncementInfo", target, fillMissingOnly, report);
            MergeObjectAsGlobal(root, "Timers", target, fillMissingOnly, report);

            report.ImportedGuiNew = true;
        }
        catch (Exception ex)
        {
            report.Errors.Add($"Failed to import gui.new.json: {ex.Message}");
        }
    }

    private static void MergeObjectAsGlobal(
        JsonElement root,
        string objectName,
        UnifiedConfig target,
        bool fillMissingOnly,
        ImportReport report)
    {
        if (!root.TryGetProperty(objectName, out var obj) || obj.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        foreach (var prop in obj.EnumerateObject())
        {
            JsonImportMergeHelper.MergeGlobalValue(
                target,
                $"{objectName}.{prop.Name}",
                JsonImportMergeHelper.ToJsonNode(prop.Value),
                fillMissingOnly,
                report);
        }
    }

    private static void MergeTaskQueue(
        UnifiedProfile profile,
        UnifiedConfig config,
        JsonElement taskQueue,
        bool fillMissingOnly,
        ImportReport report)
    {
        if (fillMissingOnly && profile.TaskQueue.Count > 0)
        {
            report.ConflictCount += 1;
            return;
        }

        if (!fillMissingOnly)
        {
            profile.TaskQueue.Clear();
        }

        foreach (var task in taskQueue.EnumerateArray())
        {
            if (task.ValueKind != JsonValueKind.Object)
            {
                report.Warnings.Add("TaskQueue contains non-object entry and was skipped.");
                continue;
            }

            var taskNode = JsonImportMergeHelper.ToJsonNode(task) as JsonObject;
            if (taskNode is null)
            {
                report.Warnings.Add("TaskQueue entry could not be converted to JsonObject and was skipped.");
                continue;
            }

            if (!LegacyTaskSchemaConverter.TryConvertLegacyTask(taskNode, profile, config, out var convertedTask, out var error))
            {
                if (!string.IsNullOrWhiteSpace(error))
                {
                    report.Errors.Add(error);
                }
            }

            profile.TaskQueue.Add(convertedTask);
            report.MappedFieldCount += 1;
        }
    }
}
