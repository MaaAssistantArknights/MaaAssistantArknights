using Nuke.Common.Tooling;
using System.ComponentModel;

namespace MaaBuilder;

[TypeConverter(typeof(TypeConverter<BuildConfiguration>))]
public class BuildConfiguration : Enumeration
{
    public static readonly BuildConfiguration Release = new() { Value = nameof(Release) };

    public static implicit operator string(BuildConfiguration configuration)
    {
        return configuration.Value;
    }
}
