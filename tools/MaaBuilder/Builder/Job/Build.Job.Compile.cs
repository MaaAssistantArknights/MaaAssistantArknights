using Nuke.Common;
using Nuke.Common.IO;
using Nuke.Common.Tooling;
using Nuke.Common.Tools.DotNet;
using Nuke.Common.Tools.MSBuild;
using Nuke.Common.Utilities.Collections;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

namespace MaaBuilder;

public partial class Build
{
    private Target WithCompileCoreRelease => _ => _
        .DependsOn(UseClean)
        .After(SetVersion, WithCompileWpfRelease)
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

    private Target WithCompileWpfRelease => _ => _
        .DependsOn(UseClean)
        .After(SetVersion)
        .Executes(() =>
        {
            DotNetTasks.DotNetPublish(c => c
                .SetProject(Parameters.MaaWpfProject)
                .SetConfiguration(BuildConfiguration.Release)
                .SetOutput(Parameters.BuildOutput / "wpf-release")
                .SetPlatform(Parameters.TargetPlatform));

            (Parameters.BuildOutput / BuildConfiguration.Release).CreateOrCleanDirectory();
            (Parameters.BuildOutput / "wpf-release")
                .GetFiles()
                .ForEach(x =>
                    x.MoveToDirectory(Parameters.BuildOutput / BuildConfiguration.Release));
        });

    private Target WithSyncRes => _ => _
    .DependsOn(UseClean)
    .After(SetVersion, WithCompileWpfRelease)
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
