// <copyright file="StageActivityInfo.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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

namespace MeoAsstGui
{
    /// <summary>
    /// Stage activity info
    /// </summary>
    public class StageActivityInfo
    {
        /// <summary>
        /// Gets or sets the activity tip
        /// </summary>
        public string Tip { get; set; }

        /// <summary>
        /// Gets or sets the client type
        /// </summary>
        public string ClientType { get; set; } = "Official";

        /// <summary>
        /// Gets or sets a <seealso cref="DateTimeOffset"/> object indicates the start time, expressed as UTC.
        /// </summary>
        public DateTimeOffset UtcStartTime { get; set; }

        /// <summary>
        /// Gets or sets a <seealso cref="DateTimeOffset"/> object indicates the expire time, expressed as UTC.
        /// </summary>
        public DateTimeOffset UtcExpireTime { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the activity is a resource collection activity
        /// </summary>
        /// <remarks>
        /// Sets to true indicates a resource collection activity,
        /// when the activity expires, <seealso cref="StageManager"/> will continue to check stage open days.
        /// </remarks>
        public bool IsResourceCollection { get; set; }

        /// <summary>
        /// Gets a value indicating whether the activity is started
        /// </summary>
        public bool IsStarted => DateTimeOffset.UtcNow >= UtcStartTime;

        /// <summary>
        /// Gets a value indicating whether the activity is expired
        /// </summary>
        public bool IsExpired => DateTimeOffset.UtcNow >= UtcExpireTime;

        /// <summary>
        /// Gets a value indicating whether the activity is open (started and not expired)
        /// </summary>
        public bool IsOpen => IsStarted && !IsExpired;
    }
}
