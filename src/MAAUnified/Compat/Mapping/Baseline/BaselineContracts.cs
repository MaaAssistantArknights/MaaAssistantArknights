using System.Text.Json.Serialization;

namespace MAAUnified.Compat.Mapping.Baseline;

public static class BaselineConstants
{
    public const string PriorityP0 = "P0";

    public static readonly string[] AllowedParityStatus =
    [
        "Aligned",
        "Gap",
        "Waived",
    ];
}

public sealed record BaselineFreeze
{
    [JsonPropertyName("schema_version")]
    public string SchemaVersion { get; init; } = string.Empty;

    [JsonPropertyName("frozen_at_utc")]
    public string FrozenAtUtc { get; init; } = string.Empty;

    [JsonPropertyName("wpf_baseline_commit")]
    public string WpfBaselineCommit { get; init; } = string.Empty;

    [JsonPropertyName("scope")]
    public string Scope { get; init; } = string.Empty;

    [JsonPropertyName("matrix_mode")]
    public string MatrixMode { get; init; } = string.Empty;

    [JsonPropertyName("priority_policy")]
    public string PriorityPolicy { get; init; } = string.Empty;

    [JsonPropertyName("themes")]
    public List<string> Themes { get; init; } = [];

    [JsonPropertyName("locales")]
    public List<string> Locales { get; init; } = [];

    [JsonPropertyName("items")]
    public List<FeatureBaselineItem> Items { get; init; } = [];

    [JsonPropertyName("config_key_mappings")]
    public List<ConfigKeyMappingSpec> ConfigKeyMappings { get; init; } = [];

    [JsonPropertyName("fallback_capabilities")]
    public List<FallbackCapabilitySpec> FallbackCapabilities { get; init; } = [];

    [JsonPropertyName("metadata")]
    public BaselineMetadata Metadata { get; init; } = new();
}

public sealed record BaselineMetadata
{
    [JsonPropertyName("feature_item_count")]
    public int FeatureItemCount { get; init; }

    [JsonPropertyName("system_item_count")]
    public int SystemItemCount { get; init; }

    [JsonPropertyName("config_key_count")]
    public int ConfigKeyCount { get; init; }

    [JsonPropertyName("fallback_record_count")]
    public int FallbackRecordCount { get; init; }
}

public sealed record FeatureBaselineItem
{
    [JsonPropertyName("item_id")]
    public string ItemId { get; init; } = string.Empty;

    [JsonPropertyName("kind")]
    public string Kind { get; init; } = string.Empty;

    [JsonPropertyName("group")]
    public string Group { get; init; } = string.Empty;

    [JsonPropertyName("display_name")]
    public string DisplayName { get; init; } = string.Empty;

    [JsonPropertyName("wpf_reference")]
    public string WpfReference { get; init; } = string.Empty;

    [JsonPropertyName("avalonia_path")]
    public string AvaloniaPath { get; init; } = string.Empty;

    [JsonPropertyName("parity_status")]
    public string ParityStatus { get; init; } = string.Empty;

    [JsonPropertyName("priority")]
    public string Priority { get; init; } = string.Empty;

    [JsonPropertyName("fields")]
    public List<FieldSpec> Fields { get; init; } = [];

    [JsonPropertyName("interactions")]
    public List<InteractionSpec> Interactions { get; init; } = [];

    [JsonPropertyName("visibility_rules")]
    public List<VisibilityRule> VisibilityRules { get; init; } = [];

    [JsonPropertyName("error_feedback")]
    public List<ErrorFeedbackSpec> ErrorFeedback { get; init; } = [];

    [JsonPropertyName("evidence")]
    public EvidenceSpec Evidence { get; init; } = new();

    [JsonPropertyName("waiver")]
    public WaiverSpec? Waiver { get; init; }

    [JsonPropertyName("waiver_scope")]
    public WaiverScope? WaiverScope { get; init; }
}

public sealed record FieldSpec
{
    [JsonPropertyName("field_id")]
    public string FieldId { get; init; } = string.Empty;

