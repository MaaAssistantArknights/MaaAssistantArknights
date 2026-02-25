// <copyright file="CopilotViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using JetBrains.Annotations;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Models.Copilot;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Helper.CopilotHelper;
using static MaaWpfGui.Helper.PathsHelper;
using static MaaWpfGui.Models.AsstTasks.AsstCopilotTask;
using DataFormats = System.Windows.Forms.DataFormats;
using Task = System.Threading.Tasks.Task;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// The view model of copilot.
/// </summary>
// 通过 container.Get<CopilotViewModel>(); 实例化或获取实例
// ReSharper disable once ClassNeverInstantiated.Global
public partial class CopilotViewModel : Screen
{
    private readonly RunningState _runningState;
    private static readonly ILogger _logger = Log.ForContext<CopilotViewModel>();
    private static readonly SemaphoreSlim _semaphore = new(1, 1);
    private readonly List<int> _copilotIdList = []; // 用于保存作业列表中的作业的Id，对于同一个作业，只有都执行成功才点赞
    private readonly List<int> _recentlyRatedCopilotId = []; // TODO: 可能考虑加个持久化
    private AsstTaskType _taskType = AsstTaskType.Copilot;
    private readonly Dictionary<string, string> _copilotJsonPathMap = []; // 下拉框与实际作业 json 档案路径对照表

    /// <summary>
    /// 缓存的已解析作业，非即时添加的作业会使用该缓存
    /// </summary>
    private CopilotBase? _copilotCache;
    private const string CopilotIdPrefix = "maa://";
    private static readonly string TempCopilotFile = Path.Combine(CacheDir, "_temp_copilot.json");

    // VideoRecognition 已不支持：仅保留 json 作业
    private static readonly string[] _supportExt = [".json"];
    private static readonly string CopilotJsonDir = Path.Combine(ConfigDir, "copilot");
    private const string StageNameRegex = @"(?:[a-z]{0,3})(?:\d{0,2})-(?:(?:A|B|C|D|EX|S|TR|MO)-?)?(?:\d{1,2})";
    private const string InvalidStageNameChars = @"[:',\.\(\)\|\[\]\?，。【】｛｝；：]"; // 无效字符

    [GeneratedRegex(InvalidStageNameChars)]
    private static partial Regex InvalidStageNameRegex();

    /// <summary>
    /// Gets the view models of log items.
    /// </summary>
    public ObservableCollection<LogItemViewModel> LogItemViewModels { get; } = [];

    /// <summary>
    /// Gets the file items for TreeView.
    /// </summary>
    public ObservableCollection<CopilotFileItem> FileItems { get; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether gets or sets whether the file dropdown popup is open.
    /// </summary>
    public bool IsFilePopupOpen { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or private sets the view models of Copilot items.
    /// </summary>
    public ObservableCollection<CopilotItemViewModel> CopilotItemViewModels { get; } = [];

    /// <summary>
    /// Initializes a new instance of the <see cref="CopilotViewModel"/> class.
    /// </summary>
    public CopilotViewModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        DisplayName = LocalizationHelper.GetString("Copilot");
        AddLog(LocalizationHelper.GetString("CopilotTip"), showTime: false);
        _runningState = RunningState.Instance;
        _runningState.StateChanged += (_, e) => {
            Idle = e.Idle;
            Inited = e.Inited;
            Stopping = e.Stopping;
        };

        var copilotTaskList = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotTaskList, string.Empty);
        if (string.IsNullOrEmpty(copilotTaskList))
        {
            return;
        }

        var list = JsonConvert.DeserializeObject<List<CopilotItemViewModel>>(copilotTaskList) ?? [];
        for (int i = 0; i < list.Count; i++)
        {
            list[i].Index = i;
            CopilotItemViewModels.Add(list[i]);
        }

        SaveCopilotTask();
        CopilotItemViewModels.CollectionChanged += (_, e) => {
            _logger.Information("Copilot item collection changed: {Action}", e.Action);
            if (e.Action == System.Collections.Specialized.NotifyCollectionChangedAction.Move)
            {
                for (int i = 0; i < CopilotItemViewModels.Count; i++)
                {
                    CopilotItemViewModels[i].Index = i;
                }

                SaveCopilotTask();
            }
        };
    }

    #region UI绑定及操作

    #region Log

