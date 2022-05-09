using Cake.Common.Diagnostics;
using Cake.Common.IO;

namespace MeoAssistantBuilder.Helper
{
    public static class FileCopyHelper
    {
        public static void CopyThirdPartyDlls(this MaaBuildContext context, string outputDir)
        {
            var thirdPartyDlls = Directory.GetFiles(Path.Combine(Constants.MaaProjectThirdPartyDirectory, "bin"));
            foreach (var f in thirdPartyDlls)
            {
                var fileName = new FileInfo(f).Name;
                context.CopyFile(f, Path.Combine(outputDir, fileName));
                context.Information($"Copy file: FROM {f} to {Path.Combine(outputDir, fileName)}");
            }
        }

        public static void CopyResources(this MaaBuildContext context, string outputDir)
        {
            var resourceTargetDir = Path.Combine(outputDir, "resource");
            var resourceThirdParty = Path.Combine(Constants.MaaProjectRootDirectory, "3rdparty\\resource");
            var resoutceMaa = Path.Combine(Constants.MaaProjectRootDirectory, "resource");

            var files = new List<string>();
            var directories = new List<string>();

            files.AddRange(Directory.GetFiles(resourceThirdParty));
            files.AddRange(Directory.GetFiles(resoutceMaa));
            directories.AddRange(Directory.GetDirectories(resourceThirdParty));
            directories.AddRange(Directory.GetDirectories(resoutceMaa));

            if (Directory.Exists(resourceTargetDir) is false)
            {
                Directory.CreateDirectory(resourceTargetDir);
            }

            foreach (var f in files)
            {
                var fileName = new FileInfo(f).Name;
                context.CopyFile(f, Path.Combine(resourceTargetDir, fileName));
                context.Information($"Copy file: FROM {f} TO {Path.Combine(resourceTargetDir, fileName)}");
            }

            foreach (var d in directories)
            {
                var dirName = new DirectoryInfo(d).Name;
                context.CopyDirectory(d, Path.Combine(resourceTargetDir, dirName));
                context.Information($"Copy directory: FROM {d} TO {Path.Combine(resourceTargetDir, dirName)}");
            }
        }
    }
}
