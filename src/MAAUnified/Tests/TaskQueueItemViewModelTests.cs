using System.Linq;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.Tests;

public sealed class TaskQueueItemViewModelTests
{
    [Fact]
    public void DefaultStatus_ShouldMapToIdleClass()
    {
        var vm = new TaskQueueItemViewModel("Fight", "fight", true);

        Assert.Equal("Idle", vm.Status);
        AssertMapped(vm, running: false, success: false, error: false, skipped: false, idle: true);
    }

    [Theory]
    [InlineData("Running", true, false, false, false, false)]
    [InlineData("Success", false, true, false, false, false)]
    [InlineData("Error", false, false, true, false, false)]
    [InlineData("Skipped", false, false, false, true, false)]
    [InlineData("Idle", false, false, false, false, true)]
    public void KnownStatuses_ShouldMapToExpectedStatusClass(
        string status,
        bool running,
        bool success,
        bool error,
        bool skipped,
        bool idle)
    {
        var vm = new TaskQueueItemViewModel("Fight", "fight", true);

        vm.Status = status;

        AssertMapped(vm, running, success, error, skipped, idle);
    }

    [Theory]
    [InlineData("running", true, false, false, false, false)]
    [InlineData("SUCCESS", false, true, false, false, false)]
    [InlineData("eRrOr", false, false, true, false, false)]
    [InlineData("skipped", false, false, false, true, false)]
    [InlineData("IDLE", false, false, false, false, true)]
    public void StatusMatching_ShouldBeCaseInsensitive(
        string status,
        bool running,
        bool success,
        bool error,
        bool skipped,
        bool idle)
    {
        var vm = new TaskQueueItemViewModel("Fight", "fight", true);

        vm.Status = status;

        AssertMapped(vm, running, success, error, skipped, idle);
    }

    [Fact]
    public void UnknownStatus_ShouldFallbackToIdleClass()
    {
        var vm = new TaskQueueItemViewModel("Fight", "fight", true)
        {
            Status = "UnexpectedStatus",
        };

        AssertMapped(vm, running: false, success: false, error: false, skipped: false, idle: true);
    }

    [Fact]
    public void StatusUpdate_ShouldRaiseAllStatusClassPropertyChangedNotifications()
    {
        var vm = new TaskQueueItemViewModel("Fight", "fight", true);
        var changed = new HashSet<string>(StringComparer.Ordinal);
        vm.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName is not null)
            {
                changed.Add(e.PropertyName);
            }
        };

        vm.Status = "Running";

        Assert.Contains(nameof(TaskQueueItemViewModel.IsStatusRunning), changed);
        Assert.Contains(nameof(TaskQueueItemViewModel.IsStatusSuccess), changed);
        Assert.Contains(nameof(TaskQueueItemViewModel.IsStatusError), changed);
        Assert.Contains(nameof(TaskQueueItemViewModel.IsStatusSkipped), changed);
        Assert.Contains(nameof(TaskQueueItemViewModel.IsStatusIdle), changed);
    }

    private static void AssertMapped(
        TaskQueueItemViewModel vm,
        bool running,
        bool success,
        bool error,
        bool skipped,
        bool idle)
    {
        Assert.Equal(running, vm.IsStatusRunning);
        Assert.Equal(success, vm.IsStatusSuccess);
        Assert.Equal(error, vm.IsStatusError);
        Assert.Equal(skipped, vm.IsStatusSkipped);
        Assert.Equal(idle, vm.IsStatusIdle);

        var activeCount = new[]
        {
            vm.IsStatusRunning,
            vm.IsStatusSuccess,
            vm.IsStatusError,
            vm.IsStatusSkipped,
            vm.IsStatusIdle,
        }.Count(active => active);

        Assert.Equal(1, activeCount);
    }
}
