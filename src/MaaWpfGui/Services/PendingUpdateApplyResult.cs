// <copyright file="PendingUpdateApplyResult.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Services;

internal sealed record PendingUpdateApplyResult(
    PendingUpdateApplyResult.StatusKind Status,
    bool RequiresManualRecovery = false,
    string? FailureReason = null)
{
    internal enum StatusKind
    {
        /// <summary>
        /// Indicates that there is no pending update package to apply.
        /// </summary>
        NoPendingPackage,

        /// <summary>
        /// Indicates that the pending update package was successfully applied and the application is ready to restart.
        /// </summary>
        Succeeded,

        /// <summary>
        /// Indicates that the pending update package has been handed off to an external updater.
        /// The current process must exit so the updater can finish the replacement.
        /// </summary>
        Delegated,

        /// <summary>
        /// Indicates that the pending update package is invalid or corrupted.
        /// </summary>
        InvalidPackage,

        /// <summary>
        /// Indicates that the external updater executable required for delegated update is missing.
        /// </summary>
        MissingUpdaterExecutable,

        /// <summary>
        /// Indicates that the pending update package failed to apply due to an unexpected error.
        /// Requires manual recovery to restore the application to a stable state.
        Failed,
    }

    public bool Succeeded => Status == StatusKind.Succeeded;

    public bool Delegated => Status == StatusKind.Delegated;
}
