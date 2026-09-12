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
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums.Core;

namespace MaaWpfGui.Configuration.Single.Settings.ConnectionExtra;

public class Win32Extra : BaseExtra, IJsonOnDeserialized
{
    public AsstWin32ScreencapMethod ScreencapMethod { get; set; } = AsstWin32ScreencapMethod.PrintWindow;

    public AsstWin32InputMethod MouseMethod { get; set; } = AsstWin32InputMethod.SendMessageWithWindowPos;

    public AsstWin32KeyboardInputMethod KeyboardMethod { get; set; } = AsstWin32KeyboardInputMethod.SendMessage;

    public bool MuteWhileRunning { get; set; }

    public void OnDeserialized()
    {
        // 纯消息鼠标输入（SendMessage/PostMessage）在界面中永久禁用，历史配置若存有该值则回落默认
        if (MouseMethod is AsstWin32InputMethod.SendMessage or AsstWin32InputMethod.PostMessage)
        {
            MouseMethod = AsstWin32InputMethod.SendMessageWithCursorPos;
        }

        if (MouseMethod == AsstWin32InputMethod.SendMessageWithWindowPos)
        {
            ScreencapMethod = AsstWin32ScreencapMethod.PrintWindow;
        }
    }
}
