using MAAUnified.App;

namespace MAAUnified.DebugHost;

internal static class Program
{
    private const string CrashLogName = "debug-host-crash.log";

    [STAThread]
    private static int Main(string[] args)
    {
        AppDomain.CurrentDomain.UnhandledException += (_, eventArgs) =>
        {
            if (eventArgs.ExceptionObject is Exception exception)
            {
                WriteCrashReport("AppDomain.CurrentDomain.UnhandledException", exception);
            }
            else
            {
                WriteCrashReport(
                    "AppDomain.CurrentDomain.UnhandledException",
                    new InvalidOperationException(
                        $"Unhandled exception payload type: {eventArgs.ExceptionObject?.GetType().FullName ?? "<null>"}"));
            }
        };

        try
        {
            return DebugHostEntry.Run(args);
        }
        catch (Exception ex)
        {
            WriteCrashReport("DebugHost.Main", ex);
            PauseForInspection();
            return 1;
        }
    }

    private static void WriteCrashReport(string scope, Exception exception)
    {
        var payload =
            $"[{DateTimeOffset.UtcNow:O}] {scope}{Environment.NewLine}" +
            $"BaseDir={AppContext.BaseDirectory}{Environment.NewLine}" +
            $"CurrentDir={Environment.CurrentDirectory}{Environment.NewLine}" +
            $"{exception}{Environment.NewLine}";

        try
        {
            Console.Error.WriteLine(payload);
        }
        catch
        {
            // best effort
        }

        try
        {
            var debugDir = Path.Combine(AppContext.BaseDirectory, "debug");
            Directory.CreateDirectory(debugDir);
            File.AppendAllText(Path.Combine(debugDir, CrashLogName), payload + Environment.NewLine);
        }
        catch
        {
            // best effort
        }
    }

    private static void PauseForInspection()
    {
        if (Console.IsInputRedirected)
        {
            return;
        }

        try
        {
            Console.Error.WriteLine("MAAUnified.Debug encountered a fatal error. Press Enter to close.");
            Console.ReadLine();
        }
        catch
        {
            // best effort
        }
    }
}
