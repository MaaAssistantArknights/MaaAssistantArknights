using Microsoft.VisualBasic;
using Nuke.Common;
using Nuke.Common.Execution;
using Nuke.Common.Git;
using Nuke.Common.IO;
using Nuke.Common.Tooling;
using Nuke.Common.Tools.MSBuild;
using Serilog;
using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

namespace MeoAssistantBuilder;

[CheckBuildProjectConfigurations]
public partial class Build : NukeBuild
{
    public static int Main()
    {
        var osValidation = 
            RuntimeInformation.IsOSPlatform(OSPlatform.Windows) &&
            RuntimeInformation.OSArchitecture == Architecture.X64;
        
        if (osValidation is false)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Error.WriteLine("仅在 Windows x64 平台可用");

            Console.Error.WriteLine($"当前系统：{RuntimeInformation.OSDescription}");
            Console.Error.WriteLine($"当前系统架构：{RuntimeInformation.OSArchitecture}");
      
            return 1;
        }
        return Execute<Build>(_ => _.Default);
    }
    
    BuildParameters Parameters;

    const string MasterBrach = "master";
    const string DevBranch = "dev";
    const string ReleaseTagPrefix = "v";

    const string MaaBundlePackageName = "MaaBundle-Dev-{VERSION}";
    const string LegacyMaaBundlePackageName = "MeoAssistantArknights_{VERSION}";
    const string MaaCorePackageName = "MaaCore-{VERSION}";
    const string MaaResourcePackageName = "MaaResource-{VERSION}";

    //
    // 版本号类型
    // 
    // Commit 类型: {yyyy-MM-dd-HH-mm-ss}-{commitHash[..7]}
    // SemVer 类型: {Tag}
    //

    private string _version = "";
    private string _packageName = "";
    private BuildConfiguration _buildConfiguration = BuildConfiguration.Release;

    protected override void OnBuildInitialized()
    {
        Parameters = new BuildParameters(this);

        Information("==========================");

        Information("1. 工具链");
        Information($"Visual Studio 路径：{Parameters.VisualStudioPath ?? "Null"}");
        Information($"MSBuild 路径：{Parameters.MsBuildPath ?? "Null"}");

        Information("2. 仓库");
        Information($"主仓库：{Parameters.MainRepo ?? "Null"}");
        Information($"MaaCore 发布仓库：{Parameters.MaaCoreReleaseRepo ?? "Null"}");
        Information($"MaaResource 发布仓库：{Parameters.MaaResourceReleaseRepo ?? "Null"}");
        Information($"主分支：{Parameters.MasterBranchRef ?? "Null"}");
        Information($"开发分支：{Parameters.DevBranchRef ?? "Null"}");
        Information($"发布 Tag 前缀：{Parameters.ReleaseTagRefPrefix ?? "Null"}");

        Information("3. 路径");
        Information($"编译输出路径：{Parameters.BuildOutput ?? "Null"}");
        Information($"Artifact 输出路径：{Parameters.ArtifactOutput ?? "Null"}");
        Information($"Artifact 包路径：{Parameters.ArtifactBundleOutput ?? "Null"}");
        Information($"Artifact 源文件路径：{Parameters.ArtifactRawOutput ?? "Null"}");

        Information("4. 项目");
        Information($"MaaCore 项目：{Parameters.MaaCoreProject ?? "Null"}");
        Information($"MaaWpf 项目：{Parameters.MaaWpfProject ?? "Null"}");

        Information("5. 配置");
        Information($"构建时间：{Parameters.BuildTime ?? "Null"}");
        Information($"Commit Hash：{Parameters.CommitHash ?? "Null"}");

        Information("6. CI");
        Information($"在 GitHub Actions 中运行：{Parameters.IsGitHubActions}");
        Information($"Actions 名称：{Parameters.GhActionName ?? "Null"}");
        Information($"Actions 分支：{Parameters.GhBrach ?? "Null"}");
        Information($"Actions Tag：{Parameters.GhTag ?? "Null"}");

        Information("==========================");
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Performance", "CA1822:Mark members as static", Justification = "<Pending>")]
    Target Default => _ => _
        .Executes(() =>
        {
            Assert.Fail("请指定一个 Target");
        });

    #region 初始化

    Target ArtifactInitialize => _ => _
        .Executes(() =>
        {
            RecreateDirectory(Parameters.ArtifactOutput);
            RecreateDirectory(Parameters.ArtifactBundleOutput);
            RecreateDirectory(Parameters.ArtifactRawOutput);
        });

    #endregion

    #region 清理

    Target Clean => _ => _
        .After(SetConfigurationCICD, SetConfigurationRelease, SetConfigurationRelWithDebInfo)
        .Executes(() =>
        {
            Information("清理 MaaCore");
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaCoreProject)
                .SetTargets("Clean")
                .SetConfiguration(_buildConfiguration)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
            );

            // 在 MaaElectronUI 发布后移除
            Information("清理 MaaWpf");
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaWpfProject)
                .SetTargets("Clean")
                .SetConfiguration(_buildConfiguration)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
            );

            RecreateDirectory(Parameters.BuildOutput / _buildConfiguration);
        });

    #endregion

    #region 设置版本

    Target SetCommitVersion => _ => _
        .Executes(() =>
        {
            _version = $"{Parameters.BuildTime}-{Parameters.CommitHash}";
            Information($"当前版本号设置为：{_version}");
        });

    Target SetTagVersion => _ => _
        .Executes(() =>
        {
            if (Parameters.IsGitHubActions is false)
            {
                Information("未在 GitHub Actions 中运行");

                var repo = GitRepository.FromLocalDirectory(RootDirectory);
                Assert.True(repo is not null, "不在 Git Repo 中");
                
                var tag = repo.Tags.FirstOrDefault();

                if (tag is not null)
                {
                    _version = $"{tag}-Local";
                }
                else
                {
                    _version = $"v.{Parameters.CommitHash}-Local";
                }

                Information($"当前版本号设置为：{_version}");
            }
            else
            {
                Information("在 GitHub Actions 中运行");

                Assert.True(Parameters.GhTag is not null, "在 GitHub Actions 中运行，但是不存在 Tag");

                _version = Parameters.GhTag;
            }
        });

    #endregion

    #region 设置包名

    Target SetPackageNameMaaBundle => _ => _
        .After(SetCommitVersion, SetTagVersion)
        .Before(BundlePackage)
        .Executes(() =>
        {
            _packageName = MaaBundlePackageName.Replace("{VERSION}", _version);
            Information($"设置包名：{_packageName}");
        });
    Target SetPackageNameLegacyMaaBundle => _ => _
        .After(SetCommitVersion, SetTagVersion)
        .Before(BundlePackage)
        .Executes(() =>
        {
            _packageName = LegacyMaaBundlePackageName.Replace("{VERSION}", _version);
            Information($"设置包名：{_packageName}");
        });
    Target SetPackageNameMaaCore => _ => _
        .After(SetCommitVersion, SetTagVersion)
        .Before(BundlePackage)
        .Executes(() =>
        {
            _packageName = MaaCorePackageName.Replace("{VERSION}", _version);
            Information($"设置包名：{_packageName}");
        });
    Target SetPackageNameMaaResource => _ => _
        .After(SetCommitVersion, SetTagVersion)
        .Before(BundlePackage)
        .Executes(() =>
        {
            _packageName = MaaResourcePackageName.Replace("{VERSION}", _version);
            Information($"设置包名：{_packageName}");
        });

    #endregion

    #region 编译

    Target Compile => _ => _
        .After(Clean, SetCommitVersion, SetTagVersion)
        .Executes(() =>
        {
            var versionEnv = $"/DMAA_VERSION=\\\"{_version}\\\"";
            Information($"MaaCore 编译环境变量：ExternalCompilerOptions = {versionEnv}");
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaCoreProject)
                .SetTargets("ReBuild")
                .SetConfiguration(_buildConfiguration)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
                .SetProcessEnvironmentVariable("ExternalCompilerOptions", versionEnv)
            );
        });

    // 在 MaaElectronUI 发布后移除
    Target CompileWpf => _ => _
        .After(Clean)
        .Executes(() =>
        {
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaWpfProject)
                .SetTargets("ReBuild")
                .SetConfiguration(_buildConfiguration)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
            );
        });

    #endregion

    #region 编译后处理

    Target RemoveDebug => _ => _
        .After(Compile, CompileWpf)
        .Before(BundlePackage)
        .Executes(() =>
        {
            var output = Parameters.BuildOutput / _buildConfiguration;
            var files = output.GlobFiles("*.pdb", "*.lib", "*.exp", "*.exe.config");

            foreach (var file in files)
            {
                File.Delete(file);
                Information($"删除文件：{file}");
            }
        });

    #endregion

    #region 资源文件

    Target BundleResource => _ => _
        .After(Clean)
        .Executes(() =>
        {
            var output = Parameters.BuildOutput / _buildConfiguration;

            var resourceFile = RootDirectory / "resource";
            var resourceFileThirdParty = RootDirectory / "3rdparty" / "resource";

            CopyDirectory(resourceFile, output);
            Information($"复制目录：{resourceFile} -> {output}");
            CopyDirectory(resourceFileThirdParty, output);
            Information($"复制目录：{resourceFileThirdParty} -> {output}");
        });

    Target BundleThirdPartyDlls => _ => _
        .After(Clean)
        .Executes(() =>
        {
            var output = Parameters.BuildOutput / _buildConfiguration;
            var dlls = RootDirectory / "3rdparty" / "bin";

            CopyDirectory(dlls, output);
            Information($"复制目录：{dlls} -> {output}");
        });

    #endregion

    #region 打包

    Target BundlePackage => _ => _
        .After(Compile, CompileWpf, BundleResource, BundleThirdPartyDlls)
        .Executes(() =>
        {
            var output = Parameters.BuildOutput / _buildConfiguration;
            var rawOutput = Parameters.ArtifactRawOutput / _packageName;
            var bundleOutput = Parameters.ArtifactBundleOutput / $"{_packageName}.zip";

            Information($"编译输出：{output}");
            Information($"打包输出：{bundleOutput}");
            Information($"打包原始输出：{rawOutput}");

            Assert.True(Directory.Exists(output), $"输出目录 {output} 不存在");

            Information($"复制目录：{output} -> {rawOutput}");
            CopyDirectory(output, rawOutput);

            Information($"创建压缩文件：{bundleOutput}");
            ZipFile.CreateFromDirectory(rawOutput, bundleOutput, CompressionLevel.SmallestSize, false);

            var size = (new FileInfo(bundleOutput).Length / 1024.0 / 1024.0).ToString("F3");
            Information($"压缩文件大小：{size} MB");
        });

    #endregion

    #region 发布

    Target PublishArtifact => _ => _
        .After(BundlePackage)
        .When(GitHubActions is not null, _ => _
                .Produces(Parameters.ArtifactRawOutput / _packageName)
                .Executes(() =>
                {
                    Information($"在 GitHub Actions 中运行，上传 Artifact：{Parameters.ArtifactRawOutput / _packageName}");
                }))
        .Executes(() =>
        {
            if (Parameters.IsGitHubActions is false)
            {
                Information($"不在 GitHub Actions 中，跳过执行上传 Artifact");
            }
        });

    #endregion

    #region 设置 Configuration

    Target SetConfigurationRelease => _ => _
        .DependsOn(ArtifactInitialize)
        .Executes(() =>
        {
            _buildConfiguration = BuildConfiguration.Release;
            Information($"当前 Configuration：{_buildConfiguration}");
        });

    Target SetConfigurationRelWithDebInfo => _ => _
        .DependsOn(ArtifactInitialize)
        .Executes(() =>
        {
            _buildConfiguration = BuildConfiguration.RelWithDebInfo;
            Information($"当前 Configuration：{_buildConfiguration}");
        });

    Target SetConfigurationCICD => _ => _
        .DependsOn(ArtifactInitialize)
        .Executes(() =>
        {
            _buildConfiguration = BuildConfiguration.CICD;
            Information($"当前 Configuration：{_buildConfiguration}");
        });

    #endregion

    #region Target Flow

    /// <summary>
    /// 见 <see cref="ActionConfiguration.DevBuild"/>
    /// </summary>
    Target DevBuild => _ => _
        .DependsOn(SetConfigurationRelWithDebInfo)
        .DependsOn(SetCommitVersion)
        .DependsOn(SetPackageNameMaaBundle)
        .DependsOn(Clean)
        .DependsOn(Compile)
        .DependsOn(CompileWpf)
        .DependsOn(BundlePackage)
        .DependsOn(PublishArtifact);

    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaa"/>
    /// </summary>
    Target ReleaseMaa => _ => _
        .DependsOn(SetConfigurationRelease)
        .DependsOn(SetTagVersion)
        .DependsOn(SetPackageNameLegacyMaaBundle)
        .DependsOn(Clean)
        .DependsOn(Compile)
        .DependsOn(CompileWpf)
        .DependsOn(RemoveDebug)
        .DependsOn(BundlePackage)
        .DependsOn(PublishArtifact);

    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaaCore"/>
    /// </summary>
    Target ReleaseMaaCore => _ => _
        .DependsOn(SetConfigurationCICD)
        .DependsOn(SetTagVersion)
        .DependsOn(SetPackageNameMaaCore)
        .DependsOn(Clean)
        .DependsOn(Compile)
        .DependsOn(RemoveDebug)
        .DependsOn(BundleThirdPartyDlls)
        .DependsOn(BundlePackage)
        .DependsOn(PublishArtifact);

    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaaResource"/>
    /// </summary>
    Target ReleaseMaaResource => _ => _
        .DependsOn(SetConfigurationRelease)
        .DependsOn(SetCommitVersion)
        .DependsOn(SetPackageNameMaaResource)
        .DependsOn(Clean)
        .DependsOn(BundleResource)
        .DependsOn(BundlePackage)
        .DependsOn(PublishArtifact);
    

    #endregion

    #region Logger

    private readonly Action<string> Information = (string msg) =>
    {
        Log.Information("{Message}", msg);
    };

    private readonly Action<string> Warning = (string msg) =>
    {
        Log.Warning("{Message}", msg);
    };

    private readonly Action<string> Error = (string msg) =>
    {
        Log.Error("{Message}", msg);
    };

    #endregion

    #region Utilities

    private void RecreateDirectory(string directoryPath)
    {
        if (Directory.Exists(directoryPath))
        {
            Directory.Delete(directoryPath, true);
        }
        Directory.CreateDirectory(directoryPath);

        Information($"重建目录：{directoryPath}");
    }

    private static void CopyDirectory(string sourceDir, string destinationDir, bool recursive = true)
    {
        var dir = new DirectoryInfo(sourceDir);

        Assert.True(dir.Exists, $"找不到源目录: {dir.FullName}");

        DirectoryInfo[] dirs = dir.GetDirectories();

        if (Directory.Exists(destinationDir) is false)
        {
            Directory.CreateDirectory(destinationDir);
        }

        foreach (var file in dir.GetFiles())
        {
            var targetFilePath = Path.Combine(destinationDir, file.Name);
            file.CopyTo(targetFilePath, true);
        }

        if (recursive)
        {
            foreach (var subDir in dirs)
            {
                var newDestinationDir = Path.Combine(destinationDir, subDir.Name);
                CopyDirectory(subDir.FullName, newDestinationDir);
            }
        }
    }

    #endregion

    Target DevTest => _ => _
        .Executes(() =>
        {
            Information("QAQ");
        });
}
