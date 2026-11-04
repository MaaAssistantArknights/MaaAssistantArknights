// <copyright file="MirrorChyanUpdateHelper.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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

using System;
using System.Globalization;
using System.Net.Http;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.Models.UpdateCredentialModel;
using static MaaWpfGui.ViewModels.Dialogs.VersionUpdateDialogViewModel;

namespace MaaWpfGui.Helper;

public static class MirrorChyanUpdateHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(MirrorChyanUpdateHelper));

    public static DateTimeOffset? ParseExpiration(string? value)
    {
        return long.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out long seconds) &&
            seconds > 1 && seconds <= DateTimeOffset.MaxValue.ToUnixTimeSeconds()
            ? DateTimeOffset.FromUnixTimeSeconds(seconds)
            : null;
    }

    public static UpdateCredentialResult ParseCredentialResult(JObject? data)
    {
        if (data is null || !int.TryParse(data["code"]?.ToString(), out int code))
        {
            return new(UpdateCredentialStatus.Failed);
        }

        if (code == (int)MirrorChyanErrorCode.Success && data["data"] is not JObject)
        {
            return new(UpdateCredentialStatus.Failed);
        }

        var expiresAt = ParseExpiration((data["data"] as JObject)?["cdk_expired_time"]?.ToString());
        return (MirrorChyanErrorCode)code switch {
            MirrorChyanErrorCode.Success => new(UpdateCredentialStatus.Valid, expiresAt),
            MirrorChyanErrorCode.KeyExpired => new(UpdateCredentialStatus.Expired, expiresAt),
            MirrorChyanErrorCode.KeyInvalid => new(UpdateCredentialStatus.Invalid),
            MirrorChyanErrorCode.ResourceQuotaExhausted => new(UpdateCredentialStatus.Failed, ErrorKey: "VersionUpdate.MirrorChyan.CdkQuotaExhausted"),
            MirrorChyanErrorCode.KeyMismatched => new(UpdateCredentialStatus.Failed, ErrorKey: "VersionUpdate.MirrorChyan.CdkMismatched"),
            MirrorChyanErrorCode.KeyBlocked => new(UpdateCredentialStatus.Failed, ErrorKey: "VersionUpdate.MirrorChyan.CdkBlocked"),
            _ => new(UpdateCredentialStatus.Failed),
        };
    }

    public static async Task<(JObject? Data, CheckUpdateRetT Ret)> RequestAsync(string url, UpdateCredentialRequest? request, bool preserveCredentialOnFailure = false)
    {
        JObject? data = null;
        try
        {
            using var response = await Instances.HttpService.GetAsync(new Uri(url), httpCompletionOption: HttpCompletionOption.ResponseContentRead, uriPartial: UriPartial.Path,
                token: request?.CancellationToken ?? default).ConfigureAwait(false);
            data = JObject.Parse(await response.Content.ReadAsStringAsync(request?.CancellationToken ?? default).ConfigureAwait(false));
            if (!response.IsSuccessStatusCode && data["code"]?.ToString() == "0")
            {
                data = null;
            }
        }
        catch (OperationCanceledException) when (request?.CancellationToken.IsCancellationRequested == true)
        {
            return (null, CheckUpdateRetT.NoNeedToUpdate);
        }
        catch (Exception ex)
        {
            // URL 查询参数和服务端正文可能包含凭据，不写入日志。
            _logger.Warning("Failed to request MirrorChyan update: {ExceptionType}", ex.GetType().Name);
        }

        if (data is null)
        {
            if (!preserveCredentialOnFailure && request is { } failedRequest)
            {
                var credential = SettingsViewModel.VersionUpdateSettings.MirrorChyanCredential;
                if (credential.Status != UpdateCredentialStatus.Valid)
                {
                    await credential.ApplyAsync(failedRequest, new(UpdateCredentialStatus.Failed)).ConfigureAwait(false);
                }
            }

            return (null, CheckUpdateRetT.NetworkError);
        }

        var result = ParseCredentialResult(data);
        if (request is { } check)
        {
            var credential = SettingsViewModel.VersionUpdateSettings.MirrorChyanCredential;
            if (!credential.IsCurrent(check))
            {
                return (null, CheckUpdateRetT.NoNeedToUpdate);
            }

            if (preserveCredentialOnFailure && result.Status == UpdateCredentialStatus.Failed && result.ErrorKey is null)
            {
                return (null, CheckUpdateRetT.NetworkError);
            }

            if (!await credential.ApplyAsync(check, result).ConfigureAwait(false))
            {
                return (null, CheckUpdateRetT.NoNeedToUpdate);
            }

            if (result.Status == UpdateCredentialStatus.Invalid)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.MirrorChyanCdkError);
            }

            if (credential.HasError)
            {
                return (null, CheckUpdateRetT.CredentialError);
            }
        }

        return result.Status == UpdateCredentialStatus.Valid
            ? (data, CheckUpdateRetT.OK)
            : (null, data is null ? CheckUpdateRetT.NetworkError : CheckUpdateRetT.UnknownError);
    }
}
