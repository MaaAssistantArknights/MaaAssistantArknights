using Nuke.Common.Tooling;
using System.ComponentModel;

namespace MeoAssistantBuilder;

[TypeConverter(typeof(TypeConverter<BuildConfiguration>))]
public class BuildConfiguration : Enumeration
{
    /// <summary>
    /// MaaCore 在此配置下会自动复制资源文件，在直接发布 MaaBundle 时使用；
    /// MaaWpf 在此配置下开启了优化，在直接发布 MaaBundle 时使用；
    /// </summary>
    public static readonly BuildConfiguration Release = new() { Value = nameof(Release) };

    /// <summary>
    /// MaaCore 在此配置下会生成完整的 Debug 信息，在 DevBuild 中使用；
    /// MaaWpf 在此配置下关闭了优化，在 DevBuild 中使用；
    /// </summary>
    public static readonly BuildConfiguration RelWithDebInfo = new() { Value = nameof(RelWithDebInfo) };

    /// <summary>
    /// MaaCore 在此配置下不会自动复制资源文件，其余配置和 <see cref="Release"/> 相同，在发布 MaaCore 时使用
    /// MaaWpf 在此配置下与 <see cref="Release"/> 相同，不建议使用
    /// </summary>
    public static readonly BuildConfiguration CICD = new() { Value = nameof(CICD) };

    public static implicit operator string(BuildConfiguration configuration)
    {
        return configuration.Value;
    }
}
