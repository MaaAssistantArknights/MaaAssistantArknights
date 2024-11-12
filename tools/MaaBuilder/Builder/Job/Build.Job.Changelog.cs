using System.IO;
using Nuke.Common;

namespace MaaBuilder;

public partial class Build
{
    Target UseMaaChangeLog => _ => _
        .Triggers(SetMaaChangeLog)
        .After(UseTagVersion)
        .Executes(() =>
        {
            if (File.Exists(Parameters.MaaChangelogFile))
            {
                Information($"找到 {Parameters.MaaChangelogFile} 文件，读取内容作为更新日志");
                var text = File.ReadAllText(Parameters.MaaChangelogFile);
                ChangeLog = text;
            }
            else
            {
                Warning($"未发现 {Parameters.MaaChangelogFile} 文件，将使用默认值");
            }
            
            if (LatestTag is null)
            {
                ChangeLog += $"\n\n对应 Commit：[{Parameters.MainRepo}@{Parameters.CommitHash}](https://github.com/{Parameters.MainRepo}/commit/{Parameters.CommitHashFull})";
            }
            else
            {
                ChangeLog += $"\n\n**Full Changelog**: [{LatestTag} -> {Parameters.GhTag}](https://github.com/{Parameters.MainRepo}/compare/{LatestTag}...{Parameters.GhTag})";
            }
        });

    Target SetMaaChangeLog => _ => _
        .Executes(() =>
        {
            Information($"更新日志：\n================================\n{ChangeLog}\n================================");
        });
}
