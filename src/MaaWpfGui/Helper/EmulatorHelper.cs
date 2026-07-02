// <copyright file="EmulatorHelper.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.RegularExpressions;
using MaaWpfGui.Constants;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Helper;

public class EmulatorHelper
{
    [DllImport("User32.dll", EntryPoint = "FindWindow")]
    private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

    [DllImport("User32.dll", CharSet = CharSet.Auto)]
    private static extern int GetWindowThreadProcessId(IntPtr hwnd, out int id);

    private static readonly ILogger _logger = Log.ForContext<EmulatorHelper>();

    /// <summary>
    /// 一个根据连接配置判断使用关闭模拟器的方式的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    public static bool KillEmulatorModeSwitcher()
    {
        try
        {
            string emulatorMode = SettingsViewModel.ConnectSettings.ConnectConfig;
            Instances.AsstProxy.Connected = false;
            return emulatorMode switch
            {
                "Nox" => KillEmulatorNox(),
                "LDPlayer" => KillEmulatorLdPlayer(),
                "XYAZ" => KillEmulatorXyaz(),
                "BlueStacks" => KillEmulatorBlueStacks(),
                "MuMuEmulator12" => KillEmulatorMuMuEmulator(),
                _ => KillEmulatorByWindow(),
            };
        }
        catch (Exception e)
        {
            _logger.Error("Failed to close emulator: " + e.Message);
            return false;
        }
    }

    /// <summary>
    /// 将 MuMu 端口号转换为实例索引。
    /// MuMu 端口分配公式：port = 16384 + (index % 32) * 32 + floor(index/32) * 4，
    /// 简化后 index = ((k & 7) << 5) | (k >> 3)，其中 k = (port - 16384) / 4。
    /// 参见 https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17112
    /// </summary>
    /// <param name="port">MuMu ADB 端口号。</param>
    /// <returns>实例索引。</returns>
    private static int MuMuPortToIndex(int port)
    {
        int k = (port - 16384) / 4;
        return ((k & 7) << 5) | (k >> 3);
    }

    /// <summary>
    /// 从连接地址或 MuMu Bridge 连接配置推导 MuMu 模拟器实例索引。
    /// </summary>
    /// <param name="address">连接地址，如 127.0.0.1:16384。</param>
    /// <returns>推导出的实例索引；无法推导时返回 null。</returns>
    private static int? GetMuMuIndex(string address)
    {
        if (address == "127.0.0.1:16384")
        {
            return 0;
        }

        if (SettingsViewModel.ConnectSettings.MuMuEmulatorExtras.MuMuBridgeConnection)
        {
            return int.TryParse(SettingsViewModel.ConnectSettings.MuMuEmulatorExtras.Index, out var indexParse) ? indexParse : 0;
        }

        if (address.Contains(':'))
        {
            string portStr = address.Split(':')[1];
            if (!int.TryParse(portStr, out int port))
            {
                _logger.Error("Failed to parse port from address {Address}", address);
                return null;
            }

            return port switch
            {
                >= 16384 => MuMuPortToIndex(port),
                7555 => 0,
                >= 5555 => (port - 5555) / 2,
                _ => null,
            };
        }

        if (address.StartsWith("emulator-", StringComparison.OrdinalIgnoreCase))
        {
            string[] parts = address.Split('-');
            if (parts.Length >= 2 && int.TryParse(parts[1], out int port))
            {
                return (port - 5554) / 2;
            }

            _logger.Error("Failed to parse port from emulator style address {Address}", address);
            return null;
        }

        _logger.Error("Unsupported address format: {Address}", address);
        return null;
    }