    [JsonPropertyName("name")]
    public string Name { get; init; } = string.Empty;

    [JsonPropertyName("default_value")]
    public string DefaultValue { get; init; } = string.Empty;

    [JsonPropertyName("source")]
    public string Source { get; init; } = string.Empty;

    [JsonPropertyName("notes")]
    public string Notes { get; init; } = string.Empty;
}

public sealed record InteractionSpec
{
    [JsonPropertyName("interaction_id")]
    public string InteractionId { get; init; } = string.Empty;

    [JsonPropertyName("action")]
    public string Action { get; init; } = string.Empty;

    [JsonPropertyName("expected_result")]
    public string ExpectedResult { get; init; } = string.Empty;

    [JsonPropertyName("fallback_behavior")]
    public string FallbackBehavior { get; init; } = string.Empty;
}

public sealed record VisibilityRule
{
    [JsonPropertyName("rule_id")]
    public string RuleId { get; init; } = string.Empty;

    [JsonPropertyName("condition")]
    public string Condition { get; init; } = string.Empty;

    [JsonPropertyName("expected_visibility")]
    public string ExpectedVisibility { get; init; } = string.Empty;
}

public sealed record ErrorFeedbackSpec
{
    [JsonPropertyName("code")]
    public string Code { get; init; } = string.Empty;

    [JsonPropertyName("trigger")]
    public string Trigger { get; init; } = string.Empty;

    [JsonPropertyName("user_message")]
    public string UserMessage { get; init; } = string.Empty;

    [JsonPropertyName("log_path")]
    public string LogPath { get; init; } = string.Empty;

    [JsonPropertyName("non_crash")]
    public bool NonCrash { get; init; }
}

public sealed record ConfigKeyMappingSpec
{
    [JsonPropertyName("key")]
    public string Key { get; init; } = string.Empty;

    [JsonPropertyName("owner_item_id")]
    public string OwnerItemId { get; init; } = string.Empty;

    [JsonPropertyName("parity_status")]
    public string ParityStatus { get; init; } = string.Empty;

    [JsonPropertyName("priority")]
    public string Priority { get; init; } = string.Empty;

    [JsonPropertyName("mapping_target")]
    public string? MappingTarget { get; init; }

    [JsonPropertyName("notes")]
    public string Notes { get; init; } = string.Empty;

    [JsonPropertyName("evidence")]
    public EvidenceSpec Evidence { get; init; } = new();

    [JsonPropertyName("waiver")]
    public WaiverSpec? Waiver { get; init; }
}

public sealed record FallbackCapabilitySpec
{
    [JsonPropertyName("capability_id")]
    public string CapabilityId { get; init; } = string.Empty;

    [JsonPropertyName("platform")]
    public string Platform { get; init; } = string.Empty;

    [JsonPropertyName("expected_mode")]
    public string ExpectedMode { get; init; } = string.Empty;

    [JsonPropertyName("current_mode")]
    public string CurrentMode { get; init; } = string.Empty;

    [JsonPropertyName("parity_status")]
    public string ParityStatus { get; init; } = string.Empty;

    [JsonPropertyName("priority")]
    public string Priority { get; init; } = string.Empty;

    [JsonPropertyName("visible")]
    public bool Visible { get; init; }

    [JsonPropertyName("recorded")]
    public bool Recorded { get; init; }

    [JsonPropertyName("locatable")]
    public bool Locatable { get; init; }

    [JsonPropertyName("message")]
    public string Message { get; init; } = string.Empty;

    [JsonPropertyName("evidence")]
    public EvidenceSpec Evidence { get; init; } = new();

    [JsonPropertyName("waiver")]
    public WaiverSpec? Waiver { get; init; }

    [JsonPropertyName("waiver_scope")]
    public WaiverScope? WaiverScope { get; init; }
}

public sealed record EvidenceSpec
{
    [JsonPropertyName("ui_path")]
    public string UiPath { get; init; } = string.Empty;

    [JsonPropertyName("log_path")]
    public string LogPath { get; init; } = string.Empty;

