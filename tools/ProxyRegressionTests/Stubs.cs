// <copyright file="Stubs.cs" company="MaaAssistantArknights">
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

using System.Collections.Concurrent;
using System.ComponentModel;
using System.Net.Http;

// Only UI, filesystem and logging dependencies are stubbed. All network behavior
// comes from the production HttpService.cs linked by the test project.
namespace JetBrains.Annotations
{
    [AttributeUsage(AttributeTargets.All)]
    internal sealed class UsedImplicitlyAttribute : Attribute
    {
    }
}

namespace MaaWpfGui.Constants
{
    internal static class TestConstants
    {
    }
}

namespace MaaWpfGui.Helper
{
    internal static class PathsHelper
    {
        public static string BaseDir => AppContext.BaseDirectory;
    }
}

namespace MaaWpfGui.Extensions
{
    internal static class ResponseLogging
    {
        public static void Log(this HttpResponseMessage response, UriPartial partial, double milliseconds)
        {
        }
    }
}

namespace MaaWpfGui.ViewModels.UserControl.Settings
{
    internal sealed class VersionUpdateSettingsUserControlModel : INotifyPropertyChanged
    {
        private string _proxy = string.Empty;
        private string _proxyType = "System";

        public static VersionUpdateSettingsUserControlModel Instance { get; } = new();

        public event PropertyChangedEventHandler? PropertyChanged;

        public string Proxy
        {
            get => _proxy;
            set
            {
                _proxy = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Proxy)));
            }
        }

        public string ProxyType
        {
            get => _proxyType;
            set
            {
                _proxyType = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProxyType)));
            }
        }
    }
}

namespace MaaWpfGui.ViewModels.UI
{
    internal static class SettingsViewModel
    {
        public static UserControl.Settings.VersionUpdateSettingsUserControlModel VersionUpdateSettings
            => UserControl.Settings.VersionUpdateSettingsUserControlModel.Instance;
    }
}

namespace MaaWpfGui.ViewModels.Dialogs
{
    internal static class VersionUpdateDialogViewModel
    {
        public static void OutputDownloadProgress()
        {
        }

        public static void OutputDownloadProgress(long value, long total, int valueInOneSecond, double seconds)
        {
        }
    }
}

namespace Serilog
{
    internal interface ILogger
    {
        void Information(string message, params object?[] values);

        void Warning(string message, params object?[] values);

        void Error(Exception exception, string message, params object?[] values);
    }

    internal static class Log
    {
        public static ConcurrentQueue<string> Messages { get; } = new();

        public static ILogger ForContext<T>() => new RecordingLogger();

        private sealed class RecordingLogger : ILogger
        {
            public void Information(string message, params object?[] values) => Record(message, values);

            public void Warning(string message, params object?[] values) => Record(message, values);

            public void Error(Exception exception, string message, params object?[] values) => Record(message, values);

            private static void Record(string message, object?[] values)
                => Messages.Enqueue(message + " " + string.Join(" ", values));
        }
    }
}
