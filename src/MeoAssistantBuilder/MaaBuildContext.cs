using Cake.Common;
using Cake.Common.Tools.VSWhere;
using Cake.Core;
using Cake.Core.Diagnostics;
using Cake.Frosting;

namespace MeoAssistantBuilder;

public class MaaBuildContext : FrostingContext
{
    public string MsBuildExecutable { get; set; }

    public MaaBuildContext(ICakeContext context) : base(context)
    {
        MsBuildExecutable = context.Argument<string>("tool", "");
        if (MsBuildExecutable == "")
        {
            var vsPath = context.VSWhereLatest();
            if (vsPath == null)
            {
                context.Log.Write(Verbosity.Normal, LogLevel.Fatal, "Can not find Visual Studio install path");
                System.Environment.Exit(-1);
            }
            MsBuildExecutable = Path.Combine(vsPath.FullPath, "Msbuild\\Current\\Bin\\MsBuild.exe");
        }
        context.Log.Information($"Use MsBuild at: {MsBuildExecutable}");
    }
}
