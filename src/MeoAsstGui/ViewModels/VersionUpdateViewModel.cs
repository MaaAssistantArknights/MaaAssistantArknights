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
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class VersionUpdateViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        public VersionUpdateViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
        }

        [DllImport("MeoAssistant.dll")] private static extern IntPtr AsstGetVersion();

        private readonly string _curVersion = Marshal.PtrToStringAnsi(AsstGetVersion());
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

        private const string RequestUrl = "https://api.github.com/repos/MistEO/MeoAssistantArknights/releases";
        private const string RequestUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.99 Safari/537.36 Edg/97.0.1072.76";
        private JObject _lastestJson;
        private JObject _assetsObject;

        /// <summary>
        /// 检查是否有已下载的更新包，如果有立即更新并重启进程
        /// </summary>
        /// <returns>操作成功返回 true，反之则返回 false</returns>

        public bool CheckAndUpdateNow()
        {
            if (UpdateTag == string.Empty
                || UpdatePackageName == string.Empty
                || !File.Exists(UpdatePackageName))
            {
                return false;
            }

            Execute.OnUIThread(() =>
            {
                using (var toast = new ToastNotification("检测到新版本包"))
                {
                    toast.AppendContentText("正在解压，请稍等……")
                        .AppendContentText(UpdateTag)
                        .ShowUpdateVersion(row: 2);
                }
            });

            string extractDir = Directory.GetCurrentDirectory() + "\\NewVersionExtract";
            // 解压
            try
            {
                System.IO.Compression.ZipFile.ExtractToDirectory(UpdatePackageName, extractDir);
            }
            catch (InvalidDataException)
            {
                File.Delete(UpdatePackageName);
                Execute.OnUIThread(() =>
                {
                    using (var toast = new ToastNotification("更新文件不正确！"))
                    {
                        toast.AppendContentText("文件名: " + UpdatePackageName)
                            .AppendContentText("已将其删除！")
                            .ShowUpdateVersion();
                    }
                });
                return false;
            }

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
            var newProcess = new Process();
            newProcess.StartInfo.FileName = AppDomain.CurrentDomain.FriendlyName;
            newProcess.StartInfo.WorkingDirectory = Directory.GetCurrentDirectory();
            newProcess.Start();
            Application.Current.Shutdown();

            return true;
        }

        /// <summary>
        /// 检查更新，并下载更新包
        /// </summary>
        /// <returns>操作成功返回 true，反之则返回 false</returns>

        public bool CheckAndDownloadUpdate()
        {
            // 检查更新
            if (!CheckUpdate())
            {
                return false;
            }
            // 保存新版本的信息
            UpdatePackageName = _assetsObject["name"].ToString();
            UpdateTag = _lastestJson["name"].ToString();
            UpdateInfo = _lastestJson["body"].ToString();

            var openUrlToastButton = (
                text: "前往页面查看",
                action: new Action(() =>
                {
                    if (!string.IsNullOrWhiteSpace(_lastestJson["html_url"].ToString()))
                    {
                        Process.Start(_lastestJson["html_url"].ToString());
                    }
                })
            );

            if (_container.Get<SettingsViewModel>().AutoDownloadUpdatePackage)
            {
                Execute.OnUIThread(() =>
                {
                    using (var toast = new ToastNotification("检测到新版本"))
                    {
                        toast.AppendContentText("正在后台下载……")
                            .AppendContentText("新版本: " + UpdateTag)
                            .AppendContentText("更新信息: " + UpdateInfo)
                            .AddButtonLeft(openUrlToastButton.text, openUrlToastButton.action)
                            .ShowUpdateVersion();
                    }
                });
            }
            else
            {
                Execute.OnUIThread(() =>
                {
                    using (var toast = new ToastNotification("检测到新版本"))
                    {
                        toast.AppendContentText("新版本: " + UpdateTag)
                            .AppendContentText("更新信息: " + UpdateInfo)
                            .AddButtonLeft(openUrlToastButton.text, openUrlToastButton.action)
                            .ShowUpdateVersion();
                    }
                });
                return false;
            }

            // 下载压缩包
            const int DownloadRetryMaxTimes = 3;
            var downloaded = false;
            for (int i = 1; i <= DownloadRetryMaxTimes; i++)
            {
                var mirroredAssets = (JObject)_assetsObject.DeepClone();
                mirroredAssets.Property("browser_download_url").Remove();
                mirroredAssets.Add("browser_download_url", _assetsObject["browser_download_url"].ToString().Replace("github.com", "download.fastgit.org"));
                if (DownloadGithubAssets(mirroredAssets) || DownloadGithubAssets(_assetsObject))
                {
                    downloaded = true;
                    break;
                }
            }

            if (!downloaded)
            {
                Execute.OnUIThread(() =>
                {
                    using (var toast = new ToastNotification("新版本下载失败"))
                    {
                        toast.AppendContentText("请尝试手动下载后，将压缩包放到目录下_(:з」∠)_")
                            .AddButtonLeft(openUrlToastButton.text, openUrlToastButton.action)
                            .Show();
                    }
                });
                return false;
            }

            // 把相关信息存下来，更新完之后启动的时候显示
            Execute.OnUIThread(() =>
            {
                using (var toast = new ToastNotification("新版本下载完成"))
                {
                    toast.AppendContentText("软件将在下次启动时自动更新！")
                        .AppendContentText("✿✿ヽ(°▽°)ノ✿")
                        .ShowUpdateVersion(row: 2);
                }
            });

            return true;
        }

        /// <summary>
        /// 检查更新
        /// </summary>
        /// <returns>操作成功返回 true，反之则返回 false</returns>

        public bool CheckUpdate()
        {
            var settings = _container.Get<SettingsViewModel>();
            const int requestRetryMaxTimes = 5;
            var response = RequestApi(RequestUrl);
            for (int i = 0; response.Length == 0 && i >= requestRetryMaxTimes; i++)
            {
                response = RequestApi(RequestUrl);
            }
            if (response.Length == 0)
            {
                return false;
            }

            try
            {
                var releaseArray = JsonConvert.DeserializeObject(response) as JArray;

                for (int i = 0; i <= releaseArray.Count; i++)
                {
                    if (((bool)releaseArray[i]["prerelease"]) && settings.UpdateBeta)
                    {
                        _lastestJson = releaseArray[i] as JObject;
                        break;
                    }
                    else if ((!(bool)releaseArray[i]["prerelease"]) && (!settings.UpdateBeta))
                    {
                        _lastestJson = releaseArray[i] as JObject;
                        break;
                    }
                }

                _latestVersion = _lastestJson["tag_name"].ToString();
                if ((string.Compare(_latestVersion, _curVersion) <= 0) || (ViewStatusStorage.Get("VersionUpdate.Ignore", string.Empty) == _latestVersion))
                {
                    return false;
                }
                _assetsObject = _lastestJson["assets"][0] as JObject;
            }
            catch (Exception)
            {
                return false;
            }
            return true;
        }

        /// <summary>
        /// 访问 API
        /// </summary>
        /// <param name="url">API 地址</param>
        /// <returns>返回 API 的返回值，如出现错误则返回空字符串</returns>

        private string RequestApi(string url)
        {
            try
            {
                HttpWebRequest httpWebRequest = (HttpWebRequest)HttpWebRequest.Create(url);
                httpWebRequest.Method = "GET";
                httpWebRequest.UserAgent = RequestUserAgent;
                httpWebRequest.Accept = "application/vnd.github.v3+json";
                var settings = _container.Get<SettingsViewModel>();
                if (settings.Proxy.Length > 0)
                {
                    httpWebRequest.Proxy = new WebProxy(settings.Proxy);
                }

                var httpWebResponse = httpWebRequest.GetResponse() as HttpWebResponse;
                var streamReader = new StreamReader(httpWebResponse.GetResponseStream(), Encoding.UTF8);
                var responseContent = streamReader.ReadToEnd();
                streamReader.Close();
                httpWebResponse.Close();
                return responseContent;
            }
            catch (Exception info)
            {
                Console.WriteLine(info.Message);
                return string.Empty;
            }
        }

        /// <summary>
        /// 获取 GitHub Assets 对象对应的文件
        /// </summary>
        /// <param name="assetsObject">Github Assets 对象</param>
        /// <param name="downloader">下载方式，如为空则使用 CSharp 原生方式下载</param>
        /// <param name="saveTo">保存至的文件夹，如为空则使用当前位置</param>
        /// <returns>操作成功返回 true，反之则返回 false</returns>

        public bool DownloadGithubAssets(JObject assetsObject, string downloader = null, string saveTo = null)
        {
            return DownloadFile(
                url: assetsObject["browser_download_url"].ToString(),
                fileName: assetsObject["name"].ToString(), contentType:
                assetsObject["content_type"].ToString(),
                downloader: downloader,
                saveTo: saveTo
                );
        }

        /// <summary>
        /// 通过网络获取文件资源
        /// </summary>
        /// <param name="url">网络资源地址</param>
        /// <param name="fileName">保存此文件使用的文件名</param>
        /// <param name="contentType">获取对象的物联网通用类型</param>
        /// <param name="downloader">下载方式，如为空则使用 CSharp 原生方式下载</param>
        /// <param name="saveTo">保存至的文件夹，如为空则使用当前位置</param>
        /// <returns>操作成功返回 true，反之则返回 false</returns>

        public bool DownloadFile(string url, string fileName, string contentType = null, string downloader = null, string saveTo = null)
        {
            string usedDownloader;
            string filePath;
            bool returned;

            if (saveTo != null)
            {
                filePath = Path.GetFullPath(saveTo);
            }
            else
            {
                filePath = Directory.GetCurrentDirectory();
            }

            if (downloader != null)
            {
                usedDownloader = downloader;
            }
            else
            {
                usedDownloader = _container.Get<SettingsViewModel>().UseAria2 ? "ARIA2" : "NATIVE";
            }

            var fileNameWithTemp = fileName + ".temp";
            var fullFilePath = Path.GetFullPath(filePath + "/" + fileName);
            var fullFilePathWithTemp = Path.GetFullPath(filePath + "/" + fileNameWithTemp);

            try
            {
                if (usedDownloader == "ARIA2")
                {
                    returned = DownloadFileForAria2(url: url, filePath: filePath, fileName: fileNameWithTemp);
                }
                else // 如对应下载器不存在则默认使用 Native 方式下载
                {
                    returned = DownloadFileForCSharpNative(url: url, filePath: fullFilePathWithTemp, contentType: contentType);
                }
            }
            catch (Exception info)
            {
                Console.WriteLine(info.Message);
                returned = false;
            }

            if (returned)
            {
                // 重命名文件
                File.Move(fullFilePathWithTemp, fullFilePath);
                return true;
            }
            else
            {
                // 删除错误的文件
                File.Delete(fullFilePathWithTemp);
                return false;
            }
        }


        private bool DownloadFileForAria2(string url, string filePath, string fileName)
        {
            var aria2FilePath = Path.GetFullPath(Directory.GetCurrentDirectory() + "/aria2c.exe");
            var aria2Args = "\"" + url + "\" --continue=true --dir=\"" + filePath + "\" --out=\"" + fileName + "\" --user-agent=\"" + RequestUserAgent + "\"";

            var settings = _container.Get<SettingsViewModel>();
            if (settings.Proxy.Length > 0)
            {
                aria2Args += " --all-proxy=\"" + settings.Proxy + "\"";
            }

            var aria2Process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = aria2FilePath,
                    Arguments = aria2Args,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                },
                EnableRaisingEvents = true
            };

            aria2Process.Start();
            aria2Process.WaitForExit();
            aria2Process.Close();
            return aria2Process.ExitCode == 0;
        }


        private bool DownloadFileForCSharpNative(string url, string filePath, string contentType = null)
        {
            // 创建 Http 请求
            var httpWebRequest = WebRequest.Create(url) as HttpWebRequest;
            // 设定相关属性
            var settings = _container.Get<SettingsViewModel>();
            httpWebRequest.Method = "GET";
            httpWebRequest.UserAgent = RequestUserAgent;
            if (contentType != null)
            {
                httpWebRequest.Accept = contentType;
            }
            if (settings.Proxy.Length > 0)
            {
                httpWebRequest.Proxy = new WebProxy(settings.Proxy);
            }

            // 转换为 HttpWebResponse
            var httpWebResponse = httpWebRequest.GetResponse() as HttpWebResponse;
            // 获取输入输出流
            var responseStream = httpWebResponse.GetResponseStream();
            var fileStream = new FileStream(filePath, FileMode.Create, FileAccess.Write);
            // 获取并写入流
            var byteArray = new byte[1024];
            var byteArraySize = responseStream.Read(byteArray, 0, byteArray.Length);
            while (byteArraySize > 0)
            {
                fileStream.Write(byteArray, 0, byteArraySize);
                byteArraySize = responseStream.Read(byteArray, 0, byteArray.Length);
            }
            // 关闭流
            responseStream.Close();
            fileStream.Close();

            return true;
        }

        public bool ResourceOTA()
        {
            const string req_base_url = "https://api.github.com/repos/MistEO/MeoAssistantArknights/commits?path=";
            const string repositorie_base = "MistEO/MeoAssistantArknights";
            const string branche_base = "master";

            // cdn接口地址组
            // new string[]
            // {
            //      下载域名地址,
            //      连接地址的参数格式: {0}是文件路径, {1}是 sha 值
            // }
            var down_base_url = new List<string[]>() {
                 new string[]{$"https://cdn.jsdelivr.net/gh/{repositorie_base}@" , "{1}/{0}" },
                 new string[]{$"https://pd.zwc365.com/seturl/https://raw.githubusercontent.com/{repositorie_base}/{branche_base}/" , "{0}?{1}" },
                 new string[]{$"https://cdn.staticaly.com/gh/{repositorie_base}/{branche_base}/" , "{0}?{1}" },
                 new string[]{$"https://ghproxy.fsou.cc/https://github.com/{repositorie_base}/blob/{branche_base}/" , "{0}?{1}" },
            };

            // 资源文件在仓库中的路径，与实际打包后的路径并不相同，需要使用dict
            var update_dict = new Dictionary<string, string>()
            {
                { "3rdparty/resource/penguin-stats-recognize/json/stages.json" , "resource/penguin-stats-recognize/json/stages.json"},
                { "resource/recruit.json", "resource/recruit.json" }
            };

            bool updated = false;
            string message = string.Empty;

            foreach (var item in update_dict)
            {
                string url = item.Key;
                string filename = item.Value;

                string cur_sha = ViewStatusStorage.Get(filename, string.Empty);

                string response = RequestApi(req_base_url + url);
                if (string.IsNullOrWhiteSpace(response))
                {
                    continue;
                }
                string cloud_sha;
                string cur_message = string.Empty;
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

                bool downloaded = false;
                string tempname = filename + ".tmp";
                foreach (var down_item in down_base_url)
                {
                    var download_url = down_item[0];
                    var download_args_format = down_item[1];

                    download_url += string.Format(download_args_format, url, cloud_sha);
                    if (DownloadFile(download_url, tempname, downloader: "NATIVE"))
                    {
                        downloaded = true;
                        break;
                    }
                }

                if (!downloaded)
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
                // 保存最新的 sha 到配置文件
                ViewStatusStorage.Set(filename, cloud_sha);
                File.Delete(tempname);
            }

            if (!updated)
            {
                return false;
            }

            Execute.OnUIThread(() =>
            {
                using (var toast = new ToastNotification("资源已更新"))
                {
                    toast.AppendContentText("重启软件生效！")
                        .AppendContentText(message)
                        .ShowUpdateVersion();
                }
            });

            return true;
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
            UpdateTag = string.Empty;
            UpdateInfo = string.Empty;
        }
    }
}
