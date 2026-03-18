using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using MAAUnified.App.Services;
using MAAUnified.App.ViewModels;
using MAAUnified.App.Views;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;

namespace MAAUnified.App;

public partial class App : Avalonia.Application
{
    private static readonly object GlobalExceptionGate = new();
    private static readonly HashSet<string> ReportedGlobalExceptions = new(StringComparer.Ordinal);
    private static bool _globalExceptionHandlersRegistered;
    private static AppCrashCaptureService? _crashCaptureService;

    public static MAAUnifiedRuntime Runtime { get; private set; } = null!;

    public override void Initialize()
    {
        AvaloniaXamlLoader.Load(this);
    }

    public override void OnFrameworkInitializationCompleted()
    {
        Program.RecordStartupStage(
            "FrameworkInit.Enter",
            $"lifetime={ApplicationLifetime?.GetType().FullName ?? "<null>"}");

        try
        {
            Program.RecordStartupStage("FrameworkInit.RuntimeCreate.Begin", $"baseDir={AppContext.BaseDirectory}");
            Runtime = MAAUnifiedRuntimeFactory.Create(AppContext.BaseDirectory);
            Program.RecordStartupStage("FrameworkInit.RuntimeCreate.End", "MAAUnified runtime created.");
        }
        catch (Exception ex)
        {
            Program.RecordStartupStage("FrameworkInit.RuntimeCreate.Fail", "MAAUnified runtime creation failed.", ex);
            throw;
        }

        _crashCaptureService = new AppCrashCaptureService(AppContext.BaseDirectory);
        Program.RecordStartupStage("FrameworkInit.CrashCapture.Ready", "Crash capture service created.");
        RegisterGlobalExceptionHandlers();
        Program.RecordStartupStage("FrameworkInit.ExceptionHandlers.Ready", "Global exception handlers registered.");

        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            Program.RecordStartupStage("FrameworkInit.DesktopLifetime.Begin", "Configuring classic desktop lifetime.");
            var appLifecycleService = new AvaloniaDesktopAppLifecycleService(desktop);
            Runtime.AppLifecycleService = appLifecycleService;
            Runtime.PostActionFeatureService = new PostActionFeatureService(
                Runtime.ConfigurationService,
                Runtime.DiagnosticsService,
                Runtime.Platform.PostActionExecutorService,
                Runtime.CoreBridge,
                appLifecycleService,
                new AvaloniaPostActionPromptService(desktop));

            var vm = new MainShellViewModel(Runtime);
            Program.RecordStartupStage("FrameworkInit.ViewModel.Created", "MainShellViewModel created.");
            var mainWindow = new MainWindow
            {
                DataContext = vm,
                IsEnabled = false,
            };
            Program.RecordStartupStage("FrameworkInit.MainWindow.Created", "MainWindow created and disabled pending initialization.");
            desktop.MainWindow = mainWindow;
            Program.RecordStartupStage("FrameworkInit.MainWindow.Assigned", "Desktop MainWindow assigned.");
            desktop.Exit += (_, _) => Runtime.DisposeAsync().AsTask().GetAwaiter().GetResult();

            _ = InitializeShellAsync(vm, mainWindow);
            Program.RecordStartupStage("FrameworkInit.InitializeShell.Scheduled", "Shell initialization scheduled.");
        }
        else
        {
            Program.RecordStartupStage(
                "FrameworkInit.NonDesktopLifetime",
                $"Skipping desktop shell setup because lifetime is {ApplicationLifetime?.GetType().FullName ?? "<null>"}.");
        }

        Program.RecordStartupStage("FrameworkInit.Complete", "Framework initialization completed.");

