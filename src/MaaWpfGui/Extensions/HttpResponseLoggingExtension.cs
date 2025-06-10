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
using System.Net.Http;
using Serilog;

namespace MaaWpfGui.Extensions
{
    public static class HttpResponseLoggingExtension
    {
        private static readonly ILogger _logger = Serilog.Log.ForContext("SourceContext", "HttpResponseLoggingExtension");

        public static void Log(this HttpResponseMessage response, UriPartial uriPartial = UriPartial.Query)
        {
            var method = response?.RequestMessage?.Method;
            var uri = response?.RequestMessage?.RequestUri;
            var statusCode = response?.StatusCode.ToString();

            if (response is { IsSuccessStatusCode: true })
            {
                _logger.Information("HTTP: {StatusCode} {Method} {Url}", statusCode, method, uri?.GetLeftPart(uriPartial));
            }
            else
            {
                _logger.Warning("HTTP: {StatusCode} {Method} {Url}", statusCode, method, uri?.GetLeftPart(uriPartial));
            }
        }
    }
}
