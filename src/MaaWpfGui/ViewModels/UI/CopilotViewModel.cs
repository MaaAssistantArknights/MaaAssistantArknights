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
    public class CopilotViewModel : Screen
    {
        private readonly RunningState runningState;

        /// <summary>
        /// Gets or sets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="CopilotViewModel"/> class.
        /// </summary>
        public CopilotViewModel()
        {
            DisplayName = LocalizationHelper.GetString("Copilot");
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            AddLog(LocalizationHelper.GetString("CopilotTip"));
            runningState = RunningState.Instance;
            runningState.IdleChanged += RunningState_IdleChanged;
        }

        private void RunningState_IdleChanged(object sender, bool e)
        {
            Idle = e;
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
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
        public void ClearLog()
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
                int tempCopilotId = numberStyles;
                jsonStr = await RequestCopilotServer(tempCopilotId);
                if (!string.IsNullOrEmpty(jsonStr))
                {
                    IsDataFromWeb = true;
                    CopilotId = tempCopilotId;
                }
            }
            else if (int.TryParse(filename, out _))
            {
                int.TryParse(filename, out var numberStyles);
                int tempCopilotId = numberStyles;
                jsonStr = await RequestCopilotServer(tempCopilotId);
                if (!string.IsNullOrEmpty(jsonStr))
                {
                    IsDataFromWeb = true;
                    CopilotId = tempCopilotId;
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

        private async Task<string> RequestCopilotServer(int copilotID)
        {
            try
            {
                var jsonResponse = await Instances.HttpService.GetStringAsync(new Uri($@"https://prts.maa.plus/copilot/get/{copilotID}"));
                var json = (JObject)JsonConvert.DeserializeObject(jsonResponse);
                if (json != null && json.ContainsKey("status_code") && json["status_code"].ToString() == "200")
                {
                    return json["data"]["content"].ToString();
                }
                else
                {
                    AddLog(LocalizationHelper.GetString("CopilotNoFound"), UiLogColor.Error);
                    return string.Empty;
                }
            }
            catch (Exception)
            {
                AddLog(LocalizationHelper.GetString("NetworkServiceError"), UiLogColor.Error);
                return string.Empty;
            }
        }

        private const string TempCopilotFile = "cache/_temp_copilot.json";
        private string TaskType = "General";

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
                if (doc != null && doc.ContainsKey("title"))
                {
                    title = doc["title"].ToString();
                }

                if (title.Length != 0)
                {
                    string title_color = UiLogColor.Message;
                    if (doc.ContainsKey("title_color"))
                    {
                        title_color = doc["title_color"].ToString();
                    }

                    AddLog(title, title_color);
                }

                string details = string.Empty;
                if (doc != null && doc.ContainsKey("details"))
                {
                    details = doc["details"].ToString();
                }

                if (details.Length != 0)
                {
                    string details_color = UiLogColor.Message;
                    if (doc.ContainsKey("details_color"))
                    {
                        details_color = doc["details_color"].ToString();
                    }

                    AddLog(details, details_color);
                    {
                        Url = CopilotUiUrl;
                        var linkParser = new Regex(@"(BV.*?).{10}", RegexOptions.Compiled | RegexOptions.IgnoreCase);
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
                foreach (var oper in json["opers"].Cast<JObject>())
                {
                    count++;
                    AddLog(string.Format("{0}, {1} 技能", oper["name"], oper["skill"]), UiLogColor.Message);
                }

                if (json.ContainsKey("groups"))
                {
                    foreach (var group in json["groups"].Cast<JObject>())
                    {
                        count++;
                        string group_name = group["name"] + ": ";
                        var operInfos = new List<string>();
                        foreach (var oper in group["opers"].Cast<JObject>())
                        {
                            operInfos.Add(string.Format("{0} {1}", oper["name"], oper["skill"]));
                        }

                        AddLog(group_name + string.Join(" / ", operInfos), UiLogColor.Message);
                    }
                }

                AddLog(string.Format("共 {0} 名干员", count), UiLogColor.Message);

                if (json.ContainsKey("type"))
                {
                    var type = json["type"].ToString();
                    if (type == "SSS")
                    {
                        TaskType = "SSSCopilot";

                        if (json.ContainsKey("tool_men"))
                        {
                            AddLog("编队工具人：\n" + json["tool_men"].ToString(), UiLogColor.Message);
                        }

                        if (json.ContainsKey("equipment"))
                        {
                            AddLog("开局装备（横向）：\n" + json["equipment"].ToString(), UiLogColor.Message);
                        }

                        if (json.ContainsKey("strategy"))
                        {
                            AddLog("开局策略：" + json["strategy"].ToString(), UiLogColor.Message);
                        }
                    }
                }
                else
                {
                    TaskType = "Copilot";
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

        private static readonly string[] SupportExt = { ".json", ".mp4", ".m4s", ".mkv", ".flv", ".avi" };

        /// <summary>
        /// Drops file.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments.</param>
        public void DropFile(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                var filename = ((Array)e.Data.GetData(DataFormats.FileDrop))?.GetValue(0).ToString();
                DropFile(filename);
            }
        }

        private void DropFile(string filename)
        {
            if (string.IsNullOrEmpty(filename))
            {
                return;
            }

            var filenameLower = filename.ToLower();
            bool support = false;
            foreach (var ext in SupportExt)
            {
                if (filenameLower.EndsWith(ext))
                {
                    support = true;
                    break;
                }
            }

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
        public void OnDropDownOpened(object sender, EventArgs e)
        {
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox != null)
            {
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
        }

        private bool _form = false;

        /// <summary>
        /// Gets or sets a value indicating whether to use auto-formation.
        /// </summary>
        public bool Form
        {
            get => _form;
            set => SetAndNotify(ref _form, value);
        }

        public bool Loop { get; set; } = false;

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

        private bool _caught = false;

        /// <summary>
        /// Starts copilot.
        /// </summary>
        public async void Start()
        {
            /*
            if (_form)
            {
                AddLog(Localization.GetString("AutoSquadTip"), LogColor.Message);
            }*/
            runningState.SetIdle(false);

            if (_isVideoTask)
            {
                StartVideoTask();
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

            bool ret = Instances.AsstProxy.AsstStartCopilot(IsDataFromWeb ? TempCopilotFile : Filename, Form, TaskType,
                Loop ? LoopTimes : 1);
            if (ret)
            {
                AddLog(LocalizationHelper.GetString("Running"));
            }
            else
            {
                runningState.SetIdle(true);
                AddLog(LocalizationHelper.GetString("CopilotFileReadError"), UiLogColor.Error);
            }
        }

        public bool StartVideoTask()
        {
            return Instances.AsstProxy.AsstStartVideoRec(Filename);
        }

        /// <summary>
        /// Stops copilot.
        /// </summary>
        public void Stop()
        {
            Instances.AsstProxy.AsstStop();
            runningState.SetIdle(true);
        }

        private bool _isVideoTask = false;

        private readonly string _copilotRatingUrl = "https://prts.maa.plus/copilot/rating";
        private readonly List<int> _recentlyRatedCopilotId = new List<int>(); // TODO: 可能考虑加个持久化

        private bool _couldLikeWebJson = false;

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

        private bool _isDataFromWeb = false;

        public bool IsDataFromWeb
        {
            get => _isDataFromWeb;
            set
            {
                SetAndNotify(ref _isDataFromWeb, value);
                UpdateCouldLikeWebJson();
            }
        }

        private int _copilotId = 0;

        public int CopilotId
        {
            get => _copilotId;
            set
            {
                SetAndNotify(ref _copilotId, value);
                UpdateCouldLikeWebJson();
            }
        }

        public void LikeWebJson()
        {
            RateWebJson("Like");
        }

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
            var response = await Instances.HttpService.PostAsJsonAsync(new Uri(_copilotRatingUrl), new { id = CopilotId, rating });
            if (response == null)
            {
                AddLog(LocalizationHelper.GetString("FailedToLikeWebJson"), UiLogColor.Error);
                return;
            }

            _recentlyRatedCopilotId.Add(CopilotId);
            AddLog(LocalizationHelper.GetString("ThanksForLikeWebJson"), UiLogColor.Info);
        }

        private readonly string CopilotUiUrl = MaaUrls.PrtsPlus;
        private string _url = MaaUrls.PrtsPlus;
        private string _urlText = LocalizationHelper.GetString("PrtsPlus");

        /// <summary>
        /// Gets or sets the UrlText.
        /// </summary>
        public string UrlText
        {
            get => _urlText;
            set => SetAndNotify(ref _urlText, value);
        }

        /// <summary>
        /// Gets or sets the copilot URL.
        /// </summary>
        public string Url
        {
            get => _url;
            set
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
