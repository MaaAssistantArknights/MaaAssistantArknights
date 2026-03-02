using System.Text.Json.Nodes;
using System.Text.Json.Serialization;

namespace MAAUnified.Application.Models;

public sealed class UnifiedConfig
{
    public const int LatestSchemaVersion = 2;

    public int SchemaVersion { get; set; } = LatestSchemaVersion;

    public string CurrentProfile { get; set; } = "Default";

    public Dictionary<string, UnifiedProfile> Profiles { get; set; } = new(StringComparer.OrdinalIgnoreCase)
    {
        ["Default"] = new UnifiedProfile(),
    };

    public Dictionary<string, JsonNode?> GlobalValues { get; set; } = new(StringComparer.OrdinalIgnoreCase);

    public UnifiedMigrationMetadata Migration { get; set; } = new();
}

public sealed class UnifiedProfile
{
    public Dictionary<string, JsonNode?> Values { get; set; } = new(StringComparer.OrdinalIgnoreCase);

    public List<UnifiedTaskItem> TaskQueue { get; set; } = [];
}

public sealed class UnifiedTaskItem
{
    public string Type { get; set; } = "Unknown";

    public string Name { get; set; } = "UnnamedTask";

    public bool IsEnabled { get; set; } = true;

    public JsonObject Params { get; set; } = [];

    // Compatibility read path for schema v1 config.
    [JsonPropertyName("RawTask")]
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public JsonObject? LegacyRawTask { get; set; }
}

public sealed class UnifiedMigrationMetadata
{
    public DateTimeOffset ImportedAt { get; set; } = DateTimeOffset.UtcNow;

    public string ImportedBy { get; set; } = "MAAUnified";

    public bool ImportedFromGuiNew { get; set; }

    public bool ImportedFromGui { get; set; }

    public List<string> Warnings { get; set; } = [];
}
