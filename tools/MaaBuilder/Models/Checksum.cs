using System.Text.Json.Serialization;

namespace MaaBuilder.Models;

public record Checksum
{
    [JsonPropertyName("file_name")]
    public string FileName { get; set; }
    
    [JsonPropertyName("package_type")]
    public string PackageType { get; set; }
    
    [JsonPropertyName("file_hash")]
    public string FileHash { get; set; }
    
    [JsonPropertyName("file_identity")]
    public string FileIdentity { get; set; }
}
