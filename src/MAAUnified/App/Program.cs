using System.Text;
using Avalonia;

namespace MAAUnified.App;

internal static class Program
{
    private const string StartupScope = "App.Startup";
    private const string StartupNoDisplayCode = "UiStartupNoDisplay";
    private const string StartupUnhandledCode = "UiStartupUnhandled";

    [STAThread]
    public static int Main(string[] args)
    {
        if (OperatingSystem.IsLinux() && !HasLinuxDesktopDisplay())
        {
            ReportStartupFailure(
                StartupNoDisplayCode,
                $"No Linux graphical display detected. Set DISPLAY or WAYLAND_DISPLAY before launching MAAUnified. {BuildDisplayEnvironmentSnapshot()}");
            return 2;
        }

        try
        {
            return BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        }
        catch (Exception ex) when (IsDisplayInitializationFailure(ex))
        {
            ReportStartupFailure(
                StartupNoDisplayCode,
                $"Failed to open Linux display server. Verify DISPLAY/WAYLAND_DISPLAY and desktop session permissions. {BuildDisplayEnvironmentSnapshot()}",
                ex);
            return 2;
        }
        catch (Exception ex)
        {
            ReportStartupFailure(StartupUnhandledCode, "Unhandled startup failure.", ex);
            return 1;
        }
    }

    public static AppBuilder BuildAvaloniaApp()
        => AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .WithInterFont()
            .LogToTrace();

    internal static bool HasLinuxDesktopDisplay()
    {
        if (!string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("DISPLAY")))
        {
            return true;
        }

        return !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("WAYLAND_DISPLAY"));
    }

    internal static bool IsDisplayInitializationFailure(Exception exception)
    {
        return ContainsMessage(exception, "XOpenDisplay failed")
            || ContainsMessage(exception, "unable to open display");
    }

    private static bool ContainsMessage(Exception? exception, string fragment)
    {
        for (var current = exception; current is not null; current = current.InnerException)
        {
            if (current.Message.Contains(fragment, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
        }

        return false;
    }

    private static string BuildDisplayEnvironmentSnapshot()
    {
        static string Snapshot(string key)
        {
            var value = Environment.GetEnvironmentVariable(key);
            return string.IsNullOrWhiteSpace(value) ? $"{key}=<unset>" : $"{key}={value}";
        }

        return $"{Snapshot("DISPLAY")} {Snapshot("WAYLAND_DISPLAY")}";
    }

    private static void ReportStartupFailure(string code, string message, Exception? exception = null)
    {
        var line = new StringBuilder()
            .Append(DateTimeOffset.UtcNow.ToString("O"))
            .Append(" [FAILED] [")
            .Append(StartupScope)
            .Append("] code=")
            .Append(code)
            .Append(" message=")
            .Append(message);

        if (exception is not null)
        {
            line.Append(" | ").Append(exception.GetType().Name).Append(": ").Append(exception.Message);
        }

        Console.Error.WriteLine(line.ToString());

        var payload = line.ToString();
        if (exception is not null)
        {
            payload += Environment.NewLine + exception;
        }

        TryAppendStartupLog(payload);
    }

    private static void TryAppendStartupLog(string payload)
    {
        try
        {
            var debugDirectory = Path.Combine(AppContext.BaseDirectory, "debug");
            Directory.CreateDirectory(debugDirectory);
            var path = Path.Combine(debugDirectory, "avalonia-ui-errors.log");
            File.AppendAllText(path, payload + Environment.NewLine);
        }
        catch
        {
            // Never throw from startup error reporting.
        }
    }
}
