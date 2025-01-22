// <copyright file="EmulatorHelper.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
                "MuMuEmulator12" => KillEmulatorMuMuEmulator12(),
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
    /// 一个用于调用 MuMu12 模拟器控制台关闭 MuMu12 的方法
    /// </summary>
    /// <returns>是否关闭成功</returns>
    private static bool KillEmulatorMuMuEmulator12()
    {
        string address = SettingsViewModel.ConnectSettings.ConnectAddress;
        int emuIndex;
        if (address == "127.0.0.1:16384")
        {
            emuIndex = 0;
        }
        else
        {
            string portStr = address.Split(':')[1];
            int port = int.Parse(portStr);
            emuIndex = (port - 16384) / 32;
        }

        Process[] processes = Process.GetProcessesByName("MuMuPlayer");
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
            _logger.Error("Error: Failed to get the main module of the emulator process.");
            _logger.Error(e.Message);
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

        string consolePath = Path.Combine(emuLocation, "MuMuManager.exe");

        if (File.Exists(consolePath))
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
            {
                Arguments = $"api -v {emuIndex} shutdown_player",
                CreateNoWindow = true,
                UseShellExecute = false,
            };
            var process = Process.Start(startInfo);
            if (process != null && process.WaitForExit(5000))
            {
                _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                return true;
            }

            _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
            return KillEmulatorByWindow();
        }

        _logger.Error($"Error: `{consolePath}` not found, try to kill emulator by window.");
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
            _logger.Error("Error: Failed to get the main module of the emulator process.");
            _logger.Error(e.Message);
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
                _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                return true;
            }

            _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
            return KillEmulatorByWindow();
        }

        _logger.Information($"Error: `{consolePath}` not found, try to kill emulator by window.");
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
            _logger.Error("Error: Failed to get the main module of the emulator process.");
            _logger.Error(e.Message);
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
                _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                return true;
            }

            _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
            return KillEmulatorByWindow();
        }

        _logger.Information($"Error: `{consolePath}` not found, try to kill emulator by window.");
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
            _logger.Error("Error: Failed to get the main module of the emulator process.");
            _logger.Error(e.Message);
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
                _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                return true;
            }

            _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
            return KillEmulatorByWindow();
        }

        _logger.Information($"Error: `{consolePath}` not found, try to kill emulator by window.");
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
            _logger.Error("Error: Failed to get the main module of the emulator process.");
            _logger.Error(e.Message);
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

        string consolePath = Path.Combine(emuLocation, "bsconsole.exe");

        if (File.Exists(consolePath))
        {
            _logger.Information($"Info: `{consolePath}` has been found. This may be the BlueStacks China emulator, try to kill the emulator by window.");
            return KillEmulatorByWindow();
        }

        _logger.Information($"Info: `{consolePath}` not found. This may be the BlueStacks International emulator, try to kill the emulator by the port.");
        if (KillEmulator())
        {
            return true;
        }

        _logger.Information("Info: Failed to kill emulator by the port, try to kill emulator process with PID.");

        if (processes.Length > 1)
        {
            _logger.Warning("Warning: The number of elements in processes exceeds one, abort closing the emulator");
            return false;
        }

        try
        {
            processes[0].Kill();
            return processes[0].WaitForExit(20000);
        }
        catch (Exception ex)
        {
            _logger.Error($"Error: Failed to kill emulator process with PID {processes[0].Id}. Exception: {ex.Message}");
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
        };
        foreach (string i in windowName)
        {
            var hwnd = FindWindow(null, i);
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
                    _logger.Information($"Emulator with process ID {pid} killed successfully.");
                    KillEmulator();
                    return true;
                }

                _logger.Error($"Failed to kill emulator with process ID {pid}.");
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
    /// Kills emulator.
    /// </summary>
    /// <returns>Whether the operation is successful.</returns>
    public static bool KillEmulator()
    {
        int pid = 0;
        string address = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);
        var port = address.StartsWith("127") ? address.Substring(10) : "5555";
        _logger.Information($"address: {address}, port: {port}");

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
