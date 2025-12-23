// <copyright file="MessageBoxHelper.cs" company="MaaAssistantArknights">
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

#pragma warning disable CS0618
#pragma warning disable SA1401

using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security;
using System.Windows;
using System.Windows.Forms;
using HandyControl.Data;
using MaaWpfGui.Constants;

[assembly: SecurityCritical]
[assembly: SecurityTreatAsSafe]

namespace MaaWpfGui.Helper;

/// <summary>
/// 消息框弹窗格式管理
/// </summary>
public static class MessageBoxHelper
{
    /// <summary>
    /// OK text
    /// </summary>
    // ReSharper disable once InconsistentNaming
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

    public static MessageBoxResult Show(MessageBoxInfo info) => HandyControl.Controls.MessageBox.Show(info);

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
        bool useNativeMethod = false) => Show(WindowHandle.None, messageBoxText, caption, buttons, icon, iconKey, iconBrushKey, ok, cancel, yes, no, useNativeMethod);

    public static MessageBoxResult Show(
        WindowHandle ownerWindow,
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
        bool useNativeMethod = false)
    {
        caption = string.IsNullOrEmpty(caption) ? LocalizationHelper.GetString("Tip") : caption;
        ok = string.IsNullOrEmpty(ok) ? LocalizationHelper.GetString("Ok") : ok;
        cancel = string.IsNullOrEmpty(cancel) ? LocalizationHelper.GetString("ManualRestart") : cancel;
        yes = string.IsNullOrEmpty(yes) ? LocalizationHelper.GetString("Yes") : yes;
        no = string.IsNullOrEmpty(no) ? LocalizationHelper.GetString("No") : no;

        if (useNativeMethod)
        {
            return ShowNative(ownerWindow, messageBoxText, string.Empty, caption, buttons, icon, false, ok, cancel, yes, no);
        }
        else
        {
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
            };

            DateTime startTime = DateTime.Now;

            var result = HandyControl.Controls.MessageBox.Show(info);

            var duration = DateTime.Now - startTime;
            if (duration.TotalSeconds <= 1)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.QuickCloser);
            }

            return result;
        }
    }

    private unsafe readonly ref struct DisposablePin<T>
        where T : unmanaged
    {
        private readonly GCHandle _handle;

        public DisposablePin(object obj)
        {
            _handle = GCHandle.Alloc(obj, GCHandleType.Pinned);
        }

        public T* AddrOfPinnedObject => (T*)_handle.AddrOfPinnedObject();

        public void Dispose()
        {
            _handle.Free();
        }
    }

    public static MessageBoxResult ShowNative(
       WindowHandle ownerWindow,
       string messageBoxText,
       string mainInstruction = "",
       string windowTitle = "",
       MessageBoxButton buttons = MessageBoxButton.OK,
       MessageBoxImage icon = MessageBoxImage.None,
       bool alwaysAllowCancel = false,
       string ok = "",
       string cancel = "",
       string yes = "",
       string no = "")
    {
        using var contentPin = new DisposablePin<char>(messageBoxText);
        using var instructionPin = new DisposablePin<char>(mainInstruction);
        using var titlePin = new DisposablePin<char>(windowTitle);

        // Use TaskDialogPage / TaskDialog.ShowDialog to display native-style dialog page
        var page = new TaskDialogPage
        {
            Caption = windowTitle,
            Heading = mainInstruction,
            Text = messageBoxText,
            SizeToContent = true,
        };

        // Icon
        switch (icon)
        {
            case MessageBoxImage.Information:
                page.Icon = TaskDialogIcon.Information;
                break;
            case MessageBoxImage.Hand:
                page.Icon = TaskDialogIcon.Error;
                break;
            case MessageBoxImage.Exclamation:
                page.Icon = TaskDialogIcon.Warning;
                break;
            case MessageBoxImage.Question:
                page.Icon = TaskDialogIcon.Shield;
                break;
            case MessageBoxImage.None:
                page.Icon = TaskDialogIcon.None;
                break;
        }

        // Buttons
        switch (buttons)
        {
            case MessageBoxButton.OK:
                page.Buttons.Add(TaskDialogButton.OK);
                break;
            case MessageBoxButton.OKCancel:
                page.Buttons.Add(TaskDialogButton.OK);
                page.Buttons.Add(TaskDialogButton.Cancel);
                break;
            case MessageBoxButton.YesNo:
                page.Buttons.Add(TaskDialogButton.Yes);
                page.Buttons.Add(TaskDialogButton.No);
                break;
            case MessageBoxButton.YesNoCancel:
                page.Buttons.Add(TaskDialogButton.Yes);
                page.Buttons.Add(TaskDialogButton.No);
                page.Buttons.Add(TaskDialogButton.Cancel);
                break;
        }

        // Custom labels
        if (!string.IsNullOrEmpty(ok))
        {
            page.Buttons[0].Text = ok;
        }

        if (!string.IsNullOrEmpty(cancel))
        {
            page.Buttons.FirstOrDefault(b => b == System.Windows.Forms.TaskDialogButton.Cancel)?.Text = cancel;
        }

        if (!string.IsNullOrEmpty(yes))
        {
            page.Buttons.FirstOrDefault(b => b == TaskDialogButton.Yes)?.Text = yes;
        }

        if (!string.IsNullOrEmpty(no))
        {
            page.Buttons.FirstOrDefault(b => b == TaskDialogButton.No)?.Text = no;
        }

        // Allow cancel
        if (alwaysAllowCancel)
        {
            page.AllowCancel = true;
        }

        // Owner window
        IWin32Window? owner = null;
        if (ownerWindow.Handle != IntPtr.Zero)
        {
            owner = new WpfWin32Window(System.Windows.Application.Current?.MainWindow ?? new Window());
        }
        else if (System.Windows.Application.Current?.MainWindow != null)
        {
            owner = new WpfWin32Window(System.Windows.Application.Current.MainWindow);
        }

        // Show dialog
        var tdResult = owner != null ? TaskDialog.ShowDialog(owner, page) : TaskDialog.ShowDialog(page);

        if (tdResult == TaskDialogButton.OK)
        {
            return MessageBoxResult.OK;
        }
        else if (tdResult == TaskDialogButton.Cancel)
        {
            return MessageBoxResult.Cancel;
        }
        else if (tdResult == TaskDialogButton.Yes)
        {
            return MessageBoxResult.Yes;
        }
        else if (tdResult == TaskDialogButton.No)
        {
            return MessageBoxResult.No;
        }

        return MessageBoxResult.None;
    }

    private class WpfWin32Window(Window w) : IWin32Window, System.Windows.Interop.IWin32Window
    {
        public IntPtr Handle => _helper.Handle;

        private readonly System.Windows.Interop.WindowInteropHelper _helper = new(w);
    }
}
