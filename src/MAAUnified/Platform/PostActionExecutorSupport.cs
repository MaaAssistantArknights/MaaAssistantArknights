using System.Diagnostics;
using System.Text.RegularExpressions;

namespace MAAUnified.Platform;

internal static class PostActionExecutorSupport
{
    public static PostActionCapabilityMatrix BuildCapabilityMatrix(PostActionExecutorRequest? request)
    {
        var exitArknights = BuildLegacyCommandCapability(request, "Exit Arknights");
        var backToHome = BuildLegacyCommandCapability(request, "Back to Android home");
        var exitSelf = BuildLegacyCommandCapability(request, "Exit MAA");
        var exitEmulator = CombineWithLegacyFallback(
            BuildExitEmulatorCapability(request),
            BuildLegacyCommandCapability(request, "Exit emulator"));

        return new PostActionCapabilityMatrix(
            ExitArknights: exitArknights,
            BackToAndroidHome: backToHome,
            ExitEmulator: exitEmulator,
            ExitSelf: exitSelf,
            Hibernate: BuildPowerCapability(PostActionType.Hibernate),
            Shutdown: BuildPowerCapability(PostActionType.Shutdown),
            Sleep: BuildPowerCapability(PostActionType.Sleep));
    }

    public static async Task<PlatformOperationResult> ExecuteLegacyCommandAsync(
        PostActionType action,
        PostActionExecutorRequest? request,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(request?.CommandLine))
        {
            return PlatformOperation.Failed(
                "command",
                $"Legacy command fallback is not configured for {action}.",
                PlatformErrorCodes.PostActionUnsupported,
                $"post-action.{action}");
        }

