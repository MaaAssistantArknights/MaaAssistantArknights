using Cake.Common.Diagnostics;
using Cake.Common.IO;

namespace MeoAssistantBuilder.Helper;
public static class CleanDirectoryHelper
{
    public static void CleanArtifacts(this MaaBuildContext context)
    {
        context.CleanDirectory(Constants.MaaProjectArtifactDirectory);
        if (Directory.Exists(Constants.MaaProjectArtifactDirectory))
        {
            Directory.CreateDirectory(Constants.MaaProjectArtifactDirectory);
        }
    }

    public static void CleanCore(this MaaBuildContext context)
    {
        context.CleanDirectory(Constants.MaaBuildOutputDirectory);
        context.CleanDirectory(Constants.MaaCoreBuildIntermediatOutputDirectory);
    }

    public static void CleanWpf(this MaaBuildContext context)
    {
        context.CleanDirectory(Constants.MaaBuildOutputDirectory);
        context.CleanDirectory(Path.Combine(Constants.MaaWpfProjectDirectory, "bin"));
        context.CleanDirectory(Path.Combine(Constants.MaaWpfProjectDirectory, "obj"));
    }

    public static void CleanAll(this MaaBuildContext context)
    {
        CleanCore(context);
        CleanWpf(context);
    }

    public static void RemoveExtraCore(this MaaBuildContext context, string buildOutput)
    {
        foreach (var extra in Constants.MaaCoreExtraFiles)
        {
            var file = Path.Combine(buildOutput, extra);
            if (File.Exists(file))
            {
                File.Delete(file);
                context.Information($"Delete file: {file}");
            }
        }
    }

    public static void RemoveExtraWpf(this MaaBuildContext context, string buildOutput)
    {
        foreach (var extra in Constants.MaaWpfExtraFiles)
        {
            var file = Path.Combine(buildOutput, extra);
            if (File.Exists(file))
            {
                File.Delete(file);
                context.Information($"Delete file: {file}");
            }
        }
    }
}
