// <copyright file="WineBridgeAvailability.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.WineCompat;

/// <summary>
/// Availability status of the Wine compatibility layer bridge.
/// </summary>
public enum WineBridgeAvailability
{
    /// <summary>Wine bridge is fully operational and functioning correctly.</summary>
    Operational,

    /// <summary>Wine bridge has encountered errors or configuration issues.</summary>
    Faulted,

    /// <summary>Wine bridge is not available or cannot be initialized.</summary>
    NotAvailable,
}
