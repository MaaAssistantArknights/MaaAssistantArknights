// <copyright file="CopilotItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

[JsonObject(MemberSerialization.OptIn)]
public class CopilotItemViewModel : PropertyChangedBase
{
    /// <summary>
    /// Initializes a new instance of the <see cref="CopilotItemViewModel"/> class.
    /// </summary>
    /// <param name="name">The name</param>
    /// <param name="filePath">The original Name of file</param>
    /// <param name="isRaid">是否为突袭关</param>
    /// <param name="copilotId">作业站对应 id，本地作业应为默认值 0</param>
    /// <param name="isChecked">isChecked</param>
    /// <param name="isNavNameOverride">是否覆盖导航识别名</param>
    public CopilotItemViewModel(string name, string filePath, bool isRaid = false, int copilotId = 0, bool isChecked = true, bool isNavNameOverride = false)
    {
        Name = name;
        FilePath = filePath;
        _isRaid = isRaid;
        CopilotId = copilotId;
        _isChecked = isChecked;
        IsNavNameOverride = isNavNameOverride;
    }

    [System.Text.Json.Serialization.JsonConstructor]
    public CopilotItemViewModel()
    {
    }

    /// <summary>
    /// Gets the name.
    /// </summary>
    [JsonProperty("name")]
    public string Name { get; set; } = string.Empty;

    public bool IsNavNameOverride { get; set; }

    /// <summary>
    /// Gets the original_name.
    /// </summary>
    [JsonProperty("file_path")]
    public string FilePath { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 作业站对应 id，本地作业应为默认值 0
    /// </summary>
    [JsonProperty("copilot_id")]
    public int CopilotId { get; set; }

    [JsonProperty("is_raid")]
    [JsonInclude]
    [JsonPropertyName("IsRaid")]
    private bool _isRaid;

    /// <summary>
    /// Gets or sets a value indicating whether 突袭关
    /// </summary>
    [System.Text.Json.Serialization.JsonIgnore]
    public bool IsRaid
    {
        get => _isRaid;
        set {
            SetAndNotify(ref _isRaid, value);
            Instances.CopilotViewModel.SaveCopilotTask();
        }
    }

    [JsonProperty("is_checked")]
    [JsonInclude]
    [JsonPropertyName("IsChecked")]
    private bool _isChecked;

    /// <summary>
    /// Gets or sets a value indicating whether the key is checked.
    /// </summary>
    [System.Text.Json.Serialization.JsonIgnore]
    public bool IsChecked
    {
        get => _isChecked;
        set {
            SetAndNotify(ref _isChecked, value);
            Instances.CopilotViewModel.SaveCopilotTask();
        }
    }

    public int Index { get; set => SetAndNotify(ref field, value); }
}
