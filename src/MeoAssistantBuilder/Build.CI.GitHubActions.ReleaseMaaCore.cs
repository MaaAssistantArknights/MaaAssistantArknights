/*using Nuke.Common.CI.GitHubActions;

namespace MeoAssistantBuilder;

[GitHubActions(
    name: "release-maa-core",
    image: GitHubActionsImage.WindowsLatest,
    OnPushBranches = new[] { MasterBranch },
    OnPushTags = new[] { $"v*" },
    InvokedTargets = new[] { nameof(ReleaseMaaCore) },
    ImportSecrets = new[] { "GITHUB_PAT" },
    EnableGitHubToken = true,
    PublishArtifacts = true
    )]
public partial class Build { }
*/

// 在 MaaElectronUI 发布前不启用
