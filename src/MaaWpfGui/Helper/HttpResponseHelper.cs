// <copyright file="HttpResponseHelper.cs" company="MaaAssistantArknights">
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
#nullable enable

using System;
using System.IO;
using System.Net.Http;
using System.Threading.Tasks;
using Serilog;

namespace MaaWpfGui.Helper
{
    public static class HttpResponseHelper
    {
        private static readonly ILogger _logger = Log.ForContext("SourceContext", "HttpResponseHelper");

        public static async Task<bool> SaveResponseToFileAsync(HttpResponseMessage? response, string saveTo, bool saveAndDeleteTmp = true)
        {
            saveTo = Path.Combine(Environment.CurrentDirectory, saveTo);

            var tempFile = saveTo + ".tmp";
            if (File.Exists(tempFile))
            {
                File.Delete(tempFile);
            }

            try
            {
                await using var stream = await GetStreamAsync(response);
                await using var fileStream = new FileStream(tempFile, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true);
                await stream!.CopyToAsync(fileStream).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to save response to file: " + e.Message);
                return false;
            }

            // ReSharper disable once InvertIf
            if (saveAndDeleteTmp)
            {
                File.Copy(tempFile, saveTo, true);
                File.Delete(tempFile);
            }

            return true;
        }

        // ReSharper disable once MemberCanBePrivate.Global
        public static async Task<Stream?> GetStreamAsync(HttpResponseMessage? response)
        {
            if (response == null)
            {
                return null;
            }

            try
            {
                return await response.Content.ReadAsStreamAsync().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to get response as stream: " + e.Message);
                return null;
            }
        }

        public static async Task<string> GetStringAsync(HttpResponseMessage? response)
        {
            if (response == null)
            {
                return string.Empty;
            }

            try
            {
                return await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to get response as string: " + e.Message);
                return string.Empty;
            }
        }
    }
}
