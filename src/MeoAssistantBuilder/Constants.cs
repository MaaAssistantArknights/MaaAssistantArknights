namespace MeoAssistantBuilder;

public static class Constants
{
    public const string MaaProjectRootDirectory = "../../";
    public const string MaaProjectThirdPartyDirectory = "../../3rdparty";
    public const string MaaProjectResourceDirectory = "../../resource";
    public const string MaaProjectArtifactDirectory = "../../artifacts";

    public const string MaaCoreProjectDirectory = "../MeoAssistant";
    public const string MaaWpfProjectDirectory = "../MeoAsstGui";

    public const string MaaCoreProjectFilePath = $"{MaaCoreProjectDirectory}/MeoAssistant.vcxproj";
    public const string MaaWpfProjectFilePath = $"{MaaWpfProjectDirectory}/MeoAsstGui.csproj";

    public const string MaaBuildOutputDirectory = $"{MaaProjectRootDirectory}/x64";
   
    public const string MaaCoreBuildIntermediatOutputDirectory = $"{MaaCoreProjectDirectory}/x64";

    public static readonly string[] MaaCoreExtraFiles = new [] { "MeoAssistant.pdb", "MeoAssistant.lib", "MeoAssistant.exp" };

    public static readonly string[] MaaWpfExtraFiles = new[] { "MeoAsstGui.pdb", "MeoAsstGui.exe.config" };
}
