using Nuke.Common;

namespace MaaBuilder;

public partial class Build
{
    Target DevBuildDefault => _ => _
        .OnlyWhenStatic(() => IsReleaseSimulation == false)
        .WhenSkipped(DependencyBehavior.Skip)
        .DependsOn(UseCommitVersion)
        .DependsOn(UseMaaDevBundle)
        .DependsOn(UsePublishArtifact);

    Target DevBuildReleaseSimulation => _ => _
        .OnlyWhenStatic(() => IsReleaseSimulation == true)
        .WhenSkipped(DependencyBehavior.Skip)
        .DependsOn(UseRsVersion)
        .DependsOn(UseMaaRelease)
        .DependsOn(UseMaaChangeLog)
        .DependsOn(UsePublishArtifact);

    /// <summary>
    /// 见 <see cref="ActionConfiguration.DevBuild"/>
    /// </summary>
    Target DevBuild => _ => _
        .DependsOn(DevBuildDefault)
        .DependsOn(DevBuildReleaseSimulation);


    /// <summary>
    /// 见 <see cref="ActionConfiguration.ReleaseMaa"/>
    /// </summary>
    Target ReleaseMaa => _ => _
        .DependsOn(UseTagVersion)
        .DependsOn(UseMaaRelease)
        .DependsOn(UseMaaChangeLog)
        .DependsOn(UsePublishArtifact)
        .DependsOn(UsePublishRelease);
}