        return await ExecuteShellCommandAsync(action, request.CommandLine, cancellationToken);
    }

    public static async Task<PlatformOperationResult> ExecuteExitEmulatorAsync(
        PostActionExecutorRequest? request,
        CancellationToken cancellationToken)
    {
        var nativeResult = await ExecuteNativeExitEmulatorAsync(request, cancellationToken);
        if (nativeResult.Success
            || !string.Equals(nativeResult.ErrorCode, PlatformErrorCodes.PostActionUnsupported, StringComparison.Ordinal)
            || string.IsNullOrWhiteSpace(request?.CommandLine))
        {
            return nativeResult;
        }

        return await ExecuteLegacyCommandAsync(PostActionType.ExitEmulator, request, cancellationToken);
    }

    public static async Task<PlatformOperationResult> ExecutePowerActionAsync(
        PostActionType action,
        CancellationToken cancellationToken)
    {
        var command = GetPowerCommand(action);
        if (command is null)
        {
            return PlatformOperation.Failed(
                "system",
                $"Power action {action} is unsupported on current platform.",
                PlatformErrorCodes.PostActionUnsupported,
                $"post-action.{action}");
        }

        return await ExecuteDirectCommandAsync(
            "system",
            $"post-action.{action}",
            $"{action} command accepted.",
            command.Value.fileName,
            command.Value.arguments,
            cancellationToken);
    }

    internal static PlatformCapabilityStatus BuildLegacyCommandCapability(PostActionExecutorRequest? request, string actionName)
    {
        if (!CanExecuteShellCommands())
        {
            return new PlatformCapabilityStatus(
                false,
                $"Legacy command fallback for {actionName} is unsupported on current platform.",
                Provider: "command",
                HasFallback: true,
                FallbackMode: "legacy-command");
        }

        if (string.IsNullOrWhiteSpace(request?.CommandLine))
        {
            return new PlatformCapabilityStatus(
                false,
                $"Legacy command fallback for {actionName} is not configured.",
                Provider: "command",
                HasFallback: true,
                FallbackMode: "legacy-command");
        }

        return new PlatformCapabilityStatus(
            true,
            $"{actionName} is available via legacy command fallback.",
            Provider: "command");
    }

    internal static PlatformCapabilityStatus BuildExitEmulatorCapability(PostActionExecutorRequest? request)
    {
        if (OperatingSystem.IsWindows())
        {
            return WindowsEmulatorExitHelper.GetCapability(request);
        }

        if (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS())
        {
            return AndroidEmulatorAdbHelper.GetCapability(request);
        }

        return new PlatformCapabilityStatus(
            false,
            "Exit emulator is unsupported on current platform.",
            Provider: "post-action");
    }

    internal static PlatformCapabilityStatus BuildPowerCapability(PostActionType action)
    {
        return GetPowerCommand(action) is null
            ? new PlatformCapabilityStatus(
                false,
                $"Power action {action} is unsupported on current platform.",
                Provider: "system")
            : new PlatformCapabilityStatus(
                true,
                $"Power action {action} is available via native system command.",
                Provider: "system");
    }

    internal static async Task<PlatformOperationResult> ExecuteShellCommandAsync(
        PostActionType action,
        string commandLine,
        CancellationToken cancellationToken)
    {
        var shellCommand = BuildShellCommand(commandLine);
        if (shellCommand is null)
        {
            return PlatformOperation.Failed(
                "command",
                "Shell execution is unsupported on current platform.",
                PlatformErrorCodes.PostActionUnsupported,
                $"post-action.{action}");
        }

        return await ExecuteDirectCommandAsync(
            "command",
            $"post-action.{action}",
            $"Legacy post action command accepted: {action}.",
            shellCommand.Value.fileName,
            shellCommand.Value.arguments,
            cancellationToken);
    }

    internal static async Task<PlatformOperationResult> ExecuteDirectCommandAsync(
        string provider,
        string operationId,
        string successMessage,
        string fileName,
        string arguments,
        CancellationToken cancellationToken)
    {
        try
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = fileName,
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = new Process { StartInfo = startInfo };
            process.Start();
            var stdoutTask = process.StandardOutput.ReadToEndAsync(cancellationToken);
            var stderrTask = process.StandardError.ReadToEndAsync(cancellationToken);
            await process.WaitForExitAsync(cancellationToken);
            var stdout = (await stdoutTask).Trim();
            var stderr = (await stderrTask).Trim();

            if (process.ExitCode == 0)
            {
                var message = string.IsNullOrWhiteSpace(stdout)
                    ? successMessage
                    : $"{successMessage} {stdout}";
                return PlatformOperation.NativeSuccess(provider, message.Trim(), operationId);
            }

            var failedMessage = string.IsNullOrWhiteSpace(stderr)
                ? $"{successMessage} Exit code {process.ExitCode}."
                : $"{successMessage} Exit code {process.ExitCode}: {stderr}";
            return PlatformOperation.Failed(
                provider,
                failedMessage,
                PlatformErrorCodes.PostActionExecutionFailed,
                operationId);
        }
        catch (Exception ex)
        {
            return PlatformOperation.Failed(
                provider,
                $"{successMessage} {ex.Message}",
                PlatformErrorCodes.PostActionExecutionFailed,
                operationId);
        }
    }

    internal static bool CanExecuteShellCommands()
        => BuildShellCommand("echo") is not null;

    internal static (string fileName, string arguments)? BuildShellCommand(string commandLine)
    {
        if (OperatingSystem.IsWindows())
        {
            return ("cmd.exe", $"/C {commandLine}");
        }

        if (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS())
        {
            var escaped = commandLine
                .Replace("\\", "\\\\", StringComparison.Ordinal)
                .Replace("\"", "\\\"", StringComparison.Ordinal);
            return ("/bin/bash", $"-lc \"{escaped}\"");
        }

        return null;
    }

    private static async Task<PlatformOperationResult> ExecuteNativeExitEmulatorAsync(
        PostActionExecutorRequest? request,
        CancellationToken cancellationToken)
    {
        if (OperatingSystem.IsWindows())
        {
            return await WindowsEmulatorExitHelper.ExecuteAsync(request, cancellationToken);
        }

        if (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS())
        {
            return await AndroidEmulatorAdbHelper.ExecuteAsync(request, cancellationToken);
        }

        return PlatformOperation.Failed(
            "post-action",
            "Exit emulator is unsupported on current platform.",
            PlatformErrorCodes.PostActionUnsupported,
            "post-action.ExitEmulator");
    }

    private static PlatformCapabilityStatus CombineWithLegacyFallback(
        PlatformCapabilityStatus nativeCapability,
        PlatformCapabilityStatus legacyCapability)
    {
        if (!nativeCapability.Supported && legacyCapability.Supported)
        {
            return legacyCapability;
        }

        if (!nativeCapability.Supported)
        {
            return nativeCapability;
        }

        return new PlatformCapabilityStatus(
            nativeCapability.Supported,
            nativeCapability.Message,
            nativeCapability.Provider,
            nativeCapability.HasFallback || legacyCapability.Supported,
            legacyCapability.Supported ? "legacy-command" : nativeCapability.FallbackMode);
    }

    private static (string fileName, string arguments)? GetPowerCommand(PostActionType action)
    {
        if (OperatingSystem.IsWindows())
        {
            return action switch
            {
                PostActionType.Hibernate => ("shutdown", "/h"),
                PostActionType.Shutdown => ("shutdown", "/s /t 0"),
                PostActionType.Sleep => ("rundll32.exe", "powrprof.dll,SetSuspendState 0,1,0"),
                _ => null,
            };
        }

        if (OperatingSystem.IsLinux())
        {
            return action switch
            {
                PostActionType.Hibernate => ("systemctl", "hibernate"),
                PostActionType.Shutdown => ("systemctl", "poweroff"),
                PostActionType.Sleep => ("systemctl", "suspend"),
                _ => null,
            };
        }

        if (OperatingSystem.IsMacOS())
        {
            return action switch
            {
                PostActionType.Hibernate => ("pmset", "sleepnow"),
                PostActionType.Shutdown => ("shutdown", "-h now"),
                PostActionType.Sleep => ("pmset", "sleepnow"),
                _ => null,
            };
        }

        return null;
    }
}

