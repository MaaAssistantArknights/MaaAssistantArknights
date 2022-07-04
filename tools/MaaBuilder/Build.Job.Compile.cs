using Nuke.Common;
using Nuke.Common.IO;
using Nuke.Common.Tooling;
using Nuke.Common.Tools.MSBuild;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

namespace MaaBuilder;

public partial class Build
{
    Target WithCompileCoreRelease => _ => _
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
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
                .SetProcessEnvironmentVariable("ExternalCompilerOptions", versionEnv)
            );
        });
    
    // TODO 在 MaaElectronUI 发布后移除
    Target WithCompileWpfRelease => _ => _
        .DependsOn(UseClean)
        .After(SetVersion)
        .Executes(() =>
        {
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaWpfProject)
                .SetTargets("ReBuild")
                .SetConfiguration(BuildConfiguration.Release)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
                .EnableRestore()
            );
        });

    Target WithCompileResourceRelease => _ => _
        .DependsOn(UseClean)
        .After(SetVersion)
        .Executes(() =>
        {
            var output = Parameters.BuildOutput / BuildConfiguration.Release;
            var resourceFile = RootDirectory / "resource";
            var resourceFileThirdParty = RootDirectory / "3rdparty" / "resource";

            Information($"复制目录：{resourceFile} -> {output}");
            FileSystemTasks.CopyDirectoryRecursively(resourceFile, output,
                DirectoryExistsPolicy.Merge, FileExistsPolicy.OverwriteIfNewer);
            Information($"复制目录：{resourceFileThirdParty} -> {output}");
            FileSystemTasks.CopyDirectoryRecursively(resourceFileThirdParty, output,
                DirectoryExistsPolicy.Merge, FileExistsPolicy.OverwriteIfNewer);
        });
}
