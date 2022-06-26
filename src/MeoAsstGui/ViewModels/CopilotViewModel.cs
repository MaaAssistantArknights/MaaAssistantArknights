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

        private readonly string CopilotIdPrefix = "maa://";

        private void _updateFileDoc(string filename)
        {
            ClearLog();

            string jsonStr = "";
            if (File.Exists(filename))
            {
                try
                {
                    jsonStr = File.ReadAllText(filename);
                }
                catch (Exception)
                {
                    AddLog("读取文件失败！", "DarkRed");
                    return;
                }
            }
            else if (Int32.TryParse(filename, out _))
            {
                int copilotID = 0;
                Int32.TryParse(filename, out copilotID);
                jsonStr = _requestCopilotServer(copilotID);
            }
            else if (filename.ToLower().StartsWith(CopilotIdPrefix))
            {
                var copilotIdStr = filename.ToLower().Remove(0, CopilotIdPrefix.Length);
                int copilotID = 0;
                Int32.TryParse(copilotIdStr, out copilotID);
                jsonStr = _requestCopilotServer(copilotID);
            }

            if (jsonStr != String.Empty)
            {
                _parseJsonAndShowInfo(jsonStr);
            }
        }

        private string _requestCopilotServer(int copilotID)
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
                        AddLog("未找到对应作业！", "DarkRed");
                        return String.Empty;
                    }
                }
            }
            catch (Exception)
            {
                AddLog("请求网络服务错误！", "DarkRed");
                return String.Empty;
            }
        }

        private string _curStageName = "";
        private string _curCopilotData = "";
        private const string _tempCopilotFile = "resource/_temp_copilot.json";

        private void _parseJsonAndShowInfo(string jsonStr)
        {
            try
            {
                _curCopilotData = "";
                var json = (JObject)JsonConvert.DeserializeObject(jsonStr);
                if (json == null || !json.ContainsKey("doc"))
                {
                    AddLog("解析作业文件错误！", "DarkRed");
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
                    AddLog(string.Format("{0}, {1} 技能", oper["name"], oper["skill"]), "black");
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
                            operinfos.Add(string.Format("{0} {1}", oper["name"], oper["skill"]));
                        }
                        AddLog(group_name + string.Join(" / ", operinfos), "black");
                    }
                }
                AddLog(string.Format("共 {0} 名干员", count), "black");

                _curStageName = json["stage_name"].ToString();
                _curCopilotData = json.ToString();
            }
            catch (Exception)
            {
                AddLog("解析作业文件错误！", "DarkRed");
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
                AddLog("非作业文件", "darkred");
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
                AddLog("读取 JSON 作业文件出错\n请检查文件内容", "darkred");
            }
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            Idle = true;
        }

        private static readonly string CopilotUiUrl = "https://www.prts.plus/";
        private string _url = CopilotUiUrl;

        public string Url
        {
            get => _url == CopilotUiUrl ? "作业分享站" : "视频链接";
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
