using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace MaaBuilder.Models;

public record PackageMaaDependencyNoAvx : IPackageConfiguration
{
    [JsonPropertyName("include")]
    public List<string> Include { get; set; }
    
    [JsonPropertyName("no_avx_bundle")]
    public string NoAvxBundle { get; set; }
}
