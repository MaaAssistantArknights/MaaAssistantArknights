using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text.Json;
using MaaBuilder.Models;
using Microsoft.Extensions.FileSystemGlobbing;
using Nuke.Common;
using Nuke.Common.IO;

namespace MaaBuilder;

public partial class Build
{
    void BundlePackage(AbsolutePath input, string bundleName)
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
    
    static void RemoveIgnoredFiles(AbsolutePath baseDirectory, IEnumerable<string> ignorePattern)
    {
        var match = new Matcher();
        
        // 这里反过来可以获取到需要排除的文件列表，而不是需要保留的文件列表
        match.AddIncludePatterns(ignorePattern);
        
        var ignoredFiles = match.GetResultsInFullPath(baseDirectory);
        foreach (var f in ignoredFiles)
        {
            FileSystemTasks.DeleteFile(f);
        }
    }

    static void CopyIncludeFiles(AbsolutePath originalDirectory, AbsolutePath targetDirectory, IEnumerable<string> includePattern)
    {
        var match = new Matcher();
        match.AddIncludePatterns(includePattern);
        var includeFiles = match.GetResultsInFullPath(originalDirectory);

        foreach (var file in includeFiles)
        {
            var relative = file.Replace(originalDirectory, string.Empty);
            var targetPath = targetDirectory / relative;
            
            FileSystemTasks.CopyFile(file, targetPath, FileExistsPolicy.Overwrite);
        }
    }
    
    static void RemoveDebugSymbols(AbsolutePath outputDir)
    {
        var files = outputDir.GlobFiles("*.pdb", "*.lib", "*.exp", "*.exe.config");

        foreach (var file in files)
        {
            FileSystemTasks.DeleteFile(file);
        }
    }

    static T GetPackageBundlerConfiguration<T>(Package package) where T : class, IPackageConfiguration
    {
        var configString = package.Configuration["configuration"].ToString().NotNull();
        var config = JsonSerializer.Deserialize<T>(configString).NotNull();
        return config;
    }

    void BundleMaaBundle(AbsolutePath buildOutput, string bundleName)
    {
        BundlePackage(buildOutput, bundleName);
    }

    void BundleMaaBundleOta(AbsolutePath buildOutput, string bundleName, Package package)
    {
        var context = Parameters.BuildOutput / package.PackageType.ToString();
        FileSystemTasks.CopyDirectoryRecursively(buildOutput, context, DirectoryExistsPolicy.Merge);

        var config = GetPackageBundlerConfiguration<PackageMaaBundleOta>(package);

        RemoveIgnoredFiles(context, config.Exclude);
        BundlePackage(context, bundleName);
    }

    void BundleMaaCore(AbsolutePath buildOutput, string bundleName, Package package)
    {
        var context = Parameters.BuildOutput / package.PackageType.ToString();
        FileSystemTasks.EnsureExistingDirectory(context);
        
        var config = GetPackageBundlerConfiguration<PackageMaaCore>(package);
        
        CopyIncludeFiles(buildOutput, context, config.Include);
        BundlePackage(context, bundleName);
    }

    void BundleMaaDependency(AbsolutePath buildOutput, string bundleName, Package package)
    {
        var context = Parameters.BuildOutput / package.PackageType.ToString();
        FileSystemTasks.EnsureExistingDirectory(context);
        
        var config = GetPackageBundlerConfiguration<PackageMaaDependency>(package);
        
        CopyIncludeFiles(buildOutput, context, config.Include);
        BundlePackage(context, bundleName);
    }
    
    void BundleMaaDependencyNoAvx(AbsolutePath buildOutput, string bundleName, Package package)
    {
        var context = Parameters.BuildOutput / package.PackageType.ToString();
        FileSystemTasks.EnsureExistingDirectory(context);
        
        var config = GetPackageBundlerConfiguration<PackageMaaDependencyNoAvx>(package);
        
        CopyIncludeFiles(buildOutput, context, config.Include);

        var noAvxBundle = RootDirectory / config.NoAvxBundle;
        ZipFile.ExtractToDirectory(noAvxBundle, context, overwriteFiles: true);
        
        BundlePackage(context, bundleName);
    }
}
