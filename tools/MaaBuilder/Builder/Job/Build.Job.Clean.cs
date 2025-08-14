using Nuke.Common;
using Nuke.Common.IO;

namespace MaaBuilder;

public partial class Build
{
    Target UseClean => _ => _
        .Executes(() =>
        {
            Parameters.ArtifactOutput.CreateOrCleanDirectory();
            (Parameters.BuildOutput / BuildConfiguration.Release).CreateOrCleanDirectory();

            foreach (var package in Parameters.Packages)
            {
                (Parameters.BuildOutput / package.PackageType).CreateOrCleanDirectory();
            }
        });
}
