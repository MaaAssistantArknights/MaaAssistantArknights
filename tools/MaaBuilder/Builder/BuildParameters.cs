using Newtonsoft.Json.Linq;
using Nuke.Common;
using Nuke.Common.CI;
using Nuke.Common.CI.GitHubActions;
using Nuke.Common.IO;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tools.Git;
using Serilog;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.RegularExpressions;
using MaaBuilder.Models;
using static Nuke.Common.Tools.VSWhere.VSWhereTasks;
using Nuke.Common.Tools.MSBuild;
using Nuke.Common.Utilities.Collections;

namespace MaaBuilder;

public partial class Build
{
    #region Nuke 默认全局参数

    [CI] private readonly GitHubActions GitHubActions;

    public static bool IsReleaseSimulation { get; set; }

    #endregion Nuke 默认全局参数

    public class BuildParameters
    {
        #region 方法

        private static readonly Lazy<string> GetVsPath = new(() =>
        {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows) is false)
            {
                return null;
            }

            var vsDirectory = VSWhere("-latest -nologo -property installationPath -format value -prerelease").FirstOrDefault().Text;

            return string.IsNullOrWhiteSpace(vsDirectory) ? null : vsDirectory;
        });

        private readonly Func<string, string> GetMsBuildPath = vsPath =>
        {
            var msBuildExe = Path.Combine(vsPath, @"MSBuild\Current\Bin\MSBuild.exe");
            if (File.Exists(msBuildExe) is false)
            {
                msBuildExe = Path.Combine(vsPath, @"MSBuild\15.0\Bin\MSBuild.exe");
            }

            return File.Exists(msBuildExe) ? msBuildExe : null;
        };

        #endregion 方法

        // 工具链
        public AbsolutePath MsBuildPath { get; }

        public AbsolutePath VisualStudioPath { get; }

        // 仓库
        public string MainRepo { get; }

        public string MasterBranchRef { get; }
        public string DevBranchRef { get; }
        public string ReleaseTagRefPrefix { get; }

        // 路径
        public AbsolutePath BuildOutput { get; }

        public AbsolutePath ArtifactOutput { get; }
        public AbsolutePath MaaChangelogFile { get; }

        // 项目
        public Project MaaCoreProject { get; }

        public Project MaaWpfProject { get; }

        public Project MaaSyncResProject { get; }

        // 配置
        public string BuildTime { get; }

        public string CommitHash { get; }
        public string CommitHashFull { get; }
        public string TargetPlatform { get; }
        public List<Package> Packages { get; } = null;

        // CI
        public bool IsGitHubActions { get; }

        public bool IsPullRequest { get; }
        public bool IsWorkflowDispatch { get; }
        public bool IsPreRelease { get; }
        public string GitHubPersonalAccessToken { get; }
        public string GhActionWdReason { get; }
        public string GhActionWdRsTagName { get; }
        public ActionConfiguration GhActionName { get; }
        public string GhBranch { get; }
        public string GhPullRequestId { get; }
        public string GhTag { get; }

        public BuildParameters(Build b)
        {
            // 工具链
            var vs = GetVsPath.Value;
            Assert.True(vs is not null, "找不到 Visual Studio");
            VisualStudioPath = (AbsolutePath)vs;
            var msbuild = GetMsBuildPath.Invoke(VisualStudioPath);
            Assert.True(msbuild is not null, "找不到 MSBuild");
            MsBuildPath = (AbsolutePath)msbuild;

            TargetPlatform = Environment.GetEnvironmentVariable("MAABUILDER_TARGET_PLATFORM") ?? HostPlatform;

            // 仓库
            MainRepo = "MaaAssistantArknights/MaaAssistantArknights";

            MasterBranchRef = "refs/heads/master";
            DevBranchRef = "refs/heads/dev";
            ReleaseTagRefPrefix = "refs/tags/v";

            // 路径
            BuildOutput = RootDirectory / TargetPlatform;
            ArtifactOutput = RootDirectory / "artifacts";

            MaaChangelogFile = RootDirectory / "CHANGELOG.md";

            // 项目
            var maaSolution = ProjectModelTasks.ParseSolution(RootDirectory / "MAA.sln");
            Assert.True(maaSolution is not null, "无法载入 MAA.sln");
            MaaCoreProject = maaSolution.GetProject("MaaCore");
            MaaWpfProject = maaSolution.GetProject("MaaWpfGui");
            MaaSyncResProject = maaSolution.GetProject("SyncRes");

            // 配置
            CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("zh-Hans-CN");
            BuildTime = DateTimeOffset.UtcNow.ToLocalTime().ToString("yyyy-MM-dd-HH-mm-ss");

            CommitHash = GitTasks.GitCurrentCommit();
            Assert.True(CommitHash is not null, "Commit Hash 为 Null");
            CommitHashFull = CommitHash;
            CommitHash = CommitHash[..7];

            var packageDefinitionFile = RootDirectory / "package-definition.json";
            var stream = File.OpenRead(packageDefinitionFile);
            var model = JsonSerializer.Deserialize<List<Package>>(stream);
            stream.Close();
            Packages = model;

            // CI
            IsGitHubActions = b.GitHubActions is not null;
            if (IsGitHubActions is false)
            {
                return;
            }
            GhActionName = (ActionConfiguration)b.GitHubActions!.Workflow;
            Assert.True(GhActionName is not null, $"GitHub Actions Workflow 名 {b.GitHubActions.Workflow} 无法转换为 ActionConfiguration");

            Assert.False(string.IsNullOrEmpty(b.GitHubActions.Ref), "Ref 为 Null");

            GitHubPersonalAccessToken = Environment.GetEnvironmentVariable("PUBLISH_GH_PAT");

            if (b.GitHubActions.Ref.StartsWith(ReleaseTagRefPrefix))
            {
                var tag = $"v{b.GitHubActions.Ref.Replace(ReleaseTagRefPrefix, "")}";
                var pattern = @"^v((0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?)";
                var match = Regex.Match(tag, pattern);
                if (match.Success)
                {
                    GhTag = tag;
                    var preReleasePattern = @"^v((0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*))(-)";
                    var preReleaseMatch = Regex.Match(tag, preReleasePattern);
                    IsPreRelease = preReleaseMatch.Success;
                }
                else
                {
                    Assert.Fail("Tag 与 SemVer 不匹配");
                }
            }
            else if (b.GitHubActions.Ref.StartsWith("refs/heads/"))
            {
                GhBranch = b.GitHubActions.Ref.Replace("refs/heads/", "");
            }
            else if (b.GitHubActions.IsPullRequest)
            {
                IsPullRequest = true;
                GhPullRequestId = b.GitHubActions.Ref;
            }
            else
            {
                Log.Warning("未知的 Ref：{Ref}", b.GitHubActions.Ref ?? "Null");
            }

            var ghEvent = b.GitHubActions.GitHubEvent;
            if (IsGitHubActions)
            {
                Console.WriteLine("::group::GitHub Event");
                Log.Information("GitHub Event：{Event}", ghEvent.ToString());
                Console.WriteLine("::endgroup::");
            }

            if (ghEvent.ContainsKey("inputs") && !ghEvent["inputs"].IsNullOrEmpty())
            {
                IsWorkflowDispatch = true;

                var inputs = (JObject)ghEvent["inputs"];
                foreach (var (k, v) in inputs)
                {
                    switch (k)
                    {
                        case "Reason":
                            GhActionWdReason = v.ToString();
                            break;

                        case "ReleaseSimulation":
                            var tn = v.ToString();
                            if (string.IsNullOrEmpty(tn) || tn == "none")
                            {
                                break;
                            }

                            IsReleaseSimulation = true;
                            GhActionWdRsTagName = tn;

                            break;
                    }
                }
            }

            if (b.GitHubActions.BaseRef is not null)
            {
                // Fork
                MainRepo = b.GitHubActions.Repository;
            }

            // 若是 DevBuild，Branch 必须不为 Master，或者是 PR 至 Dev，又或者是手动触发
            if (GhActionName == ActionConfiguration.DevBuild)
            {
                if (IsWorkflowDispatch)
                {
                    Assert.True(GhBranch is not null, "DevBuild -> Workflow Dispatch，Branch 为 Null");
                }
                else if (IsPullRequest)
                {
                    Assert.True(GhPullRequestId is not null, "DevBuild -> Pull Request，Pull Request Id 为 Null");
                }
                else
                {
                    Assert.True(GhBranch != MasterBranch, "DevBuild -> Auto Triggered，Branch 为 master");
                }
            }

            // 若是 ReleaseMaa，Tag 必须存在，PAT 必须存在
            if (GhActionName == ActionConfiguration.ReleaseMaa)
            {
                Assert.True(GhTag is not null, "ReleaseMaa -> Auto Triggered，Tag 为 Null");
                Assert.True(GitHubPersonalAccessToken is not null, "ReleaseMaa -> Auto Triggered，PAT 为 Null");
            }
        }
    }
}
