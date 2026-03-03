using System.Reflection;
using System.Text;
using System.Text.Json;
using MAAUnified.App.ViewModels;
using MAAUnified.Compat.Constants;
using MAAUnified.Compat.Mapping.Baseline;

namespace MAAUnified.Tests;

public sealed class BaselineContractTests
{
    [Fact]
    public void BaselineContract_ShouldDeserializeAndValidateSchema()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var acceptance = BaselineTestSupport.LoadAcceptanceTemplate();

        Assert.Equal("1.0.0", baseline.SchemaVersion);
        Assert.Matches("^[0-9a-f]{40}$", baseline.WpfBaselineCommit);
        Assert.Equal("src/MAAUnified/**", baseline.Scope);
        Assert.Equal("tiered", baseline.MatrixMode);

        Assert.Equal("1.0.0", acceptance.SchemaVersion);
        Assert.Equal("tiered", acceptance.Matrix.Strategy);
        Assert.NotEmpty(acceptance.Cases);

        Assert.All(baseline.Items, item =>
        {
            Assert.False(string.IsNullOrWhiteSpace(item.ItemId));
            Assert.False(string.IsNullOrWhiteSpace(item.Kind));
            Assert.Contains(item.ParityStatus, BaselineConstants.AllowedParityStatus);
            Assert.Equal(BaselineConstants.PriorityP0, item.Priority);

            Assert.False(string.IsNullOrWhiteSpace(item.Evidence.UiPath));
            Assert.False(string.IsNullOrWhiteSpace(item.Evidence.LogPath));
            Assert.False(string.IsNullOrWhiteSpace(item.Evidence.Scope));
            Assert.False(string.IsNullOrWhiteSpace(item.Evidence.CaseId));
        });

        Assert.All(baseline.ConfigKeyMappings, mapping =>
        {
            Assert.False(string.IsNullOrWhiteSpace(mapping.Key));
            Assert.False(string.IsNullOrWhiteSpace(mapping.OwnerItemId));
            Assert.Contains(mapping.ParityStatus, BaselineConstants.AllowedParityStatus);
            Assert.Equal(BaselineConstants.PriorityP0, mapping.Priority);

            Assert.False(string.IsNullOrWhiteSpace(mapping.Evidence.UiPath));
            Assert.False(string.IsNullOrWhiteSpace(mapping.Evidence.LogPath));
            Assert.False(string.IsNullOrWhiteSpace(mapping.Evidence.Scope));
            Assert.False(string.IsNullOrWhiteSpace(mapping.Evidence.CaseId));
        });

        Assert.All(baseline.FallbackCapabilities, capability =>
        {
            Assert.False(string.IsNullOrWhiteSpace(capability.CapabilityId));
            Assert.False(string.IsNullOrWhiteSpace(capability.Platform));
            Assert.Contains(capability.ParityStatus, BaselineConstants.AllowedParityStatus);
            Assert.Equal(BaselineConstants.PriorityP0, capability.Priority);

            Assert.True(capability.Visible);
            Assert.True(capability.Recorded);
            Assert.True(capability.Locatable);

            Assert.False(string.IsNullOrWhiteSpace(capability.Evidence.UiPath));
            Assert.False(string.IsNullOrWhiteSpace(capability.Evidence.LogPath));
            Assert.False(string.IsNullOrWhiteSpace(capability.Evidence.Scope));
            Assert.False(string.IsNullOrWhiteSpace(capability.Evidence.CaseId));
        });

        Assert.Equal(baseline.Items.Count(i => i.Kind == "Feature"), baseline.Metadata.FeatureItemCount);
        Assert.Equal(baseline.Items.Count(i => i.Kind == "System"), baseline.Metadata.SystemItemCount);
        Assert.Equal(baseline.ConfigKeyMappings.Count, baseline.Metadata.ConfigKeyCount);
        Assert.Equal(baseline.FallbackCapabilities.Count, baseline.Metadata.FallbackRecordCount);
    }

    [Fact]
    public void BaselineP0Policy_ShouldContainNoUnapprovedDefers()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        Assert.All(baseline.Items, item => Assert.Equal(BaselineConstants.PriorityP0, item.Priority));
        Assert.All(baseline.ConfigKeyMappings, mapping => Assert.Equal(BaselineConstants.PriorityP0, mapping.Priority));
        Assert.All(baseline.FallbackCapabilities, capability => Assert.Equal(BaselineConstants.PriorityP0, capability.Priority));

        var waivedMappings = baseline.ConfigKeyMappings.Where(m => m.ParityStatus == "Waived").ToList();

        foreach (var waived in waivedMappings)
        {
            Assert.NotNull(waived.Waiver);
            Assert.False(string.IsNullOrWhiteSpace(waived.Waiver!.Owner));
            Assert.False(string.IsNullOrWhiteSpace(waived.Waiver.Reason));
            Assert.False(string.IsNullOrWhiteSpace(waived.Waiver.AlternativeValidation));
            Assert.True(DateOnly.TryParse(waived.Waiver.ExpiresOn, out _));
        }
    }
}

