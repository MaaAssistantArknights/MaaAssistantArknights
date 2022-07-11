using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
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

        Information($"输出目录：{input}");
        Information($"打包输出：{bundle}");

        Assert.True(Directory.Exists(input), $"输出目录 {input} 不存在");

        Information($"创建压缩文件：{bundle}");
        ZipFile.CreateFromDirectory(input, bundle, CompressionLevel.SmallestSize, false);
    }
    
    static void MoveGlobFiles(AbsolutePath source, AbsolutePath target, IEnumerable<string> include, IEnumerable<string> exclude)
    {
        var iMatch = new Matcher();
        var eMatch = new Matcher();
        
        iMatch.AddIncludePatterns(include);
        eMatch.AddIncludePatterns(exclude);

        var includeFiles = iMatch.GetResultsInFullPath(source).ToList();
        var excludeFiles = eMatch.GetResultsInFullPath(source).ToList();

        includeFiles.RemoveAll(x => excludeFiles.Contains(x));
        
        foreach (var file in includeFiles)
        {
            var relative = file.Replace(source, string.Empty);
            var targetPath = target / relative;
            
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

    void BundleMaaBundle(AbsolutePath buildOutput, string bundleName, Package package)
    {
        var tempDir = buildOutput.Parent / package.PackageType;
        FileSystemTasks.EnsureExistingDirectory(tempDir);
        
        var include = package.Configuration.Include;
        var exclude = package.Configuration.Exclude;
        var noAvxBundle = package.Configuration.NoAvxBundle;
        
        MoveGlobFiles(buildOutput, tempDir, include, exclude);

        if (string.IsNullOrEmpty(noAvxBundle) is false)
        {
            var file = RootDirectory / noAvxBundle;
            ZipFile.ExtractToDirectory(file, tempDir, true);
        }
        
        BundlePackage(tempDir, bundleName);
    }
}