    [JsonPropertyName("scope")]
    public string Scope { get; init; } = string.Empty;

    [JsonPropertyName("case_id")]
    public string CaseId { get; init; } = string.Empty;
}

public sealed record WaiverSpec
{
    [JsonPropertyName("owner")]
    public string Owner { get; init; } = string.Empty;

    [JsonPropertyName("reason")]
    public string Reason { get; init; } = string.Empty;

    [JsonPropertyName("expires_on")]
    public string ExpiresOn { get; init; } = string.Empty;

    [JsonPropertyName("alternative_validation")]
    public string AlternativeValidation { get; init; } = string.Empty;
}

public sealed record WaiverScope
{
    [JsonPropertyName("platforms")]
    public List<string> Platforms { get; init; } = [];

    [JsonPropertyName("themes")]
    public List<string> Themes { get; init; } = [];

    [JsonPropertyName("locales")]
    public List<string> Locales { get; init; } = [];
}

public sealed record AcceptanceTemplate
{
    [JsonPropertyName("schema_version")]
    public string SchemaVersion { get; init; } = string.Empty;

    [JsonPropertyName("baseline_ref")]
    public string BaselineRef { get; init; } = string.Empty;

    [JsonPropertyName("matrix")]
    public AcceptanceMatrix Matrix { get; init; } = new();

    [JsonPropertyName("global_requirements")]
    public List<string> GlobalRequirements { get; init; } = [];

    [JsonPropertyName("waiver_policy")]
    public WaiverPolicy WaiverPolicy { get; init; } = new();

    [JsonPropertyName("cases")]
    public List<AcceptanceCaseSpec> Cases { get; init; } = [];
}

public sealed record AcceptanceMatrix
{
    [JsonPropertyName("strategy")]
    public string Strategy { get; init; } = string.Empty;

    [JsonPropertyName("tier_1")]
    public MatrixTier Tier1 { get; init; } = new();

    [JsonPropertyName("tier_2")]
    public MatrixTier Tier2 { get; init; } = new();
}

public sealed record MatrixTier
{
    [JsonPropertyName("scope")]
    public string Scope { get; init; } = string.Empty;

    [JsonPropertyName("platforms")]
    public List<string> Platforms { get; init; } = [];

    [JsonPropertyName("themes")]
    public List<string> Themes { get; init; } = [];

    [JsonPropertyName("locales")]
    public List<string> Locales { get; init; } = [];

    [JsonPropertyName("extra_rule")]
    public string? ExtraRule { get; init; }
}

public sealed record WaiverPolicy
{
    [JsonPropertyName("allow_waiver")]
    public bool AllowWaiver { get; init; }

    [JsonPropertyName("required_fields")]
    public List<string> RequiredFields { get; init; } = [];

    [JsonPropertyName("rule")]
    public string Rule { get; init; } = string.Empty;
}

public sealed record AcceptanceCaseSpec
{
    [JsonPropertyName("case_id")]
    public string CaseId { get; init; } = string.Empty;

    [JsonPropertyName("tier")]
    public string Tier { get; init; } = string.Empty;

    [JsonPropertyName("item_id")]
    public string ItemId { get; init; } = string.Empty;

    [JsonPropertyName("platforms")]
    public List<string> Platforms { get; init; } = [];

    [JsonPropertyName("themes")]
    public List<string> Themes { get; init; } = [];

    [JsonPropertyName("locales")]
    public List<string> Locales { get; init; } = [];

    [JsonPropertyName("waiver")]
    public WaiverSpec? Waiver { get; init; }

    [JsonPropertyName("waiver_scope")]
    public WaiverScope? WaiverScope { get; init; }

    [JsonPropertyName("steps")]
    public List<string> Steps { get; init; } = [];

    [JsonPropertyName("expected")]
    public List<string> Expected { get; init; } = [];

    [JsonPropertyName("non_crash_required")]
    public bool NonCrashRequired { get; init; }

    [JsonPropertyName("requires_fallback_trace")]
    public bool RequiresFallbackTrace { get; init; }
}
