namespace MAAUnified.Tests;

public sealed class BaselineTargetPackageClosureTests
{
    private static readonly string[] TargetItemIds =
    [
        "TaskQueueRoot",
        "System.Connect",
        "System.TrayMenu.HideTray",
        "System.TrayMenu.Restart",
        "Task.StartUp",
        "Task.Fight",
        "Task.Recruit",
        "Task.Infrast",
        "Task.Mall",
        "Task.Award",
        "Task.PostAction",
        "Task.Roguelike",
        "Task.Reclamation",
        "Task.Custom",
        "Settings.Connect",
        "Settings.Game",
        "Settings.Start",
        "Settings.Performance",
        "Settings.Timer",
        "Settings.Gui",
        "Settings.Background",
        "Settings.HotKey",
        "Settings.RemoteControl",
        "Settings.ExternalNotification",
    ];

    private static readonly string[] TargetMappingOwners =
    [
        "Task.StartUp",
        "Task.Fight",
        "Task.Recruit",
        "Task.Infrast",
        "Task.Mall",
        "Task.Award",
        "Task.PostAction",
        "Task.Roguelike",
        "Task.Reclamation",
        "Task.Custom",
        "Settings.Connect",
        "Settings.Game",
        "Settings.Start",
        "Settings.Performance",
        "Settings.Timer",
        "Settings.Gui",
        "Settings.Background",
        "Settings.HotKey",
        "Settings.RemoteControl",
        "Settings.ExternalNotification",
    ];

    private static readonly string[] TargetWindowCapabilities =
    [
        "Tray",
        "Notification",
        "Hotkey",
        "Autostart",
        "Overlay",
    ];

    [Fact]
    public void TargetItems_ShouldNotRemainGap()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var statuses = baseline.Items.ToDictionary(i => i.ItemId, i => i.ParityStatus, StringComparer.Ordinal);

        foreach (var itemId in TargetItemIds)
        {
            Assert.True(statuses.TryGetValue(itemId, out var status), $"Missing baseline item: {itemId}");
            Assert.NotEqual("Gap", status);
        }
    }

    [Fact]
    public void TargetConfigKeyMappings_ShouldNotRemainGap()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var targetOwnerSet = TargetMappingOwners.ToHashSet(StringComparer.Ordinal);

        var remainedGap = baseline.ConfigKeyMappings
            .Where(mapping => targetOwnerSet.Contains(mapping.OwnerItemId) && string.Equals(mapping.ParityStatus, "Gap", StringComparison.Ordinal))
            .Select(mapping => $"{mapping.OwnerItemId}:{mapping.Key}")
            .ToList();

        Assert.True(remainedGap.Count == 0, $"Target mapping gaps remain: {string.Join(", ", remainedGap)}");
    }

    [Fact]
    public void TargetWindowsFallbackCapabilities_ShouldNotRemainGap()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var capabilitySet = TargetWindowCapabilities.ToHashSet(StringComparer.Ordinal);

        var remainedGap = baseline.FallbackCapabilities
            .Where(row => string.Equals(row.Platform, "windows", StringComparison.OrdinalIgnoreCase))
            .Where(row => capabilitySet.Contains(row.CapabilityId))
            .Where(row => string.Equals(row.ParityStatus, "Gap", StringComparison.Ordinal))
            .Select(row => $"{row.CapabilityId}:{row.Platform}")
            .ToList();

        Assert.True(remainedGap.Count == 0, $"Windows fallback gaps remain: {string.Join(", ", remainedGap)}");
    }
}
