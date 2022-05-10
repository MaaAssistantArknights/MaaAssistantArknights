using Cake.Common.Tools.MSBuild;

namespace MeoAssistantBuilder.Helper;

public static class BuildWpfHelper
{
    public static void BuildWpf(this MaaBuildContext context, string configuration)
    {
        context.MSBuild(Constants.MaaWpfProjectFilePath, new MSBuildSettings
        {
            WorkingDirectory = Constants.MaaProjectRootDirectory,
            Restore = true,
            ToolPath = context.MsBuildExecutable,
            Configuration = configuration,
            PlatformTarget = PlatformTarget.x64
        });
    }
}
