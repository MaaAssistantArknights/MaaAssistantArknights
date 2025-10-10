// <copyright file="PowerManagement.cs" company="MaaAssistantArknights">
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

using System;
using System.Diagnostics;
using Serilog;
using Windows.Win32;

namespace MaaWpfGui.Utilities;

public class PowerManagement
{
    private static readonly ILogger _logger = Log.ForContext<PowerManagement>();

    /// <summary>
    /// 关机，使用 shutdown.exe
    /// </summary>
    /// <param name="delaySeconds">等待关机时间</param>
    public static void Shutdown(int delaySeconds = 70)
    {
        try
        {
            _logger.Information("Scheduling shutdown in {DelaySeconds} seconds.", delaySeconds);
            Process.Start("shutdown.exe", $"-s -t {delaySeconds}");
        }
        catch (Exception ex)
        {
            _logger.Error("Shutdown failed: {ExMessage}", ex.Message);
        }
    }

    /// <summary>
    /// 取消正在进行的 shutdown.exe
    /// </summary>
    public static void AbortShutdown()
    {
        try
        {
            _logger.Information("Aborting shutdown.");
            Process.Start("shutdown.exe", "-a");
        }
        catch (Exception ex)
        {
            _logger.Error("Abort shutdown failed: {ExMessage}", ex.Message);
        }
    }

    public static bool Hibernate()
    {
        try
        {
            return PInvoke.SetSuspendState(true, true, true);
        }
        catch (Exception ex)
        {
            _logger.Error("Hibernate error: {ExMessage}", ex.Message);
            return false;
        }
    }

    public static bool Sleep()
    {
        try
        {
            return PInvoke.SetSuspendState(false, true, true);
        }
        catch (Exception ex)
        {
            _logger.Error("Sleep error: {ExMessage}", ex.Message);
            return false;
        }
    }
}
