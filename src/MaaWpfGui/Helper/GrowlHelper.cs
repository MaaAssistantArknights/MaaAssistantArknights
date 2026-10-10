// <copyright file="GrowlHelper.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Collections.Generic;
using HandyControl.Controls;
using HandyControl.Data;
using Stylet;

namespace MaaWpfGui.Helper;

/// <summary>
/// Growl 统一入口：主窗口可见时立即显示；不可见（未创建或最小化到托盘）时暂存，
/// 待窗口显示或从托盘恢复后补发，避免此期间的 Growl 因无容器或不可见而静默丢失。
/// </summary>
public static class GrowlHelper
{
    private static readonly List<Action> _pending = [];

    private static bool _instantiatedSubscribed;

    /// <summary>
    /// 显示信息级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Info(string message)
        => Show(() => Growl.Info(message));

    /// <summary>
    /// 显示信息级 Growl 通知。
    /// </summary>
    /// <param name="info">通知内容与样式。</param>
    public static void Info(GrowlInfo info)
        => Show(() => Growl.Info(info));

    /// <summary>
    /// 显示警告级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Warning(string message)
        => Show(() => Growl.Warning(message));

    /// <summary>
    /// 显示错误级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Error(string message)
        => Show(() => Growl.Error(message));

    /// <summary>
    /// 显示成功级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Success(string message)
        => Show(() => Growl.Success(message));

    /// <summary>
    /// 显示成功级 Growl 通知。
    /// </summary>
    /// <param name="info">通知内容与样式。</param>
    public static void Success(GrowlInfo info)
        => Show(() => Growl.Success(info));

    /// <summary>
    /// Growl 显示的完全自定义底层入口：主窗口可见时立即执行，不可见时暂存，
    /// 待窗口显示或从托盘恢复后在 UI 线程上补发。需要围绕显示做自定义前后处理时使用。
    /// </summary>
    /// <param name="showGrowl">执行 Growl 显示的闭包，将在 UI 线程上调用。</param>
    public static void Show(Action showGrowl)
    {
        Execute.OnUIThread(() => {
            var win = Instances.MainWindowManager?.GetWindowIfVisible();
            if (win == null)
            {
                _pending.Add(showGrowl);
                EnsureHandlersAttached();
                return;
            }

            showGrowl();
        });
    }

    private static void EnsureHandlersAttached()
    {
        if (Instances.MainWindowManager is not null)
        {
            // 常驻订阅 restored（先减后加幂等），覆盖暂存后窗口再次最小化/恢复的循环
            Instances.MainWindowManager.WindowRestored -= OnWindowRestored;
            Instances.MainWindowManager.WindowRestored += OnWindowRestored;
            return;
        }

        if (_instantiatedSubscribed)
        {
            return;
        }

        _instantiatedSubscribed = true;

        // 事件触发时 MainWindowManager 必然已就绪，回调内完成 restored 挂载后补发
        Instances.MainWindowManagerInstantiated += (_, _) => {
            EnsureHandlersAttached();
            TryShowPending();
        };
    }

    private static void OnWindowRestored(object? sender, EventArgs e)
    {
        TryShowPending();
    }

    private static void TryShowPending()
    {
        Execute.OnUIThread(() => {
            // Instantiated 时窗口可能仍不可见（启动即最小化到托盘），此时保留队列等 restored
            var win = Instances.MainWindowManager?.GetWindowIfVisible();
            if (win == null)
            {
                return;
            }

            foreach (var showGrowl in _pending)
            {
                showGrowl();
            }

            _pending.Clear();
        });
    }
}
