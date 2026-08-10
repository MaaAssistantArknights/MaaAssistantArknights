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
using System.Text.Json.Serialization;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using Serilog;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class Win32Extra() : ExtraConfig
{
    private static readonly ILogger _logger = Log.ForContext<Win32Extra>();

    #region Enums
#pragma warning disable SA1602 // Enumeration items should be documented
    // 遵循 AsstCaller.h 中的定义，确保与 AsstCaller.h 中的枚举值对应
    public enum AsstWin32ScreencapMethod
    {
        FramePool = 2,
        PrintWindow = 16,
        ScreenDC = 32,
        DesktopDupWindow = 8,
    }

    public enum AsstWin32InputMethod
    {
        Seize = 1,
        SendMessageWithCursorPos = 32,
        SendMessageWithWindowPos = 128,
    }

    public enum AsstWin32KeyboardInputMethod
    {
        Seize = 1,
        SendMessage = 2,
        PostMessage = 4,
    }
#pragma warning restore SA1602 // Enumeration items should be documented
    #endregion Enums

    public Win32Extra(AsstWin32ScreencapMethod screencapMethod, AsstWin32InputMethod inputMethod, AsstWin32KeyboardInputMethod keyboardInputMethod)
        : this()
    {
        _screencapMethod = screencapMethod;
        _mouseMethod = inputMethod;
        _KeyboardMethod = keyboardInputMethod;
    }

    /// <summary>
    /// Gets win32 截图方式枚举（与 AsstCaller.h 中 AsstWin32ScreencapMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32ScreencapMethod>> _screencapMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowScreencapFramePool"),  AsstWin32ScreencapMethod.FramePool),
        new(LocalizationHelper.GetString("AttachWindowScreencapPrintWindow"),  AsstWin32ScreencapMethod.PrintWindow),
        new(LocalizationHelper.GetString("AttachWindowScreencapScreenDC"),  AsstWin32ScreencapMethod.ScreenDC),
        new(LocalizationHelper.GetString("AttachWindowScreencapDesktopDupWindow"),  AsstWin32ScreencapMethod.DesktopDupWindow),
    ];

    [JsonIgnore]
    public List<GenericCombinedData<AsstWin32ScreencapMethod>> ScreencapMethodList => _screencapMethodList;

    [JsonInclude]
    [JsonPropertyName("ScreencapMethod")]
    private AsstWin32ScreencapMethod _screencapMethod = AsstWin32ScreencapMethod.FramePool; // 默认 FramePool

    /// <summary>
    /// Gets or sets the screencap method for AttachWindow mode.
    /// </summary>
    [JsonIgnore]
    public AsstWin32ScreencapMethod ScreencapMethod
    {
        get => _screencapMethod;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _screencapMethod, value);
        }
    }

    /// <summary>
    /// Win32 鼠标输入方式枚举（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32InputMethod>> _mouseMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowInputSeize"),  AsstWin32InputMethod.Seize),
        new(LocalizationHelper.GetString("AttachWindowInputSendWithCursor"),  AsstWin32InputMethod.SendMessageWithCursorPos),
        new(LocalizationHelper.GetString("AttachWindowInputSendWithWindowPos"),  AsstWin32InputMethod.SendMessageWithWindowPos),
    ];

    [JsonIgnore]
    public List<GenericCombinedData<AsstWin32InputMethod>> MouseMethodList => _mouseMethodList;

    [JsonInclude]
    [JsonPropertyName("MouseMethod")]
    private AsstWin32InputMethod _mouseMethod = AsstWin32InputMethod.SendMessageWithCursorPos; // 默认 SendMessageWithCursor

    /// <summary>
    /// Gets or sets the mouse input method for AttachWindow mode.
    /// </summary>
    [JsonIgnore]
    public AsstWin32InputMethod MouseMethod
    {
        get => _mouseMethod;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _mouseMethod, value);
        }
    }

    [JsonInclude]
    [JsonPropertyName("WindowCursorAvoidance")]
    private bool _windowCursorAvoidance = true;

    /// <summary>
    /// Gets or sets a value indicating whether WindowPos input should move the target window away from the physical cursor.
    /// </summary>
    [JsonIgnore]
    public bool WindowCursorAvoidance
    {
        get => _windowCursorAvoidance;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _windowCursorAvoidance, value);
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

    [JsonIgnore]
    public List<GenericCombinedData<AsstWin32KeyboardInputMethod>> KeyboardMethodList => _KeyboardMethodList;

    [JsonInclude]
    [JsonPropertyName("KeyboardMethod")]
    private AsstWin32KeyboardInputMethod _KeyboardMethod = AsstWin32KeyboardInputMethod.SendMessage; // 默认 SendMessage

    /// <summary>
    /// Gets or sets the keyboard input method for AttachWindow mode.
    /// </summary>
    [JsonIgnore]
    public AsstWin32KeyboardInputMethod KeyboardMethod
    {
        get => _KeyboardMethod;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _KeyboardMethod, value);
        }
    }
}