internal static class BaselineTestSupport
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = false,
        ReadCommentHandling = JsonCommentHandling.Skip,
    };

    public static BaselineFreeze LoadBaseline()
    {
        var path = Path.Combine(GetMaaUnifiedRoot(), "Compat", "Mapping", "Baseline", "baseline.freeze.v1.json");
        var json = File.ReadAllText(path);
        var baseline = JsonSerializer.Deserialize<BaselineFreeze>(json, JsonOptions);
        Assert.NotNull(baseline);
        return baseline!;
    }

    public static AcceptanceTemplate LoadAcceptanceTemplate()
    {
        var path = Path.Combine(GetMaaUnifiedRoot(), "Compat", "Mapping", "Baseline", "acceptance.template.v1.json");
        var json = File.ReadAllText(path);
        var template = JsonSerializer.Deserialize<AcceptanceTemplate>(json, JsonOptions);
        Assert.NotNull(template);
        return template!;
    }

    public static string ReadDoc(string relativeDocPath)
    {
        var path = Path.Combine(GetMaaUnifiedRoot(), "Docs", relativeDocPath);
        return NormalizeLineEndings(File.ReadAllText(path));
    }

    public static string NormalizeLineEndings(string text)
    {
        return text.Replace("\r\n", "\n").Replace('\r', '\n');
    }

    public static string GetMaaUnifiedRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            var appDir = Path.Combine(current.FullName, "App");
            var testsDir = Path.Combine(current.FullName, "Tests");
            if (Directory.Exists(appDir) && Directory.Exists(testsDir))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Cannot locate src/MAAUnified root from test runtime path.");
    }

    public static IReadOnlyList<string> GetLegacyConfigurationKeys()
    {
        return typeof(ConfigurationKeys)
            .GetFields(BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly)
            .Where(f => f.IsLiteral && !f.IsInitOnly && f.FieldType == typeof(string))
            .Select(f => (string)f.GetRawConstantValue()!)
            .ToList();
    }

    public static string RenderBaselineMarkdown(BaselineFreeze baseline)
    {
        var featureItems = baseline.Items.Where(i => i.Kind == "Feature").ToList();
        var systemItems = baseline.Items.Where(i => i.Kind == "System").ToList();

        var featureAligned = featureItems.Count(i => i.ParityStatus == "Aligned");
        var featureGap = featureItems.Count(i => i.ParityStatus == "Gap");
        var cfgAligned = baseline.ConfigKeyMappings.Count(i => i.ParityStatus == "Aligned");
        var cfgGap = baseline.ConfigKeyMappings.Count(i => i.ParityStatus == "Gap");

        var alignedMappings = baseline.ConfigKeyMappings
            .Where(m => m.ParityStatus == "Aligned" && !string.IsNullOrWhiteSpace(m.MappingTarget))
            .OrderBy(m => m.Key, StringComparer.Ordinal)
            .ToList();

        var sb = new StringBuilder();
        sb.AppendLine("# MAAUnified 基线冻结 v1");
        sb.AppendLine();
        sb.AppendLine("## 摘要");
        sb.AppendLine($"- Frozen at (UTC): `{baseline.FrozenAtUtc}`");
        sb.AppendLine($"- WPF baseline commit: `{baseline.WpfBaselineCommit}`");
        sb.AppendLine($"- Scope: `{baseline.Scope}`");
        sb.AppendLine($"- Matrix mode: `{baseline.MatrixMode}`");
        sb.AppendLine($"- Themes: `{string.Join(", ", baseline.Themes)}`");
        sb.AppendLine($"- Locales: `{string.Join(", ", baseline.Locales)}`");
        sb.AppendLine($"- Feature items: `{featureItems.Count}`");
        sb.AppendLine($"- System items: `{systemItems.Count}`");
        sb.AppendLine($"- Config keys: `{baseline.ConfigKeyMappings.Count}`");
        sb.AppendLine($"- Fallback records: `{baseline.FallbackCapabilities.Count}`");
        sb.AppendLine();
        sb.AppendLine("## Feature Parity");
        sb.AppendLine($"- Aligned: `{featureAligned}`");
        sb.AppendLine($"- Gap: `{featureGap}`");
        sb.AppendLine();
        sb.AppendLine("| Item ID | Group | Parity | Avalonia Path |");
        sb.AppendLine("| --- | --- | --- | --- |");
        foreach (var item in featureItems)
        {
            sb.AppendLine($"| `{item.ItemId}` | {item.Group} | {item.ParityStatus} | `{item.AvaloniaPath}` |");
        }

        sb.AppendLine();
        sb.AppendLine("## System Entry Parity");
        sb.AppendLine("| Item ID | Parity | Avalonia Path |");
        sb.AppendLine("| --- | --- | --- |");
        foreach (var item in systemItems)
        {
            sb.AppendLine($"| `{item.ItemId}` | {item.ParityStatus} | `{item.AvaloniaPath}` |");
        }

        sb.AppendLine();
        sb.AppendLine("## Config Key Mapping Summary");
        sb.AppendLine($"- Aligned: `{cfgAligned}`");
        sb.AppendLine($"- Gap: `{cfgGap}`");
        sb.AppendLine("- Waived: `0`");
        sb.AppendLine();
        sb.AppendLine("### Aligned Config Keys");
        foreach (var mapping in alignedMappings)
        {
            sb.AppendLine($"- `{mapping.Key}` -> `{mapping.MappingTarget}`");
        }

        sb.AppendLine();
        sb.AppendLine("## Platform Fallback Records");
        sb.AppendLine("| Capability | Platform | Expected | Current | Parity | Visible | Recorded | Locatable |");
        sb.AppendLine("| --- | --- | --- | --- | --- | --- | --- | --- |");
        foreach (var row in baseline.FallbackCapabilities)
        {
            sb.AppendLine($"| {row.CapabilityId} | {row.Platform} | {row.ExpectedMode} | {row.CurrentMode} | {row.ParityStatus} | {row.Visible} | {row.Recorded} | {row.Locatable} |");
        }

        sb.AppendLine();
        sb.AppendLine("## Notes");
        sb.AppendLine("- This file is generated from `baseline.freeze.v1.json` during Package A freeze.");
        sb.AppendLine("- Any baseline change must follow `baseline-change-control.v1.md`.");

        return sb.ToString();
    }

    public static string RenderAcceptanceMarkdown(AcceptanceTemplate acceptance)
    {
        var sb = new StringBuilder();

        sb.AppendLine("# MAAUnified 验收清单模板 v1");
        sb.AppendLine();
        sb.AppendLine("## 摘要");
        sb.AppendLine($"- Schema version: `{acceptance.SchemaVersion}`");
        sb.AppendLine($"- Baseline ref: `{acceptance.BaselineRef}`");
        sb.AppendLine($"- Case count: `{acceptance.Cases.Count}`");
        sb.AppendLine();
        sb.AppendLine("## Matrix Strategy");
        sb.AppendLine("- Tier-1: Root + Settings + System entries, full platform/theme/locale matrix.");
        sb.AppendLine("- Tier-2: Other feature pages, main path plus all-language text/error key validation.");
        sb.AppendLine();
        sb.AppendLine("| Tier | Platforms | Themes | Locales |");
        sb.AppendLine("| --- | --- | --- | --- |");
        sb.AppendLine($"| Tier-1 | {string.Join(", ", acceptance.Matrix.Tier1.Platforms)} | {string.Join(", ", acceptance.Matrix.Tier1.Themes)} | {string.Join(", ", acceptance.Matrix.Tier1.Locales)} |");
        sb.AppendLine($"| Tier-2 | {string.Join(", ", acceptance.Matrix.Tier2.Platforms)} | {string.Join(", ", acceptance.Matrix.Tier2.Themes)} | {string.Join(", ", acceptance.Matrix.Tier2.Locales)} |");
        sb.AppendLine();
        sb.AppendLine("## Global Requirements");
        foreach (var requirement in acceptance.GlobalRequirements)
        {
            sb.AppendLine($"- {requirement}");
        }

        sb.AppendLine();
        sb.AppendLine("## Acceptance Cases");
        sb.AppendLine("| Case ID | Tier | Item ID | Platforms | Themes | Locales |");
        sb.AppendLine("| --- | --- | --- | --- | --- | --- |");
        foreach (var testCase in acceptance.Cases)
        {
            sb.AppendLine($"| `{testCase.CaseId}` | {testCase.Tier} | `{testCase.ItemId}` | {string.Join(", ", testCase.Platforms)} | {string.Join(", ", testCase.Themes)} | {string.Join(", ", testCase.Locales)} |");
        }

        sb.AppendLine();
        sb.AppendLine("## Waiver Policy");
        sb.AppendLine($"- Allowed: `{acceptance.WaiverPolicy.AllowWaiver}`");
        sb.AppendLine($"- Required fields: `{string.Join(", ", acceptance.WaiverPolicy.RequiredFields)}`");
        sb.AppendLine($"- Rule: {acceptance.WaiverPolicy.Rule}");
        sb.AppendLine();
        sb.AppendLine("## Notes");
        sb.AppendLine("- This checklist file is generated from `acceptance.template.v1.json`.");

        return sb.ToString();
    }
}