    /// <summary>
    /// Adds log.
    /// </summary>
    /// <param name="content">The content.</param>
    /// <param name="color">The font color.</param>
    /// <param name="weight">The font weight.</param>
    /// <param name="showTime">Whether show time.</param>
    public void AddLog(string? content, string color = UiLogColor.Trace, string weight = "Regular", bool showTime = true)
    {
        if (string.IsNullOrEmpty(content))
        {
            return;
        }
        Execute.OnUIThread(() => {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight, "HH':'mm':'ss", showTime: showTime));
            if (showTime)
            {
                switch (color)
                {
                    case UiLogColor.Error:
                        _logger.Error("{Content}", content);
                        break;
                    case UiLogColor.Warning:
                        _logger.Warning("{Content}", content);
                        break;
                    default:
                        _logger.Information("{Content}", content);
                        break;
                }
            }
        });

        // LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
    }

    /// <summary>
    /// Clears log.
    /// </summary>
    private void ClearLog()
    {
        Execute.OnUIThread(() => {
            foreach (var log in LogItemViewModels)
            {
                if (log.ToolTip is ToolTip t)
                {
                    t.IsOpen = false;
                    t.Content = null;
                    ToolTipService.SetPlacementTarget(t, null);
                    t.PlacementTarget = null;
                }
            }

            LogItemViewModels.Clear();
            AddLog(LocalizationHelper.GetString("CopilotTip"), showTime: false);
        });
    }

    #endregion Log

    #region 属性

    /// <summary>
    /// Gets a value indicating whether it is idle.
    /// </summary>
    public bool Idle { get => field; private set => SetAndNotify(ref field, value); }

    public bool Inited { get => field; set => SetAndNotify(ref field, value); }

    public bool Stopping { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether the start button is enabled.
    /// </summary>
    public bool StartEnabled { get => field; set => SetAndNotify(ref field, value); } = true;

    private int _copilotTabIndex = 0;

    /// <summary>
    /// Gets or sets 作业类型，0：主线/故事集/SS 1：保全派驻 2：悖论模拟 3：其他活动
    /// </summary>
    public int CopilotTabIndex
    {
        get => _copilotTabIndex;
        set {
            if (!Idle)
            {
                return;
            }

            if (!SetAndNotify(ref _copilotTabIndex, value))
            {
                return;
            }
        }
    }

    private string _displayFilename = string.Empty;

    /// <summary>
    /// Gets or sets the display filename (relative path).
    /// </summary>
    public string DisplayFilename
    {
        get => _displayFilename;
        set {
            SetAndNotify(ref _displayFilename, value);
            if (string.IsNullOrEmpty(value))
            {
                Filename = string.Empty;
                return;
            }

            var copilotRoot = Path.Combine(ResourceDir, "copilot");
            var fullPath = Path.IsPathRooted(value) ? value : Path.Combine(copilotRoot, value);

            /* 相对/绝对路径 */
            if (File.Exists(fullPath))
            {
                Filename = fullPath;
            }
            /* copilot 文件夹下的文件名 */
            else if (_copilotJsonPathMap.TryGetValue(Path.GetFileName(value), out var mappedPath))
            {
                Filename = mappedPath;
            }
            /* maybe 是神秘代码，交给 FileName 处理 */
            else
            {
                Filename = value;
            }
        }
    }

    /// <summary>
    /// Gets or sets the filename.
    /// </summary>
    public string Filename
    {
        get => field;
        set {
            var processedValue = ProcessFilePath(value);
            SetAndNotify(ref field, processedValue);
            UpdateDisplayFilename(processedValue);
            ClearLog();
            UpdateCopilotUrl(processedValue);
            _ = UpdateFilename(processedValue);
        }
    } = string.Empty;

    private string ProcessFilePath(string value)
    {
        if (string.IsNullOrWhiteSpace(value) || File.Exists(value))
        {
            return value;
        }

        // 从对照表取得完整 json 档案路径
        if (_copilotJsonPathMap.TryGetValue(value, out var fullPath))
        {
            return fullPath;
        }

        var resourceFile = Path.Combine(ResourceDir, "copilot", Path.GetFileName(value));
        return File.Exists(resourceFile) ? resourceFile : value;
    }

    private void UpdateDisplayFilename(string filename)
    {
        if (string.IsNullOrEmpty(filename))
        {
            _displayFilename = string.Empty;
        }
        else
        {
            var copilotRoot = Path.Combine(ResourceDir, "copilot");
            _displayFilename = filename.StartsWith(copilotRoot, StringComparison.OrdinalIgnoreCase)
                ? Path.GetRelativePath(copilotRoot, filename)
                : filename;
        }
        NotifyOfPropertyChange(nameof(DisplayFilename));
    }

    private void UpdateCopilotUrl(string filename)
    {
        CopilotUrl = string.IsNullOrWhiteSpace(filename) ? CopilotUiUrl : CopilotUrl;
    }

    private bool _form;

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    [PropertyDependsOn(nameof(CopilotTabIndex))]
    public bool Form
    {
        get {
            // Tab=1/2 不支持自动编队，根据 CopilotTabIndex 综合判断返回值
            if (CopilotTabIndex is 1 or 2)
            {
                return false;
            }
            return _form;
        }
        set => SetAndNotify(ref _form, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    public bool AddTrust { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    public bool IgnoreRequirements { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether 真正有干员被忽略了要求
    /// </summary>
    public bool HasRequirementIgnored { get; set; } = false;

    public bool UseSanityPotion { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    public bool AddUserAdditional
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotAddUserAdditional, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotAddUserAdditional, false);

    private string _userAdditional = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotUserAdditional, string.Empty).Trim();

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    public string UserAdditional
    {
        get => _userAdditional;
        set {
            value = value.Trim();
            SetAndNotify(ref _userAdditional, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotUserAdditional, value);
        }
    }

    [PropertyDependsOn(nameof(UserAdditional))]
    public string UserAdditionalPrettyJson
    {
        get {
            if (string.IsNullOrWhiteSpace(UserAdditional))
            {
                return string.Empty;
            }

            try
            {
                return JToken.Parse(UserAdditional).ToString(Formatting.None).Replace("},", "},\n");
            }
            catch
            {
                return UserAdditional; // 解析失败时返回原始字符串
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the UserAdditional popup is open.
    /// </summary>
    public bool IsUserAdditionalPopupOpen { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets the view models of UserAdditional items.
    /// </summary>
    public ObservableCollection<UserAdditionalItemViewModel> UserAdditionalItems { get; } = [];

    /// <summary>
    /// Opens the UserAdditional popup for editing.
    /// </summary>
    public void OpenUserAdditionalPopup()
    {
        // 清空列表
        UserAdditionalItems.Clear();

        // 优先按 JSON 反序列化
        if (!string.IsNullOrWhiteSpace(UserAdditional))
        {
            try
            {
                var list = JsonConvert.DeserializeObject<List<UserAdditional>>(UserAdditional) ?? [];
                foreach (var op in list)
                {
                    if (string.IsNullOrWhiteSpace(op.Name))
                    {
                        continue;
                    }

                    int skill = op.Skill;
                    if (skill < 1)
                    {
                        skill = 1;
                    }
                    else if (skill > 3)
                    {
                        skill = 3;
                    }

                    var item = new UserAdditionalItemViewModel {
                        Name = op.Name,
                        Skill = skill,
                        Module = op.Module,
                    };
                    UserAdditionalItems.Add(item);
                }
            }
            catch
            {
                // 兼容旧格式：以 ; 和 , 解析 "name,skill;name,skill" 形式
                try
                {
                    Regex regex = new(@"(?<=;)(?<name>[^,;]+)(?:, *(?<skill>\d))?(?=;)", RegexOptions.Compiled);
                    var matches = regex.Matches(";" + UserAdditional + ";");
                    foreach (Match match in matches)
                    {
                        var name = match.Groups["name"].Value.Trim();
                        var skillStr = match.Groups["skill"].Value;
                        int skill = string.IsNullOrEmpty(skillStr) ? 1 : int.Parse(skillStr);
                        if (skill < 1)
                        {
                            skill = 1;
                        }
                        else if (skill > 3)
                        {
                            skill = 3;
                        }

                        var item = new UserAdditionalItemViewModel {
                            Name = name,
                            Skill = skill,
                            Module = 0,
                        };
                        UserAdditionalItems.Add(item);
                    }
                }
                catch
                {
                    // ignore
                }
            }
        }

        // 如果列表为空，添加一行空行
        if (UserAdditionalItems.Count == 0)
        {
            var newItem = new UserAdditionalItemViewModel { Name = string.Empty, Skill = 1, Module = 0 };
            UserAdditionalItems.Add(newItem);
        }

        IsUserAdditionalPopupOpen = true;
    }

    /// <summary>
    /// Saves the UserAdditional value from the popup.
    /// </summary>
    public void SaveUserAdditional()
    {
        // 将列表转换为 UserAdditional 并序列化为 JSON
        var list = new List<UserAdditional>();
        foreach (var item in UserAdditionalItems)
        {
            if (string.IsNullOrWhiteSpace(item.Name))
            {
                continue; // 跳过空行
            }

            int skill = item.Skill;
            if (skill < 1)
            {
                skill = 1;
            }
            else if (skill > 3)
            {
                skill = 3;
            }

            list.Add(new UserAdditional {
                Name = item.Name.Trim(),
                Skill = skill,
                Module = item.Module,
            });
        }

        UserAdditional = list.Count > 0
            ? JsonConvert.SerializeObject(list, new JsonSerializerSettings { DefaultValueHandling = DefaultValueHandling.Ignore })
            : string.Empty;

        IsUserAdditionalPopupOpen = false;
    }

    /// <summary>
    /// Cancels editing UserAdditional and closes the popup.
    /// </summary>
    public void CancelUserAdditionalEdit()
    {
        IsUserAdditionalPopupOpen = false;
    }

    /// <summary>
    /// Gets a value indicating whether can add a new UserAdditional item.
    /// </summary>
    public bool CanAddUserAdditionalItem => UserAdditionalItems.All(item => !string.IsNullOrWhiteSpace(item.Name));

    /// <summary>
    /// Adds a new UserAdditional item.
    /// </summary>
    public void AddUserAdditionalItem()
    {
        if (!CanAddUserAdditionalItem)
        {
            return;
        }

        var newItem = new UserAdditionalItemViewModel { Name = string.Empty, Skill = 1, Module = 0 };
        UserAdditionalItems.Add(newItem);
    }

    /// <summary>
    /// Removes a UserAdditional item.
    /// </summary>
    /// <param name="item">The item to remove.</param>
    public void RemoveUserAdditionalItem(UserAdditionalItemViewModel item)
    {
        if (item != null && UserAdditionalItems.Contains(item))
        {
            UserAdditionalItems.Remove(item);
        }
    }

    public static Dictionary<string, int> ModuleMapping { get; } = new()
    {
        { LocalizationHelper.GetString("CopilotWithoutModule"), 0 },
        { "χ", 1 },
        { "γ", 2 },
        { "α", 3 },
        { "Δ", 4 },
    };

    public class UserAdditionalItemViewModel : PropertyChangedBase
    {
        private string _name = string.Empty;

        /// <summary>
        /// Gets or sets the operator name.
        /// </summary>
        public string Name
        {
            get => _name;
            set => SetAndNotify(ref _name, value);
        }

        private int _skill;

        /// <summary>
        /// Gets or sets the skill number.
        /// </summary>
        public int Skill
        {
            get => _skill;
            set => SetAndNotify(ref _skill, value);
        }

        private int _module;

        /// <summary>
        /// Gets or sets the module number.
        /// -1: 不切换模组 / 无要求, 0: 不使用模组, 1-4: 不同模组
        /// </summary>
        public int Module
        {
            get => _module;
            set => SetAndNotify(ref _module, value);
        }
    }

    private bool _useFormation;

    public bool UseFormation
    {
        get => _useFormation;
        set => SetAndNotify(ref _useFormation, value);
    }

    public List<GenericCombinedData<int>> FormationSelectList { get; } =
    [
        new() { Display = "1", Value = 1 },
        new() { Display = "2", Value = 2 },
        new() { Display = "3", Value = 3 },
        new() { Display = "4", Value = 4 },
    ];

    private int _formationIndex = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotSelectFormation, 1);

    public int FormationIndex
    {
        get => _formationIndex;
        set {
            SetAndNotify(ref _formationIndex, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotSelectFormation, value.ToString());
        }
    }

    private bool _useSupportUnitUsage;

    public bool UseSupportUnitUsage
    {
        get => _useSupportUnitUsage;
        set => SetAndNotify(ref _useSupportUnitUsage, value);
    }

    private int _supportUnitUsage = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotSupportUnitUsage, 1);

    public int SupportUnitUsage
    {
        get => _supportUnitUsage;
        set {
            SetAndNotify(ref _supportUnitUsage, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotSupportUnitUsage, value.ToString());
        }
    }

    public List<GenericCombinedData<int>> SupportUnitUsageList { get; } =
    [
        /* new() { Display = LocalizationHelper.GetString("SupportUnitUsage.None"), Value = 0 }, */
        new() { Display = LocalizationHelper.GetString("SupportUnitUsage.WhenNeeded"), Value = 1 },
        new() { Display = LocalizationHelper.GetString("SupportUnitUsage.Random"), Value = 3 },
    ];

    private bool _useCopilotList;

    /// <summary>
    /// Gets or sets a value indicating whether 自动编队.
    /// </summary>
    [PropertyDependsOn(nameof(CopilotTabIndex))]
    public bool UseCopilotList
    {
        get {
            if (CopilotTabIndex is 1 or 3)
            {
                return false;
            }
            return _useCopilotList;
        }

        set {
            if (value)
            {
                // _taskType 应该只由选择的作业文件决定，不在此强制修改
                Form = true;
            }

            SetAndNotify(ref _useCopilotList, value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    private string? _copilotTaskName = string.Empty;

    public string? CopilotTaskName
    {
        get => _copilotTaskName;
        set {
            value = InvalidStageNameRegex().Replace(value ?? string.Empty, string.Empty).Trim();
            SetAndNotify(ref _copilotTaskName, value);
        }
    }

    public bool Loop { get; set; }

    private int _loopTimes = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotLoopTimes, 1);

    public int LoopTimes
    {
        get => _loopTimes;
        set {
            SetAndNotify(ref _loopTimes, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotLoopTimes, value.ToString());
        }
    }

    private string _urlText = LocalizationHelper.GetString("PrtsPlus");

    /// <summary>
    /// Gets or private sets the UrlText.
    /// </summary>
    [PropertyDependsOn(nameof(CopilotUrl))]
    public string UrlText => CopilotUrl == CopilotUiUrl ? LocalizationHelper.GetString("PrtsPlus") : LocalizationHelper.GetString("VideoLink");

    private const string CopilotUiUrl = MaaUrls.PrtsPlus;

    private string _copilotUrl = CopilotUiUrl;

    /// <summary>
    /// Gets or private sets the copilot URL.
    /// </summary>
    public string CopilotUrl
    {
        get => _copilotUrl;
        private set {
            SetAndNotify(ref _copilotUrl, value);
        }
    }

    private const string MapUiUrl = MaaUrls.MapPrts;

    private string _mapUrl = MapUiUrl;

    public string MapUrl
    {
        get => _mapUrl;
        private set => SetAndNotify(ref _mapUrl, value);
    }

    private bool _couldLikeWebJson;

    public bool CouldLikeWebJson
    {
        get => _couldLikeWebJson;
        set => SetAndNotify(ref _couldLikeWebJson, value);
    }

    #endregion 属性

    #region 方法

    /// <summary>
    /// Selects file.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void SelectFile()
    {
        var dialog = new OpenFileDialog {
            Filter = "JSON|*.json",
        };

        if (dialog.ShowDialog() == true)
        {
            Filename = dialog.FileName;
        }
    }

    /// <summary>
    /// Paste clipboard contents.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void PasteClipboard()
    {
        if (Clipboard.ContainsText())
        {
            Filename = Clipboard.GetText().Trim();
        }
        else if (Clipboard.ContainsFileDropList())
        {
            DropFile(Clipboard.GetFileDropList()[0]);
        }
    }

    /// <summary>
    /// Paste clipboard contents.
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task PasteClipboardCopilotSet()
    {
        if (CopilotTabIndex is 1)
        {
            return;
        }

        StartEnabled = false;
        UseCopilotList = true;
        ClearLog();
        if (Clipboard.ContainsText())
        {
            await GetCopilotSetAsync(Clipboard.GetText().Trim());
            CopilotUrl = CopilotUiUrl;
        }

        StartEnabled = true;
    }

    /// <summary>
    /// 批量导入作业
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task ImportFiles()
    {
        var dialog = new OpenFileDialog {
            Filter = "JSON|*.json",
            Multiselect = true,
        };

        if (dialog.ShowDialog() != true)
        {
            return;
        }

        CopilotId = 0;
        _copilotCache = null;
        foreach (var file in dialog.FileNames)
        {
            var fileInfo = new FileInfo(file);
            if (!fileInfo.Exists)
            {
                AddLog(LocalizationHelper.GetString("CopilotNoFound") + file, showTime: false);
                return;
            }

            try
            {
                using var reader = new StreamReader(File.OpenRead(file));
                var str = await reader.ReadToEndAsync();
                var payload = JsonConvert.DeserializeObject<CopilotBase>(str, new CopilotContentConverter());
                if (payload is CopilotModel copilot)
                {
                    var difficulty = copilot.Difficulty;
                    if (difficulty == CopilotModel.DifficultyFlags.None)
                    {
                        difficulty = CopilotModel.DifficultyFlags.Normal;
                    }

                    await AddCopilotTaskToList(copilot, difficulty);
                }
                else if (payload is SSSCopilotModel)
                {
                    AddLog(LocalizationHelper.GetString("CopilotSSSNotSupport"), UiLogColor.Error, showTime: false);
                }
            }
            catch
            {
                AddLog(LocalizationHelper.GetString("CopilotFileReadError"), UiLogColor.Error, showTime: false);
                return;
            }
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public async Task AddCopilotTask()
    {
        await AddCopilotTaskToList(CopilotTaskName, false);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public async Task AddCopilotTask_Adverse()
    {
        await AddCopilotTaskToList(CopilotTaskName, true);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void SelectCopilotTask(object? sender, MouseButtonEventArgs? e = null)
    {
        if (e?.Source is FrameworkElement element && element.Tag is int index)
        {
            Filename = CopilotItemViewModels[index].FilePath; // 假设原方法接受int参数
            if (e.ChangedButton == MouseButton.Right)
            {
                UseCopilotList = false;
            }
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DeleteCopilotTask(int index)
    {
        CopilotItemViewModels.RemoveAt(index);
        CopilotItemIndexChanged();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void CleanUnableCopilotTask()
    {
        foreach (var item in CopilotItemViewModels.Where(model => !model.IsChecked).ToList())
        {
            CopilotItemViewModels.Remove(item);
        }

        CopilotItemIndexChanged();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void ClearCopilotTask()
    {
        CopilotItemViewModels.Clear();
        SaveCopilotTask();

        try
        {
            Directory.Delete(CopilotJsonDir, true);
        }
        catch
        {
            // ignored
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public async Task LikeWebJson()
    {
        CouldLikeWebJson = false;
        if (await RateCopilot(CopilotId) == PrtsStatus.Success)
        {
            AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.CopilotLikeGroup);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DislikeWebJson()
    {
        CouldLikeWebJson = false;
        _ = RateCopilot(CopilotId, false);
    }

    #endregion 方法

    #endregion UI绑定及操作

    private async Task UpdateFilename(string filename)
    {
        StartEnabled = false;
        await UpdateFileDoc(filename);
        StartEnabled = true;
    }

    private async Task UpdateFileDoc(string filename)
    {
        ClearLog();
        CopilotUrl = CopilotUiUrl;
        MapUrl = MapUiUrl;
        IsDataFromWeb = false;
        CopilotId = 0;
        _copilotCache = null;

        int copilotId = 0;
        bool writeToCache = false;
        object? payload;

        if (string.IsNullOrEmpty(filename))
        {
            return;
        }
        if (File.Exists(filename))
        {
            var fileSize = new FileInfo(filename).Length;
            bool isJsonFile = filename.ToLower().EndsWith(".json") || fileSize < 4 * 1024 * 1024;
            if (!isJsonFile)
            {
                AddLog(LocalizationHelper.GetString("NotCopilotJson"), UiLogColor.Error, showTime: false);
                return;
            }

            try
            {
                using var reader = new StreamReader(File.OpenRead(filename));
                var str = await reader.ReadToEndAsync();
                payload = JsonConvert.DeserializeObject<CopilotBase>(str, new CopilotContentConverter());
            }
            catch (Exception e)
            {
                AddLog(LocalizationHelper.GetString("CopilotFileReadError") + $"\n{e.Message}", UiLogColor.Error, showTime: false);
                return;
            }
        }
        else if (filename.StartsWith(CopilotIdPrefix, StringComparison.OrdinalIgnoreCase) || int.TryParse(filename, out _))
        {
            (copilotId, payload) = await GetCopilotAsync(filename);
            if (payload is not null)
            {
                IsDataFromWeb = true;
                writeToCache = true;
                CopilotId = copilotId;
            }
        }
        else
        {
            payload = null;
        }

        switch (payload)
        {
            case CopilotModel copilot:
                await ParseCopilotAsync(copilot, writeToCache, UseCopilotList, copilotId);
                return;
            case SSSCopilotModel sss:
                await ParseSSSCopilot(sss, writeToCache);
                return;
            default:
                AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
                return;
        }
    }

    /// <summary>
    /// 为自动战斗列表匹配名字
    /// </summary>
    /// <param name="names">用于匹配的名字</param>
    /// <returns>关卡名 or string.Empty</returns>
    private static string? FindStageName(params string[] names)
    {
        names = names.Where(str => !string.IsNullOrEmpty(str)).ToArray();
        if (names.Length == 0)
        {
            return string.Empty;
        }

        // 一旦有由小写字母、数字、'-'组成的name则视为关卡名直接使用
        var directName = names.FirstOrDefault(name => Regex.IsMatch(name.ToLower(), @"^[0-9a-z\-]+$"));
        if (!string.IsNullOrEmpty(directName))
        {
            return directName;
        }

        var regex = new Regex(StageNameRegex, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        return names.Select(str => regex.Match(str)).FirstOrDefault(result => result.Success)?.Value ?? string.Empty;
    }

    #region 作业解析

    private async Task<(int CopilotId, CopilotBase? Payload)> GetCopilotAsync(string copilotCodeString)
    {
        if (copilotCodeString.StartsWith(CopilotIdPrefix, StringComparison.OrdinalIgnoreCase))
        {
            copilotCodeString = copilotCodeString[CopilotIdPrefix.Length..];
        }

        if (int.TryParse(copilotCodeString, out var copilotCode))
        {
            return await GetCopilotAsync(copilotCode);
        }

        AddLog(LocalizationHelper.GetString("CopilotNoFound") + $":{copilotCodeString}", UiLogColor.Error, showTime: false);
        return (0, null);
    }

    private async Task<(int CopilotId, CopilotBase? Payload)> GetCopilotAsync(int copilotId)
    {
        var (status, copilotset) = await RequestCopilotAsync(copilotId);
        if (status == PrtsStatus.NetworkError)
        {
            return (0, null);
        }
        else if (status == PrtsStatus.Success && copilotset is PrtsCopilotModel { StatusCode: 200 })
        {
            if (copilotset.Data?.Content is CopilotModel { } copilot)
            {
                return (copilotId, copilot); // await ParseCopilotAsync(copilot, true, copilotList, copilotId));
            }
            else if (copilotset.Data?.Content is SSSCopilotModel { } sss)
            {
                return (copilotId, sss); // await ParseSSSCopilot(sss, true));
            }

            AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
            return (0, null);
        }

        AddLog(LocalizationHelper.GetString("CopilotNoFound") + $":{copilotId}", UiLogColor.Error, showTime: false);
        return (0, null);
    }

    private async Task<bool> ParseCopilotAsync(CopilotModel copilot, bool writeToCache, bool copilotList, int copilotId)
    {
        if (string.IsNullOrEmpty(copilot.StageName))
        {
            AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
            return false;
        }

        _taskType = AsstTaskType.Copilot;
        _copilotCache = copilot;
        if (copilot.Documentation?.Details is not null)
        {
            CopilotUrl = CopilotUiUrl;
            var linkParser = new Regex(@"(?:av\d+|bv[a-z0-9]{10})(?:\/\?p=\d+)?", RegexOptions.Compiled | RegexOptions.IgnoreCase);
            var match = linkParser.Match(copilot.Documentation.Details);
            if (match.Success)
            {
                CopilotUrl = MaaUrls.BilibiliVideo + match.Value; // 视频链接
            }
        }

        var list = copilot.Opers.Concat(copilot.Groups.SelectMany(g => g.Opers)).ToList();
        foreach (var oper in list)
        {
            int rarity = DataHelper.GetCharacterByNameOrAlias(oper.Name)?.Rarity ?? -1;
            if (oper.Skill == 3 && rarity < 6)
            {
                AddLog(LocalizationHelper.GetStringFormat("UnsupportedSkill", DataHelper.GetLocalizedCharacterName(oper.Name) ?? oper.Name, oper.Skill), UiLogColor.Warning, showTime: false);
                oper.Skill = 0;
            }
            else if (oper.Skill == 2 && rarity < 4)
            {
                AddLog(LocalizationHelper.GetStringFormat("UnsupportedSkill", DataHelper.GetLocalizedCharacterName(oper.Name) ?? oper.Name, oper.Skill), UiLogColor.Warning, showTime: false);
                oper.Skill = 0;
            }
            else if (oper.Skill == 1 && rarity < 3)
            {
                AddLog(LocalizationHelper.GetStringFormat("UnsupportedSkill", DataHelper.GetLocalizedCharacterName(oper.Name) ?? oper.Name, oper.Skill), UiLogColor.Warning, showTime: false);
                oper.Skill = 0;
            }
        }
        foreach (var (output, color) in copilot.Output())
        {
            AddLog(output, color ?? UiLogColor.Message, showTime: false); // 作业信息输出
        }

        MapUrl = MapUiUrl.Replace("areas", "map/" + copilot.StageName);
        var mapInfo = DataHelper.FindMap(copilot.StageName);
        var navigateName = mapInfo?.Code;
        if (navigateName is null)
        {
            // 不支持的关卡
            AddLog(LocalizationHelper.GetString("UnsupportedStages") + $"  {copilot.StageName}", UiLogColor.Error, showTime: false);
            navigateName = FindStageName(copilot.Documentation?.Title ?? string.Empty);
            _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
        }
        CopilotTaskName = navigateName;

        if (mapInfo?.StageId is { } stageId)
        {
            if (stageId.StartsWith("mem_"))
            {
                CopilotTabIndex = 2;
            }
        }

        if (!writeToCache)
        {// 现在是暂时将所有本地作业不添加到列表
        }
        else if (CopilotTabIndex is 1)
        { // 保全 不使用多作业列表
        }
        else if (copilotList)
        {
            switch (copilot.Difficulty)
            {
                case CopilotModel.DifficultyFlags.None:
                    await AddCopilotTaskToList(copilot, CopilotModel.DifficultyFlags.Normal, navigateName, copilotId);
                    break;
                default:
                    await AddCopilotTaskToList(copilot, copilot.Difficulty, navigateName, copilotId);
                    break;
            }
        }
        else
        {
            try
            {
                await File.WriteAllTextAsync(TempCopilotFile, JsonConvert.SerializeObject(copilot, Formatting.Indented));
            }
            catch
            {
                _logger.Error("Could not save copilot task to file: " + TempCopilotFile);
                return false;
            }
        }

        return true;
    }

    private async Task<bool> ParseSSSCopilot(SSSCopilotModel copilot, bool writeToCache)
    {
        if (string.IsNullOrEmpty(copilot.StageName) || copilot.Type != new SSSCopilotModel().Type)
        {
            return false;
        }

        _taskType = AsstTaskType.SSSCopilot;
        CopilotTabIndex = 1;
        _copilotCache = copilot;
        MapUrl = MapUiUrl.Replace("areas", "map/" + copilot.StageName);
        if (copilot.Documentation?.Details is not null)
        {
            CopilotUrl = CopilotUiUrl;
            var linkParser = new Regex(@"(?:av\d+|bv[a-z0-9]{10})(?:\/\?p=\d+)?", RegexOptions.Compiled | RegexOptions.IgnoreCase);
            var match = linkParser.Match(copilot.Documentation.Details);
            if (match.Success)
            {
                CopilotUrl = MaaUrls.BilibiliVideo + match.Value; // 视频链接
            }
        }

        foreach (var (output, color) in copilot.Output())
        {
            AddLog(output, color ?? UiLogColor.Message, showTime: false);
        }

        // 不支持的关卡
        var stages = copilot.Stages?.Select(copilot => DataHelper.FindMap(copilot.StageName));
        if (stages?.Any(i => i is null) is null or true)
        {
            AddLog(LocalizationHelper.GetString("UnsupportedStages") + $"  {copilot.StageName}", UiLogColor.Error, showTime: false);
            _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
        }

        if (writeToCache)
        {
            try
            {
                await File.WriteAllTextAsync(TempCopilotFile, JsonConvert.SerializeObject(copilot, Formatting.Indented));
            }
            catch
            {
                _logger.Error("Could not save copilot task to file: " + TempCopilotFile);
                return false;
            }
        }

        return true;
    }

    #endregion 作业解析

    #region 作业集解析

    private async Task GetCopilotSetAsync(string copilotCodeString)
    {
        if (copilotCodeString.StartsWith(CopilotIdPrefix, StringComparison.OrdinalIgnoreCase))
        {
            copilotCodeString = copilotCodeString[CopilotIdPrefix.Length..];
        }

        if (int.TryParse(copilotCodeString, out var copilotCode))
        {
            await GetCopilotSetAsync(copilotCode);
            return;
        }

        AddLog(LocalizationHelper.GetString("CopilotNoFound") + $"  {copilotCodeString}", UiLogColor.Error, showTime: false);
    }

    private async Task GetCopilotSetAsync(int copilotCode)
    {
        var (status, copilotset) = await RequestCopilotSetAsync(copilotCode);
        if (status == PrtsStatus.NetworkError)
        {
            return;
        }
        else if (status == PrtsStatus.Success && copilotset is PrtsCopilotSetModel { StatusCode: 200 })
        {
            await ParseCopilotSetAsync(copilotset.Data);
            return;
        }

        AddLog(LocalizationHelper.GetString("CopilotNoFound") + $"  {copilotCode}", UiLogColor.Error, showTime: false);
        return;
    }

    private async Task ParseCopilotSetAsync(PrtsCopilotSetModel.CopilotSetData? copilotSet)
    {
        CopilotId = 0;
        _copilotCache = null;
        ClearLog();
        if (copilotSet?.CopilotIds is null)
        {
            AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
            return;
        }
        else if (copilotSet.CopilotIds.Count == 0)
        {
            Log(copilotSet.Name, copilotSet.Description);
            return;
        }

        var list = copilotSet.CopilotIds.Select(async (copilotId) => await GetCopilotAsync(copilotId)).ToList();
        foreach (var task in list)
        {
            var (copilotId, payload) = await task;
            if (payload is CopilotModel copilot)
            {
                await ParseCopilotAsync(copilot, true, true, copilotId);
            }
        }

        Log(copilotSet.Name, copilotSet.Description);
        _copilotCache = null;
        return;

        void Log(string? name, string? description)
        {
            if (!string.IsNullOrWhiteSpace(name))
            {
                AddLog(name, UiLogColor.Message, showTime: false);
            }

            if (!string.IsNullOrWhiteSpace(description))
            {
                AddLog(description, UiLogColor.Message, showTime: false);
            }
        }
    }

    #endregion 作业集解析

    /// <summary>
    /// Drops file.
    /// </summary>
    /// <param name="sender">The sender.</param>
    /// <param name="e">The event arguments.</param>
    /// TODO: 不知道为啥现在拖放不用了，之后瞅瞅
    [UsedImplicitly]
    public void DropFile(object sender, DragEventArgs e)
    {
        if (!e.Data.GetDataPresent(DataFormats.FileDrop))
        {
            return;
        }

        var filename = ((Array?)e.Data.GetData(DataFormats.FileDrop))?.GetValue(0)?.ToString();
        DropFile(filename);
    }

    private void DropFile(string? filename)
    {
        if (string.IsNullOrEmpty(filename))
        {
            return;
        }

        var filenameLower = filename.ToLower();
        bool support = _supportExt.Any(ext => filenameLower.EndsWith(ext));

        if (support)
        {
            Filename = filename;
        }
        else
        {
            Filename = string.Empty;
            ClearLog();
            AddLog(LocalizationHelper.GetString("NotCopilotJson"), UiLogColor.Error, showTime: false);
        }
    }

    /// <summary>
    /// Load file items for TreeView.
    /// </summary>
    [UsedImplicitly]
    public void LoadFileItems()
    {
        try
        {
            _copilotJsonPathMap.Clear();
            FileItems.Clear();

            var copilotRoot = Path.Combine(ResourceDir, "copilot");

            // 获取根目录下的所有目录和文件
            var directories = Directory.GetDirectories(copilotRoot);
            var rootFiles = Directory.GetFiles(copilotRoot, "*.json");

            // 添加根目录下的文件
            foreach (var file in rootFiles)
            {
                var fileName = Path.GetFileName(file);
                var relativePath = Path.GetRelativePath(copilotRoot, file);
                _copilotJsonPathMap[fileName] = file;

                FileItems.Add(new CopilotFileItem {
                    Name = fileName,
                    FullPath = file,
                    RelativePath = relativePath,
                    IsFolder = false,
                });
            }

            // 添加根目录下的文件夹（支持嵌套，old 文件夹放在最后）
            var oldFolderItem = (CopilotFileItem?)null;
            foreach (var dir in directories)
            {
                var dirName = Path.GetFileName(dir);
                var folderItem = LoadFolderItem(dir, copilotRoot);
                if (folderItem != null)
                {
                    if (dirName.Equals("old", StringComparison.OrdinalIgnoreCase))
                    {
                        oldFolderItem = folderItem;
                    }
                    else
                    {
                        FileItems.Add(folderItem);
                    }
                }
            }

            // 将 old 文件夹添加到最后
            if (oldFolderItem != null)
            {
                FileItems.Add(oldFolderItem);
            }
        }
        catch (Exception exception)
        {
            FileItems.Clear();
            AddLog(exception.Message, UiLogColor.Error, showTime: false);
        }
    }

    /// <summary>
    /// 递归加载文件夹项（支持嵌套子文件夹）
    /// </summary>
    /// <param name="dirPath">文件夹路径</param>
    /// <param name="copilotRoot">copilot 根目录路径</param>
    /// <returns>文件项，如果文件夹为空则返回 null</returns>
    private CopilotFileItem? LoadFolderItem(string dirPath, string copilotRoot)
    {
        var dirName = Path.GetFileName(dirPath);
        var folderItem = new CopilotFileItem {
            Name = dirName,
            IsFolder = true,
        };

        // 获取文件夹下的所有文件
        var folderFiles = Directory.GetFiles(dirPath, "*.json");
        foreach (var file in folderFiles)
        {
            var fileName = Path.GetFileName(file);
            var relativePath = Path.GetRelativePath(copilotRoot, file);
            _copilotJsonPathMap[fileName] = file;

            folderItem.Children.Add(new CopilotFileItem {
                Name = fileName,
                FullPath = file,
                RelativePath = relativePath,
                IsFolder = false,
            });
        }

        // 获取文件夹下的所有子文件夹（递归加载）
        var subDirectories = Directory.GetDirectories(dirPath);
        foreach (var subDir in subDirectories)
        {
            var subFolderItem = LoadFolderItem(subDir, copilotRoot);
            if (subFolderItem != null)
            {
                folderItem.Children.Add(subFolderItem);
            }
        }

        // 如果文件夹为空（既没有文件也没有子文件夹），返回 null
        if (folderItem.Children.Count == 0)
        {
            return null;
        }

        return folderItem;
    }

    /// <summary>
    /// Handle file selection from TreeView.
    /// </summary>
    /// <param name="fileItem">The selected file item.</param>
    [UsedImplicitly]
    public void OnFileSelected(CopilotFileItem? fileItem)
    {
        if (fileItem == null || fileItem.IsFolder || string.IsNullOrEmpty(fileItem.FullPath))
        {
            return;
        }

        Filename = fileItem.FullPath;
        IsFilePopupOpen = false;
    }

    /// <summary>
    /// Toggle file popup.
    /// </summary>
    [UsedImplicitly]
    public void ToggleFilePopup()
    {
        if (!IsFilePopupOpen)
        {
            LoadFileItems();
        }

        IsFilePopupOpen = !IsFilePopupOpen;
    }

    private async Task AddCopilotTaskToList(string? stageName, bool isRaid)
    {
        if (string.IsNullOrEmpty(stageName) || InvalidStageNameRegex().IsMatch(stageName))
        {
            AddLog(LocalizationHelper.GetString("CopilotInvalidStageNameForNavigation"), UiLogColor.Error, showTime: false);
            return;
        }

        try
        {
            if (_copilotCache is null)
            {
            }
            else if (_copilotCache is CopilotModel { } copilot)
            {
                await AddCopilotTaskToList(copilot, !isRaid ? CopilotModel.DifficultyFlags.Normal : CopilotModel.DifficultyFlags.Raid, stageName, CopilotId);
            }
            else if (_copilotCache is SSSCopilotModel { } sss)
            {
                AddLog(LocalizationHelper.GetString("CopilotSSSNotSupport"), UiLogColor.Error, showTime: false);
            }
        }
        catch (Exception ex)
        {
            AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
            _logger.Error(ex, "Exception caught");
        }
    }

    /// <summary>
    /// 将作业添加到列表
    /// </summary>
    /// <param name="copilot">作业</param>
    /// <param name="flags">难度等级</param>
    /// <param name="navigateName">关卡 code，用于导航</param>
    /// <param name="copilotId">作业站 id</param>
    /// <returns>是否添加了作业</returns>
    private async Task<bool> AddCopilotTaskToList(CopilotModel copilot, CopilotModel.DifficultyFlags flags, string? navigateName = null, int copilotId = 0)
    {
        if (string.IsNullOrEmpty(copilot.StageName))
        {
            _logger.Error("Could not add copilot task with empty stage");
            return false;
        }

        if (!Path.Exists(CopilotJsonDir))
        {
            try
            {
                Directory.CreateDirectory(CopilotJsonDir);
            }
            catch
            {
                return false;
            }
        }

        var mapInfo = DataHelper.FindMap(copilot.StageName);
        var stageCode = mapInfo?.Code;
        var stageId = mapInfo?.StageId;
        if (mapInfo is null)
        {
            AddLog(string.Format(LocalizationHelper.GetString("CopilotStageNameNotFound"), stageCode), UiLogColor.Error, showTime: false);
            return false;
        }

        navigateName = string.IsNullOrEmpty(navigateName) ? stageCode : navigateName;
        if (stageCode != navigateName)
        {
            stageCode = navigateName;
            AddLog(LocalizationHelper.GetString("CopilotStageNameNotEqualWithNavigateName"), UiLogColor.Warning, showTime: false);
        }

        if (stageId is null || stageCode is null || navigateName is null)
        {
            return false;
        }

        var fileName = !string.IsNullOrEmpty(stageCode) ? stageCode : DateTimeOffset.Now.ToUnixTimeSeconds().ToString();
        var cachePath = Path.GetRelativePath(BaseDir, $"{CopilotJsonDir}/{fileName}.json");
        await _semaphore.WaitAsync();
        if (File.Exists(cachePath) && CopilotItemViewModels.Any(i => i.FilePath == cachePath))
        {
            cachePath = Path.GetRelativePath(BaseDir, $"{CopilotJsonDir}/{fileName}_{DateTimeOffset.Now.ToUnixTimeMilliseconds()}.json");
            if (CopilotItemViewModels.Any(i => i.FilePath == cachePath))
            {
                _logger.Error("Could not add copilot task with duplicate stage name: {StageName}", copilot.StageName);
                _semaphore.Release();
                return false;
            }
        }

        try
        {
            await File.WriteAllTextAsync(cachePath, JsonConvert.SerializeObject(copilot, Formatting.Indented));
        }
        catch
        {
            AddLog(LocalizationHelper.GetString("CopilotCouldNotSaveFile") + cachePath, UiLogColor.Error, showTime: false);
            _semaphore.Release();
            return false;
        }

        if (CopilotTabIndex == 2)
        {
            string? name = null;
            if (stageId?.Length > 6)
            {
                var codeName = stageId[4..^2];
                var characterInfo = DataHelper.GetCharacterByCodeName(codeName);
                name = DataHelper.GetLocalizedCharacterName(characterInfo);
            }

            name ??= stageCode;

            var item = new CopilotItemViewModel(name, cachePath, false, copilotId) { Index = CopilotItemViewModels.Count, TabIndex = CopilotTabIndex, };
            CopilotItemViewModels.Add(item);
        }
        else
        {
            if (flags.HasFlag(CopilotModel.DifficultyFlags.Normal))
            {
                var item = new CopilotItemViewModel(stageCode, cachePath, false, copilotId) { Index = CopilotItemViewModels.Count, TabIndex = CopilotTabIndex, };
                CopilotItemViewModels.Add(item);
            }

            if (flags.HasFlag(CopilotModel.DifficultyFlags.Raid))
            {
                var item = new CopilotItemViewModel(stageCode, cachePath, true, copilotId) { Index = CopilotItemViewModels.Count, TabIndex = CopilotTabIndex, };
                CopilotItemViewModels.Add(item);
            }
        }

        _semaphore.Release();
        SaveCopilotTask();
        return true;
    }

    public void SaveCopilotTask()
    {
        var task = JsonConvert.SerializeObject(CopilotItemViewModels, new JsonSerializerSettings() { DefaultValueHandling = DefaultValueHandling.Ignore });
        ConfigurationHelper.SetValue(ConfigurationKeys.CopilotTaskList, task);
    }

    /// <summary>
    /// 战斗列表的当前战斗任务成功
    /// </summary>
    public void CopilotTaskSuccess()
    {
        Execute.OnUIThread(() => {
            foreach (var model in CopilotItemViewModels)
            {
                if (!model.IsChecked)
                {
                    continue;
                }

                model.IsChecked = false;

                if (model.CopilotId > 0 && _copilotIdList.Remove(model.CopilotId) && _copilotIdList.IndexOf(model.CopilotId) == -1 && !HasRequirementIgnored)
                {
                    _ = RateCopilot(model.CopilotId);
                }

                break;
            }

            SaveCopilotTask();
        });
    }

    /// <summary>
    /// 更新任务顺序
    /// </summary>
    public void CopilotItemIndexChanged()
    {
        for (int i = 0; i < CopilotItemViewModels.Count; i++)
        {
            CopilotItemViewModels[i].Index = i;
        }

        SaveCopilotTask();
    }

    /// <summary>
    /// Starts copilot.
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task Start()
    {
        /*
        if (_form)
        {
            AddLog(Localization.GetString("AutoSquadTip"), LogColor.Message);
        }*/
        _runningState.SetIdle(false);

        Instances.OverlayViewModel.LogItemsSource = LogItemViewModels;

        // if (_taskType == AsstTaskType.VideoRecognition)
        // {
        //     _ = StartVideoTask();
        //     return;
        // }

        // 统一前置校验：先按 CopilotTabIndex 分发，再判断对应选项（UseCopilotList 等）
        if (!await ValidateStartAsync())
        {
            _runningState.SetIdle(true);
            return;
        }

        await RunStartsWithScriptAsync();

        if (!await ConnectToEmulatorAsync())
        {
            Stop();
            return;
        }

        var userAdditional = ParseUserAdditionals();

        bool ret;
        try
        {
            ret = await AppendAndStartCopilotAsync(userAdditional);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to start copilot task");
            AddLog(LocalizationHelper.GetString("CopilotStartError") + ex.Message, UiLogColor.Error, showTime: false);
            ret = false;
        }

        if (ret)
        {
            AddLog(LocalizationHelper.GetString("Running"));
        }
        else
        {
            if (!Instances.AsstProxy.AsstStop())
            {
                _logger.Warning("Failed to stop Asst");
            }

            _runningState.SetIdle(true);
            AddLog(LocalizationHelper.GetString("CopilotFileReadError"), UiLogColor.Error, showTime: false);
        }
    }

    private async Task<bool> ValidateStartAsync()
    {
        if (UseCopilotList)
        {
            // 列表模式：只校验列表本身，不检查输入框里的单文件作业类型
            return await ValidateTaskListStrictAsync(tabIndex: CopilotTabIndex);
        }

        // 非列表模式必须有当前作业
        if (_copilotCache is null)
        {
            AddLog(LocalizationHelper.GetString("CopilotEmptyError"), UiLogColor.Error, showTime: false);
            return false;
        }

        // 非列表模式：检查单文件作业的 _taskType 与 CopilotTabIndex 是否匹配
        if ((_taskType == AsstTaskType.SSSCopilot && CopilotTabIndex != 1) || (_taskType != AsstTaskType.SSSCopilot && CopilotTabIndex == 1))
        {
            AddLog(LocalizationHelper.GetString("CopilotTaskTypeMismatch"), UiLogColor.Error, showTime: false);
            return false;
        }

        return true;
    }

    private async Task<bool> ValidateTaskListStrictAsync(int tabIndex)
    {
        var selected = CopilotItemViewModels.Where(i => i.IsChecked).ToArray();

        // 空列表：提示并失败
        if (selected.Length == 0)
        {
            AddLog(LocalizationHelper.GetString("CopilotStartWithEmptyList"), UiLogColor.Error, showTime: false);
            return false;
        }

        // 列表必须严格归属页签：不兼容旧版本缺少 TabIndex 的条目
        if (selected.Any(i => i.TabIndex is null))
        {
            AddLog(LocalizationHelper.GetString("CopilotTaskListLegacyItemNotSupported"), UiLogColor.Error, showTime: false);
            return false;
        }

        var tabs = selected.Select(i => i.TabIndex!.Value).Distinct().ToArray();
        if (tabs.Length > 1)
        {
            AddLog(LocalizationHelper.GetString("CopilotTaskListMixedModeNotAllowed"), UiLogColor.Error, showTime: false);
            return false;
        }

        var listTab = tabs[0];
        if (listTab != tabIndex)
        {
            AddLog(LocalizationHelper.GetStringFormat(
                "CopilotTaskListTabMismatch",
                GetCopilotTabName(tabIndex),
                GetCopilotTabName(listTab)), UiLogColor.Error, showTime: false);
            return false;
        }

        // 先判断 CopilotTabIndex，再判断对应选项
        if (tabIndex == 2)
        {
            return VerifyParadoxTasks(selected);
        }

        return await VerifyCopilotListTask(selected);
    }

    private async Task RunStartsWithScriptAsync()
    {
        if (!SettingsViewModel.GameSettings.CopilotWithScript)
        {
            return;
        }

        await Task.Run(() => SettingsViewModel.GameSettings.RunScript("StartsWithScript", showLog: false));
        if (!string.IsNullOrWhiteSpace(SettingsViewModel.GameSettings.StartsWithScript))
        {
            AddLog(LocalizationHelper.GetString("StartsWithScript"));
        }
    }

    private async Task<bool> ConnectToEmulatorAsync()
    {
        AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));

        string errMsg = string.Empty;
        bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        if (caught)
        {
            return true;
        }

        AddLog(errMsg, UiLogColor.Error);
        return false;
    }

    private IEnumerable<UserAdditional> ParseUserAdditionals()
    {
        // 反序列化自定义追加干员列表（JSON），兼容旧格式
        if (string.IsNullOrWhiteSpace(UserAdditional))
        {
            return [];
        }

        try
        {
            var list = JsonConvert.DeserializeObject<List<UserAdditional>>(UserAdditional) ?? [];
            foreach (var op in list)
            {
                if (string.IsNullOrWhiteSpace(op.Name))
                {
                    continue;
                }

                int skill = op.Skill;
                if (skill < 1)
                {
                    skill = 1;
                }
                else if (skill > 3)
                {
                    skill = 3;
                }

                op.Skill = skill;
            }

            return list.Where(op => !string.IsNullOrWhiteSpace(op.Name));
        }
        catch
        {
            // 兼容旧格式：以 ; 和 , 解析 "name,skill;name,skill" 形式
            try
            {
                Regex regex = new(@"(?<=;)(?<name>[^,;]+)(?:, *(?<skill>\d))?(?=;)", RegexOptions.Compiled);
                var matches = regex.Matches(";" + UserAdditional + ";").ToList();
                return matches.Select(match => {
                    var name = match.Groups[1].Value.Trim();
                    var skillStr = match.Groups[2].Value;
                    int skill = string.IsNullOrEmpty(skillStr) ? 1 : int.Parse(skillStr);
                    if (skill < 1)
                    {
                        skill = 1;
                    }
                    else if (skill > 3)
                    {
                        skill = 3;
                    }

                    return new UserAdditional {
                        Name = name,
                        Skill = skill,
                        Module = 0,
                    };
                }).Where(op => !string.IsNullOrWhiteSpace(op.Name));
            }
            catch
            {
                return [];
            }
        }
    }

    private async Task<bool> AppendAndStartCopilotAsync(IEnumerable<UserAdditional> userAdditional)
    {
        if (!UseCopilotList)
        {
        }
        else if (CopilotTabIndex == 0)
        {
            _copilotIdList.Clear();

            var t = CopilotItemViewModels.Where(i => i.IsChecked).Select(i => {
                _copilotIdList.Add(i.CopilotId);
                return new MultiTask { FileName = i.FilePath, IsRaid = i.IsRaid, StageName = i.Name, };
            });

            var task = new AsstCopilotTask() {
                MultiTasks = [.. t],
                Formation = Form,
                SupportUnitUsage = UseSupportUnitUsage ? SupportUnitUsage : 0,
                AddTrust = AddTrust,
                IgnoreRequirements = IgnoreRequirements,
                UserAdditionals = AddUserAdditional ? [.. userAdditional] : [],
                UseSanityPotion = UseSanityPotion,
                FormationIndex = UseFormation ? FormationIndex : 0,
            };

            // 能用列表的是主线/ss/故事集/悖论，都是 Copilot 类型
            var ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, task);
            return ret && Instances.AsstProxy.AsstStart();
        }
        else if (CopilotTabIndex == 2)
        {
            _copilotIdList.Clear();

            var t = CopilotItemViewModels.Where(i => i.IsChecked).Select(i => {
                _copilotIdList.Add(i.CopilotId);
                return i.FilePath;
            });

            var task = new AsstParadoxCopilotTask() { MultiTasks = [.. t], };
            var ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, task);
            return ret && Instances.AsstProxy.AsstStart();
        }
        else
        {
            return false;
        }

        if (IsDataFromWeb)
        {
            try
            {
                await File.WriteAllTextAsync(TempCopilotFile, JsonConvert.SerializeObject(_copilotCache, Formatting.Indented));
            }
            catch
            {
                AddLog(LocalizationHelper.GetString("CopilotCouldNotSaveFile") + TempCopilotFile, UiLogColor.Error);
                return false;
            }
        }

        bool appended;
        if (CopilotTabIndex == 2)
        {
            var singleTask = new AsstParadoxCopilotTask() { FileName = TempCopilotFile };
            appended = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, singleTask);
        }
        else
        {
            var singleTask = new AsstCopilotTask() {
                FileName = IsDataFromWeb ? TempCopilotFile : Filename,
                Formation = Form,
                SupportUnitUsage = UseSupportUnitUsage ? SupportUnitUsage : 0,
                AddTrust = AddTrust,
                IgnoreRequirements = IgnoreRequirements,
                UserAdditionals = AddUserAdditional ? [.. userAdditional] : [],
                LoopTimes = Loop ? LoopTimes : 1,
                UseSanityPotion = false,
                FormationIndex = UseFormation ? FormationIndex : 0,
            };

            // 单作业需要区分 Copilot / SSSCopilot
            appended = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, _taskType, singleTask.Serialize().Params);
        }

        return appended && Instances.AsstProxy.AsstStart();
    }

    // private bool StartVideoTask()
    // {
    //     return Instances.AsstProxy.AsstStartVideoRec(Filename);
    // }

    /// <summary>
    /// Stops copilot.
    /// UI 绑定的方法
    /// </summary>
    public void Stop()
    {
        if (SettingsViewModel.GameSettings.CopilotWithScript && SettingsViewModel.GameSettings.ManualStopWithScript)
        {
            Task.Run(() => SettingsViewModel.GameSettings.RunScript("EndsWithScript", showLog: false));
            if (!string.IsNullOrWhiteSpace(SettingsViewModel.GameSettings.EndsWithScript))
            {
                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("EndsWithScript"));
            }
        }

        if (!Instances.AsstProxy.AsstStop())
        {
            _logger.Warning("Failed to stop Asst");
        }

        _runningState.SetIdle(true);
    }

    private bool _isDataFromWeb;

    private bool IsDataFromWeb
    {
        get => _isDataFromWeb;
        set => SetAndNotify(ref _isDataFromWeb, value);
    }

    private int _copilotId;

    private int CopilotId
    {
        get => _copilotId;
        set {
            SetAndNotify(ref _copilotId, value);
            CouldLikeWebJson = value > 0;
        }
    }

    private async Task<PrtsStatus> RateCopilot(int copilotId, bool isLike = true)
    {
        if (copilotId <= 0 || _recentlyRatedCopilotId.Contains(copilotId))
        {
            return PrtsStatus.NotFound;
        }

        var result = await RateWebJsonAsync(copilotId, isLike ? "Like" : "Dislike");
        switch (result)
        {
            case PrtsStatus.Success:
                _recentlyRatedCopilotId.Add(copilotId);
                AddLog(LocalizationHelper.GetString("ThanksForLikeWebJson"), UiLogColor.Info, showTime: false);
                break;
            case PrtsStatus.NetworkError:
                AddLog(LocalizationHelper.GetString("FailedToLikeWebJson"), UiLogColor.Error, showTime: false);
                break;
        }

        return result;
    }

    private async Task<bool> VerifyCopilotListTask()
    {
        return await VerifyCopilotListTask(null);
    }

    private async Task<bool> VerifyCopilotListTask(IEnumerable<CopilotItemViewModel>? items)
    {
        var copilotItemViewModels = (items ?? CopilotItemViewModels.Where(i => i.IsChecked)).ToArray();
        switch (copilotItemViewModels.Length)
        {
            case 0:
                AddLog(LocalizationHelper.GetString("CopilotStartWithEmptyList"), UiLogColor.Error, showTime: false);
                return false;
            case 1:
                AddLog(LocalizationHelper.GetString("CopilotSingleTaskWarning"), UiLogColor.Warning, showTime: false);
                break; // 降级为警告, 有用户炸就派uuu
        }

        if (copilotItemViewModels.Any(i => string.IsNullOrEmpty(i.Name?.Trim())))
        {
            AddLog(LocalizationHelper.GetString("CopilotTasksWithEmptyName"), UiLogColor.Error, showTime: false);
            return false;
        }

        var stageNames = copilotItemViewModels.Select(i => i.FilePath).ToHashSet().Select(async path => {
            if (!File.Exists(path))
            {
                AddLog(LocalizationHelper.GetString("CopilotNoFound") + path, UiLogColor.Error, showTime: false);
                return null;
            }

            try
            {
                var str = await File.ReadAllTextAsync(path);
                return JsonConvert.DeserializeObject<CopilotModel>(str)?.StageName;
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "could not read & parse copilot file: {Path}", path);
                return null;
            }
        });
        foreach (var stageName in stageNames)
        {
            var name = await stageName;
            if (!string.IsNullOrEmpty(name) && DataHelper.FindMap(name) is not null)
            {
                continue;
            }

            AddLog(LocalizationHelper.GetString("UnsupportedStages") + $"  {name}", UiLogColor.Error, showTime: false);
            _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
            return false;
        }

        return true;
    }

    private bool VerifyParadoxTasks(IEnumerable<CopilotItemViewModel>? items = null)
    {
        var ok = true;
        foreach (var task in items ?? CopilotItemViewModels.Where(i => i.IsChecked))
        {
            if (!DataHelper.Operators.Any(op => op.Value.Name == DataHelper.GetLocalizedCharacterName(task.Name, "zh-cn")))
            {
                AddLog("illegal oper name: " + task.Name, UiLogColor.Error, showTime: false);
                _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
                ok = false;
            }
        }

        return ok;
    }

    private static string GetCopilotTabName(int tabIndex)
    {
        return tabIndex switch {
            0 => LocalizationHelper.GetString("MainStageStoryCollectionSideStory"),
            1 => LocalizationHelper.GetString("SSS"),
            2 => LocalizationHelper.GetString("ParadoxSimulation"),
            3 => LocalizationHelper.GetString("OtherActivityStage"),
            _ => tabIndex.ToString(),
        };
    }

    /// <summary>
    /// 点击后移除界面中元素焦点
    /// </summary>
    /// <param name="sender">点击事件发送者</param>
    /// <param name="e">点击事件</param>
    [UsedImplicitly]
    public void MouseDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is not UIElement element)
        {
            return;
        }

        DependencyObject scope = FocusManager.GetFocusScope(element);
        FocusManager.SetFocusedElement(scope, element);
        Keyboard.ClearFocus();
    }

    /// <summary>
    /// 回车键点击后移除界面中元素焦点
    /// </summary>
    /// <param name="sender">点击事件发送者</param>
    /// <param name="e">点击事件</param>
    [UsedImplicitly]
    public void KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key != Key.Enter)
        {
            return;
        }

        if (sender is not UIElement element)
        {
            return;
        }

        DependencyObject scope = FocusManager.GetFocusScope(element);
        FocusManager.SetFocusedElement(scope, element);
        Keyboard.ClearFocus();
    }
}
