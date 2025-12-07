// <copyright file="RecastConstants.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants;

/// <summary>
/// Constants for Roguelike Coppers Recast feature.
/// </summary>
public static class RecastConstants
{
    /// <summary>
    /// Gets the task name for Roguelike Coppers Recast.
    /// </summary>
    public const string TaskName = "MiniGame@CoppersRecast@Begin";

    /// <summary>
    /// Metric type constants for recast conditions.
    /// </summary>
    public static class MetricTypes
    {
        /// <summary>
        /// Gets the metric type for recast count.
        /// </summary>
        public const string RecastCount = "recast_count";

        /// <summary>
        /// Gets the metric type for HP.
        /// </summary>
        public const string Hp = "hp";

        /// <summary>
        /// Gets the metric type for hope.
        /// </summary>
        public const string Hope = "hope";

        /// <summary>
        /// Gets the metric type for ingot.
        /// </summary>
        public const string Ingot = "ingot";

        /// <summary>
        /// Gets the metric type for ticket.
        /// </summary>
        public const string Ticket = "ticket";
    }

    /// <summary>
    /// Comparator type constants for recast conditions.
    /// </summary>
    public static class ComparatorTypes
    {
        /// <summary>
        /// Gets the comparator type for less than.
        /// </summary>
        public const string Less = "less";

        /// <summary>
        /// Gets the comparator type for less than or equal.
        /// </summary>
        public const string LessOrEqual = "less_or_equal";

        /// <summary>
        /// Gets the comparator type for equal.
        /// </summary>
        public const string Equal = "equal";

        /// <summary>
        /// Gets the comparator type for greater than.
        /// </summary>
        public const string Greater = "greater";

        /// <summary>
        /// Gets the comparator type for greater than or equal.
        /// </summary>
        public const string GreaterOrEqual = "greater_or_equal";
    }
}