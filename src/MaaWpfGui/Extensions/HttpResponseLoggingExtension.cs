// <copyright file="HttpResponseLoggingExtension.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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

        public static void Log(this HttpResponseMessage response, bool logQuery = true)
        {
            var method = response?.RequestMessage?.Method;
            var uri = response?.RequestMessage?.RequestUri;
            var statusCode = response?.StatusCode.ToString();

            if (response is { IsSuccessStatusCode: true })
            {
                _logger.Information("HTTP: {StatusCode} {Method} {Url}", statusCode, method, uri?.GetLeftPart(logQuery ? UriPartial.Query : UriPartial.Path));
            }
            else
            {
                _logger.Warning("HTTP: {StatusCode} {Method} {Url}", statusCode, method, uri?.GetLeftPart(logQuery ? UriPartial.Query : UriPartial.Path));
            }
        }
    }
}
