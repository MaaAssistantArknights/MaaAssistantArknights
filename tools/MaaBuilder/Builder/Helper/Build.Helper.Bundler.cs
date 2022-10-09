using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using MaaBuilder.Models;
using Microsoft.Extensions.FileSystemGlobbing;
using Nuke.Common;
using Nuke.Common.IO;
using Serilog;

namespace MaaBuilder;

public partial class Build
{
    void BundlePackage(AbsolutePath input, string bundleName, string packageType)
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

        var sourceDirectory = new DirectoryInfo(input);
        var allFiles = sourceDirectory.GetFiles("*.*", SearchOption.AllDirectories);
        var allHashes = allFiles
            .Select(x => FileSystemTasks.GetFileHash(x.FullName).ToUpperInvariant() +
                         ((AbsolutePath)x.FullName).GetUnixRelativePathTo(input))
            .Select(GetStringMd5)
            .OrderBy(x => x);
        var hashCombinedBuilder = new StringBuilder();
        foreach (var h in allHashes)
        {
            hashCombinedBuilder.Append(h);
        }
        var combinedHashString = hashCombinedBuilder.ToString();
        var hash = GetStringMd5(combinedHashString);

        var bundleHash = FileSystemTasks.GetFileHash(bundle).ToUpperInvariant();
        
        ArtifactChecksums.Add(new Checksum
        {
            FileHash = bundleHash,
            FileIdentity = hash,
            FileName = packName,
            PackageType = packageType
        });
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
        var files = outputDir.GlobFiles("*.pdb", "*.lib", "*.exp", "*.exe.config", "*.xml");

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

        if (package.PackageType != "MaaDebugSymbol")
        {
            exclude = Enumerable.Concat(exclude, new[] { "*.pdb", "*.lib", "*.exp", "*.exe.config", "*.xml" });
        }

        MoveGlobFiles(buildOutput, tempDir, include, exclude);

        if (string.IsNullOrEmpty(noAvxBundle) is false)
        {
            var file = RootDirectory / noAvxBundle;
            ZipFile.ExtractToDirectory(file, tempDir, true);
        }
        
        BundlePackage(tempDir, bundleName, package.PackageType);
    }
    
    static string GetStringMd5(string input)
    {
        using var md5 = MD5.Create();
        var inputBytes = Encoding.ASCII.GetBytes(input);
        var hashBytes = md5.ComputeHash(inputBytes);
        return Convert.ToHexString(hashBytes);
    }
}
