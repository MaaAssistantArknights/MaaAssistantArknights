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
            AddLog("小提示：请手动打开游戏有“开始行动”按钮的界面再使用本功能；\n\n如果想借好友助战可以关闭“自动编队”，手动选择好干员后再开始；\n\n模拟悖论则需要关闭“自动编队”，并自己选好技能处于“开始模拟”按钮的界面再开始\n\n“特别关注”的干员暂时无法被识别，请取消特别关注或手动编队", "dark");
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
        private string _fromServer ="";

        public string FromServer
        {
            get => _fromServer;
            set
            {
               _fromServer = value;
            }
        }

        private void _updateFileDoc(string filename)
        {
            ClearLog();
            string jsonStr = "";
            try
            {
                if (File.Exists(filename))
                {
                    jsonStr = File.ReadAllText(filename);
                    FromServer = "";
                }
                else
                {
                    //测试是否键入的为神秘代码
                    int copilotID = 0;
                    if (string.IsNullOrEmpty(filename))
                    {
                        // 重置数据
                        FromServer = "";
                        return;
                    }
                    Int32.TryParse(filename, out copilotID);
                    if (copilotID == 0) throw new Exception("神秘代码非法");
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
                            jsonStr = responseObject["data"]["content"].ToString();
                            FromServer = jsonStr;
                        }
                        else throw new Exception("未找到对应作业");
                    }
                }
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

        public void SelectFile()
        {
            if (String.IsNullOrEmpty(FromServer))
            {
                var dialog = new Microsoft.Win32.OpenFileDialog();

                dialog.Filter = "作业文件|*.json";

                if (dialog.ShowDialog() == true)
                {
                    Filename = dialog.FileName;
                }
            }
            // 什么都不做如果从server拿
        }

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
                Filename = "";
                ClearLog();
                AddLog("此文件非json文件", "darkred");
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

            if ((Filename.Length == 0 || !File.Exists(Filename)) && String.IsNullOrEmpty(FromServer))
            {
                AddLog("作业文件不存在", "darkred");
                return;
            }

            JObject data;

            try
            {
                string jsonStr = "";
                if (string.IsNullOrEmpty(FromServer))
                {
                    jsonStr = File.ReadAllText(Filename);
                }
                else
                {
                    jsonStr = FromServer;
                }

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
            get => _url.Length > 0 ? "视频链接" : "";
            set => SetAndNotify(ref _url, value);
        }

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
