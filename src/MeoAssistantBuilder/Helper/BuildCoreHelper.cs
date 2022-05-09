using Cake.Common.Tools.MSBuild;

namespace MeoAssistantBuilder.Helper;

public static class BuildCoreHelper
{
    public static void BuildCore(this MaaBuildContext context, string configuration)
    {
        context.MSBuild(Constants.MaaCoreProjectFilePath, new MSBuildSettings
        {
            WorkingDirectory = Constants.MaaProjectRootDirectory,
            ToolPath = context.MsBuildExecutable,
            Configuration = configuration,
            PlatformTarget = PlatformTarget.x64
        });
    }
}
