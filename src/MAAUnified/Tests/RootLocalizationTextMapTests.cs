using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services.Localization;

namespace MAAUnified.Tests;

public sealed class RootLocalizationTextMapTests
{
    [Fact]
    public void MissingKey_ShouldFallbackToKey_AndReportFallbackEvent()
    {
        var fallbacks = new List<LocalizationFallbackInfo>();
        var map = new RootLocalizationTextMap("Root.Localization.Tests");
        map.FallbackReported += fallbacks.Add;
        map.Language = "ja-jp";

        var value = map["Root.Unknown.Key"];

        Assert.Equal("Root.Unknown.Key", value);
        var fallback = Assert.Single(fallbacks);
        Assert.Equal("Root.Localization.Tests", fallback.Scope);
        Assert.Equal("ja-jp", fallback.Language);
        Assert.Equal("Root.Unknown.Key", fallback.Key);
        Assert.Equal("key", fallback.FallbackSource);
    }

    [Fact]
    public void KnownKey_ShouldResolveFromCurrentLocaleDictionary()
    {
        var map = new RootLocalizationTextMap();
        map.Language = "en-us";

        Assert.Equal("Farming", map["Main.Tab.TaskQueue"]);

        map.Language = "zh-cn";
        Assert.Equal("一键长草", map["Main.Tab.TaskQueue"]);
        Assert.Equal("自动战斗", map["Main.Tab.Copilot"]);
        Assert.Equal("小工具", map["Main.Tab.Toolbox"]);
    }

    [Fact]
    public void TaskQueueSettingsModeKeys_ShouldResolveForZhAndEn()
    {
        var map = new RootLocalizationTextMap();

        map.Language = "zh-cn";
        Assert.Equal("常规设置", map["TaskQueue.Root.GeneralSettings"]);
        Assert.Equal("高级设置", map["TaskQueue.Root.AdvancedSettings"]);

        map.Language = "en-us";
        Assert.Equal("General", map["TaskQueue.Root.GeneralSettings"]);
        Assert.Equal("Advanced", map["TaskQueue.Root.AdvancedSettings"]);
        Assert.Equal("Clear", map["TaskQueue.Root.Clear"]);
        Assert.Equal("Left click", map["TaskQueue.Root.LeftClick"]);
        Assert.Equal("Task settings", map["TaskQueue.Root.TaskSettings"]);
    }

    [Fact]
    public void TaskQueueBatchModeKeys_ShouldResolveForZhAndEn()
    {
        var map = new RootLocalizationTextMap();

        map.Language = "zh-cn";
        Assert.Equal("清空", map["TaskQueue.Root.Clear"]);
        Assert.Equal("切换为{0}", map["TaskQueue.Root.SwitchBatchMode"]);
        Assert.Equal("左键", map["TaskQueue.Root.LeftClick"]);
        Assert.Equal("右键", map["TaskQueue.Root.RightClick"]);
        Assert.Equal("任务设置", map["TaskQueue.Root.TaskSettings"]);

        map.Language = "en-us";
        Assert.Equal("Clear", map["TaskQueue.Root.Clear"]);
        Assert.Equal("Switch to {0}", map["TaskQueue.Root.SwitchBatchMode"]);
        Assert.Equal("Left click", map["TaskQueue.Root.LeftClick"]);
        Assert.Equal("Right click", map["TaskQueue.Root.RightClick"]);
        Assert.Equal("Task settings", map["TaskQueue.Root.TaskSettings"]);
    }

    [Fact]
    public void SettingsActionKeys_ShouldMatchWpfBaselineForZhAndEn()
    {
        var map = new RootLocalizationTextMap("Root.Localization.Settings");

        map.Language = "zh-cn";
        Assert.Equal("测试连接", map["Settings.Action.TestRemote"]);
        Assert.Equal("查看成就", map["Settings.Action.ShowAchievement"]);
        Assert.Equal("生成日志压缩包", map["Settings.Action.BuildIssueReport"]);
        Assert.Equal("打开日志文件夹", map["Settings.Action.OpenDebugDirectory"]);
        Assert.Equal("查看公告", map["Settings.Action.CheckAnnouncement"]);

        map.Language = "en-us";
        Assert.Equal("Test Connection", map["Settings.Action.TestRemote"]);
        Assert.Equal("View Achievements", map["Settings.Action.ShowAchievement"]);
        Assert.Equal("Generate Support Payload", map["Settings.Action.BuildIssueReport"]);
        Assert.Equal("Open Debug Folder", map["Settings.Action.OpenDebugDirectory"]);
        Assert.Equal("View Announcement", map["Settings.Action.CheckAnnouncement"]);
    }
}
