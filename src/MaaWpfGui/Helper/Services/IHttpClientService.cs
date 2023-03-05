// <copyright file="IHttpClientService.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Http;
using System.Threading.Tasks;

namespace MaaWpfGui.Helper.Services
{
    public interface IHttpClientService
    {
        Task<string> GetStringAsync(Uri uri, Dictionary<string, string> extraHeader = null);

        Task<Stream> GetStreamAsync(Uri uri, Dictionary<string, string> extraHeader = null);

        Task<HttpResponseMessage> GetAsync(Uri uri, Dictionary<string, string> extraHeader = null);

        Task<bool> DownloadFileAsync(Uri uri, string fileName, string contentType = null);
    }
}
