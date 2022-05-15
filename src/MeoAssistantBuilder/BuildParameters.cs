using Nuke.Common;
using Nuke.Common.CI;
using Nuke.Common.CI.GitHubActions;
using Nuke.Common.IO;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tools.Git;
using System;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using static Nuke.Common.Tools.VSWhere.VSWhereTasks;

namespace MeoAssistantBuilder;

public partial class Build
{
    #region Nuke 默认全局参数

    [Solution] readonly Solution Solution;
    [CI] readonly GitHubActions GitHubActions;

    #endregion

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

        private readonly Func<string, string> GetMsBuildPath = (string vsPath) =>
        {
            var msBuildExe = Path.Combine(vsPath, @"MSBuild\Current\Bin\MSBuild.exe");
            if (File.Exists(msBuildExe) is false)
            {
                msBuildExe = Path.Combine(vsPath, @"MSBuild\15.0\Bin\MSBuild.exe");
            }

            return File.Exists(msBuildExe) ? msBuildExe : null;
        };

        #endregion

        // 工具链
        public AbsolutePath MsBuildPath { get; }
        public AbsolutePath VisualStudioPath { get; }

        // 仓库
        public string MainRepo { get; }
        public string MasterBranchRef { get; }
        public string DevBranchRef { get; }
        public string ReleaseTagRefPrefix { get; }
        public string MaaCoreReleaseRepo { get; }
        public string MaaResourceReleaseRepo { get; }

        // 路径
        public AbsolutePath BuildOutput { get; }
        public AbsolutePath ArtifactOutput { get; }
        public AbsolutePath ArtifactBundleOutput { get; }
        public AbsolutePath ArtifactRawOutput { get; }
        
        // 项目
        public Project MaaCoreProject { get; }
        public Project MaaWpfProject { get; }

        // 配置
        public string BuildTime { get; }
        public string CommitHash { get; }

        // CI
        public bool IsGitHubActions { get; }
        public ActionConfiguration GhActionName { get; } = null;
        public string GhBrach { get; } = null;
        public string GhTag { get; } = null;

        public BuildParameters(Build b)
        {
            // 工具链
            var vs = GetVsPath.Value;
            Assert.True(vs is not null, "找不到 Visual Studio");
            VisualStudioPath = (AbsolutePath)vs;
            var msbuild = GetMsBuildPath.Invoke(VisualStudioPath);
            Assert.True(msbuild is not null, "找不到 MSBuild");
            MsBuildPath = (AbsolutePath)msbuild;

            // 仓库
            MainRepo = "MaaAssistantArknights/MaaAssistantArknights";
            MaaCoreReleaseRepo = "MaaAssistantArknights/MaaCoreRelease";
            MaaResourceReleaseRepo = "MaaAssistantArknights/MaaResourceRelease";

            MasterBranchRef = "refs/heads/master";
            DevBranchRef = "refs/heads/dev";
            ReleaseTagRefPrefix = "refs/tags/v";

            // 路径
            BuildOutput = RootDirectory / "x64";
            ArtifactOutput = RootDirectory / "artifacts";
            ArtifactBundleOutput = ArtifactOutput / "bundle";
            ArtifactRawOutput = ArtifactOutput / "raw";

            // 项目
            MaaCoreProject = b.Solution.GetProject("MeoAssistant");
            MaaWpfProject = b.Solution.GetProject("MeoAsstGui");

            // 配置
            CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("zh-Hans-CN");
            BuildTime = DateTimeOffset.UtcNow.ToLocalTime().ToString("yyyy-MM-dd-HH-mm-ss");
            
            CommitHash = GitTasks.GitCurrentCommit();
            Assert.True(CommitHash is not null, "Commit Hash 为 Null");
            CommitHash = CommitHash[..7];

            // CI
            IsGitHubActions = b.GitHubActions is not null;
            if (IsGitHubActions)
            {
                GhActionName = (ActionConfiguration)b.GitHubActions.Action;
                if (b.GitHubActions.Ref == MasterBranchRef)
                {
                    GhBrach = MasterBrach;
                }
                else if (b.GitHubActions.Ref == DevBranchRef)
                {
                    GhBrach = DevBranch;
                }
                else if (b.GitHubActions.Ref.StartsWith(ReleaseTagRefPrefix))
                {
                    var tag = $"v{b.GitHubActions.Ref.Replace(ReleaseTagRefPrefix, "")}";
                    var pattern = @"v((0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?)";
                    var match = Regex.Match(tag, pattern);
                    if (match.Success)
                    {
                        GhTag = tag;
                    }
                    else
                    {
                        Assert.Fail("Tag 与 SemVer 不匹配");
                    }
                }
            }
        }
    }
}
