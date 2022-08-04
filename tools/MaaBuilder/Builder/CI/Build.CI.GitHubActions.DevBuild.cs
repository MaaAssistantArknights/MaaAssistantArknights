using Nuke.Common.CI.GitHubActions;

namespace MaaBuilder;

[GitHubActions(
    name: "dev-build",
    image: GitHubActionsImage.WindowsLatest,
    OnPushBranchesIgnore = new[] { MasterBranch },
    OnPullRequestBranches = new[] { DevBranch },
    OnPushIncludePaths = new[]
    {
        "src/MeoAssistant/**",
        "src/MeoAsstGui/**",  // 新 UI 发布后，移除 MeoAsstGui
        "3rdparty/**",  // 新 UI 发布后，移除 3rdparty/resource/**
        "tools/MaaBuilder/**",
        "tools/MaaBuilder.sln",
        "include/**",
        "resource/**", // 新 UI 发布后，DevBuild 不再包含 WPF Gui 内容，不再打包 MaaBundle，因此移除资源文件夹的监控
        "MeoAssistantArknights.sln"
    },
    OnPullRequestIncludePaths = new[]
    {
        "src/MeoAssistant/**",
        "src/MeoAsstGui/**",  // 新 UI 发布后，移除 MeoAsstGui
        "3rdparty/**",  // 新 UI 发布后，移除 3rdparty/resource/**
        "tools/MaaBuilder/**",
        "tools/MaaBuilder.sln",
        "include/**",
        "resource/**", // 新 UI 发布后，DevBuild 不再包含 WPF Gui 内容，不再打包 MaaBundle，因此移除资源文件夹的监控
        "MeoAssistantArknights.sln"
    },
    OnWorkflowDispatchRequiredInputs = new [] { "Reason", "ReleaseSimulation" },
    InvokedTargets = new[] { nameof(DevBuild) },
    EnableGitHubToken = true,
    PublishArtifacts = true
    )]
public partial class Build { }
