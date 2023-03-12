using Nuke.Common.CI.GitHubActions;

namespace MaaBuilder;

[GitHubActions(
    name: "dev-build-win",
    image: GitHubActionsImage.WindowsLatest,
    OnPushBranchesIgnore = new[] { MasterBranch },
    OnPullRequestBranches = new[] { DevBranch },
    OnPushIncludePaths = new[]
    {
        "src/MaaCore/**",
        "src/MaaWpfGui/**",  // 新 UI 发布后，移除 MaaWpfGui
        "tools/MaaBuilder/**",
        "tools/MaaBuilder.sln",
        "include/**",
        "resource/**", // 新 UI 发布后，DevBuild 不再包含 WPF Gui 内容，不再打包 MaaBundle，因此移除资源文件夹的监控
        "MAA.sln"
    },
    OnPullRequestIncludePaths = new[]
    {
        "src/MaaCore/**",
        "src/MaaWpfGui/**",  // 新 UI 发布后，移除 MaaWpfGui
        "tools/MaaBuilder/**",
        "tools/MaaBuilder.sln",
        "include/**",
        "resource/**", // 新 UI 发布后，DevBuild 不再包含 WPF Gui 内容，不再打包 MaaBundle，因此移除资源文件夹的监控
        "MAA.sln"
    },
    OnWorkflowDispatchRequiredInputs = new [] { "Reason", "ReleaseSimulation" },
    InvokedTargets = new[] { nameof(DevBuild) },
    EnableGitHubToken = true,
    PublishArtifacts = true
    )]
public partial class Build { }