internal static class AndroidEmulatorAdbHelper
{
    public static PlatformCapabilityStatus GetCapability(PostActionExecutorRequest? request)
    {
        return TryBuildKillCommand(request, out var serial, out _)
            ? new PlatformCapabilityStatus(
                true,
                $"Exit emulator is available via adb emu kill for {serial}.",
                Provider: "adb")
            : new PlatformCapabilityStatus(
                false,
                "Exit emulator is unsupported unless the connect address maps to a local Android Emulator instance.",
                Provider: "adb");
    }

    public static async Task<PlatformOperationResult> ExecuteAsync(
        PostActionExecutorRequest? request,
        CancellationToken cancellationToken)
    {
        if (!TryBuildKillCommand(request, out var serial, out var command))
        {
            return PlatformOperation.Failed(
                "adb",
                "Exit emulator is unsupported unless the connect address maps to a local Android Emulator instance.",
                PlatformErrorCodes.PostActionUnsupported,
                "post-action.ExitEmulator");
        }

        return await PostActionExecutorSupport.ExecuteDirectCommandAsync(
            "adb",
            "post-action.ExitEmulator",
            $"Android emulator shutdown command accepted for {serial}.",
            command.fileName,
            command.arguments,
            cancellationToken);
    }

    internal static bool TryBuildKillCommand(
        PostActionExecutorRequest? request,
        out string serial,
        out (string fileName, string arguments) command)
    {
        serial = string.Empty;
        command = default;
        if (!TryResolveLocalEmulatorSerial(request?.ConnectAddress, out serial))
        {
            return false;
        }

        var adbPath = string.IsNullOrWhiteSpace(request?.AdbPath)
            ? "adb"
            : request!.AdbPath!.Trim();
        command = (adbPath, $"-s {serial} emu kill");
        return true;
    }

