// <copyright file="HttpResponseLoggingExtension.cs" company="MaaAssistantArknights">
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
using System.Net;
using System.Net.Http;
using Serilog;

namespace MaaWpfGui.Extensions
{
    public static class HttpResponseLoggingExtension
    {
        private static readonly ILogger _logger = Serilog.Log.ForContext("SourceContext", "HttpResponseLoggingExtension");

        public static void Log(this HttpResponseMessage response, UriPartial uriPartial = UriPartial.Query, double? elapsedMs = null)
        {
            var method = response?.RequestMessage?.Method;
            var uri = response?.RequestMessage?.RequestUri;
            var statusCode = response?.StatusCode.ToString();
            var etag = response?.Headers.ETag?.Tag;
            var lastModified = response?.Content?.Headers?.LastModified?.ToString("R"); // RFC1123

            if (response != null && (response.IsSuccessStatusCode || response.StatusCode == HttpStatusCode.NotModified))
            {
                _logger.Information("HTTP: {StatusCode} {Method} {Url} {ETag} {LastModified} {Elapsed:F3}ms",
                    statusCode, method, uri?.GetLeftPart(uriPartial), etag, lastModified, elapsedMs);
            }
            else
            {
                _logger.Warning("HTTP: {StatusCode} {Method} {Url} {ETag} {LastModified} {Elapsed:F3}ms",
                    statusCode, method, uri?.GetLeftPart(uriPartial), etag, lastModified, elapsedMs);
            }
        }
    }
}
