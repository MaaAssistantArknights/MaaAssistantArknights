using Cake.Common.Build;
using Cake.Common.Diagnostics;
using Cake.Frosting;
using MeoAssistantBuilder.Helper;
using System.IO.Compression;
using System.Text.RegularExpressions;

namespace MeoAssistantBuilder.Tasks;

[TaskName("ReleaseCore")]
public sealed class ReleaseCoreTask : FrostingTask<MaaBuildContext>
{
    public override void Run(MaaBuildContext context)
    {
        context.Information("--------------------------------------------------");
        context.Information("1. Build MaaCore with CICD configuration");
        context.Information("--------------------------------------------------");

        context.CleanCore();
        context.CleanArtifacts();
        context.BuildCore("CICD");

        context.Information("--------------------------------------------------");
        context.Information("2. Remove extra files from build output");
        context.Information("--------------------------------------------------");

        var buildOutput = Path.Combine(Constants.MaaBuildOutputDirectory, "CICD");
        context.RemoveExtraCore(buildOutput);

        context.Information("--------------------------------------------------");
        context.Information("3. Copy 3rd party library DLLs to output directory");
        context.Information("--------------------------------------------------");

        var thirdPartyDlls = Directory.GetFiles(Path.Combine(Constants.MaaProjectThirdPartyDirectory, "bin"));
        foreach(var f in thirdPartyDlls)
        {
            var fileName = new FileInfo(f).Name;
            File.Copy(f, Path.Combine(buildOutput, fileName));
            context.Information($"Copy file: FROM {f} to {Path.Combine(buildOutput, fileName)}");
        }

        context.Information("--------------------------------------------------");
        context.Information("4. Read MaaCore version string");
        context.Information("--------------------------------------------------");

        var version = "UNKNOWN";
        var versionFile = Path.Combine(Constants.MaaCoreProjectDirectory, "Version.h");
        var versionFileContent = File.ReadAllText(versionFile);

        var regex = @"v((0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?)";

        var match = Regex.Match(versionFileContent, regex);
        if (match.Success is false)
        {
            context.Warning("Can not read version");
            Environment.Exit(-1);
        }
        else
        {
            version = match.Value;
        }

        var ghActions = context.GitHubActions();
        if (ghActions.IsRunningOnGitHubActions is false)
        {
            version += "-Local";
        }
        else
        {
            if (ghActions.Environment.Workflow.Workflow != "Release MaaCore")
            {
                version += $"-{ghActions.Environment.Workflow.RunId}";
            }
        }

        context.Information($"MaaCore Version: {version}");

        context.Information("--------------------------------------------------");
        context.Information("5. Bundle MaaCore DLLs");
        context.Information("--------------------------------------------------");

        var bundle = Path.Combine(Constants.MaaProjectArtifactDirectory, $"MaaCore-{version}.zip");
        ZipFile.CreateFromDirectory(buildOutput, bundle);

        context.Information($"Bundled MAACore DLLs file: {bundle}");
    }
}
