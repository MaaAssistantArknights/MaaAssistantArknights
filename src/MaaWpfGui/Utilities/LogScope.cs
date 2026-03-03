// <copyright file="LogScope.cs" company="MaaAssistantArknights">
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
using System.Runtime.CompilerServices;
using Serilog;

namespace MaaWpfGui.Utilities;

public class LogScope : IDisposable
{
    private readonly ILogger _logger;
    private readonly string _methodName;
    private readonly System.Diagnostics.Stopwatch _stopwatch;

    public LogScope(ILogger logger, [CallerMemberName] string methodName = "")
    {
        _logger = logger;
        _methodName = methodName;
        _stopwatch = System.Diagnostics.Stopwatch.StartNew();
        _logger.Information("{MethodName} Enter", _methodName);
    }

    void IDisposable.Dispose()
    {
        _stopwatch.Stop();
        _logger.Information("{MethodName} Exit, {Elapsed:#,0} ms", _methodName, _stopwatch.ElapsedMilliseconds);
    }
}
