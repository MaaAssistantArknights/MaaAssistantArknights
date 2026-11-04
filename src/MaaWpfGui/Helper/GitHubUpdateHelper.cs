// <copyright file="GitHubUpdateHelper.cs" company="MaaAssistantArknights">
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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Semver;
using Serilog;
using static MaaWpfGui.Models.UpdateCredentialModel;
using static MaaWpfGui.ViewModels.Dialogs.VersionUpdateDialogViewModel;

namespace MaaWpfGui.Helper;

public static class GitHubUpdateHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(GitHubUpdateHelper));
    private static readonly ConcurrentDictionary<string, CachedManifest> _manifestCache = new();
    internal const string RateLimitedErrorKey = "VersionUpdate.RateLimited";

    private sealed record CachedManifest(string ETag, JObject Body, int CredentialRevision);

    public static Dictionary<string, string> CreateHeaders(string token) => new() {
        ["Accept"] = "application/vnd.github+json",
        ["Authorization"] = $"Bearer {token}",
        ["X-GitHub-Api-Version"] = "2026-03-10",
    };

    public static bool IsGitHubApiUrl(Uri uri) =>
        uri.IsAbsoluteUri &&
        uri.Scheme == Uri.UriSchemeHttps &&
        uri.Host.Equals("api.github.com", StringComparison.OrdinalIgnoreCase) &&
        uri.IsDefaultPort &&
        string.IsNullOrEmpty(uri.UserInfo);

    public static string? GetBrowserDownloadUrl(JObject asset)
    {
        if (asset["name"]?.Type != JTokenType.String ||
            !Uri.TryCreate(asset["browser_download_url"]?.ToString(), UriKind.Absolute, out var uri) ||
            uri.Scheme != Uri.UriSchemeHttps || !uri.Host.Equals("github.com", StringComparison.OrdinalIgnoreCase) ||
            !uri.IsDefaultPort || !string.IsNullOrEmpty(uri.UserInfo) ||
            !string.IsNullOrEmpty(uri.Query) || !string.IsNullOrEmpty(uri.Fragment))
        {
            return null;
        }

        var segments = uri.AbsolutePath.Split('/', StringSplitOptions.RemoveEmptyEntries);
        return segments.Length == 6 && IsUpdateRepository(segments[0], segments[1]) &&
            segments[2] == "releases" && segments[3] == "download" &&
            !string.IsNullOrEmpty(Uri.UnescapeDataString(segments[4])) &&
            Uri.UnescapeDataString(segments[5]) == asset.Value<string>("name")
            ? uri.AbsoluteUri
            : null;
    }

    private static UpdateCredentialResult ParseCredentialResult(HttpResponseMessage response)
    {
        if (response.StatusCode == HttpStatusCode.Unauthorized)
        {
            return new(UpdateCredentialStatus.Invalid);
        }

        if (response.StatusCode is not (HttpStatusCode.OK or HttpStatusCode.NotModified))
        {
            return new(UpdateCredentialStatus.Failed);
        }

        DateTimeOffset? expiresAt = null;
        if (response.Headers.TryGetValues("GitHub-Authentication-Token-Expiration", out var expirationValues))
        {
            foreach (string expirationValue in expirationValues)
            {
                string value = expirationValue.Trim();
                if (value.EndsWith(" UTC", StringComparison.OrdinalIgnoreCase))
                {
                    value = value[..^4] + " +00:00";
                }

                if (DateTimeOffset.TryParse(value, CultureInfo.InvariantCulture,
                        DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out var expiration))
                {
                    expiresAt = expiration;
                    break;
                }
            }
        }

        return new(UpdateCredentialStatus.Valid, expiresAt);
    }

    public static async Task<(JObject? Data, CheckUpdateRetT Ret)> RequestAsync(string url, UpdateCredentialRequest request, bool cacheManifest = false, bool rawContent = false)
    {
        var credential = SettingsViewModel.VersionUpdateSettings.GitHubCredential;
        try
        {
            var data = await GetJsonAsync(url, request, cacheManifest, rawContent).ConfigureAwait(false);
            if (!credential.IsCurrent(request))
            {
                return (null, CheckUpdateRetT.NoNeedToUpdate);
            }

            if (credential.HasError)
            {
                return (null, CheckUpdateRetT.CredentialError);
            }

            return (data, data is null ? CheckUpdateRetT.FailedToGetInfo : CheckUpdateRetT.OK);
        }
        catch (OperationCanceledException) when (request.CancellationToken.IsCancellationRequested)
        {
            return (null, CheckUpdateRetT.NoNeedToUpdate);
        }
        catch (Exception ex)
        {
            _logger.Warning("Failed to request GitHub update: {ExceptionType}", ex.GetType().Name);
            if (credential.Status != UpdateCredentialStatus.Valid)
            {
                await credential.ApplyAsync(request, new(UpdateCredentialStatus.Failed)).ConfigureAwait(false);
            }

            return (null, CheckUpdateRetT.NetworkError);
        }
    }

    public static async Task<(JObject? Data, CheckUpdateRetT Ret)> GetLatestReleaseAsync(UpdateCredentialRequest request, string channel)
    {
        if (channel is not ("stable" or "beta" or "alpha"))
        {
            return (null, CheckUpdateRetT.FailedToGetInfo);
        }

        // Use the publisher's channel selection and combined full/OTA asset list.
        // Authentication stays on api.github.com; this cache is separate from MAA API data.
        var (manifest, ret) = await RequestAsync(
            $"{MaaUrls.GitHubApi}/repos/MaaAssistantArknights/MaaRelease/contents/MaaAssistantArknights/api/version/{channel}.json?ref=main",
            request, cacheManifest: true, rawContent: true).ConfigureAwait(false);
        if (ret != CheckUpdateRetT.OK)
        {
            return (null, ret);
        }

        var release = GetManifestRelease(manifest)?.DeepClone() as JObject;
        return (release, release is null ? CheckUpdateRetT.FailedToGetInfo : CheckUpdateRetT.OK);
    }

    public static async Task<string?> ResolveAssetApiUrlAsync(JObject asset, UpdateCredentialRequest request)
    {
        var cancellationToken = request.CancellationToken;
        cancellationToken.ThrowIfCancellationRequested();
        if (asset["name"]?.Type != JTokenType.String || string.IsNullOrEmpty(asset.Value<string>("name")))
        {
            return null;
        }

        string assetName = asset.Value<string>("name")!;
        if (Uri.TryCreate(asset["url"]?.ToString(), UriKind.Absolute, out var assetUri) && IsReleaseAssetApiUrl(assetUri))
        {
            return assetUri.AbsoluteUri;
        }

        var browserUrl = GetBrowserDownloadUrl(asset);
        if (browserUrl is null)
        {
            return null;
        }

        var browserUri = new Uri(browserUrl);
        var segments = browserUri.AbsolutePath.Split('/', StringSplitOptions.RemoveEmptyEntries);
        if (segments.Length != 6 || !IsUpdateRepository(segments[0], segments[1]) ||
            segments[2] != "releases" || segments[3] != "download")
        {
            return null;
        }

        string tag = Uri.UnescapeDataString(segments[4]);
        if (string.IsNullOrEmpty(tag) || Uri.UnescapeDataString(segments[5]) != assetName)
        {
            return null;
        }

        try
        {
            var release = await GetJsonAsync(
                $"{MaaUrls.GitHubApi}/repos/{segments[0]}/{segments[1]}/releases/tags/{Uri.EscapeDataString(tag)}",
                request).ConfigureAwait(false);
            if (release?["tag_name"]?.Value<string>() != tag || release.Value<bool>("draft") || release["assets"] is not JArray assets)
            {
                return null;
            }

            foreach (var candidate in assets.OfType<JObject>())
            {
                if (candidate["name"]?.Value<string>() == assetName &&
                    Uri.TryCreate(candidate["url"]?.ToString(), UriKind.Absolute, out assetUri) &&
                    IsReleaseAssetApiUrl(assetUri, segments[1]))
                {
                    return assetUri.AbsoluteUri;
                }
            }

            return null;
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            _logger.Warning("Failed to resolve GitHub release asset: {ExceptionType}", ex.GetType().Name);
            return null;
        }
    }

    private static JObject? GetManifestRelease(JToken? manifest)
    {
        if (manifest is not JObject data || data["version"]?.Type != JTokenType.String ||
            !SemVersion.TryParse(data.Value<string>("version"), SemVersionStyles.AllowLowerV, out _) ||
            data["details"] is not JObject release || release["assets"] is not JArray ||
            release.Value<bool>("draft") || release["tag_name"]?.Value<string>() != data.Value<string>("version"))
        {
            return null;
        }

        return release;
    }

    private static bool IsUpdateRepository(string owner, string repository) =>
        owner.Equals("MaaAssistantArknights", StringComparison.OrdinalIgnoreCase) &&
        (repository.Equals("MaaAssistantArknights", StringComparison.OrdinalIgnoreCase) || repository.Equals("MaaRelease", StringComparison.OrdinalIgnoreCase));

    private static bool IsReleaseAssetApiUrl(Uri uri, string? repository = null)
    {
        if (!IsGitHubApiUrl(uri) || !string.IsNullOrEmpty(uri.Query) || !string.IsNullOrEmpty(uri.Fragment))
        {
            return false;
        }

        var segments = uri.AbsolutePath.Split('/', StringSplitOptions.RemoveEmptyEntries);
        return segments.Length == 6 && segments[0] == "repos" && IsUpdateRepository(segments[1], segments[2]) &&
            (repository is null || repository.Equals(segments[2], StringComparison.OrdinalIgnoreCase)) &&
            segments[3] == "releases" && segments[4] == "assets" && long.TryParse(segments[5], NumberStyles.None, CultureInfo.InvariantCulture, out long id) && id > 0;
    }

    private static async Task<JObject?> GetJsonAsync(string url, UpdateCredentialRequest request, bool cacheManifest = false, bool rawContent = false)
    {
        var uri = new Uri(url);
        if (!IsGitHubApiUrl(uri))
        {
            return null;
        }

        var credential = SettingsViewModel.VersionUpdateSettings.GitHubCredential;
        if (!credential.IsCurrent(request))
        {
            return null;
        }

        // Reject malformed header values before sending them or logging an HTTP exception.
        if (string.IsNullOrWhiteSpace(request.Value) || request.Value.Any(c => c <= ' ' || c >= '\u007f'))
        {
            await credential.ApplyAsync(request, new(UpdateCredentialStatus.Invalid)).ConfigureAwait(false);
            return null;
        }

        var headers = CreateHeaders(request.Value);
        if (rawContent)
        {
            // Contents API 的 raw 媒体类型不适用于 Releases API。
            headers["Accept"] = "application/vnd.github.raw+json";
        }

        CachedManifest? cached = null;

        // 新输入的 Token 需要完整响应来读取有效期，不能只复用旧 Token 的 304 正文。
        if (cacheManifest && _manifestCache.TryGetValue(url, out cached) && cached.CredentialRevision == request.Revision)
        {
            headers["If-None-Match"] = cached.ETag;
        }

        using var response = await Instances.HttpService.GetAsync(
            uri, headers, HttpCompletionOption.ResponseContentRead, token: request.CancellationToken).ConfigureAwait(false);
        request.CancellationToken.ThrowIfCancellationRequested();
        if (response.StatusCode == HttpStatusCode.NotFound)
        {
            // A missing API resource does not mean the token is invalid.
            await credential.ApplyAsync(request, new(UpdateCredentialStatus.Valid, credential.ExpiresAt)).ConfigureAwait(false);
            return null;
        }

        var result = ParseCredentialResult(response);
        var (_, isRateLimited) = await HttpService.GetRetryInfoAsync(response, request.CancellationToken).ConfigureAwait(false);
        if (isRateLimited)
        {
            result = new(UpdateCredentialStatus.Failed, ErrorKey: RateLimitedErrorKey);
        }

        if (result.Status != UpdateCredentialStatus.Valid)
        {
            await credential.ApplyAsync(request, result).ConfigureAwait(false);
            return null;
        }

        var json = response.StatusCode == HttpStatusCode.NotModified
            ? cached?.Body.DeepClone() as JObject
            : JObject.Parse(await response.Content.ReadAsStringAsync(request.CancellationToken).ConfigureAwait(false));
        if (response.StatusCode == HttpStatusCode.NotModified &&
            !response.Headers.Contains("GitHub-Authentication-Token-Expiration"))
        {
            // A 304 response may omit authentication metadata; retain the known expiration.
            result = result with { ExpiresAt = credential.ExpiresAt };
        }

        if (!await credential.ApplyAsync(request, json is null ? new(UpdateCredentialStatus.Failed) : result).ConfigureAwait(false) ||
            credential.HasError)
        {
            return null;
        }

        if (cacheManifest && response.StatusCode != HttpStatusCode.NotModified && json is { } manifest && GetManifestRelease(manifest) is not null)
        {
            if (response.Headers.ETag is { } etag)
            {
                _manifestCache[url] = new(etag.ToString(), (JObject)manifest.DeepClone(), request.Revision);
            }
            else
            {
                _manifestCache.TryRemove(url, out _);
            }
        }

        return json;
    }
}