    /// <summary>
    /// 一个用于调用 MuMu 模拟器控制台关闭 MuMu 的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    private static bool KillEmulatorMuMuEmulator()
    {
        string address = SettingsViewModel.ConnectSettings.ConnectAddress;
        int? emuIndexOpt = GetMuMuIndex(address);
        if (emuIndexOpt is null)
        {
            return false;
        }

        int emuIndex = emuIndexOpt.Value;

        // 尝试找到正在运行的模拟器进程
        Process[] processes = Process.GetProcessesByName("MuMuNxDevice"); // 新版
        if (processes.Length == 0)
        {
            processes = Process.GetProcessesByName("MuMuPlayer"); // 兼容旧版
        }

        if (processes.Length == 0)
        {
            return false;
        }

        ProcessModule? processModule;
        try
        {
            processModule = processes[0].MainModule;
        }
        catch (Exception e)
        {
            _logger.Error("Failed to get the main module of the emulator process.");
            _logger.Error("{EMessage}", e.Message);
            return false;
        }

        string? emulatorExePath = processModule?.FileName;
        if (emulatorExePath == null)
        {
            return false;
        }

        // 从 exe 路径回推安装目录
        // 新版路径推导: nx_device\12.0\shell\MuMuNxDevice.exe → 上三级目录 = 安装目录
        // 旧版路径推导: shell\MuMuPlayer.exe → 上一级目录 = 安装目录
        var installPath = Path.GetFullPath(Path.GetFileName(emulatorExePath).Equals("MuMuNxDevice.exe", StringComparison.OrdinalIgnoreCase)
            ? Path.Combine(Path.GetDirectoryName(emulatorExePath)!, @"..\..\..")
            : Path.Combine(Path.GetDirectoryName(emulatorExePath)!, ".."));

        // 新旧路径分别尝试 MuMuManager.exe
        string newConsolePath = Path.Combine(installPath, @"nx_main\MuMuManager.exe");
        string oldConsolePath = Path.Combine(installPath, @"shell\MuMuManager.exe");

        string? consolePath = null;
        if (File.Exists(newConsolePath))
        {
            consolePath = newConsolePath;
        }
        else if (File.Exists(oldConsolePath))
        {
            consolePath = oldConsolePath;
        }

        if (consolePath != null)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
            {
                Arguments = $"control -v {emuIndex} shutdown",
                CreateNoWindow = true,
                UseShellExecute = false,
            };
            var process = Process.Start(startInfo);
            if (process != null && process.WaitForExit(5000))
            {
                _logger.Information("Emulator at index {EmuIndex} closed through console. Console path: {ConsolePath}", emuIndex, consolePath);
                return true;
            }

            _logger.Warning("Console process at index {EmuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {ConsolePath}", emuIndex, consolePath);
            return KillEmulatorByWindow();
        }

