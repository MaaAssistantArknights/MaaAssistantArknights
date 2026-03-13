using System.Reflection;
using Avalonia.Threading;
using MAAUnified.App.ViewModels;

namespace MAAUnified.Tests;

internal static class TestShellCleanup
{
    private static readonly FieldInfo? TimerScheduleTimerField =
        typeof(MainShellViewModel).GetField("_timerScheduleTimer", BindingFlags.Instance | BindingFlags.NonPublic);

    public static void StopTimerScheduler(MainShellViewModel shell)
    {
        if (TimerScheduleTimerField?.GetValue(shell) is DispatcherTimer timer)
        {
            timer.Stop();
        }
    }
}
