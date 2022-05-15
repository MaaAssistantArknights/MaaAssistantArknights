using Cake.Common.Diagnostics;
using Cake.Frosting;
using MeoAssistantBuilder.Helper;
using System.IO.Compression;

namespace MeoAssistantBuilder.Tasks;

[TaskName("DevBuild")]
public sealed class DevBuildTask : FrostingTask<MaaBuildContext>
{
    public override void Run(MaaBuildContext context)
    {
        context.Information("--------------------------------------------------");
        context.Information("1. Get build information");
        context.Information("--------------------------------------------------");

        context.CleanArtifacts();
        var (bt, hash) = context.GetBuildInformation();
        var artifact = $"MaaBundle-DevBuild-(CONF)-{hash}-{bt}.zip";
        context.Information($"Dev build of commit {hash} at {bt}");

        context.Information("--------------------------------------------------");
        context.Information("2. Build MaaBundle with configuration Release");
        context.Information("--------------------------------------------------");

        context.CleanAll();

        context.BuildCore("Release");
        context.BuildWpf("Release");
        var releaseOutput = Path.Combine(Constants.MaaBuildOutputDirectory, "Release");
        var releaseArtifact = Path.Combine(Constants.MaaProjectArtifactDirectory, artifact.Replace("(CONF)", "Release"));
        ZipFile.CreateFromDirectory(releaseOutput, releaseArtifact);

        context.Information("--------------------------------------------------");
        context.Information("3. Build MaaBundle with configuration RelWithDebInfo");
        context.Information("--------------------------------------------------");

        context.CleanAll();

        context.BuildCore("RelWithDebInfo");
        context.BuildWpf("RelWithDebInfo");
        var releaseDebugOutput = Path.Combine(Constants.MaaBuildOutputDirectory, "RelWithDebInfo");
        var releaseDebugArtifact = Path.Combine(Constants.MaaProjectArtifactDirectory, artifact.Replace("(CONF)", "RelWithDebInfo"));
        ZipFile.CreateFromDirectory(releaseDebugOutput, releaseDebugArtifact);

        context.Information("--------------------------------------------------");
        context.Information("4. Build MaaBundle with configuration CICD");
        context.Information("--------------------------------------------------");

        context.CleanAll();

        context.BuildCore("CICD");
        context.BuildWpf("CICD");
        var releaseCiOutput = Path.Combine(Constants.MaaBuildOutputDirectory, "CICD");
        var releaseCiArtifact = Path.Combine(Constants.MaaProjectArtifactDirectory, artifact.Replace("(CONF)", "CICD"));
        context.CopyThirdPartyDlls(releaseCiOutput);
        context.CopyResources(releaseCiOutput);
        ZipFile.CreateFromDirectory(releaseCiOutput, releaseCiArtifact);
    }
}
