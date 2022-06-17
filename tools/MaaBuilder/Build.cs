using Microsoft.AspNetCore.StaticFiles;
using Microsoft.Extensions.FileSystemGlobbing;
using Nuke.Common;
using Nuke.Common.Execution;
using Nuke.Common.Git;
using Nuke.Common.IO;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tooling;
using Nuke.Common.Tools.GitHub;
using Nuke.Common.Tools.MSBuild;
using Octokit;
using Serilog;
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

namespace MaaBuilder;

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

    const string MasterBranch = "master";
    const string DevBranch = "dev";

    const string MaaDevBundlePackageNameTemplate = "MaaBundle-Dev-{VERSION}";
    const string MaaLegacyBundlePackageNameTemplate = "MeoAssistantArknights_{VERSION}";
    const string MaaLegacyBundleOtaPackageNameTemplate = "MeoAssistantArknights_OTA_{VERSION}";
    const string MaaCorePackageNameTemplate = "MaaCore-{VERSION}";
    const string MaaResourcePackageNameTemplate = "MaaResource-{VERSION}";

    private string _version = "";
    private string _changeLog = "";

    private string MaaDevBundlePackageName => MaaDevBundlePackageNameTemplate.Replace("{VERSION}", _version);
    private string MaaLegacyBundlePackageName => MaaLegacyBundlePackageNameTemplate.Replace("{VERSION}", _version);
    private string MaaLegacyBundleOtaPackageName => MaaLegacyBundleOtaPackageNameTemplate.Replace("{VERSION}", _version);
    private string MaaCorePackageName => MaaCorePackageNameTemplate.Replace("{VERSION}", _version);
    private string MaaResourcePackageName => MaaResourcePackageNameTemplate.Replace("{VERSION}", _version);

    private string _latestTag = null;

    protected override void OnBuildInitialized()
    {
        Parameters = new BuildParameters(this);

        Information("==========================");

        Information("1. 工具链");
        Information($"Visual Studio 路径：{Parameters.VisualStudioPath ?? "Null"}");
        Information($"MSBuild 路径：{Parameters.MsBuildPath ?? "Null"}");

        Information("2. 仓库");
        Information($"主仓库：{Parameters.MainRepo ?? "Null"}");
        Information($"MaaResource 发布仓库：{Parameters.MaaResourceReleaseRepo ?? "Null"}");
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

    #region 清理

    Target UseCleanArtifact => _ => _
        .Executes(() =>
        {
            FileSystemTasks.EnsureCleanDirectory(Parameters.ArtifactOutput);
        });
    
    Target UseCleanRelease => _ => _
        .Executes(() =>
        {
            FileSystemTasks.EnsureCleanDirectory(Parameters.BuildOutput / BuildConfiguration.Release);
        });

    Target UseCleanReleaseOta => _ => _
        .Executes(() =>
        {
            FileSystemTasks.EnsureCleanDirectory(Parameters.BuildOutput / $"{BuildConfiguration.Release}_OTA");
        });
    
    #endregion

    #region 设置版本

    Target UseCommitVersion => _ => _
        .Triggers(SetVersion)
        .Executes(() =>
        {
            _version = $"{Parameters.BuildTime}-{Parameters.CommitHash}";
        });

    Target UseTagVersion => _ => _
        .Triggers(SetVersion)
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
            }
            else
            {
                Information("在 GitHub Actions 中运行");

                Assert.True(Parameters.GhTag is not null, "在 GitHub Actions 中运行，但是不存在 Tag");

                GitHubTasks.GitHubClient = new GitHubClient(new ProductHeaderValue(nameof(NukeBuild)))
                {
                    Credentials = new Credentials(Parameters.GitHubPersonalAccessToken)
                };

                _latestTag = Parameters.IsPreRelease ? GetLatestTag() : GetLatestReleaseTag();
                
                _version = Parameters.GhTag;
            }
        });

    Target SetVersion => _ => _
        .Executes(() =>
        {
            Information($"当前版本号设置为：{_version}");
        });

    #endregion

    #region 编译

    Target WithCompileCoreRelease => _ => _
        .DependsOn(UseCleanRelease)
        .After(SetVersion)
        .Executes(() =>
        {
            var versionEnv = $"/DMAA_VERSION=\\\"{_version}\\\"";
            Information($"MaaCore 编译环境变量：ExternalCompilerOptions = {versionEnv}");
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaCoreProject)
                .SetTargets("ReBuild")
                .SetConfiguration(BuildConfiguration.Release)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
                .SetProcessEnvironmentVariable("ExternalCompilerOptions", versionEnv)
            );
        });
    
    // TODO 在 MaaElectronUI 发布后移除
    Target WithCompileWpfRelease => _ => _
        .DependsOn(UseCleanRelease)
        .After(SetVersion)
        .Executes(() =>
        {
            MSBuild(c => c
                .SetProcessToolPath(Parameters.MsBuildPath)
                .SetProjectFile(Parameters.MaaWpfProject)
                .SetTargets("ReBuild")
                .SetConfiguration(BuildConfiguration.Release)
                .SetTargetPlatform(MSBuildTargetPlatform.x64)
                .EnableRestore()
            );
        });

    Target WithCompileResourceRelease => _ => _
        .DependsOn(UseCleanRelease)
        .After(SetVersion)
        .Executes(() =>
        {
            var output = Parameters.BuildOutput / BuildConfiguration.Release;
            var resourceFile = RootDirectory / "resource";
            var resourceFileThirdParty = RootDirectory / "3rdparty" / "resource";

            Information($"复制目录：{resourceFile} -> {output}");
            FileSystemTasks.CopyDirectoryRecursively(resourceFile, output,
                DirectoryExistsPolicy.Merge, FileExistsPolicy.OverwriteIfNewer);
            Information($"复制目录：{resourceFileThirdParty} -> {output}");
            FileSystemTasks.CopyDirectoryRecursively(resourceFileThirdParty, output,
                DirectoryExistsPolicy.Merge, FileExistsPolicy.OverwriteIfNewer);
        });

    #endregion

    #region 打包

    Target UseMaaDevBundle => _ => _
        .DependsOn(UseCleanArtifact, WithCompileCoreRelease, WithCompileWpfRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            RemoveDebugSymbols(buildOutput);
            
            BundlePackage(buildOutput, MaaDevBundlePackageName);
        });

    Target UseMaaLegacyBundle => _ => _
        .DependsOn(UseCleanArtifact, WithCompileCoreRelease, WithCompileWpfRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var releaseBuildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            RemoveDebugSymbols(releaseBuildOutput);

            BundlePackage(releaseBuildOutput, MaaLegacyBundlePackageName);
        });

    Target UseMaaLegacyBundleOta => _ => _
        .After(UseMaaLegacyBundle)
        .DependsOn(UseCleanArtifact, WithCompileCoreRelease, WithCompileWpfRelease, UseCleanReleaseOta)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var releaseBuildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            var otaPackageOutput = Parameters.BuildOutput / $"{BuildConfiguration.Release}_OTA";

            FileSystemTasks.CopyDirectoryRecursively(releaseBuildOutput, otaPackageOutput,
                DirectoryExistsPolicy.Merge, FileExistsPolicy.Overwrite);

            var ignoreFile = RootDirectory / ".maabundlerignore";
            if (ignoreFile.FileExists())
            {
                RemoveIgnoreFiles(otaPackageOutput, ignoreFile);
            }

            BundlePackage(otaPackageOutput, MaaLegacyBundleOtaPackageName);
        });

    Target UseMaaCore => _ => _
        .DependsOn(UseCleanArtifact, WithCompileCoreRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            RemoveDebugSymbols(buildOutput);
            RemoveReleaseResource(buildOutput);

            BundlePackage(buildOutput, MaaCorePackageName);
        });

    Target UseMaaResource => _ => _
        .DependsOn(UseCleanArtifact, WithCompileResourceRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            RemoveDebugSymbols(buildOutput);

            BundlePackage(buildOutput, MaaResourcePackageName);
        });

    Target SetPackageBundled => _ => _
        .After(UseMaaDevBundle, UseMaaLegacyBundle, UseMaaLegacyBundleOta, UseMaaCore, UseMaaResource)
        .Executes(() =>
        {
            Information("已完成打包");
            foreach (var file in Parameters.ArtifactOutput.GlobFiles("*.zip"))
            {
                var size = (new FileInfo(file).Length / 1024.0 / 1024.0).ToString("F3");
                Information($"找到 Artifact：{file}");
            }
        });
    
    #endregion

    #region 更新日志

    Target UseMaaChangeLog => _ => _
        .Triggers(SetMaaChangeLog)
        .After(UseTagVersion)
        .Executes(() =>
        {
            if (File.Exists(Parameters.MaaChangelogFile))
            {
                Information($"找到 {Parameters.MaaChangelogFile} 文件，读取内容作为更新日志");
                var text = File.ReadAllText(Parameters.MaaChangelogFile);
                _changeLog = text;
            }
            else
            {
                Warning($"未发现 {Parameters.MaaChangelogFile} 文件，将使用默认值");
            }
            
            if (_latestTag is null)
            {
                _changeLog += $"\n\n对应 Commit：[{Parameters.MainRepo}@{Parameters.CommitHash}](https://github.com/{Parameters.MainRepo}/commit/{Parameters.CommitHashFull})";
            }
            else
            {
                _changeLog += $"\n\n**Full Changelog**: [{Parameters.MainRepo}@{_latestTag} -> {Parameters.MainRepo}@{Parameters.GhTag}]https://github.com/{Parameters.MainRepo}/compare/{_latestTag}...{Parameters.GhTag}";
            }
        });

    Target UseMaaResourceChangeLog => _ => _
        .Triggers(SetMaaChangeLog)
        .Executes(() =>
        {
            if (File.Exists(Parameters.MaaResourceChangeLogFile))
            {
                Information($"找到 {Parameters.MaaResourceChangeLogFile} 文件，读取内容作为更新日志");
                var text = File.ReadAllText(Parameters.MaaResourceChangeLogFile);
                _changeLog = text;
            }
            else
            {
                Warning($"未发现 {Parameters.MaaResourceChangeLogFile} 文件，将使用默认值");
            }
            _changeLog += $"\n\n对应 Commit：[{Parameters.MainRepo}@{Parameters.CommitHash}](https://github.com/{Parameters.MainRepo}/commit/{Parameters.CommitHashFull})";
        });

    Target SetMaaChangeLog => _ => _
        .Executes(() =>
        {
            Information($"更新日志：\n================================\n{_changeLog}\n================================");
        });

    #endregion

    #region 发布

    Target UsePublishArtifact => _ => _
        .After(SetPackageBundled, SetMaaChangeLog)
        .Produces(RootDirectory / "artifacts" / "*.zip")
        .OnlyWhenStatic(() => GitHubActions != null);

    Target UsePublishRelease => _ => _
        .After(UsePublishArtifact)
        .OnlyWhenStatic(() => GitHubActions != null)
        .Executes(() =>
        {
            Information("在 GitHub Actions 中运行，执行发布 Release 任务");
            Information($"当前 Actions：{Parameters.GhActionName}");

            if (Parameters.GhActionName == ActionConfiguration.DevBuild)
            {
                Information($"DevBuild 跳过发布 Release");
                return;
            }

            if (GitHubTasks.GitHubClient is null)
            {
                GitHubTasks.GitHubClient = new GitHubClient(new ProductHeaderValue(nameof(NukeBuild)))
                {
                    Credentials = new Credentials(Parameters.GitHubPersonalAccessToken)
                };
            }

            if (Parameters.GhActionName == ActionConfiguration.ReleaseMaa)
            {
                Information($"运行 ReleaseMaa 将在 {Parameters.MainRepo} 创建 Release {Parameters.GhTag}");

                CreateGitHubRelease(Parameters.MainRepo, Parameters.CommitHashFull, _version);

                return;
            }

            if (Parameters.GhActionName == ActionConfiguration.ReleaseMaaCore)
            {
                Information($"运行 ReleaseMaaCore 将在 {Parameters.MainRepo} 创建 Release {Parameters.GhTag}");

                CreateGitHubRelease(Parameters.MainRepo, Parameters.CommitHashFull, _version);

                return;
            }

            if (Parameters.GhActionName == ActionConfiguration.ReleaseMaaResource)
            {
                Information($"运行 ReleaseMaaResource 将在 {Parameters.MaaResourceReleaseRepo} 创建 Release {_version}");

                CreateGitHubRelease(Parameters.MaaResourceReleaseRepo, "main", _version);

                return;
            }
        });

    #endregion

    #region Work Flow

    /// <summary>
    /// 见 <see cref="ActionConfiguration.DevBuild"/>
    /// </summary>
    Target DevBuild => _ => _
        .DependsOn(UseCommitVersion)
        .DependsOn(UseMaaDevBundle)
        .DependsOn(UsePublishArtifact);
        

    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaa"/>
    /// </summary>
    Target ReleaseMaa => _ => _
        .DependsOn(UseTagVersion)
        .DependsOn(UseMaaLegacyBundle)
        .DependsOn(UseMaaLegacyBundleOta)
        .DependsOn(UseMaaChangeLog)
        .DependsOn(UsePublishArtifact)
        .DependsOn(UsePublishRelease);

    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaaCore"/>
    /// </summary>
    Target ReleaseMaaCore => _ => _
        .DependsOn(UseTagVersion)
        .DependsOn(UseMaaCore)
        .DependsOn(UseMaaChangeLog)
        .DependsOn(UsePublishArtifact)
        .DependsOn(UsePublishRelease);

    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaaResource"/>
    /// </summary>
    Target ReleaseMaaResource => _ => _
        .DependsOn(UseCommitVersion)
        .DependsOn(UseMaaResource)
        .DependsOn(UseMaaResourceChangeLog)
        .DependsOn(UsePublishArtifact)
        .DependsOn(UsePublishRelease);


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

    #endregion

    #region Utilities

    private void RemoveIgnoreFiles(AbsolutePath baseDirectory, AbsolutePath ignoreFile)
    {
        var ignoreFileLines = File.ReadAllLines(ignoreFile);
        var ignorePattern = new List<string>();
        var keepPattern = new List<string>();
        foreach (var line in ignoreFileLines)
        {
            if (line.StartsWith("#"))
            {
                continue;
            }
            if (line.StartsWith("!"))
            {
                keepPattern.Add(line.Substring(1));
            }
            else
            {
                ignorePattern.Add(line);
            }
        }

        var match = new Matcher();
        // 这里反过来可以获取到需要排除的文件列表，而不是需要保留的文件列表
        match.AddExcludePatterns(keepPattern);
        match.AddIncludePatterns(ignorePattern);
        var ignoredFiles = match.GetResultsInFullPath(baseDirectory);
        foreach (var f in ignoredFiles)
        {
            FileSystemTasks.DeleteFile(f);
        }
    }
    
    private void BundlePackage(AbsolutePath input, string bundleName)
    {
        var packName = bundleName;
        if (packName.EndsWith(".zip") is false)
        {
            packName += ".zip";
        }
        var bundle = Parameters.ArtifactOutput / packName;

        Information($"编译输出：{input}");
        Information($"打包输出：{bundle}");

        Assert.True(Directory.Exists(input), $"输出目录 {input} 不存在");

        Information($"创建压缩文件：{bundle}");
        ZipFile.CreateFromDirectory(input, bundle, CompressionLevel.SmallestSize, false);
    }
    
    private void RemoveDebugSymbols(AbsolutePath outputDir)
    {
        var files = outputDir.GlobFiles("*.pdb", "*.lib", "*.exp", "*.exe.config");

        foreach (var file in files)
        {
            FileSystemTasks.DeleteFile(file);
        }
    }
    private void RemoveReleaseResource(AbsolutePath outputDir)
    {
        var resourceDir = outputDir / "resource";
        if (FileSystemTasks.Exists(resourceDir))
        {
            FileSystemTasks.DeleteDirectory(resourceDir);
            Information("移除了发布中的资源目录");
        }
        else
        {
            Warning("发布中的资源目录不存在");
        }
    }

    private void CreateGitHubRelease(string repo, string commitish, string releaseName)
    {
        var assets = Parameters.ArtifactOutput.GlobFiles("*.zip");
        
        var release = new NewRelease(_version)
        {
            TargetCommitish = commitish,
            Name = releaseName,
            Body = _changeLog,
            Draft = true,
            Prerelease = Parameters.IsPreRelease
        };
        var repoOwner = repo.Split('/')[0];
        var repoName = repo.Split('/')[1];

        var createdRelease = GitHubTasks.GitHubClient.Repository.Release.Create(repoOwner, repoName, release).Result;
        Information($"创建 Release {Parameters.GhTag} 成功");
        if (Parameters.IsPreRelease)
        {
            Information("当前为预发布版本");
        }
        
        foreach (var asset in assets)
        {
            UploadReleaseAssetToGitHub(createdRelease, asset);
        }

        var _ = GitHubTasks.GitHubClient.Repository.Release.Edit(repoOwner, repoName, createdRelease.Id, new ReleaseUpdate { Draft = false }).Result;
    }

    private void UploadReleaseAssetToGitHub(Release release, AbsolutePath asset)
    {
        if (new FileExtensionContentTypeProvider().TryGetContentType(asset, out var assetContentType) is false)
        {
            assetContentType = "application/x-binary";
        }

        var releaseAssetUpload = new ReleaseAssetUpload
        {
            ContentType = assetContentType,
            FileName = Path.GetFileName(asset),
            RawData = File.OpenRead(path: asset)
        };

        try
        {
            var _ = GitHubTasks.GitHubClient.Repository.Release.UploadAsset(release, releaseAssetUpload).Result;
        }
        catch
        {
            Assert.Fail($"上传文件 {asset} 到 GitHub 失败");
        }

        Information($"上传文件 {asset} 到 GitHub 成功");
    }
    
    private string GetLatestTag()
    {
        var latestRelease = GitHubTasks.GitHubClient.Repository.Release
                    .GetLatest(Parameters.MainRepo.Split('/')[0], Parameters.MainRepo.Split('/')[1])
                    .GetAwaiter().GetResult();
        Assert.True(latestRelease is not null, "获取最新发布版本失败");
        Assert.True(latestRelease.TagName is not null, "获取最新发布版本 TagName 失败");
        return latestRelease.TagName;
    }

    private string GetLatestReleaseTag()
    {
        var releases = GitHubTasks.GitHubClient.Repository.Release
            .GetAll(Parameters.MainRepo.Split('/')[0], Parameters.MainRepo.Split('/')[1], new ApiOptions { PageCount = 5 })
            .GetAwaiter().GetResult();

        var latestRelease = releases
            .FirstOrDefault(x => x.Prerelease is false && x.Draft is false);
        
        Assert.True(latestRelease is not null, "获取最新发布版本失败");
        Assert.True(latestRelease.TagName is not null, "获取最新发布版本 TagName 失败");        
        return latestRelease.TagName;
    }

    #endregion
}