    private static bool TryResolveLocalEmulatorSerial(string? address, out string serial)
    {
        serial = string.Empty;
        if (string.IsNullOrWhiteSpace(address))
        {
            return false;
        }

        var trimmed = address.Trim();
        if (trimmed.StartsWith("emulator-", StringComparison.OrdinalIgnoreCase))
        {
            serial = trimmed;
            return true;
        }

        if (!WindowsEmulatorExitHelper.TryParseHostAndPort(trimmed, out var host, out var port))
        {
            return false;
        }

        if (!WindowsEmulatorExitHelper.IsLoopbackHost(host))
        {
            return false;
        }

        if (port >= 5555 && port % 2 == 1)
        {
            serial = $"emulator-{port - 1}";
            return true;
        }

        if (port >= 5554 && port % 2 == 0)
        {
            serial = $"emulator-{port}";
            return true;
        }

        return false;
    }
}

internal static class WindowsEmulatorExitHelper
{
    private static readonly string[] WindowTitles =
    [
        "明日方舟",
        "明日方舟 - MuMu模拟器",
        "BlueStacks App Player",
        "BlueStacks",
        "Google Play Games on PC Emulator",
    ];

    public static PlatformCapabilityStatus GetCapability(PostActionExecutorRequest? request)
    {
        if (!OperatingSystem.IsWindows())
        {
            return new PlatformCapabilityStatus(false, "Windows native emulator shutdown is unavailable on current platform.", Provider: "windows-native");
        }

        if (IsRecognizedWindowsEmulator(request?.ConnectConfig) || TryResolveCandidatePorts(request?.ConnectAddress, out _))
        {
            return new PlatformCapabilityStatus(
                true,
                "Exit emulator is available via Windows native emulator shutdown.",
                Provider: "windows-native");
        }

        return new PlatformCapabilityStatus(
            false,
            "Exit emulator needs a recognized local Windows emulator connection or a legacy command fallback.",
            Provider: "windows-native");
    }

    public static async Task<PlatformOperationResult> ExecuteAsync(
        PostActionExecutorRequest? request,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedConfig = NormalizeToken(request?.ConnectConfig);

        return normalizedConfig switch
        {
            "mumu" or "mumu12" or "mumuemulator12" => await ExecuteMuMuAsync(request, cancellationToken),
            "ldplayer" => await ExecuteLdPlayerAsync(request, cancellationToken),
            "nox" => await ExecuteNoxAsync(request, cancellationToken),
            "xyaz" or "memu" => await ExecuteXyazAsync(request, cancellationToken),
            "bluestacks" => await ExecuteBlueStacksAsync(request, cancellationToken),
            _ => ExecuteWindowOrPortFallback(request, "windows-native.default"),
        };
    }

