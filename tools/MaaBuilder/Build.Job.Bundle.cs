using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using MaaBuilder.Models;
using Nuke.Common;
using Nuke.Common.IO;

namespace MaaBuilder;

public partial class Build
{
    Target UseMaaDevBundle => _ => _
        .DependsOn(WithCompileCoreRelease, WithCompileWpfRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            BundlePackage(buildOutput, MaaDevBundlePackageName);
        });

    Target UseMaaRelease => _ => _
        .DependsOn(WithCompileCoreRelease, WithCompileWpfRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var releaseBuildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            RemoveDebugSymbols(releaseBuildOutput);

            foreach (var package in Parameters.Packages)
            {
                var name = package.NameTemplate.Replace("{VERSION}", Version);
                    
                BundleMaaBundle(releaseBuildOutput, name, package);
            }
        });

    Target SetPackageBundled => _ => _
        .After(UseMaaDevBundle)
        .Executes(() =>
        {
            Information("已完成打包");
            var checksumFile = Parameters.ArtifactOutput / "checksum.txt";
            var checksum = new List<string>();

            foreach (var file in Parameters.ArtifactOutput.GlobFiles("*.zip"))
            {
                var size = (new FileInfo(file).Length / 1024.0 / 1024.0).ToString("F3");
                var hash = FileSystemTasks.GetFileHash(file);
                Information($"找到 Artifact ({hash})：{file} ({size} MB)");

                checksum.Add($"{file.Name}:{hash}");
            }
            
            File.WriteAllLines(checksumFile, checksum, Encoding.UTF8);
        });
}
