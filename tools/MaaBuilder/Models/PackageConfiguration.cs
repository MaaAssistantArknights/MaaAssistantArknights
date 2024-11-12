using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace MaaBuilder.Models;

public record PackageConfiguration
{
    [JsonPropertyName("include")]
    public IEnumerable<string> Include { get; init; } = new List<string>();
    
    [JsonPropertyName("exclude")]
    public IEnumerable<string> Exclude { get; init; } = new List<string>();

    [JsonPropertyName("no_avx_bundle")]
    public string NoAvxBundle { get; init; } = string.Empty;
}