    internal static bool TryResolveMuMuIndex(PostActionExecutorRequest? request, out int emulatorIndex)
    {
        emulatorIndex = 0;
        if (request is null)
        {
            return false;
        }

        var address = (request.ConnectAddress ?? string.Empty).Trim();
        if (string.Equals(address, "127.0.0.1:16384", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        if (request.MuMuBridgeConnection && int.TryParse(request.MuMu12Index, out emulatorIndex) && emulatorIndex >= 0)
        {
            return true;
        }

        if (TryParseHostAndPort(address, out _, out var port))
        {
            if (port >= 16384)
            {
                emulatorIndex = (port - 16384) / 32;
                return emulatorIndex >= 0;
            }

            if (port == 7555)
            {
                emulatorIndex = 0;
                return true;
            }

            if (port >= 5555)
            {
                emulatorIndex = (port - 5555) / 2;
                return emulatorIndex >= 0;
            }
        }

        if (address.StartsWith("emulator-", StringComparison.OrdinalIgnoreCase)
            && int.TryParse(address["emulator-".Length..], out port))
        {
            emulatorIndex = (port - 5554) / 2;
            return emulatorIndex >= 0;
        }

        return false;
    }

    internal static bool TryResolveLdPlayerIndex(PostActionExecutorRequest? request, out int emulatorIndex)
    {
        emulatorIndex = 0;
        if (request is null)
        {
            return false;
        }

        if (request.LdPlayerManualSetIndex && int.TryParse(request.LdPlayerIndex, out emulatorIndex) && emulatorIndex >= 0)
        {
            return true;
        }

        var address = (request.ConnectAddress ?? string.Empty).Trim();
        if (TryParseHostAndPort(address, out _, out var port))
        {
            emulatorIndex = (port - 5555) / 2;
            return emulatorIndex >= 0;
        }

        if (address.StartsWith("emulator-", StringComparison.OrdinalIgnoreCase)
            && int.TryParse(address["emulator-".Length..], out port))
        {
            emulatorIndex = (port - 5554) / 2;
            return emulatorIndex >= 0;
        }

        return false;
    }

    internal static bool TryParseHostAndPort(string? address, out string host, out int port)
    {
        host = string.Empty;
        port = 0;
        if (string.IsNullOrWhiteSpace(address))
        {
            return false;
        }

        var parts = address.Trim().Split(':', 2, StringSplitOptions.TrimEntries);
        if (parts.Length != 2 || !int.TryParse(parts[1], out port))
        {
            return false;
        }

        host = parts[0];
        return !string.IsNullOrWhiteSpace(host);
    }

    internal static bool IsLoopbackHost(string host)
    {
        return string.Equals(host, "127.0.0.1", StringComparison.OrdinalIgnoreCase)
               || string.Equals(host, "localhost", StringComparison.OrdinalIgnoreCase)
               || string.Equals(host, "::1", StringComparison.OrdinalIgnoreCase);
    }

    private static async Task<PlatformOperationResult> ExecuteMuMuAsync(PostActionExecutorRequest? request, CancellationToken cancellationToken)
    {
        if (!TryResolveMuMuIndex(request, out var emulatorIndex))
        {
            return PlatformOperation.Failed(
                "windows-native",
                "Unable to resolve MuMu emulator index from current connection.",
                PlatformErrorCodes.PostActionUnsupported,
                "post-action.ExitEmulator");
        }

        if (TryResolveMuMuConsolePath(request?.MuMu12EmulatorPath, out var consolePath))
        {
            return await PostActionExecutorSupport.ExecuteDirectCommandAsync(
                "windows-native",
                "post-action.ExitEmulator",
                $"MuMu emulator shutdown command accepted for index {emulatorIndex}.",
                consolePath,
                $"api -v {emulatorIndex} shutdown_player",
                cancellationToken);
        }

        return ExecuteWindowOrPortFallback(request, "windows-native.mumu");
    }

    private static async Task<PlatformOperationResult> ExecuteLdPlayerAsync(PostActionExecutorRequest? request, CancellationToken cancellationToken)
    {
        if (!TryResolveLdPlayerIndex(request, out var emulatorIndex))
        {
            return PlatformOperation.Failed(
                "windows-native",
                "Unable to resolve LDPlayer emulator index from current connection.",
                PlatformErrorCodes.PostActionUnsupported,
                "post-action.ExitEmulator");
        }

        if (TryResolveLdPlayerConsolePath(request?.LdPlayerEmulatorPath, out var consolePath))
        {
            return await PostActionExecutorSupport.ExecuteDirectCommandAsync(
                "windows-native",
                "post-action.ExitEmulator",
                $"LDPlayer shutdown command accepted for index {emulatorIndex}.",
                consolePath,
                $"quit --index {emulatorIndex}",
                cancellationToken);
        }

        return ExecuteWindowOrPortFallback(request, "windows-native.ldplayer");
    }

    private static async Task<PlatformOperationResult> ExecuteNoxAsync(PostActionExecutorRequest? request, CancellationToken cancellationToken)
    {
        if (!TryResolveHostPortIndex(request?.ConnectAddress, 62001, 62024, out var emulatorIndex))
        {
            return PlatformOperation.Failed(
                "windows-native",
                "Unable to resolve Nox emulator index from current connection.",
                PlatformErrorCodes.PostActionUnsupported,
                "post-action.ExitEmulator");
        }

        if (TryResolveConsolePathFromProcess("Nox", "NoxConsole.exe", out var consolePath))
        {
            return await PostActionExecutorSupport.ExecuteDirectCommandAsync(
                "windows-native",
                "post-action.ExitEmulator",
                $"Nox shutdown command accepted for index {emulatorIndex}.",
                consolePath,
                $"quit -index:{emulatorIndex}",
                cancellationToken);
        }

        return ExecuteWindowOrPortFallback(request, "windows-native.nox");
    }

    private static async Task<PlatformOperationResult> ExecuteXyazAsync(PostActionExecutorRequest? request, CancellationToken cancellationToken)
    {
        if (!TryResolveXyazIndex(request?.ConnectAddress, out var emulatorIndex))
        {
            return PlatformOperation.Failed(
                "windows-native",
                "Unable to resolve XYAZ emulator index from current connection.",
                PlatformErrorCodes.PostActionUnsupported,
                "post-action.ExitEmulator");
        }

        if (TryResolveConsolePathFromProcess("MEmu", "memuc.exe", out var consolePath))
        {
            return await PostActionExecutorSupport.ExecuteDirectCommandAsync(
                "windows-native",
                "post-action.ExitEmulator",
                $"XYAZ shutdown command accepted for index {emulatorIndex}.",
                consolePath,
                $"stop -i {emulatorIndex}",
                cancellationToken);
        }

        return ExecuteWindowOrPortFallback(request, "windows-native.xyaz");
    }

    private static Task<PlatformOperationResult> ExecuteBlueStacksAsync(PostActionExecutorRequest? request, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var windowFallback = ExecuteWindowOrPortFallback(request, "windows-native.bluestacks");
        if (windowFallback.Success)
        {
            return Task.FromResult(windowFallback);
        }

        if (TryKillSingleProcessByName("HD-Player", out var message))
        {
            return Task.FromResult(PlatformOperation.NativeSuccess("windows-native", message, "post-action.ExitEmulator"));
        }

        return Task.FromResult(PlatformOperation.Failed(
            "windows-native",
            "Unable to close BlueStacks with window or process fallback.",
            PlatformErrorCodes.PostActionExecutionFailed,
            "post-action.ExitEmulator"));
    }

    private static PlatformOperationResult ExecuteWindowOrPortFallback(PostActionExecutorRequest? request, string provider)
    {
        if (TryCloseWindowProcess(out var windowMessage))
        {
            return PlatformOperation.NativeSuccess(provider, windowMessage, "post-action.ExitEmulator");
        }

        if (TryKillProcessByPort(request?.ConnectAddress, out var portMessage))
        {
            return PlatformOperation.NativeSuccess(provider, portMessage, "post-action.ExitEmulator");
        }

        return PlatformOperation.Failed(
            provider,
            "Unable to locate a local emulator window or owning process for current connection.",
            PlatformErrorCodes.PostActionUnsupported,
            "post-action.ExitEmulator");
    }

    private static bool IsRecognizedWindowsEmulator(string? connectConfig)
    {
        return NormalizeToken(connectConfig) is "mumu" or "mumu12" or "mumuemulator12" or "ldplayer" or "nox" or "xyaz" or "memu" or "bluestacks";
    }

    private static bool TryResolveMuMuConsolePath(string? emulatorPath, out string consolePath)
    {
        if (TryResolveMuMuConsolePathFromInput(emulatorPath, out consolePath))
        {
            return true;
        }

        if (TryResolveMainModulePath("MuMuNxDevice", out var newPath) && TryResolveMuMuConsolePathFromInput(newPath, out consolePath))
        {
            return true;
        }

        return TryResolveMainModulePath("MuMuPlayer", out var oldPath) && TryResolveMuMuConsolePathFromInput(oldPath, out consolePath);
    }

    private static bool TryResolveMuMuConsolePathFromInput(string? path, out string consolePath)
    {
        consolePath = string.Empty;
        foreach (var candidate in EnumerateMuMuConsoleCandidates(path))
        {
            if (File.Exists(candidate))
            {
                consolePath = candidate;
                return true;
            }
        }

        return false;
    }

    private static IEnumerable<string> EnumerateMuMuConsoleCandidates(string? path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            yield break;
        }

        foreach (var root in EnumerateCandidateRoots(path))
        {
            yield return Path.Combine(root, "nx_main", "MuMuManager.exe");
            yield return Path.Combine(root, "shell", "MuMuManager.exe");
        }
    }

    private static bool TryResolveLdPlayerConsolePath(string? emulatorPath, out string consolePath)
    {
        consolePath = string.Empty;
        foreach (var root in EnumerateCandidateRoots(emulatorPath))
        {
            var candidate = Path.Combine(root, "ldconsole.exe");
            if (File.Exists(candidate))
            {
                consolePath = candidate;
                return true;
            }
        }

        return TryResolveConsolePathFromProcess("dnplayer", "ldconsole.exe", out consolePath);
    }

    private static IEnumerable<string> EnumerateCandidateRoots(string? path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            yield break;
        }

        if (File.Exists(path))
        {
            var fileName = Path.GetFileName(path);
            var directory = Path.GetDirectoryName(path);
            if (!string.IsNullOrWhiteSpace(directory))
            {
                yield return directory;
                yield return Path.GetFullPath(Path.Combine(directory, ".."));
                if (string.Equals(fileName, "MuMuNxDevice.exe", StringComparison.OrdinalIgnoreCase))
                {
                    yield return Path.GetFullPath(Path.Combine(directory, "..", "..", ".."));
                }
            }

            yield break;
        }

        if (Directory.Exists(path))
        {
            yield return path;
            yield return Path.GetFullPath(Path.Combine(path, ".."));
        }
    }

