// <copyright file="MessageBoxHelper.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1401 // 按钮文案为公开字段，供全局定制

#nullable enable

using System;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows;
using HandyControl.Data;
using MaaWpfGui.Constants;
using Serilog;
using HcLang = HandyControl.Properties.Langs.Lang;

namespace MaaWpfGui.Helper;

/// <summary>
/// 消息框弹窗格式管理
/// </summary>
public static class MessageBoxHelper
{
    private const string Separator = "---------------------------";

    private static readonly ILogger _logger = Log.ForContext(typeof(MessageBoxHelper));

    private static int _showCount;

    /// <summary>
    /// OK text
    /// </summary>
    public static string OK = "OK";

    /// <summary>
    /// Cancel text
    /// </summary>
    public static string Cancel = "Cancel";

    /// <summary>
    /// Abort text
    /// </summary>
    public static string Abort = "Abort";

    /// <summary>
    /// Retry text
    /// </summary>
    public static string Retry = "Retry";

    /// <summary>
    /// Ignore text
    /// </summary>
    public static string Ignore = "Ignore";

    /// <summary>
    /// Yes text
    /// </summary>
    public static string Yes = "Yes";

    /// <summary>
    /// No text
    /// </summary>
    public static string No = "No";

    private static void SetImage(MessageBoxImage messageBoxImage, ref string iconKey, ref string iconBrushKey)
    {
        var key = string.Empty;
        var brushKey = string.Empty;

        switch (messageBoxImage)
        {
            case MessageBoxImage.Question:
                key = ResourceToken.AskGeometry;
                brushKey = ResourceToken.AccentBrush;
                break;

            case MessageBoxImage.Error:
                key = ResourceToken.ErrorGeometry;
                brushKey = ResourceToken.DangerBrush;
                break;

            case MessageBoxImage.Warning:
                key = ResourceToken.WarningGeometry;
                brushKey = ResourceToken.WarningBrush;
                break;

            case MessageBoxImage.Information:
                key = ResourceToken.InfoGeometry;
                brushKey = ResourceToken.InfoBrush;
                break;
        }

        iconKey = string.IsNullOrEmpty(iconKey) ? key : iconKey;
        iconBrushKey = string.IsNullOrEmpty(iconBrushKey) ? brushKey : iconBrushKey;
    }

    public static MessageBoxResult Show(MessageBoxInfo info) => ShowHandyControl(info).Result;

