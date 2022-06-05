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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Notification.Wpf.Constants;
using Notification.Wpf.Controls;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class CopilotViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        public CopilotViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "自动战斗 Beta";
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
        }

        public void AddLog(string content, string color = "Gray", string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));
            //LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        public void AddLogWithUrl(string content, string url, string color = "Gray", string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));
            //LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        private bool _idel = true;

        public bool Idle
        {
            get => _idel;
            set
            {
                _idel = value;
                NotifyOfPropertyChange(() => Idle);
            }
        }

        public void ClearLog()
        {
            LogItemViewModels.Clear();
        }

        private string _filename = "";

        public string Filename
        {
            get => _filename;
            set
            {
                SetAndNotify(ref _filename, value);
                _updateFileDoc(_filename);
            }
        }

        private void _updateFileDoc(string filename)
        {
            ClearLog();

            if (File.Exists(filename))
            {
                try
                {
                    string jsonStr = File.ReadAllText(filename);

                    var json = (JObject)JsonConvert.DeserializeObject(jsonStr);
                    if (json == null || !json.ContainsKey("doc"))
                    {
                        return;
                    }
                    var doc = (JObject)json["doc"];

                    string title = "";
                    if (doc.ContainsKey("title"))
                    {
                        title = doc["title"].ToString();
                    }
                    if (title.Length != 0)
                    {
                        string title_color = "black";
                        if (doc.ContainsKey("title_color"))
                        {
                            title_color = doc["title_color"].ToString();
                        }
                        AddLog(title, title_color);
                    }

                    string details = "";
                    if (doc.ContainsKey("details"))
                    {
                        details = doc["details"].ToString();
                    }
                    if (details.Length != 0)
                    {
                        string details_color = "black";
                        if (doc.ContainsKey("details_color"))
                        {
                            details_color = doc["details_color"].ToString();
                        }
                        AddLog(details, details_color);

                        {
                            Url = "";
                            var linkParser = new Regex(@"(?:https?://)\S+\b", RegexOptions.Compiled | RegexOptions.IgnoreCase);

                            foreach (Match m in linkParser.Matches(details))
                            {
                                Url = m.Value;
                                break;
                            }
                        }
                    }

                    AddLog("", "black");
                    int count = 0;
                    foreach (JObject oper in json["opers"])
                    {
                        count++;
                        AddLog(string.Format("{0}, {1}技能", oper["name"], oper["skill"]), "black");
                    }

                    if (json.ContainsKey("groups"))
                    {
                        foreach (JObject group in json["groups"])
                        {
                            count++;
                            string group_name = group["name"].ToString() + ": ";
                            var operinfos = new List<string>();
                            foreach (JObject oper in group["opers"])
                            {
                                operinfos.Add(string.Format("{0}{1}", oper["name"], oper["skill"]));
                            }
                            AddLog(group_name + string.Join("/", operinfos), "black");
                        }
                    }
                    AddLog(string.Format("共{0}名干员", count), "black");
                }
                catch (Exception)
                {
                }
            }
        }

        public void SelectFile()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();

            dialog.Filter = "作业文件|*.json";

            if (dialog.ShowDialog() == true)
            {
                Filename = dialog.FileName;
            }
        }

        private bool _form = false;

        public bool Form
        {
            get => _form;
            set => SetAndNotify(ref _form, value);
        }

        private bool _catched = false;

        public async void Start()
        {
            ClearLog();
            AddLog("正在连接模拟器……");

            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = "";
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg);
            });
            _catched = await task;
            if (!_catched)
            {
                AddLog(errMsg, "darkred");
                return;
            }
            if (errMsg.Length != 0)
            {
                AddLog(errMsg, "darkred");
            }

            if (Filename.Length == 0 || !File.Exists(Filename))
            {
                AddLog("作业文件不存在", "darkred");
                return;
            }

            JObject data;

            try
            {
                string jsonStr = File.ReadAllText(Filename);

                // 文件存在但为空，会读出来一个null，感觉c#这库有bug，如果是null 就赋值一个空JObject
                data = (JObject)JsonConvert.DeserializeObject(jsonStr) ?? new JObject();
            }
            catch (Exception)
            {
                AddLog("作业文件读取出错", "darkred");
                return;
            }

            const string _newfilename = "resource/_temp_copilot.json";
            File.WriteAllText(_newfilename, data.ToString());
            bool ret = asstProxy.AsstStartCopilot(data["stage_name"].ToString(), _newfilename, Form);
            if (ret)
            {
                Idle = false;
                AddLog("Star Burst Stream!");
            }
            else
            {
                AddLog("读取 JSON 作业文件出错\n请检查文件内容", "darkred");
            }
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            Idle = true;
        }

        private string _url = "";

        public string Url
        {
            get => _url;
            set => SetAndNotify(ref _url, value);
        }

        public void Hyperlink_Click(string url)
        {
            try
            {
                if (!string.IsNullOrEmpty(url))
                {
                    Process.Start(new ProcessStartInfo(url));
                }
            }
            catch (Exception)
            {
            }
        }
    }
}
