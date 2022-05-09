using Cake.Common.Build;
using Cake.Common.Diagnostics;
using Cake.Frosting;
using MeoAssistantBuilder.Helper;
using System.IO.Compression;

namespace MeoAssistantBuilder.Tasks;

[TaskName("ReleaseWpf")]
public sealed class ReleaseWpfTask : FrostingTask<MaaBuildContext>
{
    public override void Run(MaaBuildContext context)
    {
        context.Information("--------------------------------------------------");
        context.Information("1. Build MaaWpf with ReleaseCore configuration");
        context.Information("--------------------------------------------------");

        var (hash, bt) = context.GetBuildInformation();
        context.CleanWpf();
        context.CleanArtifacts();
        context.BuildWpf("CICD");

        context.Information("--------------------------------------------------");
        context.Information("2. Remove extra files from build output");
        context.Information("--------------------------------------------------");

        var buildOutput = Path.Combine(Constants.MaaBuildOutputDirectory, "CICD");
        context.RemoveExtraWpf(buildOutput);

        context.Information("--------------------------------------------------");
        context.Information("3. Set MaaWpf Version Number");
        context.Information("--------------------------------------------------");

        var version = $"{hash}-{bt}";
        context.Information($"MaaWpf Version: {version}");

        context.Information("--------------------------------------------------");
        context.Information("4. Bundle MaaWpf Executables");
        context.Information("--------------------------------------------------");

        var bundle = Path.Combine(Constants.MaaProjectArtifactDirectory, $"MaaWpf-{version}.zip");
        ZipFile.CreateFromDirectory(buildOutput, bundle);

        context.Information($"Bundled MaaWpf Executables file: {bundle}");

        context.Information("--------------------------------------------------");
        context.Information("5. Upload Artifact");
        context.Information("--------------------------------------------------");

        var gh = context.GitHubActions();
        if (gh.IsRunningOnGitHubActions)
        {
            context.Information("Upload artifacts to GitHub Actions");
            gh.Commands.UploadArtifact(Cake.Core.IO.FilePath.FromString(bundle), $"MaaWpf-{version}.zip").Wait();
        }
        else
        {
            context.Information("Skip");
        }
    }
}
