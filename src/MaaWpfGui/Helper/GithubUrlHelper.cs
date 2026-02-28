// <copyright file="GithubUrlHelper.cs" company="MaaAssistantArknights">
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

using System;
using MaaWpfGui.Constants;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.Helper;

/// <summary>
/// Helper class for replacing GitHub domains in URLs.
/// </summary>
public static class GithubUrlHelper
{
    /// <summary>
    /// Replaces GitHub domains in a URL with user-configured domains.
    /// </summary>
    /// <param name="url">The original URL.</param>
    /// <returns>The URL with replaced domains.</returns>
    public static string ReplaceGithubDomain(string url)
    {
        if (string.IsNullOrEmpty(url))
        {
            return url;
        }

        var settings = SettingsViewModel.VersionUpdateSettings;

        // Replace github.com
        if (!string.IsNullOrEmpty(settings.GithubMainDomain) && settings.GithubMainDomain != "github.com")
        {
            url = url.Replace("github.com", settings.GithubMainDomain);
        }

        // Replace raw.githubusercontent.com
        if (!string.IsNullOrEmpty(settings.GithubRawDomain) && settings.GithubRawDomain != "raw.githubusercontent.com")
        {
            url = url.Replace("raw.githubusercontent.com", settings.GithubRawDomain);
        }

        // Replace api.github.com
        if (!string.IsNullOrEmpty(settings.GithubApiDomain) && settings.GithubApiDomain != "api.github.com")
        {
            url = url.Replace("api.github.com", settings.GithubApiDomain);
        }

        return url;
    }

    /// <summary>
    /// Replaces GitHub domains in a URI with user-configured domains.
    /// </summary>
    /// <param name="uri">The original URI.</param>
    /// <returns>The URI with replaced domains.</returns>
    public static Uri ReplaceGithubDomain(Uri uri)
    {
        if (uri == null)
        {
            return uri;
        }

        var urlString = uri.ToString();
        var replacedUrl = ReplaceGithubDomain(urlString);

        return replacedUrl == urlString ? uri : new Uri(replacedUrl);
    }
}
