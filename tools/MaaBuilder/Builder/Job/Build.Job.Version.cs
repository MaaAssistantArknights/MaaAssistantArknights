using System;
using System.Linq;
using Nuke.Common;
using Nuke.Common.Git;
using Nuke.Common.Tools.GitHub;
using Nuke.Common.Utilities;
using Octokit;

namespace MaaBuilder;

public partial class Build
{
    private Target UseRsVersion => _ => _
        .Triggers(SetVersion)
        .Executes(() =>
        {
            Information($"Release Simulation: {Parameters.GhActionWdRsTagName}");

            Version = Parameters.GhActionWdRsTagName;

            GitHubTasks.GitHubClient = new GitHubClient(new ProductHeaderValue(nameof(NukeBuild)));
        });

    private Target UseCommitVersion => _ => _
        .Triggers(SetVersion)
        .Executes(() =>
        {
            var envReleaseTag = Environment.GetEnvironmentVariable("MAA_BUILDER_MAA_VERSION");
            Version = !envReleaseTag.IsNullOrEmpty() ? envReleaseTag : $"{Parameters.BuildTime}-{Parameters.CommitHash}";
        });

    private Target UseTagVersion => _ => _
        .Triggers(SetVersion)
        .Executes(() =>
        {
            if (Parameters.IsGitHubActions is false)
            {
                Information("未在 GitHub Actions 中运行");

                var repo = GitRepository.FromLocalDirectory(RootDirectory);
                Assert.True(repo is not null, "不在 Git Repo 中");

                var tag = repo.Tags.ToList().Find(v => v.StartsWith("v"));

                if (tag is not null)
                {
                    Version = $"{tag}-Local";
                }
                else
                {
                    Version = $"v.{Parameters.CommitHash}-Local";
                }
            }
            else
            {
                Information("在 GitHub Actions 中运行");

                Assert.True(Parameters.GhTag is not null, "在 GitHub Actions 中运行，但是不存在 Tag");

                GitHubTasks.GitHubClient = new GitHubClient(new ProductHeaderValue(nameof(NukeBuild)))
                {
                    Credentials = new Credentials(Parameters.GitHubPersonalAccessToken)
                };

                LatestTag = Parameters.IsPreRelease ? GetLatestTag() : GetLatestReleaseTag();

                Version = Parameters.GhTag;
            }
        });

    private Target SetVersion => _ => _
        .Executes(() =>
        {
            Information($"当前版本号设置为：{Version}");
        });
}
