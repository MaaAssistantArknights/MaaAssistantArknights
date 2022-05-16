using Nuke.Common.CI.GitHubActions;

namespace MeoAssistantBuilder;

[GitHubActions(
    name: "release-maa-resource",
    image: GitHubActionsImage.WindowsLatest,
    OnPushBranches = new[] { MasterBranch },
    OnPushIncludePaths = new[]
    {
      "resource/**",
      "3rdparty/resource/**"
    },
    InvokedTargets = new[] { nameof(ReleaseMaaResource) },
    ImportSecrets = new[] { "PUBLISH_GH_PAT" },
    EnableGitHubToken = true,
    PublishArtifacts = true
    )]
public partial class Build { }
