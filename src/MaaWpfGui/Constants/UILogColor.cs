// <copyright file="UILogColor.cs" company="MaaAssistantArknights">
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

using JetBrains.Annotations;

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
        public const string Error = "ErrorLogBrush";

        /// <summary>
        /// The recommended color for warning logs.
        /// </summary>
        public const string Warning = "WarningLogBrush";

        /// <summary>
        /// The recommended color for info logs.
        /// </summary>
        public const string Info = "InfoLogBrush";

        /// <summary>
        /// The recommended color for trace logs.
        /// </summary>
        public const string Trace = "TraceLogBrush";

        /// <summary>
        /// The recommended color for message logs.
        /// </summary>
        public const string Message = "MessageLogBrush";

        /// <summary>
        /// The recommended color for rare operator logs.
        /// </summary>
        public const string RareOperator = "RareOperatorLogBrush";

        /// <summary>
        /// The recommended color for robot operator logs.
        /// </summary>
        [UsedImplicitly]
        public const string RobotOperator = "RobotOperatorLogBrush";

        /// <summary>
        /// The recommended color for 1-star operators (also used for robot operators).
        /// </summary>
        public const string Star1Operator = "Star1OperatorLogBrush";

        /// <summary>
        /// The recommended color for 2-star operators.
        /// </summary>
        public const string Star2Operator = "Star2OperatorLogBrush";

        /// <summary>
        /// The recommended color for 3-star operators.
        /// </summary>
        public const string Star3Operator = "Star3OperatorLogBrush";

        /// <summary>
        /// The recommended color for 4-star operators.
        /// </summary>
        public const string Star4Operator = "Star4OperatorLogBrush";

        /// <summary>
        /// The recommended color for 5-star operators.
        /// </summary>
        public const string Star5Operator = "Star5OperatorLogBrush";

        /// <summary>
        /// The recommended color for 6-star operators.
        /// </summary>
        public const string Star6Operator = "Star6OperatorLogBrush";

        /// <summary>
        /// The recommended color for file downloading or downloaded or download failed.
        /// </summary>
        public const string Download = "DownloadLogBrush";

        /// <summary>
        /// The recommended color for MuMu special screenshot.
        /// </summary>
        public const string MuMuSpecialScreenshot = "MuMuSpecialScreenshot";

        /// <summary>
        /// The recommended color for LD special screenshot.
        /// </summary>
        public const string LdSpecialScreenshot = "LdSpecialScreenshot";

        // I.S. Colors

        /// <summary>
        /// The recommended color for success fights.
        /// </summary>
        public const string SuccessIS = "SuccessIS";

        /// <summary>
        /// The recommended color for Safe House events.
        /// </summary>
        public const string SafehouseIS = "SafehouseIS";

        /// <summary>
        /// The recommended color for Trader events.
        /// </summary>
        public const string TraderIS = "TraderIS";

        /// <summary>
        /// The recommended color for standard events.
        /// </summary>
        public const string EventIS = "EventIS";

        /// <summary>
        /// The recommended color for truth events.
        /// </summary>
        public const string TruthIS = "TruthIS";

        /// <summary>
        /// The recommended color for standard fights.
        /// </summary>
        public const string CombatIS = "CombatIS";

        /// <summary>
        /// The recommended color for emergency fights.
        /// </summary>
        public const string EmergencyIS = "EmergencyIS";

        /// <summary>
        /// The recommended color for boss fights.
        /// </summary>
        public const string BossIS = "BossIS";

        /// <summary>
        /// The recommended color for abandoned.
        /// </summary>
        public const string ExplorationAbandonedIS = "ExplorationAbandonedIS";

        // 颜色在MaaWpfGui\Res\Themes中定义
        // Brush are defined in MaaWpfGui\Res\Themes
    }
}
