// <copyright file="GithubMirrorHelper.cs" company="MaaAssistantArknights">
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

using System;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.ViewModels.UserControl.Settings;

namespace MaaWpfGui.Helper;

/// <summary>
/// GitHub 镜像站加速：为 GitHub 相关的下载链接添加用户配置的镜像前缀
/// </summary>
public static class GithubMirrorHelper
{
    /// <summary>
    /// 若当前更新源为 ｢GitHub 镜像｣ 且链接指向 GitHub 相关域名，则为其添加镜像站前缀。
    /// <para>例如前缀为 <c>https://gh-proxy.org/</c> 时，<c>https://github.com/a/b/file.zip</c> 会变为
    /// <c>https://gh-proxy.org/https://github.com/a/b/file.zip</c></para>
    /// </summary>
    /// <param name="url">原始下载链接</param>
    /// <returns>镜像化后的链接；未启用镜像或非 GitHub 链接时原样返回</returns>
    public static string ApplyMirrorIfNeeded(string url)
    {
        var settings = VersionUpdateSettingsUserControlModel.Instance;
        if (settings.UpdateSource != UpdateSource.GitHubMirror)
        {
            return url;
        }

        return ApplyMirror(settings.GithubMirrorUrl, url);
    }

    /// <summary>
    /// 为 GitHub 相关域名（github.com、*.githubusercontent.com 等）的链接添加镜像前缀
    /// </summary>
    /// <param name="mirrorUrl">镜像站前缀地址</param>
    /// <param name="url">原始下载链接</param>
    /// <returns>镜像化后的链接；前缀为空或非 GitHub 链接时原样返回</returns>
    public static string ApplyMirror(string mirrorUrl, string url)
    {
        mirrorUrl = mirrorUrl?.Trim() ?? string.Empty;
        if (mirrorUrl.Length == 0 || string.IsNullOrEmpty(url))
        {
            return url;
        }

        // 前缀必须是合法的绝对 HTTP(S) 地址，无效配置直接回退原链接，避免把错误配置拼进下载链接
        if (!Uri.TryCreate(mirrorUrl, UriKind.Absolute, out var mirrorUri)
            || (mirrorUri.Scheme != Uri.UriSchemeHttps && mirrorUri.Scheme != Uri.UriSchemeHttp))
        {
            return url;
        }

        try
        {
            var host = new Uri(url).Host;
            if (host.Equals("github.com", StringComparison.OrdinalIgnoreCase)
                || host.EndsWith(".githubusercontent.com", StringComparison.OrdinalIgnoreCase)
                || host.Equals("githubusercontent.com", StringComparison.OrdinalIgnoreCase))
            {
                var mirrored = mirrorUrl.TrimEnd('/') + "/" + url;

                // 拼接结果同样要求是合法的绝对地址，否则回退原链接，防止后续探测 new Uri 时抛异常导致更新检测崩溃
                return Uri.TryCreate(mirrored, UriKind.Absolute, out _) ? mirrored : url;
            }
        }
        catch (UriFormatException)
        {
            // 非法链接，原样返回
        }

        return url;
    }
}
