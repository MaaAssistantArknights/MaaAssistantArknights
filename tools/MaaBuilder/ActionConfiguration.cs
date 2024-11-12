using Nuke.Common.Tooling;
using System.ComponentModel;

namespace MaaBuilder;

[TypeConverter(typeof(TypeConverter<ActionConfiguration>))]
public class ActionConfiguration : Enumeration
{
    public static readonly ActionConfiguration DevBuild = new() { Value = "dev-build-win" };
    public static readonly ActionConfiguration ReleaseMaa = new() { Value = "release-maa-win" };

    public static implicit operator string(ActionConfiguration configuration)
    {
        return configuration.Value;
    }

    public static explicit operator ActionConfiguration(string value) => value switch
    {
        "dev-build-win" => DevBuild,
        "release-maa-win" => ReleaseMaa,
        _ => null
    };
}
