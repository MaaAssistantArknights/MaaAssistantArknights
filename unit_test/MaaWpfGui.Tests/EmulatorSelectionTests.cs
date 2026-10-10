// <copyright file="EmulatorSelectionTests.cs" company="MaaAssistantArknights">
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

using System.Linq;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using Xunit;

namespace MaaWpfGui.Tests;

public sealed class EmulatorSelectionTests
{
    [Theory]
    [InlineData(null, "10.0.2.100:5555", "10.0.2.200:5555")]
    [InlineData("", "10.0.2.100:5555", "10.0.2.200:5555")]
    [InlineData(null, null, "10.0.2.200:5555")]
    [InlineData("", null, "10.0.2.200:5555")]
    [InlineData(null, "10.0.2.100:5555", null)]
    [InlineData("", "10.0.2.100:5555", null)]
    public void SelectingSecondArmInstanceWithoutAdbPathPreservesItsEndpoint(string? adbPath, string? firstAddress, string? secondAddress)
    {
        var emulators = new[] {
            new WinAdapter.DetectedEmulatorInfo(ConnectConfig.MuMuArm, adbPath, firstAddress),
            new WinAdapter.DetectedEmulatorInfo(ConnectConfig.MuMuArm, adbPath, secondAddress),
        };

        // The selection dialog returns the chosen display text, not the object.
        var selectedText = emulators[1].SelectionDisplayText;
        var selected = emulators.First(emulator => emulator.SelectionDisplayText == selectedText);

        Assert.Same(emulators[1], selected);
        Assert.Equal(secondAddress, selected.Address);
    }
}
