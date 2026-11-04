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
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Semver;
using Serilog;

namespace MaaWpfGui.Helper;

public static class GitHubUpdateHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(GitHubUpdateHelper));
    private static readonly ConcurrentDictionary<string, CachedManifest> _manifestCache = new();

    private sealed record CachedManifest(string ETag, JObject Body);

    public enum GitHubTokenValidationResult
    {
        Valid,
        Invalid,
        Failed,
    }

    public readonly record struct GitHubTokenValidation(GitHubTokenValidationResult Result, DateTimeOffset? ExpiresAt = null);

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

    public static async Task<GitHubTokenValidation> ValidateTokenAsync(string token, CancellationToken cancellationToken = default)
    {
        // Reject malformed header values before sending them or logging an HTTP exception.
        if (string.IsNullOrWhiteSpace(token) || token.Any(c => c <= ' ' || c >= '\u007f'))
        {
            return new(GitHubTokenValidationResult.Invalid);
        }

        try
        {
            using var response = await Instances.HttpService.GetAsync(
                new Uri($"{MaaUrls.GitHubApi}/rate_limit"), CreateHeaders(token), token: cancellationToken).ConfigureAwait(false);
            cancellationToken.ThrowIfCancellationRequested();
            DateTimeOffset? expiresAt = null;
            if (response.StatusCode == HttpStatusCode.OK &&
                response.Headers.TryGetValues("GitHub-Authentication-Token-Expiration", out var expirationValues))
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

            return response.StatusCode switch {
                HttpStatusCode.OK => new(GitHubTokenValidationResult.Valid, expiresAt),
                HttpStatusCode.Unauthorized => new(GitHubTokenValidationResult.Invalid),
                _ => new(GitHubTokenValidationResult.Failed),
            };
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            _logger.Warning("Failed to validate GitHub token: {ExceptionType}", ex.GetType().Name);
            return new(GitHubTokenValidationResult.Failed);
        }
    }

    public static async Task<JObject?> GetLatestReleaseAsync(string token, string channel, CancellationToken cancellationToken = default)
    {
        if (channel is not ("stable" or "beta" or "alpha"))
        {
            return null;
        }

        try
        {
            // Use the publisher's channel selection and combined full/OTA asset list.
            // Authentication stays on api.github.com; this cache is separate from MAA API data.
            var manifest = await GetJsonAsync(
                $"{MaaUrls.GitHubApi}/repos/MaaAssistantArknights/MaaRelease/contents/MaaAssistantArknights/api/version/{channel}.json?ref=main",
                token, cancellationToken, cacheManifest: true).ConfigureAwait(false);
            return GetManifestRelease(manifest)?.DeepClone() as JObject;
        }
        catch (Exception ex)
        {
            if (ex is OperationCanceledException && cancellationToken.IsCancellationRequested)
            {
                throw;
            }

            _logger.Warning("Failed to fetch GitHub release: {ExceptionType}", ex.GetType().Name);
            return null;
        }
    }

    public static async Task<string?> ResolveAssetApiUrlAsync(JObject asset, string token, CancellationToken cancellationToken = default)
    {
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
                token, cancellationToken).ConfigureAwait(false) as JObject;
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

    private static async Task<JToken?> GetJsonAsync(string url, string token, CancellationToken cancellationToken, bool cacheManifest = false)
    {
        var uri = new Uri(url);
        if (!IsGitHubApiUrl(uri))
        {
            return null;
        }

        var headers = CreateHeaders(token);
        CachedManifest? cached = null;
        if (cacheManifest)
        {
            headers["Accept"] = "application/vnd.github.raw+json";
            if (_manifestCache.TryGetValue(url, out cached))
            {
                headers["If-None-Match"] = cached.ETag;
            }
        }

        using var response = await Instances.HttpService.GetAsync(
            uri, headers, HttpCompletionOption.ResponseContentRead, token: cancellationToken).ConfigureAwait(false);
        cancellationToken.ThrowIfCancellationRequested();
        if (response.StatusCode == HttpStatusCode.Unauthorized)
        {
            await SettingsViewModel.VersionUpdateSettings.InvalidateGitHubTokenAsync(token, cancellationToken).ConfigureAwait(false);
            cancellationToken.ThrowIfCancellationRequested();
        }

        if (response.StatusCode == HttpStatusCode.NotFound)
        {
            return null;
        }

        if (response.StatusCode == HttpStatusCode.NotModified)
        {
            return cached?.Body.DeepClone();
        }

        response.EnsureSuccessStatusCode();
        var json = JToken.Parse(await response.Content.ReadAsStringAsync(cancellationToken).ConfigureAwait(false));
        if (cacheManifest && json is JObject manifest && GetManifestRelease(manifest) is not null)
        {
            if (response.Headers.ETag is { } etag)
            {
                _manifestCache[url] = new(etag.ToString(), (JObject)manifest.DeepClone());
            }
            else
            {
                _manifestCache.TryRemove(url, out _);
            }
        }

        return json;
    }
}
