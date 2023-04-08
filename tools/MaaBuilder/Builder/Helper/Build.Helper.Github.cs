using System.IO;
using Microsoft.AspNetCore.StaticFiles;
using Nuke.Common;
using Nuke.Common.IO;
using Nuke.Common.Tools.GitHub;
using Octokit;

namespace MaaBuilder;

public partial class Build
{
    
    void CreateOrUpdateGitHubRelease(string repo, string commitish, string releaseName)
    {
        var assets = Parameters.ArtifactOutput.GlobFiles("*.zip");
        
        var release = new NewRelease(Version)
        {
            TargetCommitish = commitish,
            Name = releaseName,
            Body = ChangeLog,
            Draft = true,
            Prerelease = Parameters.IsPreRelease
        };
        var repoOwner = repo.Split('/')[0];
        var repoName = repo.Split('/')[1];

        Release createdRelease;
        try {
            createdRelease = GitHubTasks.GitHubClient.Repository.Release.Create(repoOwner, repoName, release).GetAwaiter().GetResult();
            Information($"创建 Release {Parameters.GhTag} 成功");
        } catch(ApiException) {
            // release exists
            createdRelease = GitHubTasks.GitHubClient.Repository.Release.Get(repoOwner, repoName, Parameters.GhTag).GetAwaiter().GetResult();
            Information($"获取 Release {Parameters.GhTag} 成功");
        }
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

    void UploadReleaseAssetToGitHub(Release release, AbsolutePath asset)
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
    
    string GetLatestTag()
    {
        var releases = GitHubTasks.GitHubClient.Repository.Release
            .GetAll(Parameters.MainRepo.Split('/')[0], 
                    Parameters.MainRepo.Split('/')[1],
                    new ApiOptions { PageCount = 5 })
            .GetAwaiter().GetResult();

        Assert.True(releases.Count > 0, "获取最新发布版本失败");

        var latestRelease = releases[0];

        Assert.True(latestRelease is not null, "获取最新发布版本失败");
        Assert.True(latestRelease.TagName is not null, "获取最新发布版本 TagName 失败");
        return latestRelease.TagName;
    }

    string GetLatestReleaseTag()
    {
        var latestRelease = GitHubTasks.GitHubClient.Repository.Release
            .GetLatest(Parameters.MainRepo.Split('/')[0], Parameters.MainRepo.Split('/')[1])
            .GetAwaiter().GetResult();
        Assert.True(latestRelease is not null, "获取最新发布版本失败");
        Assert.True(latestRelease.TagName is not null, "获取最新发布版本 TagName 失败");
        return latestRelease.TagName;
    }
}
