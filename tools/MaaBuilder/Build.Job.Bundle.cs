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
                var type = package.PackageType;
                var name = package.NameTemplate.Replace("{VERSION}", Version);

                switch (type)
                {
                    case PackageTypes.MaaBundle:
                        BundleMaaBundle(releaseBuildOutput, name);
                        break;
                    case PackageTypes.MaaBundleOta:
                        BundleMaaBundleOta(releaseBuildOutput, name, package);
                        break;
                    case PackageTypes.MaaCore:
                        BundleMaaCore(releaseBuildOutput, name, package);
                        break;
                    case PackageTypes.MaaDependency:
                        BundleMaaDependency(releaseBuildOutput, name, package);
                        break;
                    case PackageTypes.MaaDependencyNoAvx:
                        BundleMaaDependencyNoAvx(releaseBuildOutput, name, package);
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        });

    Target UseMaaResource => _ => _
        .DependsOn(WithCompileResourceRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            RemoveDebugSymbols(buildOutput);

            BundlePackage(buildOutput, MaaResourcePackageName);
        });

    Target SetPackageBundled => _ => _
        .After(UseMaaDevBundle, UseMaaResource)
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
