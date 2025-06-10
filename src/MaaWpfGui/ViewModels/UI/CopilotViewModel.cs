// <copyright file="CopilotViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Models.Copilot;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using Microsoft.Win32;
using Newtonsoft.Json;
using Serilog;
using Stylet;
using static MaaWpfGui.Helper.CopilotHelper;
using static MaaWpfGui.Models.AsstTasks.AsstCopilotTask;
using DataFormats = System.Windows.Forms.DataFormats;
using Task = System.Threading.Tasks.Task;

namespace MaaWpfGui.ViewModels.UI
{
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

        /// <summary>
        /// 缓存的已解析作业，非即时添加的作业会使用该缓存
        /// </summary>
        private CopilotBase? _copilotCache;
        private const string CopilotIdPrefix = "maa://";
        private const string TempCopilotFile = "cache/_temp_copilot.json";
        private static readonly string[] _supportExt = [".json", ".mp4", ".m4s", ".mkv", ".flv", ".avi"];
        private const string CopilotJsonDir = "config/copilot";
        private const string StageNameRegex = @"(?:[a-z]{0,3})(?:\d{0,2})-(?:(?:A|B|C|D|EX|S|TR|MO)-?)?(?:\d{1,2})";
        private const string InvalidStageNameChars = @"[:',\.\(\)\|\[\]\?，。【】｛｝；：]"; // 无效字符

        [GeneratedRegex(InvalidStageNameChars)]
        private static partial Regex InvalidStageNameRegex();

        /// <summary>
        /// Gets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; } = [];

        /// <summary>
        /// Gets or private sets the view models of Copilot items.
        /// </summary>
        public ObservableCollection<CopilotItemViewModel> CopilotItemViewModels { get; } = [];

        /// <summary>
        /// Initializes a new instance of the <see cref="CopilotViewModel"/> class.
        /// </summary>
        public CopilotViewModel()
        {
            DisplayName = LocalizationHelper.GetString("Copilot");
            AddLog(LocalizationHelper.GetString("CopilotTip"), showTime: false);
            _runningState = RunningState.Instance;
            _runningState.IdleChanged += RunningState_IdleChanged;

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
        }

        private void RunningState_IdleChanged(object? sender, bool e)
        {
            Idle = e;
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
        public void AddLog(string content, string color = UiLogColor.Trace, string weight = "Regular", bool showTime = true)
        {
            Execute.OnUIThread(() =>
            {
                LogItemViewModels.Add(new LogItemViewModel(content, color, weight, "HH':'mm':'ss", showTime: showTime));
                if (showTime)
                {
                    _logger.Information(content);
                }
            });

            // LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        /// <summary>
        /// Clears log.
        /// </summary>
        private void ClearLog()
        {
            Execute.OnUIThread(() => LogItemViewModels.Clear());
        }

        #endregion Log

        #region 属性

        private bool _idle;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set => SetAndNotify(ref _idle, value);
        }

        private bool _startEnabled = true;

        /// <summary>
        /// Gets or sets a value indicating whether the start button is enabled.
        /// </summary>
        public bool StartEnabled
        {
            get => _startEnabled;
            set => SetAndNotify(ref _startEnabled, value);
        }

        private string _filename = string.Empty;

        /// <summary>
        /// Gets or sets the filename.
        /// </summary>
        public string Filename
        {
            get => _filename;
            set
            {
                SetAndNotify(ref _filename, value);
                ClearLog();
                if (string.IsNullOrWhiteSpace(value))
                {
                    AddLog(LocalizationHelper.GetString("CopilotTip"), showTime: false);
                }
                else
                {
                    UpdateFilename(value);
                }
            }
        }

        private bool _form;

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        public bool Form
        {
            get => _form;
            set => SetAndNotify(ref _form, value);
        }

        private bool _addTrust;

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        public bool AddTrust
        {
            get => _addTrust;
            set => SetAndNotify(ref _addTrust, value);
        }

        private bool _useSanityPotion;

        public bool UseSanityPotion
        {
            get => _useSanityPotion;
            set => SetAndNotify(ref _useSanityPotion, value);
        }

        private bool _addUserAdditional = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CopilotAddUserAdditional, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        public bool AddUserAdditional
        {
            get => _addUserAdditional;
            set
            {
                SetAndNotify(ref _addUserAdditional, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CopilotAddUserAdditional, value.ToString());
            }
        }

        private string _userAdditional = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotUserAdditional, string.Empty).Replace("，", ",").Replace("；", ";").Trim();

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        public string UserAdditional
        {
            get => _userAdditional;
            set
            {
                value = value.Replace("，", ",").Replace("；", ";").Trim();
                SetAndNotify(ref _userAdditional, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CopilotUserAdditional, value);
            }
        }

