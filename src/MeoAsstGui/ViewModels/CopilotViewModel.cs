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
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
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

        private string _filename;

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
                    }
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

        private bool _form = true;

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
            asstProxy.AsstStartCopilot(data["stage_name"].ToString(), _newfilename, Form);
            Idle = false;
            AddLog("Star Burst Stream!");
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            Idle = true;
        }
    }
}