        _logger.Error("MuMuManager.exe not found in expected locations (new or old). Trying to kill emulator by window.");
        return KillEmulatorByWindow();
    }

    /// <summary>
    /// 一个用于调用雷电模拟器控制台关闭雷电模拟器的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    private static bool KillEmulatorLdPlayer()
    {
        string address = SettingsViewModel.ConnectSettings.ConnectAddress;
        int emuIndex;
        if (address.Contains(':'))
        {
            string portStr = address.Split(':')[1];
            int port = int.Parse(portStr);
            emuIndex = (port - 5555) / 2;
        }
        else
        {
            string portStr = address.Split('-')[1];
            int port = int.Parse(portStr);
            emuIndex = (port - 5554) / 2;
        }

        Process[] processes = Process.GetProcessesByName("dnplayer");
        if (processes.Length <= 0)
        {
            return false;
        }

        ProcessModule? processModule;
        try
        {
            processModule = processes[0].MainModule;
        }
        catch (Exception e)
        {
            _logger.Error("Failed to get the main module of the emulator process.");
            _logger.Error("{EMessage}", e.Message);
            return false;
        }

        if (processModule == null)
        {
            return false;
        }

        string? emuLocation = processModule.FileName;
        emuLocation = Path.GetDirectoryName(emuLocation);
        if (emuLocation == null)
        {
            return false;
        }

        string consolePath = Path.Combine(emuLocation, "ldconsole.exe");

        if (File.Exists(consolePath))
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
            {
                Arguments = $"quit --index {emuIndex}",
                CreateNoWindow = true,
                UseShellExecute = false,
            };
            var process = Process.Start(startInfo);
            if (process != null && process.WaitForExit(5000))
            {
                _logger.Information("Emulator at index {EmuIndex} closed through console. Console path: {ConsolePath}", emuIndex, consolePath);
                return true;
            }

            _logger.Warning("Console process at index {EmuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {ConsolePath}", emuIndex, consolePath);
            return KillEmulatorByWindow();
        }

        _logger.Error("`{ConsolePath}` not found, try to kill emulator by window.", consolePath);
        return KillEmulatorByWindow();
    }

    /// <summary>
    /// 一个用于调用夜神模拟器控制台关闭夜神模拟器的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    private static bool KillEmulatorNox()
    {
        string address = SettingsViewModel.ConnectSettings.ConnectAddress;
        int emuIndex;
        if (address == "127.0.0.1:62001")
        {
            emuIndex = 0;
        }
        else
        {
            string portStr = address.Split(':')[1];
            int port = int.Parse(portStr);
            emuIndex = port - 62024;
        }

        Process[] processes = Process.GetProcessesByName("Nox");
        if (processes.Length <= 0)
        {
            return false;
        }

        ProcessModule? processModule;
        try
        {
            processModule = processes[0].MainModule;
        }
        catch (Exception e)
        {
            _logger.Error("Failed to get the main module of the emulator process.");
            _logger.Error("{EMessage}", e.Message);
            return false;
        }

        if (processModule == null)
        {
            return false;
        }

        string? emuLocation = processModule.FileName;
        emuLocation = Path.GetDirectoryName(emuLocation);
        if (emuLocation == null)
        {
            return false;
        }

        string consolePath = Path.Combine(emuLocation, "NoxConsole.exe");

        if (File.Exists(consolePath))
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
            {
                Arguments = $"quit -index:{emuIndex}",
                CreateNoWindow = true,
                UseShellExecute = false,
            };
            var process = Process.Start(startInfo);
            if (process != null && process.WaitForExit(5000))
            {
                _logger.Information("Emulator at index {EmuIndex} closed through console. Console path: {ConsolePath}", emuIndex, consolePath);
                return true;
            }

            _logger.Warning("Console process at index {EmuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {ConsolePath}", emuIndex, consolePath);
            return KillEmulatorByWindow();
        }

        _logger.Error("Error: `{ConsolePath}` not found, try to kill emulator by window.", consolePath);
        return KillEmulatorByWindow();
    }

    /// <summary>
    /// 一个用于调用逍遥模拟器控制台关闭逍遥模拟器的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    private static bool KillEmulatorXyaz()
    {
        string address = SettingsViewModel.ConnectSettings.ConnectAddress;
        string portStr = address.Split(':')[1];
        int port = int.Parse(portStr);
        var emuIndex = (port - 21503) / 10;

        Process[] processes = Process.GetProcessesByName("MEmu");
        if (processes.Length <= 0)
        {
            return false;
        }

        ProcessModule? processModule;
        try
        {
            processModule = processes[0].MainModule;
        }
        catch (Exception e)
        {
            _logger.Error("Failed to get the main module of the emulator process.");
            _logger.Error("{EMessage}", e.Message);
            return false;
        }

        if (processModule == null)
        {
            return false;
        }

        string? emuLocation = processModule.FileName;
        emuLocation = Path.GetDirectoryName(emuLocation);
        if (emuLocation == null)
        {
            return false;
        }

        string consolePath = Path.Combine(emuLocation, "memuc.exe");

        if (File.Exists(consolePath))
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
            {
                Arguments = $"stop -i {emuIndex}",
                CreateNoWindow = true,
                UseShellExecute = false,
            };
            var process = Process.Start(startInfo);
            if (process != null && process.WaitForExit(5000))
            {
                _logger.Information("Emulator at index {EmuIndex} closed through console. Console path: {ConsolePath}", emuIndex, consolePath);
                return true;
            }

            _logger.Warning("Console process at index {EmuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {ConsolePath}", emuIndex, consolePath);
            return KillEmulatorByWindow();
        }

        _logger.Error("`{ConsolePath}` not found, try to kill emulator by window.", consolePath);
        return KillEmulatorByWindow();
    }

    /// <summary>
    /// 一个用于关闭蓝叠模拟器的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    private static bool KillEmulatorBlueStacks()
    {
        Process[] processes = Process.GetProcessesByName("HD-Player");
        if (processes.Length <= 0)
        {
            return false;
        }

        ProcessModule? processModule;
        try
        {
            processModule = processes[0].MainModule;
        }
        catch (Exception e)
        {
            _logger.Error("Failed to get the main module of the emulator process.");
            _logger.Error("{EMessage}", e.Message);
            processModule = null;
        }

        if (processModule != null)
        {
            string? emuLocation = processModule.FileName;
            emuLocation = Path.GetDirectoryName(emuLocation);
            if (emuLocation != null)
            {
                string consolePath = Path.Combine(emuLocation, "bsconsole.exe");

                if (File.Exists(consolePath))
                {
                    _logger.Information("`{ConsolePath}` has been found. This may be the BlueStacks China emulator, try to kill the emulator by window.", consolePath);
                    return KillEmulatorByWindow();
                }

                _logger.Information("`{ConsolePath}` not found. This may be the BlueStacks International emulator, try to kill the emulator by the port.", consolePath);
            }
            else
            {
                _logger.Information("Failed to get emulator location from MainModule, try to kill the emulator by the port.");
            }
        }
        else
        {
            _logger.Information("Failed to get MainModule, try to kill the emulator by the port.");
        }
        if (KillEmulator())
        {
            return true;
        }

        _logger.Information("Failed to kill emulator by the port, try to kill emulator process with PID.");

        if (processes.Length > 1)
        {
            _logger.Warning("The number of elements in processes exceeds one, abort closing the emulator");
            return false;
        }

        try
        {
            processes[0].Kill();
            return processes[0].WaitForExit(20000);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to kill emulator process with PID {ProcessId}. Exception: {ExceptionMessage}", processes[0].Id, ex.Message);
        }

        return false;
    }

    /// <summary>
    /// Kills emulator by Window hwnd.
    /// </summary>
    /// <returns>Whether the operation is successful.</returns>
    private static bool KillEmulatorByWindow()
    {
        int pid = 0;
        var windowName = new[]
        {
            "明日方舟",
            "明日方舟 - MuMu模拟器",
            "BlueStacks App Player",
            "BlueStacks",
            "Google Play Games on PC Emulator",
        };
        foreach (string i in windowName)
        {
            var hwnd = FindWindow(null!, i);
            if (hwnd == IntPtr.Zero)
            {
                continue;
            }

            GetWindowThreadProcessId(hwnd, out pid);
            break;
        }

        if (pid == 0)
        {
            return KillEmulator();
        }

        try
        {
            var emulator = Process.GetProcessById(pid);
            emulator.CloseMainWindow();
            if (!emulator.WaitForExit(5000))
            {
                emulator.Kill();
                if (emulator.WaitForExit(5000))
                {
                    _logger.Information("Emulator with process ID {Pid} killed successfully.", pid);
                    KillEmulator();
                    return true;
                }

                _logger.Error("Failed to kill emulator with process ID {Pid}.", pid);
                return false;
            }

            // 尽管已经成功 CloseMainWindow()，再次尝试 killEmulator()
            // Refer to https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/1878
            KillEmulator();

            // 已经成功 CloseMainWindow()，所以不管 killEmulator() 的结果如何，都返回 true
            return true;
        }
        catch
        {
            _logger.Error("Kill emulator by window error");
        }

        return KillEmulator();
    }

    /// <summary>
    /// 检测 MuMu 模拟器的后台保活是否开启。
    /// 通过 MuMuManager.exe 读取 app_keptlive 配置项。
    /// </summary>
    /// <returns>true 表示保活已开启（可能导致截图和操作异常），false 表示未开启或检测失败。</returns>
    public static bool CheckMuMuKeepAlive()
    {
        try
        {
            // 从连接地址推导实例索引
            string address = SettingsViewModel.ConnectSettings.ConnectAddress;
            int? emuIndexOpt = GetMuMuIndex(address);
            if (emuIndexOpt is null)
            {
                _logger.Information("Cannot determine MuMu index for keep alive check, address: {Address}", address);
                return false;
            }

            int emuIndex = emuIndexOpt.Value;

            // 查找 MuMuManager.exe 路径
            string? consolePath = FindMuMuManagerPath();
            if (consolePath == null)
            {
                _logger.Information("MuMuManager.exe not found, skip keep alive check");
                return false;
            }

            // 调用 MuMuManager.exe setting -v {emuIndex} -k app_keptlive
            // 返回 JSON 格式: { "app_keptlive": "true" }
            var startInfo = new ProcessStartInfo(consolePath)
            {
                Arguments = $"setting -v {emuIndex} -k app_keptlive",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                CreateNoWindow = true,
            };

            using var process = Process.Start(startInfo);
            if (process == null || !process.WaitForExit(5000))
            {
                _logger.Warning("MuMuManager.exe did not exit within timeout");
                return false;
            }

            string output = process.StandardOutput.ReadToEnd().Trim();
            _logger.Information("MuMuManager app_keptlive output: {Output}", output);

            if (string.IsNullOrEmpty(output))
            {
                return false;
            }

            // 解析 JSON 输出
            using var doc = JsonDocument.Parse(output);
            if (doc.RootElement.TryGetProperty("app_keptlive", out var value))
            {
                return value.GetString() == "true";
            }

            return false;
        }
        catch (Exception e)
        {
            _logger.Warning("Failed to check MuMu keep alive: {EMessage}", e.Message);
            return false;
        }
    }

    /// <summary>
    /// 查找 MuMuManager.exe 的路径，优先从运行中的 MuMu 进程获取，回退到用户配置的安装路径。
    /// </summary>
    /// <returns>MuMuManager.exe 的完整路径，找不到则返回 null。</returns>
    private static string? FindMuMuManagerPath()
    {
        // 优先从运行中的模拟器进程获取安装目录
        Process[] processes = Process.GetProcessesByName("MuMuNxDevice");
        if (processes.Length == 0)
        {
            processes = Process.GetProcessesByName("MuMuPlayer");
        }

        if (processes.Length > 0)
        {
            try
            {
                string? emulatorExePath = processes[0].MainModule?.FileName;
                if (emulatorExePath != null)
                {
                    // 新版: nx_device\12.0\shell\MuMuNxDevice.exe → 上三级 = 安装目录
                    // 旧版: shell\MuMuPlayer.exe → 上一级 = 安装目录
                    var installPath = Path.GetFullPath(
                        Path.GetFileName(emulatorExePath).Equals("MuMuNxDevice.exe", StringComparison.OrdinalIgnoreCase)
                            ? Path.Combine(Path.GetDirectoryName(emulatorExePath)!, @"..\..\..")
                            : Path.Combine(Path.GetDirectoryName(emulatorExePath)!, ".."));

                    string newConsolePath = Path.Combine(installPath, @"nx_main\MuMuManager.exe");
                    if (File.Exists(newConsolePath))
                    {
                        return newConsolePath;
                    }

                    string oldConsolePath = Path.Combine(installPath, @"shell\MuMuManager.exe");
                    if (File.Exists(oldConsolePath))
                    {
                        return oldConsolePath;
                    }
                }
            }
            catch (Exception e)
            {
                _logger.Warning("Failed to get MuMu process module path: {EMessage}", e.Message);
            }
        }

        // 回退：从用户配置的 MuMu 安装路径查找
        string? configuredPath = SettingsViewModel.ConnectSettings.MuMuEmulatorExtras.EmulatorPath;
        if (!string.IsNullOrEmpty(configuredPath) && Directory.Exists(configuredPath))
        {
            string newConsolePath = Path.Combine(configuredPath, @"nx_main\MuMuManager.exe");
            if (File.Exists(newConsolePath))
            {
                return newConsolePath;
            }

            string oldConsolePath = Path.Combine(configuredPath, @"shell\MuMuManager.exe");
            if (File.Exists(oldConsolePath))
            {
                return oldConsolePath;
            }
        }

        return null;
    }

    /// <summary>
    /// Kills emulator.
    /// </summary>
    /// <returns>Whether the operation is successful.</returns>
    public static bool KillEmulator()
    {
        int pid = 0;
        string address = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);
        var port = address.StartsWith("127") ? address[10..] : "5555";
        _logger.Information("address: {Address}, port: {Port}", address, port);

        string portCmd = "netstat -ano|findstr \"" + port + "\"";
        Process checkCmd = new Process
        {
            StartInfo =
                {
                    FileName = "cmd.exe",
                    UseShellExecute = false,
                    RedirectStandardInput = true,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                },
        };
        try
        {
            checkCmd.Start();
        }
        catch
        {
            _logger.Error("Failed to start cmd.exe");
            checkCmd.Close();
            return false;
        }

        checkCmd.StandardInput.WriteLine(portCmd);
        checkCmd.StandardInput.WriteLine("exit");
        Regex reg = new Regex("\\s+", RegexOptions.Compiled);
        while (true)
        {
            var line = checkCmd.StandardOutput.ReadLine();
            line = line?.Trim();

            if (line == null)
            {
                break;
            }

            if (!line.StartsWith("TCP", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            line = reg.Replace(line, ",");

            try
            {
                string[] arr = line.Split(',');
                if (arr.Length >= 2
                    && Convert.ToBoolean(string.Compare(arr[1], address, StringComparison.Ordinal)))
                {
                    continue;
                }

                pid = int.Parse(arr[4]);
                break;
            }
            catch (Exception e)
            {
                _logger.Error("Failed to parse cmd.exe output: " + e.Message);
            }
        }

        if (pid == 0)
        {
            _logger.Error("Failed to get emulator PID");
            return false;
        }

        try
        {
            Process emulator = Process.GetProcessById(pid);
            emulator.Kill();
        }
        catch
        {
            return false;
        }
        finally
        {
            checkCmd.Close();
        }

        return true;
    }
}
