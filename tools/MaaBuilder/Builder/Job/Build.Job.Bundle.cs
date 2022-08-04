using System.IO;
using System.Text;
using System.Text.Json;
using Nuke.Common;

namespace MaaBuilder;

public partial class Build
{
    Target UseMaaDevBundle => _ => _
        .DependsOn(WithCompileCoreRelease, WithCompileWpfRelease)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            BundlePackage(buildOutput, MaaDevBundlePackageName, "DevBundle");
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
            var checksumFile = Parameters.ArtifactOutput / "checksum.json";

            foreach (var chs in ArtifactChecksums)
            {
                Information($"Artifact：{chs}");
            }

            var str = JsonSerializer.Serialize(ArtifactChecksums);
            
            // Encoding.Default => UTF8
            // Encoding.UTF8 => UTF8-with-BOM
            File.WriteAllText(checksumFile, str, Encoding.Default);
        });
}
