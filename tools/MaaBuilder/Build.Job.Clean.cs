using Nuke.Common;
using Nuke.Common.IO;

namespace MaaBuilder;

public partial class Build
{
    Target UseClean => _ => _
        .Executes(() =>
        {
            FileSystemTasks.EnsureCleanDirectory(Parameters.ArtifactOutput);
            FileSystemTasks.EnsureCleanDirectory(Parameters.BuildOutput / BuildConfiguration.Release);

            foreach (var package in Parameters.Packages)
            {
                FileSystemTasks.EnsureCleanDirectory(Parameters.BuildOutput / package.PackageType);
            }
        });
}
