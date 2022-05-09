using Cake.Common.Build;
using Cake.Common.Diagnostics;
using Cake.Frosting;
using MeoAssistantBuilder.Helper;
using System.IO.Compression;
using System.Text.RegularExpressions;

namespace MeoAssistantBuilder.Tasks;

[TaskName("ReleaseBundle")]
public sealed class ReleaseBundleTask : FrostingTask<MaaBuildContext>
{
    public override void Run(MaaBuildContext context)
    {
        context.Information("--------------------------------------------------");
        context.Information("1. Read MaaCore version string");
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
            context.Information($"Running in GitHub Action with workflow name: {ghActions.Environment.Workflow.Workflow}, ref: {ghActions.Environment.Workflow.Ref}");
            if (ghActions.Environment.Workflow.Workflow != "Release MaaCore")
            {
                version += $"-{ghActions.Environment.Workflow.RunId}";
            }
            else 
            {
                var tag = ghActions.Environment.Workflow.Ref.Replace("refs/tags/", "");
                if (tag != version)
                {
                    context.Error($"Version do not match. From File: {version}. From Ref: {tag}");
                    Environment.Exit(-1);
                }
            }
        }

        context.Information($"MaaCore Version: {version}");

        context.Information("--------------------------------------------------");
        context.Information("2. Release MaaCore with third party dlls and resource");
        context.Information("--------------------------------------------------");

        var buildOutput = Path.Combine(Constants.MaaBuildOutputDirectory, "Release");
        context.CleanAll();
        context.CleanArtifacts();

        context.BuildCore("Release");
        context.RemoveExtraCore(buildOutput);

        context.Information("--------------------------------------------------");
        context.Information("3. Release MaaWpf with third party executables");
        context.Information("--------------------------------------------------");

        context.BuildWpf("Release");
        context.RemoveExtraWpf(buildOutput);

        context.Information("--------------------------------------------------");
        context.Information("4. Bundle MaaBundle package");
        context.Information("--------------------------------------------------");

        var bundle = Path.Combine(Constants.MaaProjectArtifactDirectory, $"MaaBundle-{version}.zip");
        ZipFile.CreateFromDirectory(buildOutput, bundle);

        context.Information($"Bundled MaaBundle file: {bundle}");
    }
}