        private bool _useCopilotList;

        /// <summary>
        /// Gets or sets a value indicating whether 自动编队.
        /// </summary>
        public bool UseCopilotList
        {
            get => _useCopilotList;
            set
            {
                if (value)
                {
                    _taskType = AsstTaskType.Copilot;
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
            set
            {
                value = InvalidStageNameRegex().Replace(value ?? string.Empty, string.Empty).Trim();
                SetAndNotify(ref _copilotTaskName, value);
            }
        }

        public bool Loop { get; set; }

        private int _loopTimes = int.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CopilotLoopTimes, "1"));

        public int LoopTimes
        {
            get => _loopTimes;
            set
            {
                SetAndNotify(ref _loopTimes, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CopilotLoopTimes, value.ToString());
            }
        }

        private string _urlText = LocalizationHelper.GetString("PrtsPlus");

        /// <summary>
        /// Gets or private sets the UrlText.
        /// </summary>
        public string UrlText
        {
            get => _urlText;
            private set => SetAndNotify(ref _urlText, value);
        }

        private const string CopilotUiUrl = MaaUrls.PrtsPlus;

        private string _copilotUrl = CopilotUiUrl;

        /// <summary>
        /// Gets or private sets the copilot URL.
        /// </summary>
        public string CopilotUrl
        {
            get => _copilotUrl;
            private set
            {
                UrlText = value == CopilotUiUrl ? LocalizationHelper.GetString("PrtsPlus") : LocalizationHelper.GetString("VideoLink");
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
        /// </summary>
        // ReSharper disable once UnusedMember.Global
        public void SelectFile()
        {
            var dialog = new OpenFileDialog
            {
                Filter = "JSON|*.json|Video|*.mp4;*.m4s;*.mkv;*.flv;*.avi",
            };

            if (dialog.ShowDialog() == true)
            {
                Filename = dialog.FileName;
            }
        }

        /// <summary>
        /// Paste clipboard contents.
        /// </summary>
        // ReSharper disable once UnusedMember.Global
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
        /// </summary>
        // ReSharper disable once UnusedMember.Global
        public async void PasteClipboardCopilotSet()
        {
            StartEnabled = false;
            UseCopilotList = true;
            ClearLog();
            if (Clipboard.ContainsText())
            {
                await GetCopilotSetAsync(Clipboard.GetText().Trim());
            }

            StartEnabled = true;
        }

        /// <summary>
        /// 批量导入作业
        /// </summary>
        // ReSharper disable once UnusedMember.Global
        public async void ImportFiles()
        {
            var dialog = new OpenFileDialog
            {
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
                        AddLog($"unsupport type: SSS", UiLogColor.Error, showTime: false);
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
        // ReSharper disable once UnusedMember.Global
        public void AddCopilotTask()
        {
            AddCopilotTaskToList(CopilotTaskName, false);
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void AddCopilotTask_Adverse()
        {
            AddCopilotTaskToList(CopilotTaskName, true);
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void SelectCopilotTask(int index)
        {
            Filename = CopilotItemViewModels[index].FilePath;
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void DeleteCopilotTask(int index)
        {
            CopilotItemViewModels.RemoveAt(index);
            CopilotItemIndexChanged();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void CleanUnableCopilotTask()
        {
            foreach (var item in CopilotItemViewModels.Where(model => !model.IsChecked).ToList())
            {
                CopilotItemViewModels.Remove(item);
            }

            CopilotItemIndexChanged();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
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

        // ReSharper disable once UnusedMember.Global
        public void LikeWebJson()
        {
            Task.Run(async () =>
            {
                if (await RateCopilot(CopilotId) == PrtsStatus.Success)
                {
                    AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.CopilotLikeGroup);
                }
            });
        }

        // ReSharper disable once UnusedMember.Global
        public void DislikeWebJson()
        {
            _ = RateCopilot(CopilotId, false);
        }

        #endregion 方法

        #endregion UI绑定及操作

        private async void UpdateFilename(string filename)
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
            if (File.Exists(filename))
            {
                var fileSize = new FileInfo(filename).Length;
                bool isJsonFile = filename.ToLower().EndsWith(".json") || fileSize < 4 * 1024 * 1024;
                if (!isJsonFile)
                {
                    _taskType = AsstTaskType.VideoRecognition;
                    return;
                }

                try
                {
                    using var reader = new StreamReader(File.OpenRead(filename));
                    var str = await reader.ReadToEndAsync();
                    payload = JsonConvert.DeserializeObject<CopilotBase>(str, new CopilotContentConverter());
                }
                catch
                {
                    AddLog(LocalizationHelper.GetString("CopilotFileReadError"), UiLogColor.Error, showTime: false);
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

            if (payload is CopilotModel copilot)
            {
                AddLog(LocalizationHelper.GetString("CopilotTip"), showTime: false);
                await ParseCopilotAsync(copilot, writeToCache, UseCopilotList, copilotId);
                return;
            }
            else if (payload is SSSCopilotModel sss)
            {
                AddLog(LocalizationHelper.GetString("CopilotTip"), showTime: false);
                await ParseSSSCopilot(sss, writeToCache);
                return;
            }

            AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
            return;
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

            foreach (var (output, color) in copilot.Output())
            {
                AddLog(output, color ?? UiLogColor.Message, showTime: false);
            }

            MapUrl = MapUiUrl.Replace("areas", "map/" + copilot.StageName);
            var navigateName = DataHelper.FindMap(copilot.StageName)?.Code;
            if (navigateName is null)
            {
                // 不支持的关卡
                AddLog(LocalizationHelper.GetString("UnsupportedStages") + $"  {copilot.StageName}", UiLogColor.Error, showTime: false);
                navigateName = FindStageName(copilot.Documentation?.Title ?? string.Empty);
                _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
            }

            CopilotTaskName = navigateName;
            if (!writeToCache)
            {// 现在是暂时将所有本地作业不添加到列表
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

            ClearLog();
            Log(copilotSet.Name, copilotSet.Description);
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
        // ReSharper disable once UnusedMember.Global
        // ReSharper disable once UnusedParameter.Global
        // TODO: 不知道为啥现在拖放不用了，之后瞅瞅
        public void DropFile(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                return;
            }

            var filename = ((Array)e.Data.GetData(DataFormats.FileDrop))?.GetValue(0)?.ToString();
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
        /// On comboBox drop down opened.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments.</param>
        // ReSharper disable once UnusedMember.Global
        // ReSharper disable once UnusedParameter.Global
        public void OnDropDownOpened(object sender, EventArgs e)
        {
            if (sender is not ComboBox comboBox)
            {
                return;
            }

            try
            {
                comboBox.ItemsSource = Directory.GetFiles(@".\resource\copilot\", "*.json");
            }
            catch (Exception exception)
            {
                comboBox.ItemsSource = null;
                AddLog(exception.Message, UiLogColor.Error, showTime: false);
            }
        }

        private async void AddCopilotTaskToList(string? stageName, bool isRaid)
        {
            if (string.IsNullOrEmpty(stageName) || InvalidStageNameRegex().IsMatch(stageName))
            {
                AddLog("Invalid stage name for navigation", UiLogColor.Error, showTime: false);
                return;
            }

            try
            {
                if (_copilotCache is CopilotModel { } copilot)
                {
                    await AddCopilotTaskToList(copilot, !isRaid ? CopilotModel.DifficultyFlags.Normal : CopilotModel.DifficultyFlags.Raid, stageName, CopilotId);
                }
            }
            catch (Exception ex)
            {
                AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error, showTime: false);
                _logger.Error(ex.ToString());
            }
        }

        /// <summary>
        /// 将作业添加到列表
        /// </summary>
        /// <param name="copilot">作业</param>
        /// <param name="flags">难度等级</param>
        /// <param name="navigateName">关卡code，用于导航</param>
        /// <param name="copilotId">作业站id</param>
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
                {// ignored
                }
            }

            var stageName = DataHelper.FindMap(copilot.StageName)?.Code;
            if (navigateName is null && stageName is null)
            {
                AddLog(stageName + " not found, and also no stage name specified", UiLogColor.Error, showTime: false);
                return false;
            }

            navigateName = string.IsNullOrEmpty(navigateName) ? stageName : navigateName;
            if (stageName != navigateName)
            {
                stageName = navigateName;
                AddLog("stageName NOT equal with navigateName, using navigateName", UiLogColor.Warning, showTime: false);
            }

            var fileName = !string.IsNullOrEmpty(stageName!) ? stageName : DateTimeOffset.Now.ToUnixTimeSeconds().ToString();
            var cachePath = $"{CopilotJsonDir}/{fileName}.json";
            await _semaphore.WaitAsync();
            if (File.Exists(cachePath) && CopilotItemViewModels.Any(i => i.FilePath == cachePath))
            {
                cachePath = $"{CopilotJsonDir}/{fileName}_{DateTimeOffset.Now.ToUnixTimeMilliseconds()}.json";
                if (CopilotItemViewModels.Any(i => i.FilePath == cachePath))
                {
                    _logger.Error("Could not add copilot task with duplicate stage name: " + copilot.StageName);
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

            if (flags.HasFlag(CopilotModel.DifficultyFlags.Normal))
            {
                var item = new CopilotItemViewModel(stageName, cachePath, false, copilotId)
                {
                    Index = CopilotItemViewModels.Count,
                };
                CopilotItemViewModels.Add(item);
            }

            if (flags.HasFlag(CopilotModel.DifficultyFlags.Raid))
            {
                var item = new CopilotItemViewModel(stageName, cachePath, true, copilotId)
                {
                    Index = CopilotItemViewModels.Count,
                };
                CopilotItemViewModels.Add(item);
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
            Execute.OnUIThread(() =>
            {
                foreach (var model in CopilotItemViewModels)
                {
                    if (!model.IsChecked)
                    {
                        continue;
                    }

                    model.IsChecked = false;

                    if (model.CopilotId > 0 && _copilotIdList.Remove(model.CopilotId) && _copilotIdList.IndexOf(model.CopilotId) == -1)
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
        // UI 绑定的方法
        // ReSharper disable once MemberCanBePrivate.Global
        public void CopilotItemIndexChanged()
        {
            Execute.OnUIThread(() =>
            {
                for (int i = 0; i < CopilotItemViewModels.Count; i++)
                {
                    CopilotItemViewModels[i].Index = i;
                }

                SaveCopilotTask();
            });
        }

        /// <summary>
        /// Starts copilot.
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public async void Start()
        {
            /*
            if (_form)
            {
                AddLog(Localization.GetString("AutoSquadTip"), LogColor.Message);
            }*/
            _runningState.SetIdle(false);

            if (_taskType == AsstTaskType.VideoRecognition)
            {
                _ = StartVideoTask();
                return;
            }

            if (_taskType == AsstTaskType.Copilot && UseCopilotList && !await VerifyCopilotListTask())
            {
                // 校验作业合法性
                _runningState.SetIdle(true);
                return;
            }
            else if (_taskType == AsstTaskType.Copilot && !UseCopilotList && _copilotCache is null)
            {
                AddLog("copilot is empty", UiLogColor.Error, showTime: false);
                _runningState.SetIdle(true);
                return;
            }

            if (SettingsViewModel.GameSettings.CopilotWithScript)
            {
                await Task.Run(() => SettingsViewModel.GameSettings.RunScript("StartsWithScript", showLog: false));
                if (!string.IsNullOrWhiteSpace(SettingsViewModel.GameSettings.StartsWithScript))
                {
                    AddLog(LocalizationHelper.GetString("StartsWithScript"));
                }
            }

            AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));

            string errMsg = string.Empty;
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                AddLog(errMsg, UiLogColor.Error);
                Stop();
                return;
            }

            Regex regex = new Regex(@"(?<=;)(?<name>[^,;]+)(?:, *(?<skill>\d))?(?=;)", RegexOptions.Compiled);

            var userAdditional = regex.Matches(";" + UserAdditional + ";").ToList().Select(match => new UserAdditional
            {
                Name = match.Groups[1].Value.Trim(),
                Skill = string.IsNullOrEmpty(match.Groups[2].Value) ? 0 : int.Parse(match.Groups[2].Value),
            });

            bool ret = true;
            if (UseCopilotList)
            {
                _copilotIdList.Clear();
                var tasks = CopilotItemViewModels.Where(i => i.IsChecked).Select(model =>
                 {
                     _copilotIdList.Add(model.CopilotId);
                     var task = new AsstCopilotTask()
                     {
                         FileName = model.FilePath,
                         Formation = _form,
                         AddTrust = _addTrust,
                         UserAdditionals = AddUserAdditional ? userAdditional.ToList() : [],
                         NeedNavigate = UseCopilotList,
                         StageName = model.Name,
                         IsRaid = model.IsRaid,
                         LoopTimes = Loop ? LoopTimes : 1,
                         UseSanityPotion = _useSanityPotion,
                     };
                     var (type, param) = task.Serialize();
                     return Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, type, param);
                 }).ToList();

                if (tasks.Count > 0)
                {
                    ret = tasks.All(t => t) && Instances.AsstProxy.AsstStart();
                }
                else
                {// 一个都没启动，怎会有如此无聊之人
                    _runningState.SetIdle(true);
                    return;
                }
            }
            else
            {
                if (IsDataFromWeb)
                {
                    try
                    {
                        await File.WriteAllTextAsync(TempCopilotFile, JsonConvert.SerializeObject(_copilotCache, Formatting.Indented));
                    }
                    catch
                    {
                        AddLog(LocalizationHelper.GetString("CopilotCouldNotSaveFile") + TempCopilotFile, UiLogColor.Error);
                        Stop();
                        return;
                    }
                }

                var task = new AsstCopilotTask()
                {
                    FileName = IsDataFromWeb ? TempCopilotFile : Filename,
                    Formation = _form,
                    AddTrust = _addTrust,
                    UserAdditionals = AddUserAdditional ? userAdditional.ToList() : [],
                    NeedNavigate = false,
                    LoopTimes = Loop ? LoopTimes : 1,
                    UseSanityPotion = _useSanityPotion,
                };
                ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.Copilot, _taskType, task.Serialize().Params);
                ret &= Instances.AsstProxy.AsstStart();
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

        private bool StartVideoTask()
        {
            return Instances.AsstProxy.AsstStartVideoRec(Filename);
        }

        /// <summary>
        /// Stops copilot.
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
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
            set
            {
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
                    CouldLikeWebJson = false;
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
            var list = CopilotItemViewModels.Where(i => i.IsChecked);
            if (list.Any(i => string.IsNullOrEmpty(i.Name.Trim())))
            {
                AddLog("copilot tasks with empty name", UiLogColor.Error, showTime: false);
                return false;
            }

            if (CopilotItemViewModels.Count == 1)
            {
                AddLog(LocalizationHelper.GetString("CopilotSingleTaskWarning"), UiLogColor.Warning, showTime: false);
            }

            var stageNames = list.Select(i => i.FilePath).ToHashSet().Select(async path =>
            {
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
                    _logger.Error("could not read & parse copilot file: " + path, ex);
                    return null;
                }
            });
            foreach (var stageName in stageNames)
            {
                var name = await stageName;
                if (string.IsNullOrEmpty(name) || DataHelper.FindMap(name) is null)
                {
                    AddLog(LocalizationHelper.GetString("UnsupportedStages") + $"  {name}", UiLogColor.Error, showTime: false);
                    _ = Task.Run(ResourceUpdater.ResourceUpdateAndReloadAsync);
                    AchievementTrackerHelper.Instance.Unlock(AchievementIds.MapOutdated);
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// 点击后移除界面中元素焦点
        /// </summary>
        /// <param name="sender">点击事件发送者</param>
        /// <param name="e">点击事件</param>
        // ReSharper disable once UnusedMember.Global
        // ReSharper disable once UnusedParameter.Global
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
        // ReSharper disable once UnusedMember.Global
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
}
