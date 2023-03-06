using System.IO;
using System.Text;
using System.Text.Json;
using Nuke.Common;

namespace MaaBuilder;

public partial class Build
{
    private Target UseMaaDevBundle => _ => _
        .DependsOn(WithCompileCoreRelease, WithCompileWpfRelease, WithSyncRes)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var buildOutput = Parameters.BuildOutput / BuildConfiguration.Release;
            BundlePackage(buildOutput, MaaDevBundlePackageName, "DevBundle");
        });

    private Target UseMaaRelease => _ => _
        .DependsOn(WithCompileCoreRelease, WithCompileWpfRelease, WithSyncRes)
        .Triggers(SetPackageBundled)
        .Executes(() =>
        {
            var releaseBuildOutput = Parameters.BuildOutput / BuildConfiguration.Release;

            foreach (var package in Parameters.Packages)
            {
                var name = package.NameTemplate.Replace("{VERSION}", Version).Replace("{PLATFORM}", Parameters.TargetPlatform.ToLower());

                BundleMaaBundle(releaseBuildOutput, name, package);
            }
        });

    private Target SetPackageBundled => _ => _
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
