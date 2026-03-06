using MAAUnified.App;

namespace MAAUnified.Tests;

public sealed class StartupProgramTests
{
    [Fact]
    public void HasLinuxDesktopDisplay_WhenDisplaySet_ReturnsTrue()
    {
        var originalDisplay = Environment.GetEnvironmentVariable("DISPLAY");
        var originalWayland = Environment.GetEnvironmentVariable("WAYLAND_DISPLAY");
        try
        {
            Environment.SetEnvironmentVariable("DISPLAY", ":99");
            Environment.SetEnvironmentVariable("WAYLAND_DISPLAY", null);

            Assert.True(Program.HasLinuxDesktopDisplay());
        }
        finally
        {
            Environment.SetEnvironmentVariable("DISPLAY", originalDisplay);
            Environment.SetEnvironmentVariable("WAYLAND_DISPLAY", originalWayland);
        }
    }

    [Fact]
    public void HasLinuxDesktopDisplay_WhenWaylandSet_ReturnsTrue()
    {
        var originalDisplay = Environment.GetEnvironmentVariable("DISPLAY");
        var originalWayland = Environment.GetEnvironmentVariable("WAYLAND_DISPLAY");
        try
        {
            Environment.SetEnvironmentVariable("DISPLAY", null);
            Environment.SetEnvironmentVariable("WAYLAND_DISPLAY", "wayland-0");

            Assert.True(Program.HasLinuxDesktopDisplay());
        }
        finally
        {
            Environment.SetEnvironmentVariable("DISPLAY", originalDisplay);
            Environment.SetEnvironmentVariable("WAYLAND_DISPLAY", originalWayland);
        }
    }

    [Fact]
    public void HasLinuxDesktopDisplay_WhenBothUnset_ReturnsFalse()
    {
        var originalDisplay = Environment.GetEnvironmentVariable("DISPLAY");
        var originalWayland = Environment.GetEnvironmentVariable("WAYLAND_DISPLAY");
        try
        {
            Environment.SetEnvironmentVariable("DISPLAY", null);
            Environment.SetEnvironmentVariable("WAYLAND_DISPLAY", null);

            Assert.False(Program.HasLinuxDesktopDisplay());
        }
        finally
        {
            Environment.SetEnvironmentVariable("DISPLAY", originalDisplay);
            Environment.SetEnvironmentVariable("WAYLAND_DISPLAY", originalWayland);
        }
    }

    [Fact]
    public void IsDisplayInitializationFailure_WhenContainsXOpenDisplayError_ReturnsTrue()
    {
        var ex = new Exception("Bootstrap failed.", new InvalidOperationException("XOpenDisplay failed"));
        Assert.True(Program.IsDisplayInitializationFailure(ex));
    }

    [Fact]
    public void IsDisplayInitializationFailure_WhenContainsOpenDisplayMessage_ReturnsTrue()
    {
        var ex = new Exception("Unable to open display :1");
        Assert.True(Program.IsDisplayInitializationFailure(ex));
    }

    [Fact]
    public void IsDisplayInitializationFailure_WhenUnrelatedError_ReturnsFalse()
    {
        var ex = new Exception("Configuration load failed.");
        Assert.False(Program.IsDisplayInitializationFailure(ex));
    }
}
