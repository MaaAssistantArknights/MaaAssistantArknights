using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
using Avalonia;

namespace MAAUnified.App;

internal static class Program
{
    private const string StartupScope = "App.Startup";
    private const string StartupNoDisplayCode = "UiStartupNoDisplay";
    private const string StartupUnhandledCode = "UiStartupUnhandled";
    internal const string StartupTraceLogName = "avalonia-ui-startup.log";
    internal const string StartupErrorLogName = "avalonia-ui-errors.log";

    [STAThread]
    public static int Main(string[] args)
    {
        RecordStartupStage("Main.Entry", BuildStartupEnvironmentSnapshot(args));

        if (OperatingSystem.IsLinux() && !HasLinuxDesktopDisplay())
        {
            ReportStartupFailure(
                StartupNoDisplayCode,
                $"No Linux graphical display detected. Set DISPLAY or WAYLAND_DISPLAY before launching MAAUnified. {BuildDisplayEnvironmentSnapshot()}");
            return 2;
        }

        try
        {
            RecordStartupStage("Main.BuildApp", "Configuring Avalonia application builder.");
            var builder = BuildAvaloniaApp();
            RecordStartupStage("Main.StartLifetime", "Starting classic desktop lifetime.");
            var exitCode = builder.StartWithClassicDesktopLifetime(args);
            RecordStartupStage("Main.Exit", $"Classic desktop lifetime returned exitCode={exitCode}.");
            return exitCode;
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

    internal static string BuildStartupEnvironmentSnapshot(string[] args)
    {
        var commandLine = args.Length == 0
            ? "<none>"
            : string.Join(' ', args.Select(static arg => arg.Contains(' ', StringComparison.Ordinal) ? $"\"{arg}\"" : arg));
        var processPath = Environment.ProcessPath ?? "<unknown>";
        return string.Create(
            CultureInfo.InvariantCulture,
            $"framework={RuntimeInformation.FrameworkDescription}; os={RuntimeInformation.OSDescription}; osArch={RuntimeInformation.OSArchitecture}; processArch={RuntimeInformation.ProcessArchitecture}; baseDir={AppContext.BaseDirectory}; currentDir={Environment.CurrentDirectory}; processPath={processPath}; args={commandLine}");
    }

    internal static string BuildStartupTracePayload(string stage, string message, Exception? exception = null)
    {
        var line = new StringBuilder()
            .Append(DateTimeOffset.UtcNow.ToString("O"))
            .Append(" [STARTUP] [")
            .Append(StartupScope)
            .Append('.')
            .Append(stage)
            .Append("] ")
            .Append(message);

        if (exception is not null)
        {
            line.Append(" | ").Append(exception.GetType().Name).Append(": ").Append(exception.Message);
        }

        return line.ToString();
    }

    internal static void RecordStartupStage(string stage, string message, Exception? exception = null)
    {
        var payload = BuildStartupTracePayload(stage, message, exception);

        try
        {
            Console.Error.WriteLine(payload);
        }
        catch
        {
            // Ignore stderr failures during startup tracing.
        }

        if (exception is not null)
        {
            payload += Environment.NewLine + exception;
        }

        TryAppendDebugLog(StartupTraceLogName, payload);
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

        TryAppendDebugLog(StartupErrorLogName, payload);
    }

    private static void TryAppendDebugLog(string fileName, string payload)
    {
        try
        {
            var debugDirectory = Path.Combine(AppContext.BaseDirectory, "debug");
            Directory.CreateDirectory(debugDirectory);
            var path = Path.Combine(debugDirectory, fileName);
            File.AppendAllText(path, payload + Environment.NewLine);
        }
        catch
        {
            // Never throw from startup error reporting.
        }
    }
}
