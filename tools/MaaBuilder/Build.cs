using Nuke.Common;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using MaaBuilder.Models;

namespace MaaBuilder;

public partial class Build : NukeBuild
{
    public static int Main()
    {
        var osValidation = 
            RuntimeInformation.IsOSPlatform(OSPlatform.Windows) &&
            RuntimeInformation.OSArchitecture == Architecture.X64;

        if (osValidation)
        {
            return Execute<Build>(_ => _.Default);
        }
        
        Console.ForegroundColor = ConsoleColor.Red;
        Console.Error.WriteLine("仅在 Windows x64 平台可用");

        Console.Error.WriteLine($"当前系统：{RuntimeInformation.OSDescription}");
        Console.Error.WriteLine($"当前系统架构：{RuntimeInformation.OSArchitecture}");
      
        return 1;
    }

    BuildParameters Parameters;

    const string MasterBranch = "master";
    const string DevBranch = "dev";

    const string MaaDevBundlePackageNameTemplate = "MaaBundle-Dev-{VERSION}";

    string Version = "";
    string ChangeLog = "";

    string MaaDevBundlePackageName => MaaDevBundlePackageNameTemplate.Replace("{VERSION}", Version);

    string LatestTag;

    List<Checksum> ArtifactChecksums = new();

    protected override void OnBuildInitialized()
    {
        Parameters = new BuildParameters(this);

        Information("==========================");

        Information("1. 工具链");
        Information($"Visual Studio 路径：{Parameters.VisualStudioPath ?? "Null"}");
        Information($"MSBuild 路径：{Parameters.MsBuildPath ?? "Null"}");

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
    Target Default => _ => _
        .Executes(() =>
        {
            Assert.Fail("请指定一个 Target");
        });
}
