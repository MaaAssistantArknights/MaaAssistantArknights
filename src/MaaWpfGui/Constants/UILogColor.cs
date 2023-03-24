// <copyright file="UILogColor.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants
{
    /// <summary>
    /// The recommended colors for logs.
    /// </summary>
    public static class UiLogColor
    {
        /// <summary>
        /// The recommended color for error logs.
        /// </summary>
        public const string Error = "DarkRed";

        /// <summary>
        /// The recommended color for warning logs.
        /// </summary>
        public const string Warning = "DarkGoldenrod";

        /// <summary>
        /// The recommended color for info logs.
        /// </summary>
        public const string Info = "DarkCyan";

        /// <summary>
        /// The recommended color for trace logs.
        /// </summary>
        public const string Trace = "Gray";

        /// <summary>
        /// The recommended color for message logs.
        /// </summary>
        public const string Message = "Black";

        /// <summary>
        /// The recommended color for rare operator logs.
        /// </summary>
        public const string RareOperator = "DarkOrange";

        /// <summary>
        /// The recommended color for robot operator logs.
        /// </summary>
        public const string RobotOperator = "DarkGray";

        /// <summary>
        /// The recommended color for file downloading or downloaded or download failed.
        /// </summary>
        public const string Download = "BlueViolet";
    }
}
