using Nuke.Common.CI.GitHubActions;

namespace MeoAssistantBuilder;

[GitHubActions(
    name: "release-maa-resource",
    image: GitHubActionsImage.WindowsLatest,
    OnPushBranches = new[] { MasterBranch },
    InvokedTargets = new[] { nameof(ReleaseMaaResource) },
    ImportSecrets = new[] { "GITHUB_PAT" },
    EnableGitHubToken = true,
    PublishArtifacts = true
    )]
public partial class Build { }
