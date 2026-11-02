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
                new Uri($"{MaaUrls.GitHubApi}/user"), CreateHeaders(token), token: cancellationToken).ConfigureAwait(false);
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
        try
        {
            // MaaRelease publishes the complete OTA release set. Stable releases also belong
            // to beta and alpha; beta releases also belong to alpha, as in the MAA API.
            for (int page = 1; ; page++)
            {
                var releases = await GetJsonAsync(
                    $"{MaaUrls.GitHubApi}/repos/MaaAssistantArknights/MaaRelease/releases?per_page=30&page={page}", token, cancellationToken).ConfigureAwait(false) as JArray;
                if (releases == null)
                {
                    return null;
                }

                var release = releases.OfType<JObject>().FirstOrDefault(candidate => MatchesChannel(candidate, channel));
                if (release != null)
                {
                    string tag = release["tag_name"]!.Value<string>()!;
                    var mainRelease = await GetJsonAsync(
                        $"{MaaUrls.GitHubApi}/repos/MaaAssistantArknights/MaaAssistantArknights/releases/tags/{Uri.EscapeDataString(tag)}", token, cancellationToken).ConfigureAwait(false) as JObject;
                    if (mainRelease == null || mainRelease.Value<bool>("draft"))
                    {
                        return release;
                    }

                    // Keep the main repository's changelog and full packages, then add OTA
                    // assets from MaaRelease. A nightly release only exists in MaaRelease.
                    var assets = new JArray();
                    var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    foreach (var asset in (mainRelease["assets"] as JArray ?? new JArray())
                        .Concat(release["assets"] as JArray ?? new JArray()).OfType<JObject>())
                    {
                        if (asset.Value<string>("name") is { } name && names.Add(name))
                        {
                            assets.Add(asset.DeepClone());
                        }
                    }

                    mainRelease["assets"] = assets;
                    return mainRelease;
                }

                if (releases.Count < 30)
                {
                    return null;
                }
            }
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

    private static bool MatchesChannel(JObject release, string channel)
    {
        if (release.Value<bool>("draft") ||
            !SemVersion.TryParse(release.Value<string>("tag_name"), SemVersionStyles.AllowLowerV, out var version))
        {
            return false;
        }

        var lastIdentifier = version.PrereleaseIdentifiers.LastOrDefault().ToString();
        bool nightly = version.IsPrerelease && lastIdentifier.StartsWith('g') && lastIdentifier.Length >= 7;
        return channel switch {
            "stable" => !version.IsPrerelease && !release.Value<bool>("prerelease"),
            "beta" => !nightly,
            "alpha" => true,
            _ => false,
        };
    }

    private static async Task<JToken?> GetJsonAsync(string url, string token, CancellationToken cancellationToken)
    {
        var uri = new Uri(url);
        if (!IsGitHubApiUrl(uri))
        {
            return null;
        }

        using var response = await Instances.HttpService.GetAsync(
            uri, CreateHeaders(token), HttpCompletionOption.ResponseContentRead, token: cancellationToken).ConfigureAwait(false);
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

        response.EnsureSuccessStatusCode();
        return JToken.Parse(await response.Content.ReadAsStringAsync(cancellationToken).ConfigureAwait(false));
    }
}
