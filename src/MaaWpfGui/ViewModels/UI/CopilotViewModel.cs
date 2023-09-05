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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using DataFormats = System.Windows.Forms.DataFormats;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of copilot.
    /// </summary>
    // 通过 container.Get<CopilotViewModel>(); 实例化或获取实例，需要添加 qodana ignore rule
    // ReSharper disable once ClassNeverInstantiated.Global
    public class CopilotViewModel : Screen
    {
        private readonly RunningState _runningState;

        /// <summary>
        /// Gets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; }

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

        private bool _idle = true;

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
            Url = CopilotUiUrl;
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
                var jsonResponse = await Instances.HttpService.GetStringAsync(new Uri($"https://prts.maa.plus/copilot/get/{copilotId}"));
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

                var doc = (JObject)json["doc"];

                string title = string.Empty;
                if (doc != null && doc.TryGetValue("title", out var titleValue))
                {
                    title = titleValue.ToString();
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
                        Url = CopilotUiUrl;
                        var linkParser = new Regex("(BV.*?).{10}", RegexOptions.Compiled | RegexOptions.IgnoreCase);
                        foreach (Match match in linkParser.Matches(details))
                        {
                            Url = "https://www.bilibili.com/video/" + match.Value;
                            break;
                        }

                        if (string.IsNullOrEmpty(Url))
                        {
                            linkParser = new Regex(@"(?:https?://)\S+\b", RegexOptions.Compiled | RegexOptions.IgnoreCase);

                            foreach (Match m in linkParser.Matches(details))
                            {
                                Url = m.Value;
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

                if (IsDataFromWeb)
                {
                    File.Delete(TempCopilotFile);
                    File.WriteAllText(TempCopilotFile, json.ToString());
                }

                AddLog(LocalizationHelper.GetString("CopilotTip"));
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
        // xaml 中绑定了 action
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

            bool ret = Instances.AsstProxy.AsstStartCopilot(IsDataFromWeb ? TempCopilotFile : Filename, Form, _taskType,
                Loop ? LoopTimes : 1);
            if (ret)
            {
                AddLog(LocalizationHelper.GetString("Running"));
            }
            else
            {
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
        // xaml 中绑定了 action
        // ReSharper disable once UnusedMember.Global
        public void Stop()
        {
            Instances.AsstProxy.AsstStop();
            _runningState.SetIdle(true);
        }

        private bool _isVideoTask;

        private const string CopilotRatingUrl = "https://prts.maa.plus/copilot/rating";
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
            var response = await Instances.HttpService.PostAsJsonAsync(new Uri(CopilotRatingUrl), new { id = CopilotId, rating });
            if (response == null)
            {
                AddLog(LocalizationHelper.GetString("FailedToLikeWebJson"), UiLogColor.Error);
                return;
            }

            _recentlyRatedCopilotId.Add(CopilotId);
            AddLog(LocalizationHelper.GetString("ThanksForLikeWebJson"), UiLogColor.Info);
        }

        private const string CopilotUiUrl = MaaUrls.PrtsPlus;
        private string _url = MaaUrls.PrtsPlus;
        private string _urlText = LocalizationHelper.GetString("PrtsPlus");

        /// <summary>
        /// Gets or private sets the UrlText.
        /// </summary>
        public string UrlText
        {
            get => _urlText;
            private set => SetAndNotify(ref _urlText, value);
        }

        /// <summary>
        /// Gets or private sets the copilot URL.
        /// </summary>
        public string Url
        {
            get => _url;
            private set
            {
                UrlText = value == CopilotUiUrl ? LocalizationHelper.GetString("PrtsPlus") : LocalizationHelper.GetString("VideoLink");
                SetAndNotify(ref _url, value);
            }
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
