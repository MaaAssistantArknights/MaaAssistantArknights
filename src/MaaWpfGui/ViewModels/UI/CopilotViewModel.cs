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
using MaaWpfGui.Configuration.Factory;
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
    private const string CopilotNewIdPrefix = "prts://"; // 新格式前缀，prts://12345 为作业，prts://s12345 为作业集
    private const string CopilotNewSetIdPrefix = "prts://s"; // 新格式作业集前缀
    private static readonly string TempCopilotFile = Path.Combine(CacheDir, "_temp_copilot.json");

    // VideoRecognition 已不支持：仅保留 json 作业
    private static readonly string[] _supportExt = [".json"];
    private static readonly string CopilotJsonDir = Path.Combine(ConfigDir, "copilot");
    private const string StageNameRegex = @"(?:[a-z]{0,3})(?:\d{0,2})-(?:(?:A|B|C|D|EX|S|TR|MO)-?)?(?:\d{1,2})";
    private const string InvalidStageNameChars = @"[:',\.\(\)\|\[\]\?，。【】｛｝；：]"; // 无效字符

    [GeneratedRegex(InvalidStageNameChars)]
    private static partial Regex InvalidStageNameRegex();

    [GeneratedRegex(@"^(act\d+(side|mini)|a00\d+)_")]
    private static partial Regex SideStoryStageIdRegex();

    [GeneratedRegex(@"^(main|sub|tough|hard)_")]
    private static partial Regex MainStageIdRegex();

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
            Idle = e.NewState.Idle;
            Inited = e.NewState.Inited;
            Stopping = e.NewState.Stopping;
        };
        LocalizationHelper.LanguageChanged += () => {
            DisplayName = LocalizationHelper.GetString("Copilot");
            ClearLog();
        };
        UserAdditionalItems.CollectionChanged += (_, _) => {
            NotifyOfPropertyChange(nameof(UserAdditionalGridHeight));
            NotifyOfPropertyChange(nameof(UserAdditionalPopupVerticalOffset));
        };

        var list = ConfigFactory.CurrentConfig.Copilot.TaskList;
        for (int i = 0; i < list.Count; i++)
        {
            CopilotItemViewModels.Add(list[i]);
        }

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

            if (value == 1 || value == 3)
            {
                UseCopilotList = false;
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

            /* 神秘代码（作业站 ID），交给 FileName 处理 */
            if (IsCopilotCode(value))
            {
                Filename = value;
            }
            /* 相对/绝对路径 */
            else if (File.Exists(fullPath))
            {
                Filename = fullPath;
            }
            /* copilot 文件夹下的文件名 */
            else if (_copilotJsonPathMap.TryGetValue(Path.GetFileName(value), out var mappedPath))
            {
                Filename = mappedPath;
            }
            /* maybe 是其他神秘代码，交给 FileName 处理 */
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
        // 神秘代码不按文件路径处理，原样透传
        if (string.IsNullOrWhiteSpace(value) || IsCopilotCode(value) || File.Exists(value))
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

    /// <summary>
    /// 判断输入是否为作业站神秘代码（maa://、prts://、prts://s 前缀、s12345 或纯数字）
    /// </summary>
    private static bool IsCopilotCode(string value)
    {
        return TryParseCopilotCode(value, out _, out _);
    }

    /// <summary>
    /// 作业站代码类型
    /// </summary>
    private enum CopilotCodeType
    {
        /// <summary>不是作业站代码</summary>
        None,

        /// <summary>单个作业</summary>
        Copilot,

        /// <summary>作业集</summary>
        CopilotSet,
    }

    /// <summary>
    /// 解析作业站代码，识别所有已知格式并提取数字 ID
    /// </summary>
    /// <param name="input">原始输入（maa://12345、prts://12345、prts://s12345、s12345、12345）</param>
    /// <param name="type">解析出的类型</param>
    /// <param name="id">提取的数字 ID</param>
    /// <returns>是否成功解析</returns>
    private static bool TryParseCopilotCode(string input, out CopilotCodeType type, out int id)
    {
        type = CopilotCodeType.None;
        id = 0;

        if (string.IsNullOrWhiteSpace(input))
        {
            return false;
        }

        // 带前缀的格式（从长到短匹配，避免 prts://s 被 prts:// 抢先）
        if (input.StartsWith(CopilotNewSetIdPrefix, StringComparison.OrdinalIgnoreCase))
        {
            type = CopilotCodeType.CopilotSet;
            return int.TryParse(input[CopilotNewSetIdPrefix.Length..], out id);
        }

        if (input.StartsWith(CopilotNewIdPrefix, StringComparison.OrdinalIgnoreCase))
        {
            type = CopilotCodeType.Copilot;
            return int.TryParse(input[CopilotNewIdPrefix.Length..], out id);
        }

        if (input.StartsWith(CopilotIdPrefix, StringComparison.OrdinalIgnoreCase))
        {
            type = CopilotCodeType.Copilot;
            return int.TryParse(input[CopilotIdPrefix.Length..], out id);
        }

        // s12345 格式作业集
        if (input.Length > 1 && (input[0] is 's' or 'S') && int.TryParse(input[1..], out id))
        {
            type = CopilotCodeType.CopilotSet;
            return true;
        }

        // 纯数字，默认当单个作业
        if (int.TryParse(input, out id))
        {
            type = CopilotCodeType.Copilot;
            return true;
        }

        return false;
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

    public int CurrentCopilotId { get; set; } = -1;

    public bool UseSanityPotion { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    public bool AddUserAdditional
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Copilot.EnableUserAdditional = value;
        }
    } = ConfigFactory.CurrentConfig.Copilot.EnableUserAdditional;

    /// <summary>
    /// Gets or sets a value indicating whether to use auto-formation.
    /// </summary>
    public List<UserAdditional> UserAdditional
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Copilot.UserAdditional = value;
        }
    } = ConfigFactory.CurrentConfig.Copilot.UserAdditional;

    [PropertyDependsOn(nameof(UserAdditional))]
    public string UserAdditionalPrettyJson
    {
        get {
            if (UserAdditional.Count == 0)
            {
                return string.Empty;
            }

            return JArray.FromObject(UserAdditional).ToString(Formatting.None).Replace("},", "},\n");
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the UserAdditional popup is open.
    /// </summary>
    public bool IsUserAdditionalPopupOpen { get; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets the view models of UserAdditional items.
    /// </summary>
    public ObservableCollection<UserAdditionalItemViewModel> UserAdditionalItems { get; } = [];

    private const int MaxVisibleUserAdditionalRows = 7;

    public double UserAdditionalRowHeight => 40;

    public double UserAdditionalGridMaxHeight => 350;

    public double UserAdditionalGridHeight
    {
        get {
            var rows = Math.Clamp(UserAdditionalItems.Count, 1, MaxVisibleUserAdditionalRows);
            return UserAdditionalGridMaxHeight - ((MaxVisibleUserAdditionalRows - rows) * UserAdditionalRowHeight);
        }
    }

    public double UserAdditionalPopupVerticalOffset => (UserAdditionalGridHeight - UserAdditionalGridMaxHeight) / 2;

    /// <summary>
    /// Opens the UserAdditional popup for editing.
    /// </summary>
    public void OpenUserAdditionalPopup()
    {
        // 清空列表
        UserAdditionalItems.Clear();
        foreach (var op in UserAdditional)
        {
            if (string.IsNullOrWhiteSpace(op.Name))
            {
                continue;
            }

            var item = new UserAdditionalItemViewModel {
                Name = op.Name,
                Skill = Math.Clamp(op.Skill, 0, 3),
                Module = op.Module,
            };
            UserAdditionalItems.Add(item);
        }

        // 如果列表为空，添加一行空行
        if (UserAdditionalItems.Count == 0)
        {
            var newItem = new UserAdditionalItemViewModel { Name = string.Empty, Skill = 0, Module = 0 };
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

            list.Add(new UserAdditional {
                Name = item.Name.Trim(),
                Skill = Math.Clamp(item.Skill, 0, 3),
                Module = item.Module,
            });
        }

        UserAdditional = list;
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

        var newItem = new UserAdditionalItemViewModel { Name = string.Empty, Skill = 0, Module = 0 };
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

    public bool UseFormation { get => _useFormation; set => SetAndNotify(ref _useFormation, value); }

    public List<GenericCombinedData<int>> FormationSelectList { get; } =
    [
        new() { Display = "1", Value = 1 },
        new() { Display = "2", Value = 2 },
        new() { Display = "3", Value = 3 },
        new() { Display = "4", Value = 4 },
    ];

    public int FormationIndex
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Copilot.SelectFormation = value;
        }
    } = ConfigFactory.CurrentConfig.Copilot.SelectFormation;

    public bool UseSupportUnitUsage { get; set => SetAndNotify(ref field, value); }

    public CopilotSupportMode SupportUnitUsage
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Copilot.SupportMode = value;
        }
    } = ConfigFactory.CurrentConfig.Copilot.SupportMode;

    public List<GenericCombinedData<CopilotSupportMode>> SupportUnitUsageList { get; } =
    [
        /* new() { Display = LocalizationHelper.GetString("SupportUnitUsage.None"), Value = 0 }, */
        new() { Display = LocalizationHelper.GetString("SupportUnitUsage.WhenNeeded"), Value = CopilotSupportMode.WhenNeeded },
        new() { Display = LocalizationHelper.GetString("SupportUnitUsage.Random"), Value = CopilotSupportMode.Random },
    ];

    public enum CopilotSupportMode
    {
        /// <summary>仅补充必要</summary>
        WhenNeeded = 1,

        /// <summary>随机加一个, 刷信用点用</summary>
        Random = 3,
    }

    private bool _useCopilotList;

    /// <summary>
    /// Gets or sets a value indicating whether 自动编队.
    /// </summary>
    [PropertyDependsOn(nameof(CopilotTabIndex))]
    public bool UseCopilotList
    {
        get => _useCopilotList;
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

    public int LoopTimes
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Copilot.LoopTimes = value;
        }
    } = ConfigFactory.CurrentConfig.Copilot.LoopTimes;

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

    private string _videoUrl = string.Empty;

    /// <summary>
    /// Gets or private sets the video URL.
    /// </summary>
    public string VideoUrl
    {
        get => _videoUrl;
        private set => SetAndNotify(ref _videoUrl, value);
    }

    /// <summary>
    /// Gets a value indicating whether there is a video URL.
    /// </summary>
    [PropertyDependsOn(nameof(VideoUrl))]
    public bool HasVideoUrl => !string.IsNullOrEmpty(VideoUrl);

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
                else if (payload is SSSCopilotModel sss)
                {
                    await AddSSSCopilotTaskToList(sss);
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
        VideoUrl = string.Empty;
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
        else if (TryParseCopilotCode(filename, out var codeType, out var copilotSetId))
        {
            if (codeType == CopilotCodeType.CopilotSet)
            {
                await GetCopilotSetAsync(copilotSetId);
                return;
            }

            // 单个作业
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
        if (!TryParseCopilotCode(copilotCodeString, out _, out var copilotCode))
        {
            AddLog(LocalizationHelper.GetString("CopilotNoFound") + $":{copilotCodeString}", UiLogColor.Error, showTime: false);
            return (0, null);
        }

        return await GetCopilotAsync(copilotCode);
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

    private async Task<bool> ParseCopilotAsync(CopilotModel copilot, bool writeToCache, bool copilotList, int copilotId, bool printInfo = true)
    {
        if (string.IsNullOrEmpty(copilot.StageName))
        {
            AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
            return false;
        }

        _taskType = AsstTaskType.Copilot;
        _copilotCache = copilot;
        VideoUrl = string.Empty;
        if (copilot.Documentation?.Details is not null)
        {
            var linkParser = BVRegex();
            var match = linkParser.Match(copilot.Documentation.Details);
            if (match.Success)
            {
                VideoUrl = MaaUrls.BilibiliVideo + match.Value; // 视频链接
            }
        }

        bool is_corrected = false;
        var list = copilot.Opers.Concat(copilot.Groups.SelectMany(g => g.Opers)).ToList();
        foreach (var oper in list)
        {
            int rarity = DataHelper.GetCharacterByNameOrAlias(oper.Name)?.Rarity ?? -1;
            switch (oper.Skill)
            {
                case 3 when rarity < 6:
                case 2 when rarity < 4:
                case 1 when rarity < 3:
                    AddLog(LocalizationHelper.GetStringFormat("Copilot.UnsupportedSkill", DataHelper.GetLocalizedCharacterName(oper.Name) ?? oper.Name, oper.Skill), UiLogColor.Warning, showTime: false);
                    is_corrected = true;
                    oper.Skill = 0;
                    break;
            }
            int skillElite = oper.Skill - 1;
            int skilLevelElite = oper.Requirements?.SkillLevel switch {
                <= 4 => 0,
                <= 7 => 1,
                <= 10 => 2,
                _ => 0,
            };
            int moduleElite = oper.Requirements?.Module > 0 ? 2 : 0;
            int eliteReq = Math.Max(skillElite, Math.Max(skilLevelElite, moduleElite));
            if (eliteReq > 0)
            {
                if (oper.Requirements is null || oper.Requirements.Elite is null)
                {
                    oper.Requirements ??= new();
                    oper.Requirements.Elite = eliteReq;
                    AddLog(LocalizationHelper.GetStringFormat("Copilot.EliteEmpty", DataHelper.GetLocalizedCharacterName(oper.Name) ?? oper.Name, eliteReq), UiLogColor.Info, showTime: false);
                }
                else if (oper.Requirements.Elite < eliteReq)
                {
                    AddLog(LocalizationHelper.GetStringFormat("Copilot.EliteAjust", DataHelper.GetLocalizedCharacterName(oper.Name) ?? oper.Name, oper.Requirements.Elite, eliteReq), UiLogColor.Warning, showTime: false);
                    oper.Requirements.Elite = eliteReq;
                    is_corrected = true;
                }
            }
        }
        if (printInfo)
        {
            foreach (var (output, color) in copilot.Output())
            {
                AddLog(output, color ?? UiLogColor.Message, showTime: false); // 作业信息输出
            }
        }

        MapUrl = MapUiUrl.Replace("areas", "map/" + copilot.StageName);
        var mapInfo = DataHelper.FindMap(copilot.StageName);
        var navigateName = mapInfo?.Code;
        if (navigateName is null)
        {
            // 不支持的关卡
            AddLog(LocalizationHelper.GetStringFormat("UnsupportedStages", copilot.StageName), UiLogColor.Error, showTime: false);
            navigateName = FindStageName(copilot.Documentation?.Title ?? string.Empty);
            _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
            return true;
        }
        CopilotTaskName = navigateName;

        if (mapInfo?.StageId is { } stageId)
        {
            if (GetCopilotType(stageId) is CopilotType type and not CopilotType.Unknown)
            {
                CopilotTabIndex = (int)type;
                if (copilotList)
                {
                    UseCopilotList = CopilotTabIndex is 0 or 2;
                }
            }
        }

        if (!writeToCache)
        {// 现在是暂时将所有本地作业不添加到列表
        }
        else if (copilotList)
        {
            switch (copilot.Difficulty)
            {
                case CopilotModel.DifficultyFlags.None:
                    await AddCopilotTaskToList(copilot, CopilotModel.DifficultyFlags.Normal, navigateName, is_corrected ? default : copilotId);
                    break;
                default:
                    await AddCopilotTaskToList(copilot, copilot.Difficulty, navigateName, is_corrected ? default : copilotId);
                    break;
            }
        }
        else
        {
            try
            {
                var json = JsonConvert.SerializeObject(copilot, Formatting.Indented, new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore, });
                await File.WriteAllTextAsync(TempCopilotFile, json);
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
        VideoUrl = string.Empty;
        if (copilot.Documentation?.Details is not null)
        {
            var linkParser = new Regex(@"(?:av\d+|bv[a-z0-9]{10})(?:\/\?p=\d+)?", RegexOptions.Compiled | RegexOptions.IgnoreCase);
            var match = linkParser.Match(copilot.Documentation.Details);
            if (match.Success)
            {
                VideoUrl = MaaUrls.BilibiliVideo + match.Value; // 视频链接
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
            AddLog(LocalizationHelper.GetStringFormat("UnsupportedStages", copilot.StageName), UiLogColor.Error, showTime: false);
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

        await AddSSSCopilotTaskToList(copilot, CopilotId);
        return true;
    }

    #endregion 作业解析

    #region 作业集解析

    private async Task GetCopilotSetAsync(string copilotCodeString)
    {
        if (!TryParseCopilotCode(copilotCodeString, out _, out var copilotCode))
        {
            AddLog(LocalizationHelper.GetString("CopilotNoFound") + $"  {copilotCodeString}", UiLogColor.Error, showTime: false);
            return;
        }

        await GetCopilotSetAsync(copilotCode);
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
                if (!await ParseCopilotAsync(copilot, true, true, copilotId, false))
                {
                    AddLog(LocalizationHelper.GetString("CopilotJsonError") + $", copilotId: {copilotId}", UiLogColor.Error, showTime: false);
                    continue;
                }
                var opers = JArray.FromObject(copilot.Opers.Select(i => i.Name));
                opers = JArray.FromObject(opers.Union(JArray.FromObject(copilot.Groups.Select(i => i.Opers.Select(op => op.Name)))));
                AddLog(opers.ToString(Formatting.None), UiLogColor.Message, showTime: false);
            }
            else if (payload is SSSCopilotModel sss)
            {
                CopilotTabIndex = 1;
                await AddSSSCopilotTaskToList(sss, copilotId);
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
                await AddSSSCopilotTaskToList(sss, CopilotId);
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
            AddLog(LocalizationHelper.GetStringFormat("CopilotStageNameNotFound", $"{navigateName}"), UiLogColor.Error, showTime: false);
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

            var item = new CopilotItemViewModel(name, cachePath, false, copilotId) { Index = CopilotItemViewModels.Count, };
            CopilotItemViewModels.Add(item);
        }
        else
        {
            if (flags.HasFlag(CopilotModel.DifficultyFlags.Normal))
            {
                var item = new CopilotItemViewModel(stageCode, cachePath, false, copilotId) { Index = CopilotItemViewModels.Count, };
                CopilotItemViewModels.Add(item);
            }

            if (flags.HasFlag(CopilotModel.DifficultyFlags.Raid))
            {
                var item = new CopilotItemViewModel(stageCode, cachePath, true, copilotId) { Index = CopilotItemViewModels.Count, };
                CopilotItemViewModels.Add(item);
            }
        }

        _semaphore.Release();
        SaveCopilotTask();
        return true;
    }

    private async Task<bool> AddSSSCopilotTaskToList(SSSCopilotModel copilot, int copilotId = 0)
    {
        if (string.IsNullOrEmpty(copilot.StageName) || copilot.Type != new SSSCopilotModel().Type)
        {
            _logger.Error("Could not add SSS copilot task with empty stage");
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

        var fileName = string.Concat(copilot.StageName.Split(Path.GetInvalidFileNameChars())).Trim();
        if (string.IsNullOrWhiteSpace(fileName))
        {
            fileName = DateTimeOffset.Now.ToUnixTimeSeconds().ToString();
        }

        var cachePath = Path.GetRelativePath(BaseDir, $"{CopilotJsonDir}/{fileName}.json");
        await _semaphore.WaitAsync();
        if (File.Exists(cachePath) && CopilotItemViewModels.Any(i => i.FilePath == cachePath))
        {
            cachePath = Path.GetRelativePath(BaseDir, $"{CopilotJsonDir}/{fileName}_{DateTimeOffset.Now.ToUnixTimeMilliseconds()}.json");
            if (CopilotItemViewModels.Any(i => i.FilePath == cachePath))
            {
                _logger.Error("Could not add SSS copilot task with duplicate stage name: {StageName}", copilot.StageName);
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

        var item = new CopilotItemViewModel(copilot.StageName, cachePath, false, copilotId) { Index = CopilotItemViewModels.Count, };
        CopilotItemViewModels.Add(item);

        _semaphore.Release();
        SaveCopilotTask();
        return true;
    }

    public void SaveCopilotTask()
    {
        ConfigFactory.CurrentConfig.Copilot.TaskList = [.. CopilotItemViewModels];
    }

    /// <summary>
    /// 战斗列表的当前战斗任务成功
    /// </summary>
    public void CopilotTaskSuccess()
    {
        Execute.OnUIThread(() => {
            foreach (var model in CopilotItemViewModels)
            {
                if (!model.IsChecked || (CurrentCopilotId != -1 && model.Index != CurrentCopilotId))
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
            await Stop();
            return;
        }

        // 连接期间用户可能已点停止，需在此处拦截
        if (_runningState.GetStopping())
        {
            Instances.TaskQueueViewModel.SetStopped(SettingsViewModel.GameSettings.CopilotWithScript);
            AddLog(LocalizationHelper.GetString("Stopped"));
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

        if (tabIndex != 3)
        {
            var types = new HashSet<CopilotType>(await Task.WhenAll(selected.Select(item => GetCopilotTypeAsync(item.FilePath))));
            var concreteTypes = types.Where(type => type != CopilotType.Unknown).ToArray();
            if (types.Contains(CopilotType.Unknown))
            {
                AddLog(LocalizationHelper.GetString("CopilotTaskTypeParseFailedSkipCheck"), UiLogColor.Error, showTime: false);
            }

            if (concreteTypes.Length > 1)
            {
                AddLog(LocalizationHelper.GetString("CopilotTaskListMixedModeNotAllowed") + "\n" + string.Join(", ", concreteTypes.Select(GetCopilotTabName)), UiLogColor.Error, showTime: false);
                return false;
            }
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
        foreach (var op in UserAdditional)
        {
            if (string.IsNullOrWhiteSpace(op.Name))
            {
                continue;
            }

            op.Skill = Math.Clamp(op.Skill, 0, 3);
        }

        return UserAdditional.Where(op => !string.IsNullOrWhiteSpace(op.Name));
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
                return new MultiTask { Index = i.Index, FileName = i.FilePath, IsRaid = i.IsRaid, StageName = i.Name, };
            });

            var task = new AsstCopilotTask() {
                MultiTasks = [.. t],
                Formation = Form,
                SupportUnitUsage = UseSupportUnitUsage ? (int)SupportUnitUsage : 0,
                AddTrust = AddTrust,
                IgnoreRequirements = IgnoreRequirements,
                UserAdditionals = AddUserAdditional ? [.. userAdditional] : [],
                UseSanityPotion = UseSanityPotion,
                FormationIndex = UseFormation ? FormationIndex : 0,
            };

            // 能用列表的是主线/ss/故事集/悖论，都是 Copilot 类型
            var ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, task).IsSuccess;
            return ret && Instances.AsstProxy.AsstStart();
        }
        else if (CopilotTabIndex == 2)
        {
            _copilotIdList.Clear();

            var t = CopilotItemViewModels.Where(i => i.IsChecked).Select(i => {
                _copilotIdList.Add(i.CopilotId);
                return new AsstParadoxCopilotTask.MultiTask(i.Index, i.FilePath);
            });

            var task = new AsstParadoxCopilotTask() { MultiTasks = [.. t], };
            var ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, task).IsSuccess;
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
            var singleTask = new AsstParadoxCopilotTask() { FileName = IsDataFromWeb ? TempCopilotFile : Filename };
            appended = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, singleTask).IsSuccess;
        }
        else
        {
            var singleTask = new AsstCopilotTask() {
                FileName = IsDataFromWeb ? TempCopilotFile : Filename,
                Formation = Form,
                SupportUnitUsage = UseSupportUnitUsage ? (int)SupportUnitUsage : 0,
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
    /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
    public async Task Stop()
    {
        // 等待 Core 实际停止；回调或超时自动 SetStopped（脚本由 proxy 回调按 CopilotWithScript 设置判断）
        AddLog(LocalizationHelper.GetString("Stopping"));
        await Instances.TaskQueueViewModel.Stop();
        if (_runningState.GetIdle() && !_runningState.GetStopping())
        {
            AddLog(LocalizationHelper.GetString("Stopped"));
        }
    }

    private bool IsDataFromWeb { get => field; set => SetAndNotify(ref field, value); }

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

            AddLog(LocalizationHelper.GetStringFormat("UnsupportedStages", name), UiLogColor.Error, showTime: false);
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
                AddLog(LocalizationHelper.GetStringFormat("CopilotIllegalOperName", task.Name), UiLogColor.Error, showTime: false);
                _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
                ok = false;
            }
        }

        return ok;
    }

    private static string GetCopilotTabName(CopilotType type) =>
         type switch {
             CopilotType.MainStageAndSideStory => LocalizationHelper.GetString("MainStageStoryCollectionSideStory"),
             CopilotType.SSS => LocalizationHelper.GetString("SSS"),
             CopilotType.Paradox => LocalizationHelper.GetString("ParadoxSimulation"),
             CopilotType.Other => LocalizationHelper.GetString("OtherActivityStage"),
             _ => type.ToString(),
         };

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

    private static async Task<CopilotType> GetCopilotTypeAsync(string filePath)
    {
        try
        {
            if (!File.Exists(filePath))
            {
                _logger.Error("file not found: {Path}, return: {Return}", filePath, CopilotType.Unknown);
                return CopilotType.Unknown;
            }
            var str = await File.ReadAllTextAsync(filePath);
            var job = JsonConvert.DeserializeObject<CopilotBase>(str, new CopilotContentConverter());
            return GetCopilotType(job);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "could not read & parse copilot file: {Path}", filePath);
            return CopilotType.Unknown;
        }
    }

    private static CopilotType GetCopilotType(CopilotBase? @base)
    {
        if (@base is null)
        {
            return CopilotType.Unknown;
        }
        else if (@base is SSSCopilotModel)
        {
            return CopilotType.SSS;
        }
        else if (@base is CopilotModel copilot)
        {
            if (string.IsNullOrEmpty(copilot.StageName))
            {
                return CopilotType.Unknown;
            }
            var mapInfo = DataHelper.FindMap(copilot.StageName);
            if (mapInfo is null)
            {
                return CopilotType.Unknown;
            }
            return GetCopilotType(mapInfo.StageId);
        }

        return CopilotType.Unknown;
    }

    private static CopilotType GetCopilotType(string? stageId)
    {
        if (stageId?.StartsWith("mem_") is true)
        {
            return CopilotType.Paradox;
        }
        else if (stageId?.StartsWith("lt_") is true)
        {
            return CopilotType.SSS;
        }
        else if (!string.IsNullOrEmpty(stageId))
        {
            if (MainStageIdRegex().IsMatch(stageId))
            {
                return CopilotType.MainStageAndSideStory;
            }
            if (SideStoryStageIdRegex().IsMatch(stageId))
            {
                return CopilotType.MainStageAndSideStory;
            }
        }

        return CopilotType.Unknown;
    }

    private enum CopilotType
    {
        Unknown = -1,

        /// <summary>主线, 故事集, 支线作业</summary>
        MainStageAndSideStory = 0,

        /// <summary>保全</summary>
        SSS,

        /// <summary>悖论模拟</summary>
        Paradox,

        /// <summary>其他</summary>
        Other,
    }

    [GeneratedRegex(@"(?:av\d+|bv[a-z0-9]{10})(?:\/\?p=\d+)?", RegexOptions.IgnoreCase | RegexOptions.Compiled, "zh-CN")]
    private static partial Regex BVRegex();
}
