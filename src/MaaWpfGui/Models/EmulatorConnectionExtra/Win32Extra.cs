// <copyright file="Win32Extra.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums.Core;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class Win32Extra : ExtraConfig
{
    /// <summary>
    /// Gets win32 截图方式枚举（与 AsstCaller.h 中 AsstWin32ScreencapMethodEnum 对应）
    /// </summary>
    private static readonly List<SelectableGenericCombinedData<AsstWin32ScreencapMethod>> _screencapMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowScreencapFramePool"),  AsstWin32ScreencapMethod.FramePool),
        new(LocalizationHelper.GetString("AttachWindowScreencapPrintWindow"),  AsstWin32ScreencapMethod.PrintWindow),
        new(LocalizationHelper.GetString("AttachWindowScreencapScreenDC"),  AsstWin32ScreencapMethod.ScreenDC),
        new(LocalizationHelper.GetString("AttachWindowScreencapDesktopDupWindow"),  AsstWin32ScreencapMethod.DesktopDupWindow),
    ];

    public List<SelectableGenericCombinedData<AsstWin32ScreencapMethod>> ScreencapMethodList => _screencapMethodList;

    /// <summary>
    /// Gets or sets the screencap method for AttachWindow mode.
    /// </summary>
    public AsstWin32ScreencapMethod ScreencapMethod
    {
        get; set {
            // 鼠标输入方式为 SendMessageWithWindowPos 时，截图方式仅支持 PrintWindow
            if (MouseMethod == AsstWin32InputMethod.SendMessageWithWindowPos)
            {
                value = AsstWin32ScreencapMethod.PrintWindow;
            }

            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra.ScreencapMethod = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra.ScreencapMethod;

    /// <summary>
    /// Win32 鼠标输入方式枚举（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32InputMethod>> _mouseMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowInputSeize"), AsstWin32InputMethod.Seize),
        new(LocalizationHelper.GetString("AttachWindowInputSendWithCursor"), AsstWin32InputMethod.SendMessageWithCursorPos),
        new(LocalizationHelper.GetString("AttachWindowInputSendWithWindowPos"), AsstWin32InputMethod.SendMessageWithWindowPos),
    ];

    public List<GenericCombinedData<AsstWin32InputMethod>> MouseMethodList => _mouseMethodList;

    /// <summary>
    /// Gets or sets the mouse input method for AttachWindow mode.
    /// </summary>
    public AsstWin32InputMethod MouseMethod
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);

            // 鼠标输入方式为 SendMessageWithWindowPos 时，截图方式仅支持 PrintWindow
            if (value == AsstWin32InputMethod.SendMessageWithWindowPos)
            {
                ScreencapMethod = AsstWin32ScreencapMethod.PrintWindow;
            }

            UpdateScreencapMethodAvailability();
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra.MouseMethod = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra.MouseMethod;

    /// <summary>
    /// 根据当前鼠标输入方式刷新截图方式选项的可用状态
    /// </summary>
    public void UpdateScreencapMethodAvailability()
    {
        foreach (var item in _screencapMethodList)
        {
            item.IsEnabled = MouseMethod != AsstWin32InputMethod.SendMessageWithWindowPos || item.Value == AsstWin32ScreencapMethod.PrintWindow;
        }
    }

    /// <summary>
    /// Win32 键盘输入方式枚举（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32KeyboardInputMethod>> _KeyboardMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowInputSeize"),  AsstWin32KeyboardInputMethod.Seize),
        new(LocalizationHelper.GetString("AttachWindowInputSendMsg"),  AsstWin32KeyboardInputMethod.SendMessage),
        new(LocalizationHelper.GetString("AttachWindowInputPostMsg"),  AsstWin32KeyboardInputMethod.PostMessage),
    ];

    public List<GenericCombinedData<AsstWin32KeyboardInputMethod>> KeyboardMethodList => _KeyboardMethodList;

    /// <summary>
    /// Gets or sets the keyboard input method for AttachWindow mode.
    /// </summary>
    public AsstWin32KeyboardInputMethod KeyboardMethod
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra.KeyboardMethod = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra.KeyboardMethod;
}
