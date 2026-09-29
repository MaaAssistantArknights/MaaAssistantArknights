// <copyright file="TestDependencies.cs" company="MaaAssistantArknights">
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

using System.Net.Http;

// Compile the production downloader without the Windows-only WPF application.
namespace MaaWpfGui.Helper
{
    internal static class Instances
    {
        internal static TestHttpService HttpService { get; } = new();
    }

    internal sealed class TestHttpService
    {
        internal Func<Uri, CancellationToken, Task<HttpResponseMessage>> Handler { get; set; } =
            (_, _) => throw new InvalidOperationException("Unexpected HTTP request");

        public Task<HttpResponseMessage> GetAsync(Uri uri, CancellationToken token = default) => Handler(uri, token);
    }
}

namespace MaaWpfGui.Configuration.Factory
{
    internal static class ConfigFactory
    {
        internal static RootSettings Root { get; } = new();
        internal sealed class RootSettings
        {
            public GuiSettings Gui { get; } = new();
        }

        internal sealed class GuiSettings
        {
            public string Localization => "en-us";
        }
    }
}
