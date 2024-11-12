using Nuke.Common;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using MaaBuilder.Models;
using Nuke.Common.Tools.MSBuild;

namespace MaaBuilder;

public partial class Build : NukeBuild
{
    private readonly static string HostPlatform = RuntimeInformation.OSArchitecture switch {
        Architecture.X64 => "x64",
        Architecture.Arm64 => "ARM64",
        _ => "UNSUPPORTED"
    };

    public static int Main()
    {
        var isWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);

        if (isWindows && HostPlatform != "UNSUPPORTED")
        {
            return Execute<Build>(_ => _.Default);
        }

        Console.ForegroundColor = ConsoleColor.Red;
        Console.Error.WriteLine("仅在 Windows x64/ARM64 平台可用");

        Console.Error.WriteLine($"当前系统：{RuntimeInformation.OSDescription}");
        Console.Error.WriteLine($"当前系统架构：{RuntimeInformation.OSArchitecture}");

        return 1;
    }

    private BuildParameters Parameters;

    private const string MasterBranch = "master";
    private const string DevBranch = "dev";

    private const string MaaDevBundlePackageNameTemplate = "MAA-{VERSION}-win-{PLATFORM}-Dev";

    private string Version = "";
    private string ChangeLog = "";

    private string MaaDevBundlePackageName => MaaDevBundlePackageNameTemplate.Replace("{VERSION}", Version).Replace("{PLATFORM}", Parameters.TargetPlatform);

    private string LatestTag;

    private List<Checksum> ArtifactChecksums = new();

    protected override void OnBuildInitialized()
    {
        Parameters = new BuildParameters(this);

        Information("==========================");

        Information("1. 工具链");
        Information($"Visual Studio 路径：{Parameters.VisualStudioPath ?? "Null"}");
        Information($"MSBuild 路径：{Parameters.MsBuildPath ?? "Null"}");
        Information($"MSBuild 目标架构：{Parameters.TargetPlatform ?? "Null"}");

        Information("2. 仓库");
        Information($"主仓库：{Parameters.MainRepo ?? "Null"}");
        Information($"主分支：{Parameters.MasterBranchRef ?? "Null"}");
        Information($"开发分支：{Parameters.DevBranchRef ?? "Null"}");
        Information($"发布 Tag 前缀：{Parameters.ReleaseTagRefPrefix ?? "Null"}");

        Information("3. 路径");
        Information($"编译输出路径：{Parameters.BuildOutput ?? "Null"}");
        Information($"Artifact 输出路径：{Parameters.ArtifactOutput ?? "Null"}");

        Information("4. 项目");
        Information($"MaaCore 项目：{Parameters.MaaCoreProject ?? "Null"}");
        Information($"MaaWpf 项目：{Parameters.MaaWpfProject ?? "Null"}");
        Information($"SyncRes 项目：{Parameters.MaaSyncResProject ?? "Null"}");

        Information("5. 配置");
        Information($"构建时间：{Parameters.BuildTime ?? "Null"}");
        Information($"Commit Hash：{Parameters.CommitHash ?? "Null"}");
        Information($"Commit Hash Full：{Parameters.CommitHashFull ?? "Null"}");

        Information("6. CI");
        Information($"在 GitHub Actions 中运行：{Parameters.IsGitHubActions}");
        Information($"是 Pull Request：{Parameters.IsPullRequest}");
        Information($"是 Workflow Dispatch 触发：{Parameters.IsWorkflowDispatch}");
        Information($"是 PreRelease 版本：{Parameters.IsPreRelease}");
        Information($"Actions 名称：{Parameters.GhActionName ?? "Null"}");
        Information($"Actions 分支：{Parameters.GhBranch ?? "Null"}");
        Information($"Actions PR：{Parameters.GhPullRequestId ?? "Null"}");
        Information($"Actions Tag：{Parameters.GhTag ?? "Null"}");

        Information("==========================");
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Performance", "CA1822:Mark members as static", Justification = "<Pending>")]
    private Target Default => _ => _
            .Executes(() =>
            {
                Assert.Fail("请指定一个 Target");
            });
}
