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
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using Microsoft.Toolkit.Uwp.Notifications;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class VersionUpdateViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public VersionUpdateViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
        }

        [DllImport("MeoAssistant.dll")] private static extern IntPtr AsstGetVersion();

        private string _curVersion = Marshal.PtrToStringAnsi(AsstGetVersion());
        private string _latestVersion;

        private string _updateTag = ViewStatusStorage.Get("VersionUpdate.name", string.Empty);

        public string UpdateTag
        {
            get
            {
                return _updateTag;
            }
            set
            {
                SetAndNotify(ref _updateTag, value);
                ViewStatusStorage.Set("VersionUpdate.name", value);
            }
        }

        private string _updateInfo = ViewStatusStorage.Get("VersionUpdate.body", string.Empty);

        public string UpdateInfo
        {
            get
            {
                return _updateInfo;
            }
            set
            {
                SetAndNotify(ref _updateInfo, value);
                ViewStatusStorage.Set("VersionUpdate.body", value);
            }
        }

        private bool _isFirstBootAfterUpdate = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.firstboot", Boolean.FalseString));

        public bool IsFirstBootAfterUpdate
        {
            get
            {
                return _isFirstBootAfterUpdate;
            }
            set
            {
                SetAndNotify(ref _isFirstBootAfterUpdate, value);
                ViewStatusStorage.Set("VersionUpdate.firstboot", value.ToString());
            }
        }

        private string _updatePackageName = ViewStatusStorage.Get("VersionUpdate.package", string.Empty);

        public string UpdatePackageName
        {
            get
            {
                return _updatePackageName;
            }
            set
            {
                SetAndNotify(ref _updatePackageName, value);
                ViewStatusStorage.Set("VersionUpdate.package", value.ToString());
            }
        }

        private const string _requestUrl = "https://api.github.com/repos/MistEO/MeoAssistantArknights/releases/latest";
        private const string _requestBetaUrl = "https://api.github.com/repos/MistEO/MeoAssistantArknights/releases";
        private string _viewUrl;
        private JObject _lastestJson;
        private string _downloadUrl;

        // 检查是否有已下载的更新包，如果有立即更新并重启进程
        public bool CheckAndUpdateNow()
        {
            if (UpdateTag == string.Empty
                || UpdatePackageName == string.Empty
                || !File.Exists(UpdatePackageName))
            {
                return false;
            }
            new ToastContentBuilder()
                .AddText("检测到新版本包，正在解压，请稍等……")
                .AddText(UpdateTag)
                .Show();
            string extractDir = Directory.GetCurrentDirectory() + "\\NewVersionExtract";
            // 解压
            System.IO.Compression.ZipFile.ExtractToDirectory(UpdatePackageName, extractDir);

            var uncopiedList = new List<string>();
            // 复制新版本的所有文件到当前路径下
            foreach (var file in Directory.GetFiles(extractDir))
            {
                try
                {
                    File.Copy(file, Path.Combine(Directory.GetCurrentDirectory(), Path.GetFileName(file)), true);
                }
                catch (Exception)
                {
                    uncopiedList.Add(file);
                }
            }
            foreach (var directory in Directory.GetDirectories(extractDir))
            {
                CopyFilesRecursively(directory, Path.Combine(Directory.GetCurrentDirectory(), Path.GetFileName(directory)));
            }

            // 程序正在运行中，部分文件是无法覆写的，这里曲线操作下
            // 先将当前这些文件重命名，然后再把新的复制过来
            foreach (var oldfile in Directory.GetFiles(Directory.GetCurrentDirectory(), "*.old"))
            {
                File.Delete(oldfile);
            }
            foreach (var file in uncopiedList)
            {
                string curFileName = Path.Combine(Directory.GetCurrentDirectory(), Path.GetFileName(file));
                File.Move(curFileName, curFileName + ".old");
                File.Copy(file, curFileName);
            }

            // 操作完了，把解压的文件删了
            Directory.Delete(extractDir, true);
            File.Delete(UpdatePackageName);
            // 保存更新信息，下次启动后会弹出已更新完成的提示
            IsFirstBootAfterUpdate = true;
            UpdatePackageName = string.Empty;
            ViewStatusStorage.Save();

            // 重启进程（启动的是更新后的程序了）
            System.Diagnostics.Process newProcess = new System.Diagnostics.Process();
            newProcess.StartInfo.FileName = System.AppDomain.CurrentDomain.FriendlyName;
            newProcess.StartInfo.WorkingDirectory = Directory.GetCurrentDirectory();
            newProcess.Start();
            Application.Current.Shutdown();

            return true;
        }

        // 检查更新，并下载更新包
        public bool CheckAndDownloadUpdate()
        {
            // 检查更新
            if (!CheckUpdate())
            {
                return false;
            }
            // 保存新版本的信息
            UpdatePackageName = _downloadUrl.Substring(_downloadUrl.LastIndexOf('/') + 1);
            UpdateTag = _lastestJson["name"].ToString();
            UpdateInfo = _lastestJson["body"].ToString();

            var openUrlToastButton = new ToastButton();
            openUrlToastButton.SetContent("前往页面查看");
            openUrlToastButton.SetProtocolActivation(new Uri(_viewUrl));
            new ToastContentBuilder()
                .AddText("检测到新版本，正在后台下载……")
                .AddText(UpdateTag)
                .AddText(UpdateInfo)
                .AddButton(openUrlToastButton)
                .Show();
            // 下载压缩包
            const int downloadRetryMaxTimes = 2;
            string downloadTempFilename = UpdatePackageName + ".tmp";
            bool downloaded = false;
            for (int i = 0; i != downloadRetryMaxTimes; ++i)
            {
                if (DownloadFile(_downloadUrl.Replace("github.com", "hub.fastgit.org"), downloadTempFilename)
                    || DownloadFile(_downloadUrl, downloadTempFilename))
                {
                    downloaded = true;
                    break;
                }
            }
            if (!downloaded)
            {
                new ToastContentBuilder()
                    .AddText("新版本下载失败")
                    .AddText("请尝试手动下载后，将压缩包放到目录下_(:з」∠)_")
                    .AddButton(openUrlToastButton)
                    .Show();
                return false;
            }
            File.Copy(downloadTempFilename, UpdatePackageName, true);
            File.Delete(downloadTempFilename);
            // 把相关信息存下来，更新完之后启动的时候显示
            new ToastContentBuilder()
                .AddText("新版本下载完成")
                .AddText("软件将在下次启动时自动更新！")
                .AddText("✿✿ヽ(°▽°)ノ✿")
                .Show();
            return true;
        }

        public bool CheckUpdate()
        {
            var settings = _container.Get<SettingsViewModel>();
            string response = string.Empty;
            const int requestRetryMaxTimes = 5;
            for (int i = 0; i != requestRetryMaxTimes; ++i)
            {
                if (settings.UpdateBeta)
                {
                    response = RequestApi(_requestBetaUrl);
                }
                else
                {
                    response = RequestApi(_requestUrl);
                }
                if (response.Length != 0)
                {
                    break;
                }
            }
            if (response.Length == 0)
            {
                return false;
            }

            try
            {
                JObject json;
                if (settings.UpdateBeta)
                {
                    JArray all = (JArray)JsonConvert.DeserializeObject(response);
                    json = (JObject)all[0];
                }
                else
                {
                    json = (JObject)JsonConvert.DeserializeObject(response);
                }

                _viewUrl = json["html_url"].ToString();
                _latestVersion = json["tag_name"].ToString();
                if (string.Compare(_latestVersion, _curVersion) <= 0
                    || ViewStatusStorage.Get("VersionUpdate.Ignore", string.Empty) == _latestVersion)
                {
                    return false;
                }
                foreach (JObject asset in json["assets"])
                {
                    string downUrl = asset["browser_download_url"].ToString();
                    if (downUrl.Length != 0)
                    {
                        _downloadUrl = downUrl;
                        _lastestJson = json;
                        return true;
                    }
                }
            }
            catch (Exception)
            {
                return false;
            }
            return false;
        }

        private string RequestApi(string url)
        {
            HttpWebRequest httpWebRequest = (HttpWebRequest)HttpWebRequest.Create(url);
            httpWebRequest.Method = "GET";
            httpWebRequest.UserAgent = "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36";
            httpWebRequest.Accept = "application/vnd.github.v3+json";
            var settings = _container.Get<SettingsViewModel>();
            if (settings.Proxy.Length > 0)
            {
                httpWebRequest.Proxy = new WebProxy(settings.Proxy);
            }
            //httpWebRequest.Timeout = 20000;
            try
            {
                HttpWebResponse httpWebResponse = (HttpWebResponse)httpWebRequest.GetResponse();
                StreamReader streamReader = new StreamReader(httpWebResponse.GetResponseStream(), Encoding.UTF8);
                string responseContent = streamReader.ReadToEnd();
                streamReader.Close();
                httpWebResponse.Close();
                return responseContent;
            }
            catch (Exception)
            {
                return string.Empty;
            }
        }

        private bool DownloadFile(string url, string filename)
        {
            string cmd = Environment.CurrentDirectory + "\\aria2c.exe";
            string args = "-c " + url + " -o " + filename;

            var settings = _container.Get<SettingsViewModel>();
            if (settings.Proxy.Length > 0)
            {
                args += " --all-proxy " + settings.Proxy;
            }

            int exit_code = -1;

            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.FileName = cmd;
            startInfo.Arguments = args;
            startInfo.RedirectStandardOutput = true;
            startInfo.RedirectStandardError = true;
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;

            Process processTemp = new Process();
            processTemp.StartInfo = startInfo;
            processTemp.EnableRaisingEvents = true;
            try
            {
                processTemp.Start();
                processTemp.WaitForExit();
                exit_code = processTemp.ExitCode;
            }
            catch (Exception ex)
            {
                return false;
            }

            return exit_code == 0;
        }

        public bool ResourceOTA()
        {
            const string req_base_url = "https://api.github.com/repos/MistEO/MeoAssistantArknights/commits?path=";
            const string down_base_url = "https://cdn.jsdelivr.net/gh/MistEO/MeoAssistantArknights@";
            var update_dict = new Dictionary<string, string>()
            {
                { "3rdparty/resource/penguin-stats-recognize/json/stages.json" , "resource/penguin-stats-recognize/json/stages.json"},
                { "resource/recruit.json", "resource/recruit.json" }
            };
            bool updated = false;
            string message = "";
            foreach (var item in update_dict)
            {
                string url = item.Key;
                string filename = item.Value;
                string cur_sha = ViewStatusStorage.Get(filename, string.Empty);

                string response = RequestApi(req_base_url + url);
                if (response == String.Empty)
                {
                    continue;
                }
                string cloud_sha;
                string cur_message = "";
                try
                {
                    JArray arr = (JArray)JsonConvert.DeserializeObject(response);
                    JObject commit_info = (JObject)arr[0];
                    cloud_sha = commit_info["sha"].ToString();
                    cur_message = commit_info["commit"]["message"].ToString();
                }
                catch (Exception)
                {
                    continue;
                }
                if (cur_sha == cloud_sha)
                {
                    continue;
                }

                string tempname = filename + ".tmp";
                string download_url = down_base_url + cloud_sha + "/" + url;
                if (!DownloadFile(download_url, tempname))
                {
                    continue;
                }
                string tmp = File.ReadAllText(tempname).Replace("\r\n", "\n");

                try
                {
                    JsonConvert.DeserializeObject(tmp);
                }
                catch (Exception)
                {
                    continue;
                }

                string src = File.ReadAllText(filename).Replace("\r\n", "\n");
                if (src.Length != tmp.Length)
                {
                    File.Copy(tempname, filename, true);
                    updated = true;
                    message += cur_message + "\n";
                }
                ViewStatusStorage.Set(filename, cloud_sha);
                File.Delete(tempname);
            }
            if (updated)
            {
                new ToastContentBuilder()
                    .AddText("资源已更新，重启软件生效！")
                    .AddText(message)
                    .Show();
                return true;
            }
            else
            {
                return false;
            }
        }

        private static void CopyFilesRecursively(string sourcePath, string targetPath)
        {
            //Now Create all of the directories
            foreach (string dirPath in Directory.GetDirectories(sourcePath, "*", SearchOption.AllDirectories))
            {
                Directory.CreateDirectory(dirPath.Replace(sourcePath, targetPath));
            }

            //Copy all the files & Replaces any files with the same name
            foreach (string newPath in Directory.GetFiles(sourcePath, "*.*", SearchOption.AllDirectories))
            {
                File.Copy(newPath, newPath.Replace(sourcePath, targetPath), true);
            }
        }

        public void Close()
        {
            RequestClose();
            IsFirstBootAfterUpdate = false;
            UpdateTag = "";
            UpdateInfo = "";
        }
    }
}
