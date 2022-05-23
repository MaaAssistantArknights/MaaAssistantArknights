using Nuke.Common.Tooling;
using System.ComponentModel;

namespace MaaBuilder;

[TypeConverter(typeof(TypeConverter<ActionConfiguration>))]
public class ActionConfiguration : Enumeration
{
    /// <summary>
    /// 开发构建测试
    /// <para>触发条件</para>
    /// <list type="bullet">
    ///     <item>Push 或 PR 至 `dev` 分支</item>
    ///     <item>Maa 相关代码有更改</item>
    ///     <item>可手动触发</item>
    /// </list>
    /// <para>运行内容 (在 MaaElectronUI 发布后将会进行修改)</para>
    /// <list type="bullet">
    ///     <item>将会发布 Artifact</item>
    ///     <item>将会打包 MaaBundle</item>
    ///     <item>生成的包文件名为 `MaaBundle-Dev-{VERSION}.zip`</item>
    ///     <item>版本号为 `{yyyy-MM-dd-HH-mm-ss}-{commitHash[..7]}`</item>
    ///     <item>MaaCore 使用 `RelWithDebInfo` 配置</item>
    ///     <item>MaaWpf 使用 `RelWithDebInfo` 配置</item>
    /// </list>
    /// </summary>
    public static readonly ActionConfiguration DevBuild = new() { Value = "dev-build" };

    /// <summary>
    /// 发布 MeoAssistantArknights (在 MaaElectronUI 发布后，此 Action 将被移除，由 <see cref="ReleaseMaaCore"/> 代替)
    /// <para>触发条件</para>
    /// <list type="bullet">
    ///     <item>创建了 Tag</item>
    ///     <item>Tag 匹配 `v*` 规则</item>
    /// </list>
    /// <para>运行内容</para>
    /// <list type="bullet">
    ///     <item>将会发布 Artifact</item>
    ///     <item>将会打包 MeoAssistantArknights 和 MaaCore</item>
    ///     <item>生成的包文件名为 `MeoAssistantArknights_{VERSION}.zip` 和 `MaaCore-{VERSION}.zip`</item>
    ///     <item>GitHub Actions 中运行时，将会发布 Release 至 `MaaAssistantArknights/MaaAssistantArknights`</item>
    ///     <item>GitHub Actions 中运行时，`ref` 不匹配 `ref/tags/v*` 时将会抛错</item>
    ///     <item>GitHub Actions 中运行时，版本号为 `{Tag}`</item>
    ///     <item>
    ///         其他环境中运行时，版本号为
    ///         <list type="number">
    ///             <item>当前 HEAD 存在一个 Tag，则为 `{Tag}-Local`</item>
    ///             <item>当前 HEAD 不存在 Tag，则为 `v.{CommitHash[..7]}-Local`</item>
    ///         </list>
    ///     </item>
    ///     <item>MaaCore 使用 `Release` 配置，单独的包中使用 `CICD` 配置</item>
    ///     <item>MaaWpf 使用 `Release` 配置</item>
    ///     <item>将会移除 Debug 用文件</item>
    /// </list>
    /// </summary>
    public static readonly ActionConfiguration ReleaseMaa = new() { Value = "release-maa" };

    /// <summary>
    /// 发布 MaaCore (在 MaaElectronUI 发布前，此 Action 不执行)
    /// <para>触发条件</para>
    /// <list type="bullet">
    ///     <item>创建了 Tag</item>
    ///     <item>Tag 匹配 `v*` 规则</item>
    /// </list>
    /// <para>运行内容</para>
    /// <list type="bullet">
    ///     <item>将会发布 Artifact</item>
    ///     <item>将会打包 MaaCore</item>
    ///     <item>生成的包文件名为 `MaaCore-{VERSION}.zip`</item>
    ///     <item>GitHub Actions 中运行时，将会发布 Release 至 `MaaAssistantArknights/MaaAssistantArknights`</item>
    ///     <item>GitHub Actions 中运行时，`ref` 不匹配 `ref/tags/v*` 时将会抛错</item>
    ///     <item>GitHub Actions 中运行时，版本号为 `{Tag}`</item>
    ///     <item>
    ///         其他环境中运行时，版本号为
    ///         <list type="number">
    ///             <item>当前 HEAD 存在一个 Tag，则为 `{Tag}-Local`</item>
    ///             <item>当前 HEAD 不存在 Tag，则为 `v.{CommitHash[..7]}-Local`</item>
    ///         </list>
    ///     </item>
    ///     <item>MaaCore 使用 `CICD` 配置</item>
    ///     <item>将会移除 Debug 用文件</item>
    /// </list>
    /// </summary>    
    public static readonly ActionConfiguration ReleaseMaaCore = new() { Value = "release-maa-core" };

    /// <summary>
    /// 发布 MaaResource
    /// <para>触发条件</para>
    /// <list type="bullet">
    ///     <item>资源文件夹有更改</item>
    ///     <item>`master` 分支</item>
    /// </list>
    /// <para>运行内容</para>
    /// <list type="bullet">
    ///     <item>将会发布 Artifact</item>
    ///     <item>将会打包 MaaResource</item>
    ///     <item>生成的包文件名为 `MaaResource-{VERSION}.zip`</item>
    ///     <item>GitHub Actions 中运行时，将会发布 Release 至 `MaaAssistantArknights/MaaResourceRelease`</item>
    ///     <item>版本号为 `{yyyy-MM-dd-HH-mm-ss}-{commitHash[..7]}`</item>
    /// </list>
    /// </summary>
    public static readonly ActionConfiguration ReleaseMaaResource = new() { Value = "release-maa-resource" };

    public static implicit operator string(ActionConfiguration configuration)
    {
        return configuration.Value;
    }

    public static explicit operator ActionConfiguration(string value) => value switch
    {
        "dev-build" => DevBuild,
        "release-maa" => ReleaseMaa,
        "release-maa-core" => ReleaseMaaCore,
        "release-maa-resource" => ReleaseMaaResource,
        _ => null
    };
}