    public static MessageBoxResult Show(
        string messageBoxText,
        string caption = "",
        MessageBoxButton buttons = MessageBoxButton.OK,
        MessageBoxImage icon = MessageBoxImage.None,
        string iconKey = "",
        string iconBrushKey = "",
        string ok = "",
        string cancel = "",
        string yes = "",
        string no = "",
        MessageBoxResult? defaultResult = null)
    {
        caption = string.IsNullOrEmpty(caption) ? LocalizationHelper.GetString("Common.Tip") : caption;
        ok = string.IsNullOrEmpty(ok) ? LocalizationHelper.GetString("Common.Ok") : ok;
        cancel = string.IsNullOrEmpty(cancel) ? LocalizationHelper.GetString("Common.ManualRestart") : cancel;
        yes = string.IsNullOrEmpty(yes) ? LocalizationHelper.GetString("Common.Yes") : yes;
        no = string.IsNullOrEmpty(no) ? LocalizationHelper.GetString("Common.No") : no;

        SetImage(icon, ref iconKey, ref iconBrushKey);
        var info = new MessageBoxInfo
        {
            Message = messageBoxText,
            Caption = caption,
            Button = buttons,
            IconKey = iconKey,
            IconBrushKey = iconBrushKey,
            ConfirmContent = ok,
            CancelContent = cancel,
            YesContent = yes,
            NoContent = no,
            DefaultResult = defaultResult ?? MessageBoxResult.None,
        };

        var (result, duration) = ShowHandyControl(info);

        if (duration.TotalSeconds <= 1)
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.QuickCloser);
        }

        return result;
    }

    /// <summary>
    /// 弹出 HandyControl 弹窗并记录日志，按钮文案为 null 时取 HandyControl 内置的默认文案。
    /// </summary>
    /// <param name="info">弹窗参数。</param>
    /// <returns>弹窗结果与停留时长。</returns>
    private static (MessageBoxResult Result, TimeSpan Duration) ShowHandyControl(MessageBoxInfo info)
        => ShowWithLog(
            info.Caption,
            info.Message,
            GetVisibleButtons(
                info.Button,
                info.ConfirmContent ?? HcLang.Confirm,
                info.CancelContent ?? HcLang.Cancel,
                info.YesContent ?? HcLang.Yes,
                info.NoContent ?? HcLang.No),
            () => HandyControl.Controls.MessageBox.Show(info));

    /// <summary>
    /// 按 <paramref name="buttons"/> 列出弹窗上可见的按钮及其显示文案，顺序与 HandyControl 弹窗 Ctrl+C 复制文本一致。
    /// </summary>
    /// <param name="buttons">按钮组合。</param>
    /// <param name="ok">确定按钮文案。</param>
    /// <param name="cancel">取消按钮文案。</param>
    /// <param name="yes">是按钮文案。</param>
    /// <param name="no">否按钮文案。</param>
    /// <returns>可见按钮对应的结果值与文案。</returns>
    private static (MessageBoxResult Result, string Text)[] GetVisibleButtons(MessageBoxButton buttons, string ok, string cancel, string yes, string no)
        => buttons switch
        {
            MessageBoxButton.OK => [(MessageBoxResult.OK, ok)],
            MessageBoxButton.OKCancel => [(MessageBoxResult.OK, ok), (MessageBoxResult.Cancel, cancel)],
            MessageBoxButton.YesNo => [(MessageBoxResult.Yes, yes), (MessageBoxResult.No, no)],
            MessageBoxButton.YesNoCancel => [(MessageBoxResult.Yes, yes), (MessageBoxResult.No, no), (MessageBoxResult.Cancel, cancel)],
            _ => [],
        };

    /// <summary>
    /// 弹出前以 HandyControl 弹窗 Ctrl+C 复制文本的格式记录弹窗内容，关闭后记录用户选择与停留时长；
    /// 两条日志带同一序号，便于在并发或嵌套弹窗时配对。
    /// </summary>
    /// <param name="caption">标题。</param>
    /// <param name="text">正文。</param>
    /// <param name="visibleButtons">可见按钮及其文案。</param>
    /// <param name="show">实际弹出弹窗并返回结果的委托。</param>
    /// <returns>弹窗结果与停留时长。</returns>
    private static (MessageBoxResult Result, TimeSpan Duration) ShowWithLog(
        string? caption,
        string? text,
        (MessageBoxResult Result, string Text)[] visibleButtons,
        Func<MessageBoxResult> show)
    {
        var id = Interlocked.Increment(ref _showCount);
        var content = new StringBuilder()
            .AppendLine()
            .AppendLine(Separator)
            .AppendLine(caption)
            .AppendLine(Separator)
            .AppendLine(text)
            .AppendLine(Separator)
            .AppendLine(string.Concat(visibleButtons.Select(b => b.Text + "   ")))
            .Append(Separator)
            .ToString();
        _logger.Information("MessageBox #{Id} shown:{Content}", id, content);

        var stopwatch = Stopwatch.StartNew();
        MessageBoxResult result;
        try
        {
            result = show();
        }
        catch (Exception e)
        {
            _logger.Error(e, "MessageBox #{Id} failed", id);
            throw;
        }

        var duration = stopwatch.Elapsed;

        // HandyControl 弹窗带取消按钮时，Esc 与标题栏关闭都返回 Cancel，与点击取消无法区分；
        // 仅有确定按钮时，Esc 等同点击确定（OK），标题栏关闭返回初始值 Yes，因不对应可见按钮只记枚举值
        var buttonText = visibleButtons.FirstOrDefault(b => b.Result == result).Text;
        var selected = buttonText is null ? $"({result})" : $"{buttonText} ({result})";
        _logger.Information(
            "MessageBox #{Id} closed: {Selected}, {Duration}s",
            id,
            selected,
            duration.TotalSeconds.ToString("0.0", CultureInfo.InvariantCulture));

        return (result, duration);
    }
}
