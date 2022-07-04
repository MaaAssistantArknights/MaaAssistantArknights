using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace MaaBuilder.Models;

public record PackageMaaBundleOta : IPackageConfiguration
{
    [JsonPropertyName("exclude")]
    public List<string> Exclude { get; set; }
}
