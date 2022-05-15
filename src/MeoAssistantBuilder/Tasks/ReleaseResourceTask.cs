using Cake.Common.Diagnostics;
using Cake.Frosting;
using MeoAssistantBuilder.Helper;
using System.IO.Compression;

namespace MeoAssistantBuilder.Tasks;

[TaskName("ReleaseResource")]
public sealed class ReleaseResourceTask : FrostingTask<MaaBuildContext>
{
    public override void Run(MaaBuildContext context)
    {
        context.Information("--------------------------------------------------");
        context.Information("1. Get bundle information");
        context.Information("--------------------------------------------------");

        var (hash, bt) = context.GetBuildInformation();
        context.CleanArtifacts();
        var version = $"{hash}-{bt}";
        context.Information($"MaaResource Version: {version}");

        context.Information("--------------------------------------------------");
        context.Information("2. Copy MaaResource");
        context.Information("--------------------------------------------------");

        var resDir = Path.Combine(Constants.MaaBuildOutputDirectory, "resource");
        if (Directory.Exists(resDir) is false)
        {
            Directory.CreateDirectory(resDir);
        }
        context.CopyResources(resDir);

        context.Information("--------------------------------------------------");
        context.Information("3. Bundle MaaResource");
        context.Information("--------------------------------------------------");

        var bundle = Path.Combine(Constants.MaaProjectArtifactDirectory, $"MaaResource-{version}.zip");
        ZipFile.CreateFromDirectory(resDir, bundle);

        context.Information($"Bundled MaaResource file: {bundle}");
    }
}
