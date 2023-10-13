// <copyright file="CopilotViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using HandyControl.Tools.Extension;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using DataFormats = System.Windows.Forms.DataFormats;
using Task = System.Threading.Tasks.Task;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of copilot.
    /// </summary>
    // 通过 container.Get<CopilotViewModel>(); 实例化或获取实例
    // ReSharper disable once ClassNeverInstantiated.Global
    public class CopilotViewModel : Screen
    {
        private readonly RunningState _runningState;
        private static readonly ILogger _logger = Log.ForContext<CopilotViewModel>();

        /// <summary>
        /// Gets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; }

        /// <summary>
        /// Gets or private sets the view models of Copilot items.
        /// </summary>
        public ObservableCollection<CopilotItemViewModel> CopilotItemViewModels { get; } = new ObservableCollection<CopilotItemViewModel>();

        /// <summary>
        /// Initializes a new instance of the <see cref="CopilotViewModel"/> class.
        /// </summary>
        public CopilotViewModel()
        {
            DisplayName = LocalizationHelper.GetString("Copilot");
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            AddLog(LocalizationHelper.GetString("CopilotTip"));
            _runningState = RunningState.Instance;
            _runningState.IdleChanged += RunningState_IdleChanged;

            var copilotTaskList = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotTaskList, string.Empty);
            if (string.IsNullOrEmpty(copilotTaskList))
            {
                return;
            }

            JArray jArray = JArray.Parse(copilotTaskList);
            foreach (var item in jArray)
            {
                if (((JObject)item).TryGetValue("file_path", out var token) && File.Exists(token.ToString()))
                {
                    CopilotItemViewModels.Add(new CopilotItemViewModel((string)item["name"], (string)item["file_path"], (bool)item["is_checked"]));
                }
            }

            CopilotItemIndexChanged();
        }

        private void RunningState_IdleChanged(object sender, bool e)
        {
            Idle = e;
        }

        /// <summary>
        /// Adds log.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        public void AddLog(string content, string color = UiLogColor.Trace, string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));

            // LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

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

        /// <summary>
        /// Clears log.
        /// </summary>
        private void ClearLog()
        {
            LogItemViewModels.Clear();
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
                UpdateFilename();
            }
        }

        private async void UpdateFilename()
        {
            StartEnabled = false;
            await UpdateFileDoc(_filename);
            StartEnabled = true;
        }

        private const string CopilotIdPrefix = "maa://";

        private async Task UpdateFileDoc(string filename)
        {
            ClearLog();
            CopilotUrl = CopilotUiUrl;
            MapUrl = MapUiUrl;
            _isVideoTask = false;

            string jsonStr;
            if (File.Exists(filename))
            {
                var fileSize = new FileInfo(filename).Length;
                bool isJsonFile = filename.ToLower().EndsWith(".json") || fileSize < 4 * 1024 * 1024;
                if (!isJsonFile)
                {
                    _isVideoTask = true;
                    return;
                }

                try
                {
                    using var reader = new StreamReader(File.OpenRead(filename));
                    jsonStr = await reader.ReadToEndAsync();
                }
                catch (Exception)
                {
                    AddLog(LocalizationHelper.GetString("CopilotFileReadError"), UiLogColor.Error);
                    return;
                }

                IsDataFromWeb = false;
                CopilotId = 0;
            }
            else if (filename.ToLower().StartsWith(CopilotIdPrefix))
            {
                var copilotIdStr = filename.ToLower().Remove(0, CopilotIdPrefix.Length);
                int.TryParse(copilotIdStr, out var numberStyles);
                jsonStr = await RequestCopilotServer(numberStyles);
                if (!string.IsNullOrEmpty(jsonStr))
                {
                    IsDataFromWeb = true;
                    CopilotId = numberStyles;
                }
            }
            else if (int.TryParse(filename, out _))
            {
                int.TryParse(filename, out var numberStyles);
                jsonStr = await RequestCopilotServer(numberStyles);
                if (!string.IsNullOrEmpty(jsonStr))
                {
                    IsDataFromWeb = true;
                    CopilotId = numberStyles;
                }
            }
            else
            {
                EasterEgg(filename);
                return;
            }

            if (jsonStr != string.Empty)
            {
                ParseJsonAndShowInfo(jsonStr);
            }
        }

        private async Task<string> RequestCopilotServer(int copilotId)
        {
            try
            {
                var jsonResponse = await Instances.HttpService.GetStringAsync(new Uri(MaaUrls.PrtsPlusCopilotGet + copilotId));
                var json = (JObject)JsonConvert.DeserializeObject(jsonResponse);
                if (json != null && json.ContainsKey("status_code") && json["status_code"]?.ToString() == "200")
                {
                    return json["data"]?["content"]?.ToString();
                }

                AddLog(LocalizationHelper.GetString("CopilotNoFound"), UiLogColor.Error);
                return string.Empty;
            }
            catch (Exception)
            {
                AddLog(LocalizationHelper.GetString("NetworkServiceError"), UiLogColor.Error);
                return string.Empty;
            }
        }

        private const string TempCopilotFile = "cache/_temp_copilot.json";
        private string _taskType = "General";

        private void ParseJsonAndShowInfo(string jsonStr)
        {
            try
            {
                var json = (JObject)JsonConvert.DeserializeObject(jsonStr);
                if (json == null)
                {
                    AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error);
                    return;
                }

                if (json.TryGetValue("stage_name", out var stageNameValue))
                {
                    MapUrl = MapUiUrl.Replace("areas", "map/" + stageNameValue);
                }

                AddLog(LocalizationHelper.GetString("CopilotTip"));

                var doc = (JObject)json["doc"];
                string title = string.Empty;
                if (doc != null && doc.TryGetValue("title", out var titleValue))
                {
                    title = titleValue.ToString();

                    do
                    {
                        // 为自动作战列表匹配名字
                        var stageNameParser = new Regex(@"([a-z]{0,3})(\d{0,2})-(EX-)?(\d{1,2})", RegexOptions.Compiled | RegexOptions.IgnoreCase);
                        var stageNameResult = stageNameParser.Matches(title);
                        if (stageNameResult.Count > 0)
                        {
                            CopilotTaskName = stageNameResult[0].Value;
                            break;
                        }

                        if (!IsDataFromWeb && (stageNameResult = stageNameParser.Matches(_filename)).Count > 0)
                        {
                            CopilotTaskName = stageNameResult[0].Value;
                            break;
                        }

                        CopilotTaskName = string.Empty;
                    }
                    while (false);
                }

                if (title.Length != 0)
                {
                    string titleColor = UiLogColor.Message;
                    if (doc != null && doc.TryGetValue("title_color", out var titleColorValue))
                    {
                        titleColor = titleColorValue.ToString();
                    }

                    AddLog(title, titleColor);
                }

                string details = string.Empty;
                if (doc != null && doc.TryGetValue("details", out var detailsValue))
                {
                    details = detailsValue.ToString();
                }

                if (details.Length != 0)
                {
                    string detailsColor = UiLogColor.Message;
                    if (doc != null && doc.TryGetValue("details_color", out var detailsColorValue))
                    {
                        detailsColor = detailsColorValue.ToString();
                    }

                    AddLog(details, detailsColor);
                    {
                        CopilotUrl = CopilotUiUrl;
                        var linkParser = new Regex(@"(?:av\d+|bv[a-z0-9]{10})(?:\/\?p=\d+)?", RegexOptions.Compiled | RegexOptions.IgnoreCase);
                        foreach (Match match in linkParser.Matches(details))
                        {
                            CopilotUrl = MaaUrls.BilibiliVideo + match.Value;
                            break;
                        }

                        if (string.IsNullOrEmpty(CopilotUrl))
                        {
                            linkParser = new Regex(@"(?:https?://)\S+\b", RegexOptions.Compiled | RegexOptions.IgnoreCase);

                            foreach (Match m in linkParser.Matches(details))
                            {
                                CopilotUrl = m.Value;
                                break;
                            }
                        }
                    }
                }

                AddLog(string.Empty, UiLogColor.Message);
                int count = 0;
                foreach (var oper in json["opers"] ?? new JArray())
                {
                    count++;
                    AddLog($"{oper["name"]}, {oper["skill"]} 技能", UiLogColor.Message);
                }

                if (json.TryGetValue("groups", out var groupsValue))
                {
                    foreach (var group in groupsValue.Cast<JObject>())
                    {
                        count++;
                        string groupName = group["name"] + ": ";
                        var operInfos = group["opers"]!.Cast<JObject>().Select(oper => $"{oper["name"]} {oper["skill"]}").ToList();

                        AddLog(groupName + string.Join(" / ", operInfos), UiLogColor.Message);
                    }
                }

                AddLog($"共 {count} 名干员", UiLogColor.Message);

                if (json.TryGetValue("type", out var typeValue))
                {
                    var type = typeValue.ToString();
                    if (type == "SSS")
                    {
                        _taskType = "SSSCopilot";

                        if (json.TryGetValue("tool_men", out var toolMenValue))
                        {
                            AddLog("编队工具人：\n" + toolMenValue, UiLogColor.Message);
                        }

                        if (json.TryGetValue("equipment", out var equipmentValue))
                        {
                            AddLog("开局装备（横向）：\n" + equipmentValue, UiLogColor.Message);
                        }

                        if (json.TryGetValue("strategy", out var strategyValue))
                        {
                            AddLog("开局策略：" + strategyValue, UiLogColor.Message);
                        }
                    }
                }
                else
                {
                    _taskType = "Copilot";
                }

                if (!IsDataFromWeb)
                {
                    return;
                }

                File.Delete(TempCopilotFile);
                File.WriteAllText(TempCopilotFile, json.ToString());
            }
            catch (Exception)
            {
                AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error);
            }
        }

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
                Filename = Clipboard.GetText();
            }
            else if (Clipboard.ContainsFileDropList())
            {
                DropFile(Clipboard.GetFileDropList()[0]);
            }
        }

        private static readonly string[] _supportExt = { ".json", ".mp4", ".m4s", ".mkv", ".flv", ".avi" };

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

            var filename = ((Array)e.Data.GetData(DataFormats.FileDrop))?.GetValue(0).ToString();
            DropFile(filename);
        }

        private void DropFile(string filename)
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
                AddLog(LocalizationHelper.GetString("NotCopilotJson"), UiLogColor.Error);
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
            if (!(sender is ComboBox comboBox))
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
                AddLog(exception.Message, UiLogColor.Error);
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

        private bool _addUserAdditional = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CopilotAddUserAdditional, false.ToString()));

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

        private string _userAdditional = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotUserAdditional, string.Empty);

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        public string UserAdditional
        {
            get => _userAdditional;
            set
            {
                SetAndNotify(ref _userAdditional, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CopilotUserAdditional, value);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        private bool _useCopilotList;

        public bool UseCopilotList
        {
            get => _useCopilotList;
            set
            {
                if (value)
                {
                    _taskType = "Copilot";
                    Form = true;
                }

                SetAndNotify(ref _useCopilotList, value);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        private string _copilotTaskName = string.Empty;

        public string CopilotTaskName
        {
            get => _copilotTaskName;
            set
            {
                SetAndNotify(ref _copilotTaskName, value);
            }
        }

        private const string CopilotJsonDir = "cache/copilot";

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void AddCopilotTask()
        {
            var stageName = CopilotTaskName.Trim().Replace("突袭", "-Adverse");
            if (!stageName.IsNullOrEmpty())
            {
                AddCopilotTaskToList(stageName);
            }
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void AddCopilotTask_Adverse()
        {
            var stageName = CopilotTaskName.Trim().Replace("突袭", "-Adverse");
            if (!stageName.EndsWith("-Adverse"))
            {
                stageName += "-Adverse";
            }

            if (!stageName.IsNullOrEmpty())
            {
                AddCopilotTaskToList(stageName);
            }
        }

        private void AddCopilotTaskToList(string stageName)
        {
            var jsonPath = $"{CopilotJsonDir}/{stageName}.json";

            try
            {
                Directory.CreateDirectory(CopilotJsonDir);
            }
            catch (Exception)
            {
                // ignored
            }

            try
            {
                if (jsonPath != (IsDataFromWeb ? TempCopilotFile : Filename))
                {
                    // 相同路径跳拷贝
                    File.Copy(IsDataFromWeb ? TempCopilotFile : Filename, jsonPath, true);
                }

                var item = new CopilotItemViewModel(stageName, jsonPath)
                {
                    Index = CopilotItemViewModels.Count,
                };
                CopilotItemViewModels.Add(item);
                SaveCopilotTask();
            }
            catch (Exception ex)
            {
                AddLog(LocalizationHelper.GetString("CopilotJsonError"), UiLogColor.Error);
                Log.Error(ex.ToString());
            }
        }

        public void SaveCopilotTask()
        {
            JArray jArray = new JArray(CopilotItemViewModels.Select(item => new JObject
            {
                ["name"] = item.Name,
                ["file_path"] = item.FilePath,
                ["is_checked"] = item.IsChecked,
            }).ToList());
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotTaskList, JsonConvert.SerializeObject(jArray));
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
        }

        public void EnterCopilotTask()
        {
            Application.Current.Dispatcher.InvokeAsync(() =>
            {
                foreach (var model in CopilotItemViewModels)
                {
                    if (!model.IsChecked)
                    {
                        continue;
                    }

                    model.IsChecked = false;
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
            Application.Current.Dispatcher.InvokeAsync(() =>
            {
                for (int i = 0; i < CopilotItemViewModels.Count; i++)
                {
                    CopilotItemViewModels[i].Index = i;
                }

                SaveCopilotTask();
            });
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

        private bool _caught;

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

            if (_isVideoTask)
            {
                _ = StartVideoTask();
                return;
            }

            AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));
            if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
            {
                AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
            }

            string errMsg = string.Empty;
            _caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!_caught)
            {
                AddLog(errMsg, UiLogColor.Error);
                return;
            }

            if (errMsg.Length != 0)
            {
                AddLog(errMsg, UiLogColor.Error);
            }

            JArray mUserAdditional = new JArray();
            Regex regex = new Regex(@"(?<=;)(?<name>[^,;]+)(?:, *(?<skill>\d))? *", RegexOptions.Compiled);
            MatchCollection matches = regex.Matches(";" + UserAdditional.Replace("，", ",").Replace("；", ";"));
            foreach (Match match in matches)
            {
                mUserAdditional.Add(new JObject
                {
                    ["name"] = match.Groups[1].Value.Trim(),
                    ["skill"] = match.Groups[2].Value.IsNullOrEmpty() ? 0 : int.Parse(match.Groups[2].Value),
                });
            }

            bool ret = true;
            if (UseCopilotList)
            {
                bool startAny = false;
                foreach (var model in CopilotItemViewModels)
                {
                    if (!model.IsChecked)
                    {
                        continue;
                    }

                    ret &= Instances.AsstProxy.AsstStartCopilot(model.FilePath, Form, AddTrust, AddUserAdditional, mUserAdditional, UseCopilotList, model.Name.Replace("-Adverse", string.Empty), model.Name.Contains("-Adverse"), _taskType, Loop ? LoopTimes : 1, false);
                    startAny = true;
                }

                ret &= Instances.AsstProxy.AsstStart();
                if (!startAny)
                {
                    // 一个都没启动，怎会有如此无聊之人
                    if (!Instances.AsstProxy.AsstStop())
                    {
                        _logger.Warning("Failed to stop Asst");
                    }

                    _runningState.SetIdle(true);
                    return;
                }
            }
            else
            {
                ret &= Instances.AsstProxy.AsstStartCopilot(IsDataFromWeb ? TempCopilotFile : Filename, Form, AddTrust, AddUserAdditional, mUserAdditional, UseCopilotList, string.Empty, false, _taskType, Loop ? LoopTimes : 1);
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
                AddLog(LocalizationHelper.GetString("CopilotFileReadError"), UiLogColor.Error);
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
            if (!Instances.AsstProxy.AsstStop())
            {
                _logger.Warning("Failed to stop Asst");
            }

            _runningState.SetIdle(true);
        }

        private bool _isVideoTask;

        private readonly List<int> _recentlyRatedCopilotId = new List<int>(); // TODO: 可能考虑加个持久化

        private bool _couldLikeWebJson;

        public bool CouldLikeWebJson
        {
            get => _couldLikeWebJson;
            set => SetAndNotify(ref _couldLikeWebJson, value);
        }

        private void UpdateCouldLikeWebJson()
        {
            CouldLikeWebJson = IsDataFromWeb &&
                CopilotId > 0 &&
                _recentlyRatedCopilotId.IndexOf(CopilotId) == -1;
        }

        private bool _isDataFromWeb;

        private bool IsDataFromWeb
        {
            get => _isDataFromWeb;
            set
            {
                SetAndNotify(ref _isDataFromWeb, value);
                UpdateCouldLikeWebJson();
            }
        }

        private int _copilotId;

        private int CopilotId
        {
            get => _copilotId;
            set
            {
                SetAndNotify(ref _copilotId, value);
                UpdateCouldLikeWebJson();
            }
        }

        // ReSharper disable once UnusedMember.Global
        public void LikeWebJson()
        {
            RateWebJson("Like");
        }

        // ReSharper disable once UnusedMember.Global
        public void DislikeWebJson()
        {
            RateWebJson("Dislike");
        }

        private async void RateWebJson(string rating)
        {
            if (!CouldLikeWebJson)
            {
                return;
            }

            CouldLikeWebJson = false;

            // PostAsJsonAsync 会自动序列化为 json
            /*
            string jsonParam = JsonConvert.SerializeObject(new
            {
                id = CopilotId,
                rating,
            });
            */
            var response = await Instances.HttpService.PostAsJsonAsync(new Uri(MaaUrls.PrtsPlusCopilotRating), new { id = CopilotId, rating });
            if (response == null)
            {
                AddLog(LocalizationHelper.GetString("FailedToLikeWebJson"), UiLogColor.Error);
                return;
            }

            _recentlyRatedCopilotId.Add(CopilotId);
            AddLog(LocalizationHelper.GetString("ThanksForLikeWebJson"), UiLogColor.Info);
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

        /// <summary>
        /// 点击后移除界面中元素焦点
        /// </summary>
        /// <param name="sender">点击事件发送者</param>
        /// <param name="e">点击事件</param>
        // ReSharper disable once UnusedMember.Global
        // ReSharper disable once UnusedParameter.Global
        public void MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (!(sender is UIElement element))
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

            if (!(sender is UIElement element))
            {
                return;
            }

            DependencyObject scope = FocusManager.GetFocusScope(element);
            FocusManager.SetFocusedElement(scope, element);
            Keyboard.ClearFocus();
        }

        private void EasterEgg(string text)
        {
            switch (text)
            {
                case "/help":
                    AddLog(LocalizationHelper.GetString("HelloWorld"), UiLogColor.Message);
                    break;
            }
        }
    }
}
