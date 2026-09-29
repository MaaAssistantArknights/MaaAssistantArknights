// <copyright file="CopilotOutput.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;

namespace MaaWpfGui.Models.Copilot;

public record CopilotOutput(IReadOnlyList<CopilotOutput.Part> Parts, string? Color)
{
    public string Content => string.Concat(Parts.Select(part => part.Text));

    public static implicit operator CopilotOutput((string Text, string? Color) output)
        => new([new Part(output.Text)], output.Color);

    public record Part(string Text, string? OperName = null);
}
