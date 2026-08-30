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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums.Core;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class Win32Extra : ExtraConfig
{
    /// <summary>
    /// Win32 截图方式列表（与 AsstCaller.h 中 AsstWin32ScreencapMethodEnum 对应）
    /// </summary>
    private static readonly LocalizedObservableList<AsstWin32ScreencapMethod> _screencapMethodList =
        new(
            (AsstWin32ScreencapMethod.FramePool, "AttachWindowScreencapFramePool"),
            (AsstWin32ScreencapMethod.PrintWindow, "AttachWindowScreencapPrintWindow"),
            (AsstWin32ScreencapMethod.ScreenDC, "AttachWindowScreencapScreenDC"),
            (AsstWin32ScreencapMethod.DesktopDupWindow, "AttachWindowScreencapDesktopDupWindow"));

    public LocalizedObservableList<AsstWin32ScreencapMethod> ScreencapMethodList => _screencapMethodList;

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
    /// Win32 鼠标输入方式列表（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应，按枚举值排序）
    /// </summary>
    /// <remarks>
    /// 纯 SendMsg / PostMsg 仅列出作展示：明日方舟 PC 端按真实光标位置取坐标、不读取消息中的坐标，
    /// 纯消息点击会落点无效（原神等游戏读取消息坐标，故同类工具纯后台可用），因此永久禁用，
    /// 界面中以置灰选项呈现。
    /// </remarks>
    private static readonly LocalizedObservableList<AsstWin32InputMethod> _mouseMethodList =
        new(
            (AsstWin32InputMethod.Seize, "AttachWindowInputSeize", true),
            (AsstWin32InputMethod.SendMessage, "AttachWindowInputSendMsgDisabled", false),
            (AsstWin32InputMethod.PostMessage, "AttachWindowInputPostMsgDisabled", false),
            (AsstWin32InputMethod.SendMessageWithCursorPos, "AttachWindowInputSendWithCursor", true),
            (AsstWin32InputMethod.SendMessageWithWindowPos, "AttachWindowInputSendWithWindowPos", true));

    public LocalizedObservableList<AsstWin32InputMethod> MouseMethodList => _mouseMethodList;

    static Win32Extra()
    {
        // 列表为 static，静态构造订阅一次即可
        LocalizationHelper.LanguageChanged += RefreshListsLocalization;
    }

    private static void RefreshListsLocalization()
    {
        _screencapMethodList.RefreshLocalization();
        _mouseMethodList.RefreshLocalization();
        _keyboardMethodList.RefreshLocalization();
    }

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
        foreach (var item in _screencapMethodList.Items)
        {
            item.IsEnabled = MouseMethod != AsstWin32InputMethod.SendMessageWithWindowPos || item.Value == AsstWin32ScreencapMethod.PrintWindow;
        }
    }

    /// <summary>
    /// Win32 键盘输入方式列表（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应）
    /// </summary>
    private static readonly LocalizedObservableList<AsstWin32KeyboardInputMethod> _keyboardMethodList =
        new(
            (AsstWin32KeyboardInputMethod.Seize, "AttachWindowInputSeize"),
            (AsstWin32KeyboardInputMethod.SendMessage, "AttachWindowInputSendMsg"),
            (AsstWin32KeyboardInputMethod.PostMessage, "AttachWindowInputPostMsg"));

    public LocalizedObservableList<AsstWin32KeyboardInputMethod> KeyboardMethodList => _keyboardMethodList;

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
