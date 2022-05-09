using Cake.Common.Build;
using Cake.Common.Diagnostics;

namespace MeoAssistantBuilder.Helper;

public static class GetBuildInformationHelper
{
    public static (string, string) GetBuildInformation(this MaaBuildContext context)
    {
        var ghAction = context.GitHubActions();

        var buildTime = DateTime.UtcNow.AddHours(8);
        var bt = buildTime.ToString("yyyy-MM-dd-HH-mm-ss");
        string hash;

        if (ghAction.IsRunningOnGitHubActions)
        {
            hash = ghAction.Environment.Workflow.Sha.ToLower()[..7];
        }
        else
        {
            try
            {
                var realPath = new DirectoryInfo(Constants.MaaProjectRootDirectory).FullName;
                var repo = new LibGit2Sharp.Repository(realPath);
                var commit = repo.Commits.First();
                hash = commit.Sha.ToLower()[..7];
            }
            catch (Exception e)
            {
                context.Warning($"Failed to get git log: {e.Message}");
                hash = "Unknown";
            }
        }

        return (bt, hash);
    }
}
