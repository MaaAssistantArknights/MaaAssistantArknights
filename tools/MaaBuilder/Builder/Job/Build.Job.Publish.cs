using Nuke.Common;
using Nuke.Common.Tools.GitHub;
using Octokit;

namespace MaaBuilder;

public partial class Build
{
    Target UsePublishArtifact => _ => _
        .After(SetPackageBundled, SetMaaChangeLog)
        .Produces(RootDirectory / "artifacts" / "*.*")
        .OnlyWhenStatic(() => GitHubActions != null);

    Target UsePublishRelease => _ => _
        .After(UsePublishArtifact)
        .OnlyWhenStatic(() => GitHubActions != null)
        .Executes(() =>
        {
            Information("在 GitHub Actions 中运行，执行发布 Release 任务");
            Information($"当前 Actions：{Parameters.GhActionName}");

            if (Parameters.GhActionName == ActionConfiguration.DevBuild)
            {
                Information("DevBuild 跳过发布 Release");
                return;
            }

            GitHubTasks.GitHubClient ??= new GitHubClient(new ProductHeaderValue(nameof(NukeBuild)))
            {
                Credentials = new Credentials(Parameters.GitHubPersonalAccessToken)
            };

            // ReSharper disable once InvertIf
            if (Parameters.GhActionName == ActionConfiguration.ReleaseMaa)
            {
                Information($"运行 ReleaseMaa 将在 {Parameters.MainRepo} 创建 Release {Parameters.GhTag}");

                CreateOrUpdateGitHubRelease(Parameters.MainRepo, Parameters.CommitHashFull, Version);
            }
        });
}