        base.OnFrameworkInitializationCompleted();
    }

    private static async Task InitializeShellAsync(MainShellViewModel vm, MainWindow mainWindow)
    {
        Program.RecordStartupStage("InitializeShell.Begin", "Starting shell initialization.");
        try
        {
            Program.RecordStartupStage("InitializeShell.PendingCrashProbe.Begin", "Checking for previous crash reports.");
            await ReportPendingNativeCrashAsync();
            Program.RecordStartupStage("InitializeShell.PendingCrashProbe.End", "Previous crash report probe completed.");
            await vm.InitializeAsync();
            Program.RecordStartupStage(
                "InitializeShell.End",
                $"Shell initialized. sessionState={Runtime.SessionService.CurrentState}; errorLog={Runtime.DiagnosticsService.ErrorLogPath}");
        }
        catch (Exception ex)
        {
            Program.RecordStartupStage("InitializeShell.Fail", "Shell initialization failed.", ex);
            await ReportGlobalExceptionAsync("App.Initialize", ex, handled: true);
        }
        finally
        {
            mainWindow.IsEnabled = true;
            Program.RecordStartupStage("InitializeShell.Finally", "Main window re-enabled after shell initialization.");
        }
    }

    private static void RegisterGlobalExceptionHandlers()
    {
        if (_globalExceptionHandlersRegistered)
        {
            return;
        }

        AppDomain.CurrentDomain.UnhandledException += OnCurrentDomainUnhandledException;
        TaskScheduler.UnobservedTaskException += OnTaskSchedulerUnobservedTaskException;
        Dispatcher.UIThread.UnhandledExceptionFilter += OnDispatcherUnhandledExceptionFilter;
        Dispatcher.UIThread.UnhandledException += OnDispatcherUnhandledException;
        _globalExceptionHandlersRegistered = true;
    }

    private static void OnDispatcherUnhandledExceptionFilter(object? sender, DispatcherUnhandledExceptionFilterEventArgs e)
    {
        if (ShouldIgnoreUnhandledException(e.Exception))
        {
            return;
        }

        e.RequestCatch = true;
    }

    private static void OnDispatcherUnhandledException(object? sender, DispatcherUnhandledExceptionEventArgs e)
    {
        if (ShouldIgnoreUnhandledException(e.Exception))
        {
            return;
        }

        e.Handled = true;
        _ = ReportGlobalExceptionAsync("App.DispatcherUnhandledException", e.Exception, handled: true);
    }

    private static void OnTaskSchedulerUnobservedTaskException(object? sender, UnobservedTaskExceptionEventArgs e)
    {
        var exception = e.Exception.Flatten();
        if (ShouldIgnoreUnhandledException(exception))
        {
            e.SetObserved();
            return;
        }

        e.SetObserved();
        _ = ReportGlobalExceptionAsync("App.UnobservedTaskException", exception, handled: true);
    }

    private static void OnCurrentDomainUnhandledException(object? sender, UnhandledExceptionEventArgs e)
    {
        var exception = e.ExceptionObject as Exception
            ?? new InvalidOperationException($"Unhandled exception payload type: {e.ExceptionObject?.GetType().FullName ?? "<null>"}");
        if (ShouldIgnoreUnhandledException(exception))
        {
            return;
        }

        _ = ReportGlobalExceptionAsync("AppDomain.CurrentDomain.UnhandledException", exception, handled: false, isTerminating: e.IsTerminating);
    }

    private static bool ShouldIgnoreUnhandledException(Exception exception)
    {
        if (exception is OperationCanceledException)
        {
            return true;
        }

        if (exception is AggregateException aggregate)
        {
            var inners = aggregate.Flatten().InnerExceptions;
            return inners.Count > 0 && inners.All(static inner => inner is OperationCanceledException);
        }

        return false;
    }

    private static async Task ReportPendingNativeCrashAsync()
    {
        if (_crashCaptureService is null)
        {
            return;
        }

        try
        {
            var crashReport = await _crashCaptureService.TryGetPendingCrashReportAsync();
            if (crashReport is null)
            {
                return;
            }

            await _crashCaptureService.MarkCrashReportAsSeenAsync(crashReport);

            var timestamp = crashReport.LastWriteTimeUtc.ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss zzz");
            var message = $"Detected native crash log from a previous launch at {timestamp}.";
            var details =
                $"Crash log: {crashReport.CrashLogPath}{Environment.NewLine}" +
                $"Timestamp: {crashReport.LastWriteTimeUtc:O}{Environment.NewLine}{Environment.NewLine}" +
                crashReport.Detail;
            var result = UiOperationResult.Fail(UiErrorCode.CoreUnknown, message, details);

            Runtime.LogService.Error(message);
            await Runtime.DialogFeatureService.ReportErrorAsync("App.PreviousCrash", result);
        }
        catch (Exception ex)
        {
            await SafeRecordExceptionAsync("App.PreviousCrashProbe", "Failed to inspect previous crash log.", ex);
        }
    }

    private static async Task ReportGlobalExceptionAsync(
        string context,
        Exception exception,
        bool handled,
        bool isTerminating = false)
    {
        if (!TryMarkGlobalExceptionAsReported(context, exception))
        {
            return;
        }

        var summary = BuildUnhandledExceptionSummary(context, exception, handled, isTerminating);
        var details = BuildUnhandledExceptionDetails(context, exception, handled, isTerminating);

        try
        {
            Console.Error.WriteLine(summary);
            Console.Error.WriteLine(details);
        }
        catch
        {
            // Ignore stderr failures during crash reporting.
        }

        try
        {
            Runtime.LogService.Error(summary);
            var result = UiOperationResult.Fail(UiErrorCode.UiError, summary, details);
            await Runtime.DiagnosticsService.RecordErrorAsync(context, summary, exception);
            await Runtime.DialogFeatureService.ReportErrorAsync(context, result);
        }
        catch
        {
            // Avoid rethrowing from global exception handlers.
        }
    }

    private static bool TryMarkGlobalExceptionAsReported(string context, Exception exception)
    {
        var key = $"{context}|{exception.GetType().FullName}|{exception.Message}";
        lock (GlobalExceptionGate)
        {
            return ReportedGlobalExceptions.Add(key);
        }
    }

    private static string BuildUnhandledExceptionSummary(
        string context,
        Exception exception,
        bool handled,
        bool isTerminating)
    {
        var state = handled
            ? "handled"
            : isTerminating
                ? "terminating"
                : "unhandled";
        return $"Global exception ({state}) in {context}: {exception.GetType().Name}: {exception.Message}";
    }

    private static string BuildUnhandledExceptionDetails(
        string context,
        Exception exception,
        bool handled,
        bool isTerminating)
    {
        return
            $"Timestamp: {DateTimeOffset.UtcNow:O}{Environment.NewLine}" +
            $"Context: {context}{Environment.NewLine}" +
            $"Handled: {handled}{Environment.NewLine}" +
            $"IsTerminating: {isTerminating}{Environment.NewLine}" +
            $"BaseDirectory: {AppContext.BaseDirectory}{Environment.NewLine}" +
            $"ErrorLog: {Runtime.DiagnosticsService.ErrorLogPath}{Environment.NewLine}" +
            $"EventLog: {Runtime.DiagnosticsService.EventLogPath}{Environment.NewLine}" +
            $"PlatformLog: {Runtime.DiagnosticsService.PlatformEventLogPath}{Environment.NewLine}{Environment.NewLine}" +
            exception;
    }

    private static async Task SafeRecordExceptionAsync(string scope, string message, Exception exception)
    {
        try
        {
            Runtime.LogService.Error($"{message} {exception.GetType().Name}: {exception.Message}");
            await Runtime.DiagnosticsService.RecordErrorAsync(scope, message, exception);
        }
        catch
        {
            // Ignore logging failures during crash reporting.
        }
    }
}
