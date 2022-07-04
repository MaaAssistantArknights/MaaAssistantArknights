using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace MaaBuilder.Models;

public record PackageMaaCore : IPackageConfiguration
{
    [JsonPropertyName("include")]
    public List<string> Include { get; set; }
}
