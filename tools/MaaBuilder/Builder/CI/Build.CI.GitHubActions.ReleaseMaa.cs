using Nuke.Common.CI.GitHubActions;

namespace MaaBuilder;

[GitHubActions(
    name: "release-maa-win",
    image: GitHubActionsImage.WindowsLatest,
    OnPushTags = new[] { "v*" },
    InvokedTargets = new[] { nameof(ReleaseMaa) },
    ImportSecrets = new[] { "PUBLISH_GH_PAT" },
    EnableGitHubToken = true,
    PublishArtifacts = true
    )]
public partial class Build { }