    private static bool TryResolveConsolePathFromProcess(string processName, string consoleExeName, out string consolePath)
    {
        consolePath = string.Empty;
        if (!TryResolveMainModulePath(processName, out var executablePath))
        {
            return false;
        }

        var directory = Path.GetDirectoryName(executablePath);
        if (string.IsNullOrWhiteSpace(directory))
        {
            return false;
        }

        var candidate = Path.Combine(directory, consoleExeName);
        if (!File.Exists(candidate))
        {
            return false;
        }

        consolePath = candidate;
        return true;
    }

    private static bool TryResolveMainModulePath(string processName, out string executablePath)
    {
        executablePath = string.Empty;
        var processes = Process.GetProcessesByName(processName);
        if (processes.Length == 0)
        {
            return false;
        }

        try
        {
            executablePath = processes[0].MainModule?.FileName ?? string.Empty;
            return !string.IsNullOrWhiteSpace(executablePath);
        }
        catch
        {
            return false;
        }
    }

    private static bool TryResolveHostPortIndex(string? address, int primaryPort, int basePort, out int emulatorIndex)
    {
        emulatorIndex = 0;
        if (!TryParseHostAndPort(address, out _, out var port))
        {
            return false;
        }

        emulatorIndex = port == primaryPort ? 0 : port - basePort;
        return emulatorIndex >= 0;
    }

