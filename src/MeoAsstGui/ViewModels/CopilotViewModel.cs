// <copyright file="CopilotViewModel.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;
using DragEventArgs = System.Windows.DragEventArgs;
using Screen = Stylet.Screen;

namespace MeoAsstGui
{
    /// <summary>
    /// The view model of copilot.
    /// </summary>
    public class CopilotViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        /// <summary>
        /// Gets or sets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="CopilotViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        /// <param name="windowManager">The window manager.</param>
        public CopilotViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = Localization.GetString("Copilot");
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            AddLog(
                Localization.GetString("CopilotTip") + "\n\n" +
                Localization.GetString("CopilotTip1") + "\n\n" +
                Localization.GetString("CopilotTip2") + "\n\n" +
                Localization.GetString("CopilotTip3") + "\n\n" +
                Localization.GetString("CopilotTip4") + "\n\n" +
                Localization.GetString("CopilotTip5"),
                LogColor.Message);
        }

        /// <summary>
        /// Adds log.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        public void AddLog(string content, string color = LogColor.Trace, string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));

            // LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        /// <summary>
        /// Adds log with URL.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="url">The URL.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        public void AddLogWithUrl(string content, string url, string color = LogColor.Trace, string weight = "Regular")
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
            set
            {
                _idle = value;
                NotifyOfPropertyChange(() => Idle);
            }
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
                UpdateFileDoc(_filename);
            }
        }

        private readonly string CopilotIdPrefix = "maa://";

        private void UpdateFileDoc(string filename)
        {
            string jsonStr = string.Empty;
            if (File.Exists(filename))
            {
                try
                {
                    jsonStr = File.ReadAllText(filename);
                }
                catch (Exception)
                {
                    AddLog(Localization.GetString("CopilotFileReadError"), LogColor.Error);
                    return;
                }
            }
            else if (int.TryParse(filename, out _))
            {
                int copilotID;
                int.TryParse(filename, out copilotID);
                jsonStr = RequestCopilotServer(copilotID);
            }
            else if (filename.ToLower().StartsWith(CopilotIdPrefix))
            {
                var copilotIdStr = filename.ToLower().Remove(0, CopilotIdPrefix.Length);
                int copilotID;
                int.TryParse(copilotIdStr, out copilotID);
                jsonStr = RequestCopilotServer(copilotID);
            }

            if (jsonStr != string.Empty)
            {
                ParseJsonAndShowInfo(jsonStr);
            }
        }

        private string RequestCopilotServer(int copilotID)
        {
            try
            {
                // 创建 Http 请求
                var httpWebRequest = WebRequest.Create($@"https://api.prts.plus/copilot/get/{copilotID}") as HttpWebRequest;
                httpWebRequest.Method = "GET";
                httpWebRequest.ContentType = "application/json";
                var httpWebResponse = httpWebRequest.GetResponse() as HttpWebResponse;

                // 获取输入输出流
                using (var sr = new StreamReader(httpWebResponse.GetResponseStream()))
                {
                    var text = sr.ReadToEnd();
                    var responseObject = (JObject)JsonConvert.DeserializeObject(text);
                    if (responseObject != null && responseObject.ContainsKey("status_code") && responseObject["status_code"].ToString() == "200")
                    {
                        return responseObject["data"]["content"].ToString();
                    }
                    else
                    {
                        AddLog(Localization.GetString("CopilotNoFound"), LogColor.Error);
                        return string.Empty;
                    }
                }
            }
            catch (Exception)
            {
                AddLog(Localization.GetString("NetworkServiceError"), LogColor.Error);
                return string.Empty;
            }
        }

        private string _curStageName = string.Empty;
        private string _curCopilotData = string.Empty;
        private const string _tempCopilotFile = "resource/_temp_copilot.json";

        private void ParseJsonAndShowInfo(string jsonStr)
        {
            try
            {
                _curCopilotData = string.Empty;
                var json = (JObject)JsonConvert.DeserializeObject(jsonStr);
                if (json == null || !json.ContainsKey("doc"))
                {
                    AddLog(Localization.GetString("CopilotJsonError"), LogColor.Error);
                    return;
                }

                var doc = (JObject)json["doc"];

                string title = string.Empty;
                if (doc.ContainsKey("title"))
                {
                    title = doc["title"].ToString();
                }

                if (title.Length != 0)
                {
                    string title_color = LogColor.Message;
                    if (doc.ContainsKey("title_color"))
                    {
                        title_color = doc["title_color"].ToString();
                    }

                    AddLog(title, title_color);
                }

                string details = string.Empty;
                if (doc.ContainsKey("details"))
                {
                    details = doc["details"].ToString();
                }

                if (details.Length != 0)
                {
                    string details_color = LogColor.Message;
                    if (doc.ContainsKey("details_color"))
                    {
                        details_color = doc["details_color"].ToString();
                    }

                    AddLog(details, details_color);
                    {
                        Url = string.Empty;
                        var linkParser = new Regex(@"(?:https?://)\S+\b", RegexOptions.Compiled | RegexOptions.IgnoreCase);

                        foreach (Match m in linkParser.Matches(details))
                        {
                            Url = m.Value;
                            break;
                        }
                    }
                }

                AddLog(string.Empty, LogColor.Message);
                int count = 0;
                foreach (JObject oper in json["opers"])
                {
                    count++;
                    AddLog(string.Format("{0}, {1} 技能", oper["name"], oper["skill"]), LogColor.Message);
                }

                if (json.ContainsKey("groups"))
                {
                    foreach (JObject group in json["groups"])
                    {
                        count++;
                        string group_name = group["name"] + ": ";
                        var operInfos = new List<string>();
                        foreach (JObject oper in group["opers"])
                        {
                            operInfos.Add(string.Format("{0} {1}", oper["name"], oper["skill"]));
                        }

                        AddLog(group_name + string.Join(" / ", operInfos), LogColor.Message);
                    }
                }

                AddLog(string.Format("共 {0} 名干员", count), LogColor.Message);

                _curStageName = json["stage_name"].ToString();
                _curCopilotData = json.ToString();
                AddLog(
                    "\n\n" +
                    Localization.GetString("CopilotTip") + "\n\n" +
                    Localization.GetString("CopilotTip1") + "\n\n" +
                    Localization.GetString("CopilotTip2") + "\n\n" +
                    Localization.GetString("CopilotTip3") + "\n\n" +
                    Localization.GetString("CopilotTip4") + "\n\n" +
                    Localization.GetString("CopilotTip5"));
            }
            catch (Exception)
            {
                AddLog(Localization.GetString("CopilotJsonError"), LogColor.Error);
            }
        }

        /// <summary>
        /// Selects file.
        /// </summary>
        public void SelectFile()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();

            dialog.Filter = "JSON|*.json";

            if (dialog.ShowDialog() == true)
            {
                Filename = dialog.FileName;
            }
        }

        /// <summary>
        /// Drops file.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments.</param>
        public void DropFile(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                return;
            }

            var filename = ((Array)e.Data.GetData(DataFormats.FileDrop))?.GetValue(0).ToString();
            if (filename == null)
            {
                return;
            }

            if (filename.EndsWith(".json"))
            {
                Filename = filename;
            }
            else
            {
                Filename = string.Empty;
                ClearLog();
                AddLog(Localization.GetString("NotCopilotJson"), LogColor.Error);
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

        private bool _caught = false;

        /// <summary>
        /// Starts copilot.
        /// </summary>
        public async void Start()
        {
            ClearLog();
            if (_form)
            {
                AddLog(Localization.GetString("AutoSquadTip"), LogColor.Message);
            }

            AddLog(Localization.GetString("ConnectingToEmulator"));

            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = string.Empty;
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg);
            });
            _caught = await task;
            if (!_caught)
            {
                AddLog(errMsg, LogColor.Error);
                return;
            }

            if (errMsg.Length != 0)
            {
                AddLog(errMsg, LogColor.Error);
            }

            UpdateFileDoc(Filename);
            File.Delete(_tempCopilotFile);
            File.WriteAllText(_tempCopilotFile, _curCopilotData);

            bool ret = asstProxy.AsstStartCopilot(_curStageName, _tempCopilotFile, Form);
            if (ret)
            {
                Idle = false;
                AddLog("Star Burst Stream!");
            }
            else
            {
                AddLog(Localization.GetString("CopilotFileReadError") + "\n" + Localization.GetString("CheckTheFile"), LogColor.Error);
            }
        }

        /// <summary>
        /// Stops copilot.
        /// </summary>
        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            Idle = true;
        }

        private static readonly string CopilotUiUrl = "https://www.prts.plus/";
        private string _url = CopilotUiUrl;

        /// <summary>
        /// Gets or sets the copilot URL.
        /// </summary>
        public string Url
        {
            get => _url == CopilotUiUrl ? Localization.GetString("CopilotJSONSharing") : Localization.GetString("VideoLink");
            set => SetAndNotify(ref _url, value);
        }

        /// <summary>
        /// The event handler of clicking hyperlink.
        /// </summary>
        public void Hyperlink_Click()
        {
            try
            {
                if (!string.IsNullOrEmpty(_url))
                {
                    Process.Start(new ProcessStartInfo(_url));
                }
            }
            catch (Exception)
            {
            }
        }
    }
}
