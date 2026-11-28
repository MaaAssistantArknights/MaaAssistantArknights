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
using System.Windows;
using HandyControl.Controls;
using HandyControl.Data;
using Serilog;
using Stylet;
using Window = System.Windows.Window;

namespace MaaWpfGui.Helper;

/// <summary>
/// 在目标窗口可见时显示 Growl 通知，并在日志中记录内容。未指定目标时使用主窗口。
/// </summary>
public static class GrowlHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(GrowlHelper));

    private static readonly List<(string Type, string? Message, Action Show)> _pending = [];

    private static bool _instantiatedSubscribed;

    /// <summary>
    /// 显示信息级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Info(string message)
        => Show(InfoType.Info, message, () => Growl.Info(message));

    /// <summary>
    /// 显示信息级 Growl 通知。
    /// </summary>
    /// <param name="info">通知内容与样式。</param>
    public static void Info(GrowlInfo info)
        => Show(InfoType.Info, info.Message, () => Growl.Info(info));

    /// <summary>
    /// 显示警告级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    /// <param name="target">通知所属的窗口。省略时使用主窗口。</param>
    public static void Warning(string message, WindowTarget? target = null)
        => Show(InfoType.Warning, message, () => Growl.Warning(message, target?.Token ?? string.Empty), target);

    /// <summary>
    /// 显示错误级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Error(string message)
        => Show(InfoType.Error, message, () => Growl.Error(message));

    /// <summary>
    /// 显示成功级 Growl 通知。
    /// </summary>
    /// <param name="message">通知内容。</param>
    public static void Success(string message)
        => Show(InfoType.Success, message, () => Growl.Success(message));

    /// <summary>
    /// 显示成功级 Growl 通知。
    /// </summary>
    /// <param name="info">通知内容与样式。</param>
    public static void Success(GrowlInfo info)
        => Show(InfoType.Success, info.Message, () => Growl.Success(info));

    /// <summary>
    /// 在 UI 线程显示通知。窗口隐藏或最小化时暂存通知，待窗口恢复后补发。
    /// </summary>
    /// <param name="type">通知类型，仅用于日志。</param>
    /// <param name="message">通知内容，仅用于日志。</param>
    /// <param name="showGrowl">执行 Growl 显示的闭包，将在 UI 线程上调用。</param>
    /// <param name="target">通知所属的窗口。省略时使用主窗口。</param>
    public static void Show(InfoType type, string? message, Action showGrowl, WindowTarget? target = null)
    {
        // gui.log 模板为 {Message:lj}，非字符串值按 JSON 渲染会给枚举加引号，故先转字符串
        var typeName = type.ToString();
        Execute.OnUIThread(() => {
            if (target is not null)
            {
                target.Show(typeName, message, showGrowl);
                return;
            }

            if (Instances.MainWindowManager?.GetWindowIfVisible() is null)
            {
                _logger.Information("Growl {Type} (deferred, main window hidden): {Message}", typeName, message);
                _pending.Add((typeName, message, showGrowl));
                EnsureHandlersAttached();
                return;
            }

            Display(typeName, message, showGrowl);
        });
    }

    private static void Display(string type, string? message, Action showGrowl, bool deferred = false)
    {
        _logger.Information("Growl {Type}{Status}: {Message}", type, deferred ? " (flushed)" : string.Empty, message);
        try
        {
            showGrowl();
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to show growl {Type}: {Message}", type, message);
            if (!deferred)
            {
                throw;
            }
        }
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
            if (win == null || _pending.Count == 0)
            {
                return;
            }

            // 先取出并清空再逐个执行：单个闭包抛出时不中断其余项，已显示项也不会在下次恢复时重复补发
            var pending = _pending.ToArray();
            _pending.Clear();
            foreach (var (type, message, showGrowl) in pending)
            {
                Display(type, message, showGrowl, deferred: true);
            }
        });
    }

    /// <summary>
    /// 管理一个窗口内的待显示通知。窗口复用时，每个使用该窗口的视图模型可独立结束自己的通知。
    /// </summary>
    public sealed class WindowTarget
    {
        private readonly Queue<(string Type, string? Message, Action Show)> _pending = new();
        private Window? _window;

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowTarget"/> class.
        /// </summary>
        /// <param name="window">承载通知的窗口。</param>
        /// <param name="token">窗口内 Growl 容器注册的标识。</param>
        public WindowTarget(Window window, string token)
        {
            _window = window;
            Token = token;
            window.Loaded += OnWindowChanged;
            window.StateChanged += OnWindowChanged;
            window.IsVisibleChanged += OnIsVisibleChanged;
            window.Closed += OnClosed;
        }

        /// <summary>
        /// Gets 窗口内 Growl 容器注册的标识。
        /// </summary>
        public string Token { get; }

        internal bool IsDetached => _window is null;

        private bool CanShow => _window is { IsLoaded: true, IsVisible: true, WindowState: not WindowState.Minimized };

        /// <summary>
        /// 窗口关闭或改由其他视图模型使用时，丢弃待显示通知，并解除对窗口的订阅。
        /// </summary>
        public void Detach()
        {
            if (_window is null)
            {
                return;
            }

            _window.Loaded -= OnWindowChanged;
            _window.StateChanged -= OnWindowChanged;
            _window.IsVisibleChanged -= OnIsVisibleChanged;
            _window.Closed -= OnClosed;
            _window = null;
            while (_pending.TryDequeue(out var notification))
            {
                LogDiscarded(notification.Type, notification.Message);
            }
        }

        internal void Show(string type, string? message, Action showGrowl)
        {
            if (IsDetached)
            {
                LogDiscarded(type, message);
            }
            else if (CanShow)
            {
                Display(type, message, showGrowl);
            }
            else
            {
                _logger.Information("Growl {Type} (deferred, target window hidden): {Message}", type, message);
                _pending.Enqueue((type, message, showGrowl));
            }
        }

        private static void LogDiscarded(string type, string? message)
            => _logger.Information("Growl {Type} (discarded, target detached): {Message}", type, message);

        private void OnClosed(object? sender, EventArgs e) => Detach();

        private void OnWindowChanged(object? sender, EventArgs e) => FlushPending();

        private void OnIsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e) => FlushPending();

        private void FlushPending()
        {
            while (CanShow && _pending.TryDequeue(out var notification))
            {
                Display(notification.Type, notification.Message, notification.Show, deferred: true);
            }
        }
    }
}