    private static bool TryResolveXyazIndex(string? address, out int emulatorIndex)
    {
        emulatorIndex = 0;
        return TryParseHostAndPort(address, out _, out var port)
               && (emulatorIndex = (port - 21503) / 10) >= 0;
    }

    private static bool TryCloseWindowProcess(out string message)
    {
        message = string.Empty;
        if (!TryFindWindowProcessId(out var pid))
        {
            return false;
        }

        try
        {
            using var process = Process.GetProcessById(pid);
            if (!process.CloseMainWindow() || !process.WaitForExit(5000))
            {
                process.Kill(entireProcessTree: true);
                if (!process.WaitForExit(5000))
                {
                    return false;
                }
            }

            message = $"Closed emulator process {pid} via window fallback.";
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static bool TryFindWindowProcessId(out int pid)
    {
        pid = 0;
        foreach (var title in WindowTitles)
        {
            var hwnd = FindWindow(null!, title);
            if (hwnd == IntPtr.Zero)
            {
                continue;
            }

            GetWindowThreadProcessId(hwnd, out pid);
            if (pid > 0)
            {
                return true;
            }
        }

        return false;
    }

    private static bool TryKillSingleProcessByName(string processName, out string message)
    {
        message = string.Empty;
        var processes = Process.GetProcessesByName(processName);
        if (processes.Length != 1)
        {
            return false;
        }

        try
        {
            using var process = processes[0];
            process.Kill(entireProcessTree: true);
            if (!process.WaitForExit(20000))
            {
                return false;
            }

            message = $"Closed emulator process {process.Id} via process fallback.";
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static bool TryKillProcessByPort(string? address, out string message)
    {
        message = string.Empty;
        if (!TryResolveCandidatePorts(address, out var candidatePorts))
        {
            return false;
        }

        if (!TryGetOwningProcessId(candidatePorts, out var pid))
        {
            return false;
        }

        try
        {
            using var process = Process.GetProcessById(pid);
            process.Kill(entireProcessTree: true);
            if (!process.WaitForExit(5000))
            {
                return false;
            }

            message = $"Closed emulator process {pid} via TCP port fallback.";
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static bool TryGetOwningProcessId(IReadOnlyList<int> candidatePorts, out int pid)
    {
        pid = 0;
        try
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = "netstat",
                Arguments = "-ano -p tcp",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = new Process { StartInfo = startInfo };
            process.Start();
            var output = process.StandardOutput.ReadToEnd();
            process.WaitForExit(5000);

            foreach (var line in output.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries))
            {
                var normalized = Regex.Replace(line.Trim(), "\\s+", " ");
                var parts = normalized.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                if (parts.Length < 5 || !string.Equals(parts[0], "TCP", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (!candidatePorts.Any(port => parts[1].EndsWith($":{port}", StringComparison.OrdinalIgnoreCase)
                                                || parts[2].EndsWith($":{port}", StringComparison.OrdinalIgnoreCase)))
                {
                    continue;
                }

                if (int.TryParse(parts[^1], out pid) && pid > 0)
                {
                    return true;
                }
            }
        }
        catch
        {
            return false;
        }

        return false;
    }

    private static bool TryResolveCandidatePorts(string? address, out List<int> ports)
    {
        ports = [];
        if (string.IsNullOrWhiteSpace(address))
        {
            return false;
        }

        var trimmed = address.Trim();
        if (trimmed.StartsWith("emulator-", StringComparison.OrdinalIgnoreCase)
            && int.TryParse(trimmed["emulator-".Length..], out var emulatorPort))
        {
            ports.Add(emulatorPort);
            ports.Add(emulatorPort + 1);
            return true;
        }

        if (!TryParseHostAndPort(trimmed, out var host, out var port) || !IsLoopbackHost(host))
        {
            return false;
        }

        ports.Add(port);
        return true;
    }

    private static string NormalizeToken(string? text)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return string.Empty;
        }

        var buffer = new char[text.Length];
        var index = 0;
        foreach (var ch in text)
        {
            if (char.IsLetterOrDigit(ch))
            {
                buffer[index++] = char.ToLowerInvariant(ch);
            }
        }

        return new string(buffer, 0, index);
    }

    [System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto, EntryPoint = "FindWindow")]
    private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

    [System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
    private static extern int GetWindowThreadProcessId(IntPtr hwnd, out int id);
}
