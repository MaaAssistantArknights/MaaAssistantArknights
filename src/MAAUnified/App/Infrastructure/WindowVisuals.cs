using Avalonia.Controls;
using Avalonia.Platform;

namespace MAAUnified.App.Infrastructure;

internal static class WindowVisuals
{
    private static readonly Uri AppIconUri = new("avares://MAAUnified/Assets/newlogo.ico");

    public static void ApplyDefaultIcon(Window window)
    {
        if (window.Icon is not null)
        {
            return;
        }

        try
        {
            using var stream = AssetLoader.Open(AppIconUri);
            window.Icon = new WindowIcon(stream);
        }
        catch
        {
            // Keep window creation resilient when the embedded icon is unavailable.
        }
    }
}
