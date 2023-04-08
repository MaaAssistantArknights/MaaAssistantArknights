// <copyright file="MaaHotKeyAction.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;

namespace MaaWpfGui.Services.HotKeys
{
    [Flags]
    public enum MaaHotKeyAction
    {
        /// <summary>
        /// Shows the GUI.
        /// </summary>
        ShowGui,

        /// <summary>
        /// Starts the link.
        /// </summary>
        LinkStart,
    }
}
