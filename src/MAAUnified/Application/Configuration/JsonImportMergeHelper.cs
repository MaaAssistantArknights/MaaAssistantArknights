using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

internal static class JsonImportMergeHelper
{
    public static JsonNode? ToJsonNode(JsonElement element)
    {
        return JsonNode.Parse(element.GetRawText());
    }

    public static void MergeGlobalValue(
        UnifiedConfig config,
        string key,
        JsonNode? value,
        bool fillMissingOnly,
        ImportReport report)
    {
        if (config.GlobalValues.TryGetValue(key, out var existing))
        {
            if (fillMissingOnly)
            {
                if (!JsonNode.DeepEquals(existing, value))
                {
                    report.ConflictCount += 1;
                }

                return;
            }

            if (!JsonNode.DeepEquals(existing, value))
            {
                report.ConflictCount += 1;
            }
        }

        config.GlobalValues[key] = value?.DeepClone();
        report.MappedFieldCount += 1;
    }

    public static void MergeProfileValue(
        UnifiedProfile profile,
        string key,
        JsonNode? value,
        bool fillMissingOnly,
        ImportReport report)
    {
        if (profile.Values.TryGetValue(key, out var existing))
        {
            if (fillMissingOnly)
            {
                if (!JsonNode.DeepEquals(existing, value))
                {
                    report.ConflictCount += 1;
                }

                return;
            }

            if (!JsonNode.DeepEquals(existing, value))
            {
                report.ConflictCount += 1;
            }
        }

        profile.Values[key] = value?.DeepClone();
        report.MappedFieldCount += 1;
    }
}
