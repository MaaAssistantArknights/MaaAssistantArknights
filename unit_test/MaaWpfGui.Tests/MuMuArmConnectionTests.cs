// <copyright file="MuMuArmConnectionTests.cs" company="MaaAssistantArknights">
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
using System.IO;
using MaaWpfGui.Helper;
using Xunit;

namespace MaaWpfGui.Tests;

public sealed class MuMuArmConnectionTests : IDisposable
{
    private readonly string _directory = Path.Combine(Path.GetTempPath(), "MaaMuMuArmTests", Guid.NewGuid().ToString("N"));

    private string ProcessPath => Path.Combine(_directory, "shell", "nemux-shell-winui.exe");

    [Theory]
    [InlineData("--VmIndex 0")]
    [InlineData("\"D:\\MuMu ARM\\shell\\nemux-shell-winui.exe\" \"--VmIndex 0 --ThumbnailName example\"")]
    [InlineData("nemux-shell-winui.exe --VmIndex 0 --T token")]
    public void ReadsEndpointBeforeAdbConnect(string commandLine)
    {
        WriteState(0, "{\"ServerHost\":\"127.0.0.1\",\"ServerPort\":20027,\"AdbHost\":\"10.0.2.188\",\"AdbPort\":5555}");
        Assert.Equal("10.0.2.188:5555", MuMuArmConnection.GetAddress(ProcessPath, commandLine));
    }

    [Fact]
    public void UsesOnlyTheRunningInstanceAndRereadsChangedAddress()
    {
        WriteState(0, "{\"AdbHost\":\"10.0.2.100\",\"AdbPort\":5555}");
        WriteState(2, "{\"AdbHost\":\"10.0.2.200\",\"AdbPort\":5556}");
        Assert.Equal("10.0.2.200:5556", MuMuArmConnection.GetAddress(ProcessPath, "--VmIndex 2"));
        WriteState(2, "{\"AdbHost\":\"10.0.2.201\",\"AdbPort\":5557}");
        Assert.Equal("10.0.2.201:5557", MuMuArmConnection.GetAddress(ProcessPath, "--VmIndex 2"));
        Assert.Null(MuMuArmConnection.GetAddress(ProcessPath, "--VmIndex 1"));
    }

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    [InlineData("--VmIndex -1")]
    [InlineData("--VmIndex 0/../2")]
    [InlineData("--VmIndex 99999999999999999999")]
    [InlineData("--VmIndex 0suffix")]
    [InlineData("--OtherVmIndex 0")]
    public void DoesNotGuessInstanceFromMissingOrInvalidArguments(string? commandLine)
    {
        WriteState(0, "{\"AdbHost\":\"10.0.2.188\",\"AdbPort\":5555}");
        Assert.Null(MuMuArmConnection.GetAddress(ProcessPath, commandLine));
    }

    [Theory]
    [InlineData("")]
    [InlineData("{")]
    [InlineData("null")]
    [InlineData("[]")]
    [InlineData("{}")]
    [InlineData("{\"AdbHost\":\"10.0.2.188\"}")]
    [InlineData("{\"AdbHost\":12,\"AdbPort\":5555}")]
    [InlineData("{\"AdbHost\":\"10.0.2.188\",\"AdbPort\":\"5555\"}")]
    [InlineData("{\"AdbHost\":\"10.0.2.188\",\"AdbPort\":0}")]
    [InlineData("{\"AdbHost\":\"10.0.2.188\",\"AdbPort\":65536}")]
    [InlineData("{\"AdbHost\":\"10.0.2.188\",\"AdbPort\":1.5}")]
    [InlineData("{\"AdbHost\":\"0.0.0.0\",\"AdbPort\":5555}")]
    [InlineData("{\"AdbHost\":\"::\",\"AdbPort\":5555}")]
    [InlineData("{\"AdbHost\":\"10.0.2.188 -s other\",\"AdbPort\":5555}")]
    public void RejectsIncompleteOrInvalidState(string state)
    {
        WriteState(0, state);
        Assert.Null(MuMuArmConnection.GetAddress(ProcessPath, "--VmIndex 0"));
    }

    [Fact]
    public void FormatsIpv6Address()
    {
        WriteState(0, "{\"AdbHost\":\"fd00::2\",\"AdbPort\":5555}");
        Assert.Equal("[fd00::2]:5555", MuMuArmConnection.GetAddress(ProcessPath, "--VmIndex 0"));
    }

    [Fact]
    public void MissingProcessPathDoesNotThrow()
    {
        Assert.Null(MuMuArmConnection.GetAddress(null, "--VmIndex 0"));
    }

    public void Dispose()
    {
        if (Directory.Exists(_directory))
        {
            Directory.Delete(_directory, recursive: true);
        }
    }

    private void WriteState(int index, string json)
    {
        var directory = Path.Combine(_directory, "vms", $"vm{index}.madoa", "misc");
        Directory.CreateDirectory(directory);
        File.WriteAllText(Path.Combine(directory, "state.json"), json);
    }
}
