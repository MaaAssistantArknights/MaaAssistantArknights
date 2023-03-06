using Nuke.Common;
using Nuke.Common.Tooling;
using Nuke.Common.Tools.MSBuild;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

namespace MaaBuilder;

public partial class Build
{
    private Target WithCompileCoreRelease => _ => _
        .DependsOn(UseClean)
        .After(SetVersion)
        .Executes(() =>
        {
            var versionEnv = $"/DMAA_VERSION=\\\"{Version}\\\"";
            Information($"MaaCore 编译环境变量：ExternalCompilerOptions = {versionEnv}");
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaCoreProject)
                .SetTargets("ReBuild")
                .SetConfiguration(BuildConfiguration.Release)
                .SetTargetPlatform(Parameters.TargetPlatform)
                .SetProcessEnvironmentVariable("ExternalCompilerOptions", versionEnv)
            );
        });

    // TODO 在 MaaElectronUI 发布后移除
    private Target WithCompileWpfRelease => _ => _
        .DependsOn(UseClean)
        .After(SetVersion)
        .Executes(() =>
        {
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaWpfProject)
                .SetTargets("ReBuild")
                .SetConfiguration(BuildConfiguration.Release)
                .SetTargetPlatform(Parameters.TargetPlatform)
                .EnableRestore()
            );
        });

    private Target WithSyncRes => _ => _
    .DependsOn(UseClean)
    .After(SetVersion)
    .Executes(() =>
    {
        MSBuild(c => c
            .SetProcessToolPath(Parameters.MsBuildPath)
            .SetProjectFile(Parameters.MaaSyncResProject)
            .SetTargets("ReBuild")
            .SetConfiguration(BuildConfiguration.Release)
            .SetTargetPlatform(Parameters.TargetPlatform)
            .EnableRestore()
        );
    });
}
