// <copyright file="VersionUpdateViewModel.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Documents;
using MaaWpfGui.Helper;
using Markdig;
using Neo.Markdig.Xaml;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MaaWpfGui
{
    /// <summary>
    /// The view model of version update.
    /// </summary>
    public class VersionUpdateViewModel : Screen
    {
        private readonly SettingsViewModel _settingsViewModel;
        private readonly TaskQueueViewModel _taskQueueViewModel;

        /// <summary>
        /// Initializes a new instance of the <see cref="VersionUpdateViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public VersionUpdateViewModel(IContainer container)
        {
            _settingsViewModel = container.Get<SettingsViewModel>();
            _taskQueueViewModel = container.Get<TaskQueueViewModel>();
        }

        [DllImport("MaaCore.dll")]
        private static extern IntPtr AsstGetVersion();

        private static string AddContributorLink(string text)
        {
            /*
            //        "@ " -> "@ "
            //       "`@`" -> "`@`"
            //   "@MistEO" -> "[@MistEO](https://github.com/MistEO)"
            // "[@MistEO]" -> "[@MistEO]"
            */
            return Regex.Replace(text, @"([^\[`]|^)@([^\s]+)", "$1[@$2](https://github.com/$2)");
        }

        private readonly string _curVersion = Marshal.PtrToStringAnsi(AsstGetVersion());
        private string _latestVersion;

        private string _updateTag = ViewStatusStorage.Get("VersionUpdate.name", string.Empty);

        /// <summary>
        /// Gets or sets the update tag.
        /// </summary>
        public string UpdateTag
        {
            get => _updateTag;
            set
            {
                SetAndNotify(ref _updateTag, value);
                ViewStatusStorage.Set("VersionUpdate.name", value);
            }
        }

        private string _updateInfo = ViewStatusStorage.Get("VersionUpdate.body", string.Empty);

        // private static readonly MarkdownPipeline s_markdownPipeline = new MarkdownPipelineBuilder().UseXamlSupportedExtensions().Build();

        /// <summary>
        /// Gets or sets the update info.
        /// </summary>
        public string UpdateInfo
        {
            get
            {
                try
                {
                    return AddContributorLink(_updateInfo);
                }
                catch
                {
                    return _updateInfo;
                }
            }

            set
            {
                SetAndNotify(ref _updateInfo, value);
                ViewStatusStorage.Set("VersionUpdate.body", value);
            }
        }

        public FlowDocument UpdateInfoDoc => MarkdownXaml.ToFlowDocument(UpdateInfo,
                new MarkdownPipelineBuilder().UseXamlSupportedExtensions().Build());

        private string _updateUrl;

        /// <summary>
        /// Gets or sets the update URL.
        /// </summary>
        public string UpdateUrl
        {
            get => _updateUrl;
            set => SetAndNotify(ref _updateUrl, value);
        }

        private bool _isFirstBootAfterUpdate = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.isfirstboot", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether it is the first boot after updating.
        /// </summary>
        public bool IsFirstBootAfterUpdate
        {
            get => _isFirstBootAfterUpdate;
            set
            {
                SetAndNotify(ref _isFirstBootAfterUpdate, value);
                ViewStatusStorage.Set("VersionUpdate.isfirstboot", value.ToString());
            }
        }

        private string _updatePackageName = ViewStatusStorage.Get("VersionUpdate.package", string.Empty);

        /// <summary>
        /// Gets or sets the name of the update package.
        /// </summary>
        public string UpdatePackageName
        {
            get => _updatePackageName;
            set
            {
                SetAndNotify(ref _updatePackageName, value);
                ViewStatusStorage.Set("VersionUpdate.package", value);
            }
        }

        /// <summary>
        /// Gets the OS architecture.
        /// </summary>
        public static string OSArchitecture => RuntimeInformation.OSArchitecture.ToString().ToLower();

        /// <summary>
        /// Gets a value indicating whether the OS is arm.
        /// </summary>
        public static bool IsArm => OSArchitecture.StartsWith("arm");

        private const string RequestUrl = "repos/MaaAssistantArknights/MaaRelease/releases";
        private const string StableRequestUrl = "repos/MaaAssistantArknights/MaaAssistantArknights/releases/latest";
        private const string MaaReleaseRequestUrlByTag = "repos/MaaAssistantArknights/MaaRelease/releases/tags/";
        private const string InfoRequestUrl = "repos/MaaAssistantArknights/MaaAssistantArknights/releases/tags/";

        private JObject _latestJson;
        private JObject _assetsObject;

        /// <summary>
        /// 检查是否有已下载的更新包，如果有立即更新并重启进程。
        /// </summary>
        /// <returns>操作成功返回 <see langword="true"/>，反之则返回 <see langword="false"/>。</returns>
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
                using var toast = new ToastNotification(Localization.GetString("NewVersionZipFileFoundTitle"));
                toast.AppendContentText(Localization.GetString("NewVersionZipFileFoundDescDecompressing"))
                     .AppendContentText(UpdateTag)
                     .ShowUpdateVersion(row: 2);
            });

            string curDir = Directory.GetCurrentDirectory();
            string extractDir = Path.Combine(curDir, "NewVersionExtract");
            string oldFileDir = Path.Combine(curDir, ".old");

            // 解压
            try
            {
                if (Directory.Exists(extractDir))
                {
                    Directory.Delete(extractDir, true);
                }

                System.IO.Compression.ZipFile.ExtractToDirectory(UpdatePackageName, extractDir);
            }
            catch (InvalidDataException)
            {
                File.Delete(UpdatePackageName);
                Execute.OnUIThread(() =>
                {
                    using var toast = new ToastNotification(Localization.GetString("NewVersionZipFileBrokenTitle"));
                    toast.AppendContentText(Localization.GetString("NewVersionZipFileBrokenDescFilename") + UpdatePackageName)
                         .AppendContentText(Localization.GetString("NewVersionZipFileBrokenDescDeleted"))
                         .ShowUpdateVersion();
                });
                return false;
            }

            string removeListFile = Path.Combine(extractDir, "removelist.txt");
            if (File.Exists(removeListFile))
            {
                string[] removeList = File.ReadAllLines(removeListFile);
                foreach (string file in removeList)
                {
                    string path = Path.Combine(curDir, file);
                    if (File.Exists(path))
                    {
                        string moveTo = Path.Combine(oldFileDir, file);
                        if (File.Exists(moveTo))
                        {
                            File.Delete(moveTo);
                        }
                        else
                        {
                            Directory.CreateDirectory(Path.GetDirectoryName(moveTo));
                        }

                        File.Move(path, moveTo);
                    }
                }
            }

            Directory.CreateDirectory(oldFileDir);
            foreach (var dir in Directory.GetDirectories(extractDir, "*", SearchOption.AllDirectories))
            {
                Directory.CreateDirectory(dir.Replace(extractDir, curDir));
                Directory.CreateDirectory(dir.Replace(extractDir, oldFileDir));
            }

            // 复制新版本的所有文件到当前路径下
            foreach (var file in Directory.GetFiles(extractDir, "*", SearchOption.AllDirectories))
            {
                var fileName = Path.GetFileName(file);
                if (fileName == "removelist.txt" || fileName == "md5sum.txt")
                {
                    continue;
                }

                string curFileName = file.Replace(extractDir, curDir);
                if (File.Exists(curFileName))
                {
                    string moveTo = file.Replace(extractDir, oldFileDir);
                    if (File.Exists(moveTo))
                    {
                        File.Delete(moveTo);
                    }

                    File.Move(curFileName, moveTo);
                }

                File.Move(file, curFileName);
            }

            foreach (var oldFile in Directory.GetFiles(curDir, "*.old"))
            {
                File.Delete(oldFile);
            }

            // 操作完了，把解压的文件删了
            Directory.Delete(extractDir, true);
            File.Delete(UpdatePackageName);

            // 保存更新信息，下次启动后会弹出已更新完成的提示
            UpdatePackageName = string.Empty;
            ViewStatusStorage.Release();

            // 重启进程（启动的是更新后的程序了）
            var newProcess = new Process();
            newProcess.StartInfo.FileName = AppDomain.CurrentDomain.FriendlyName;
            newProcess.StartInfo.WorkingDirectory = Directory.GetCurrentDirectory();
            newProcess.Start();
            Application.Current.Shutdown();

            return true;
        }

        public enum CheckUpdateRetT
        {
            /// <summary>
            /// 操作成功
            /// </summary>
            OK,

            /// <summary>
            /// 未知错误
            /// </summary>
            UnknownError,

            /// <summary>
            /// 无需更新
            /// </summary>
            NoNeedToUpdate,

            /// <summary>
            /// 已经是最新版
            /// </summary>
            AlreadyLatest,

            /// <summary>
            /// 网络错误
            /// </summary>
            NetworkError,

            /// <summary>
            /// 获取信息失败
            /// </summary>
            FailedToGetInfo,

            /// <summary>
            /// 新版正在构建中
            /// </summary>
            NewVersionIsBeingBuilt,
        }

        public enum Downloader
        {
            /// <summary>
            /// 原生下载器
            /// </summary>
            Native,
        }

        /// <summary>
        /// 检查更新，并下载更新包。
        /// </summary>
        /// <param name="force">是否强制检查。</param>
        /// <returns>操作成功返回 <see langword="true"/>，反之则返回 <see langword="false"/>。</returns>
        public CheckUpdateRetT CheckAndDownloadUpdate(bool force = false)
        {
            _settingsViewModel.IsCheckingForUpdates = true;

            var checkResult = ((Func<CheckUpdateRetT>)(() =>
            {
                // 检查更新
                var checkRet = CheckUpdate(force);
                if (checkRet != CheckUpdateRetT.OK)
                {
                    return checkRet;
                }

                // 保存新版本的信息
                var name = _latestJson["name"]?.ToString();
                UpdateTag = name == string.Empty ? _latestJson["tag_name"]?.ToString() : name;
                var body = _latestJson["body"]?.ToString();
                if (body == string.Empty)
                {
                    string ComparableHash(string version)
                    {
                        if (isStdVersion(version))
                        {
                            return version;
                        }
                        else if (Semver.SemVersion.TryParse(version, Semver.SemVersionStyles.AllowLowerV, out var semVersion) &&
                            isNightlyVersion(semVersion))
                        {
                            // v4.6.6-1.g{Hash}
                            // v4.6.7-beta.2.8.g{Hash}
                            var commitHash = semVersion.PrereleaseIdentifiers.Last().ToString();
                            if (commitHash.StartsWith("g"))
                            {
                                commitHash = commitHash.Remove(0, 1);
                            }

                            return commitHash;
                        }

                        return null;
                    }

                    var curHash = ComparableHash(_curVersion);
                    var latestHash = ComparableHash(_latestVersion);

                    if (curHash != null && latestHash != null)
                    {
                        body = $"**Full Changelog**: [{curHash} -> {latestHash}](https://github.com/MaaAssistantArknights/MaaAssistantArknights/compare/{curHash}...{latestHash})";
                    }
                }

                UpdateInfo = body;
                UpdateUrl = _latestJson["html_url"]?.ToString();

                bool otaFound = _assetsObject != null;
                bool goDownload = otaFound && _settingsViewModel.AutoDownloadUpdatePackage;
#pragma warning disable IDE0042
                var openUrlToastButton = (
                    text: Localization.GetString("NewVersionFoundButtonGoWebpage"),
                    action: new Action(() =>
                    {
                        if (!string.IsNullOrWhiteSpace(UpdateUrl))
                        {
                            Process.Start(UpdateUrl);
                        }
                    }));
#pragma warning restore IDE0042
                Execute.OnUIThread(() =>
                {
                    using var toast = new ToastNotification(otaFound ?
                        Localization.GetString("NewVersionFoundTitle") :
                        Localization.GetString("NewVersionFoundButNoPackageTitle"));
                    if (goDownload)
                    {
                        OutputDownloadProgress(downloading: false, output: Localization.GetString("NewVersionDownloadPreparing"));
                        toast.AppendContentText(Localization.GetString("NewVersionFoundDescDownloading"));
                    }

                    toast.AppendContentText(Localization.GetString("NewVersionFoundDescId") + UpdateTag);

                    if (!otaFound)
                    {
                        toast.AppendContentText(Localization.GetString("NewVersionFoundButNoPackageDesc"));
                    }

                    var toastDesc = UpdateInfo.Length > 100 ?
                        UpdateInfo.Substring(0, 100) + "..." :
                        UpdateInfo;
                    toast.AppendContentText(Localization.GetString("NewVersionFoundDescInfo") + toastDesc);
                    toast.AddButtonLeft(openUrlToastButton.text, openUrlToastButton.action);
                    toast.ButtonSystemUrl = UpdateUrl;
                    toast.ShowUpdateVersion();
                });

                UpdatePackageName = _assetsObject?["name"]?.ToString() ?? string.Empty;

                if (!goDownload || string.IsNullOrWhiteSpace(UpdatePackageName))
                {
                    OutputDownloadProgress(string.Empty);
                    return CheckUpdateRetT.NoNeedToUpdate;
                }

                // 下载压缩包
                var downloaded = false;
                var mirroredReplaceMap = new List<Tuple<string, string>>
                {
                    new Tuple<string, string>("github.com", "agent.imgg.dev"),
                    new Tuple<string, string>("https://", "https://git.114514.pro/https://"),
                    new Tuple<string, string>("https://", "https://ghproxy.com/https://"),
                    new Tuple<string, string>("github.com", "ota.maa.plus"),
                    new Tuple<string, string>("github.com", "download.fastgit.org"),
                    null,
                };

                string rawUrl = _assetsObject["browser_download_url"]?.ToString();
                const int DownloadRetryMaxTimes = 1;
                for (int i = 0; i <= DownloadRetryMaxTimes && !downloaded; i++)
                {
                    var url = rawUrl;
                    foreach (var repTuple in mirroredReplaceMap)
                    {
                        if (repTuple != null)
                        {
                            url = url.Replace(repTuple.Item1, repTuple.Item2);
                        }

                        if (DownloadGithubAssets(url, _assetsObject, Downloader.Native))
                        {
                            OutputDownloadProgress(downloading: false, output: Localization.GetString("NewVersionDownloadCompletedTitle"));
                            downloaded = true;
                            break;
                        }
                    }
                }

                if (!downloaded)
                {
                    OutputDownloadProgress(downloading: false, output: Localization.GetString("NewVersionDownloadFailedTitle"));
                    Execute.OnUIThread(() =>
                    {
                        using var toast = new ToastNotification(Localization.GetString("NewVersionDownloadFailedTitle"));
                        toast.ButtonSystemUrl = UpdateUrl;
                        toast.AppendContentText(Localization.GetString("NewVersionDownloadFailedDesc"))
                                .AddButtonLeft(openUrlToastButton.text, openUrlToastButton.action)
                                .Show();
                    });
                    return CheckUpdateRetT.NoNeedToUpdate;
                }

                return CheckUpdateRetT.OK;
            }))();

            _settingsViewModel.IsCheckingForUpdates = false;
            return checkResult;
        }

        public void AskToRestart()
        {
            System.Windows.Forms.MessageBoxManager.Unregister();
            System.Windows.Forms.MessageBoxManager.Yes = Localization.GetString("Ok");
            System.Windows.Forms.MessageBoxManager.No = Localization.GetString("ManualRestart");
            System.Windows.Forms.MessageBoxManager.Register();
            var result = MessageBox.Show(
                Localization.GetString("NewVersionDownloadCompletedDesc"),
                Localization.GetString("NewVersionDownloadCompletedTitle"),
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);
            System.Windows.Forms.MessageBoxManager.Unregister();
            if (result == MessageBoxResult.Yes)
            {
                Application.Current.Shutdown();
                System.Windows.Forms.Application.Restart();
            }
        }

        /// <summary>
        /// 检查更新。
        /// </summary>
        /// <param name="force">是否强制检查。</param>
        /// <returns>检查到更新返回 <see langword="true"/>，反之则返回 <see langword="false"/>。</returns>
        private CheckUpdateRetT CheckUpdate(bool force = false)
        {
            // 自动更新或者手动触发
            if (!(_settingsViewModel.UpdateCheck || force))
            {
                return CheckUpdateRetT.NoNeedToUpdate;
            }

            // 调试版不检查更新
            if (isDebugVersion())
            {
                return CheckUpdateRetT.FailedToGetInfo;
            }

            const int RequestRetryMaxTimes = 2;
            try
            {
                if (!_settingsViewModel.UpdateBeta && !_settingsViewModel.UpdateNightly)
                {
                    // 稳定版更新使用主仓库 /latest 接口
                    // 直接使用 MaaRelease 的话，30 个可能会找不到稳定版，因为有可能 Nightly 发了很多
                    var stableResponse = RequestGithubApi(StableRequestUrl, RequestRetryMaxTimes);
                    if (string.IsNullOrEmpty(stableResponse))
                    {
                        return CheckUpdateRetT.NetworkError;
                    }

                    _latestJson = JsonConvert.DeserializeObject(stableResponse) as JObject;
                    _latestVersion = _latestJson["tag_name"].ToString();
                    stableResponse = RequestGithubApi(MaaReleaseRequestUrlByTag + _latestVersion, RequestRetryMaxTimes);

                    // 主仓库能找到版，但是 MaaRelease 找不到，说明 MaaRelease 还没有同步（一般过个十分钟就同步好了）
                    if (string.IsNullOrEmpty(stableResponse))
                    {
                        return CheckUpdateRetT.NewVersionIsBeingBuilt;
                    }

                    _latestJson = JsonConvert.DeserializeObject(stableResponse) as JObject;
                }
                else
                {
                    // 非稳定版更新使用 MaaRelease/releases 接口
                    var response = RequestGithubApi(RequestUrl, RequestRetryMaxTimes);
                    if (string.IsNullOrEmpty(response))
                    {
                        return CheckUpdateRetT.NetworkError;
                    }

                    var releaseArray = JsonConvert.DeserializeObject(response) as JArray;

                    _latestJson = null;
                    foreach (var item in releaseArray)
                    {
                        if (!_settingsViewModel.UpdateNightly && !isStdVersion(item["tag_name"].ToString()))
                        {
                            continue;
                        }

                        _latestJson = item as JObject;
                        break;
                    }
                }

                if (_latestJson == null)
                {
                    return CheckUpdateRetT.AlreadyLatest;
                }

                _latestVersion = _latestJson["tag_name"].ToString();
                var releaseAssets = _latestJson["assets"] as JArray;

                if (_settingsViewModel.UpdateNightly)
                {
                    if (_curVersion == _latestVersion)
                    {
                        return CheckUpdateRetT.AlreadyLatest;
                    }
                }
                else
                {
                    bool curParsed = Semver.SemVersion.TryParse(_curVersion, Semver.SemVersionStyles.AllowLowerV, out var curVersionObj);
                    bool latestPared = Semver.SemVersion.TryParse(_latestVersion, Semver.SemVersionStyles.AllowLowerV, out var latestVersionObj);
                    if (curParsed && latestPared)
                    {
                        if (curVersionObj.CompareSortOrderTo(latestVersionObj) >= 0)
                        {
                            return CheckUpdateRetT.AlreadyLatest;
                        }
                    }
                    else if (string.CompareOrdinal(_curVersion, _latestVersion) >= 0)
                    {
                        return CheckUpdateRetT.AlreadyLatest;
                    }
                }

                // 从主仓库获取changelog等信息
                // 非稳定版本是 Nightly 下载的，主仓库没有它的更新信息，不必请求
                if (isStdVersion(_latestVersion))
                {
                    var infoResponse = RequestGithubApi(InfoRequestUrl + _latestVersion, RequestRetryMaxTimes);
                    if (string.IsNullOrEmpty(infoResponse))
                    {
                        return CheckUpdateRetT.FailedToGetInfo;
                    }

                    _latestJson = JsonConvert.DeserializeObject(infoResponse) as JObject;
                }

                _assetsObject = null;
                foreach (var curAssets in releaseAssets)
                {
                    string name = curAssets["name"].ToString().ToLower();
                    if (name.Contains("ota") && name.Contains("win") && name.Contains($"{_curVersion}_{_latestVersion}"))
                    {
                        _assetsObject = curAssets as JObject;
                        if (IsArm ^ name.Contains("arm"))
                        {
                            continue; // 兼容旧版本，以前 ota 不区分指令集架构
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                return CheckUpdateRetT.OK;
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                return CheckUpdateRetT.UnknownError;
            }
        }

        private string RequestGithubApi(string url, int retryTimes)
        {
            string response = string.Empty;
            string[] requestSource = { "https://api.github.com/", "https://api.kgithub.com/" };
            do
            {
                for (var i = 0; i < requestSource.Length; i++)
                {
                    response = WebService.RequestGet(requestSource[i] + url);
                    if (!string.IsNullOrEmpty(response))
                    {
                        break;
                    }
                }
            }
            while (string.IsNullOrEmpty(response) && retryTimes-- > 0);
            return response;
        }

        /// <summary>
        /// 获取 GitHub Assets 对象对应的文件
        /// </summary>
        /// <param name="url">下载链接</param>
        /// <param name="assetsObject">Github Assets 对象</param>
        /// <param name="downloader">下载方式，如为空则使用 CSharp 原生方式下载</param>
        /// <param name="saveTo">保存至的文件夹，如为空则使用当前位置</param>
        /// <returns>操作成功返回 true，反之则返回 false</returns>
        private bool DownloadGithubAssets(string url, JObject assetsObject,
            Downloader downloader = Downloader.Native, string saveTo = null)
        {
            _logItemViewModels = _taskQueueViewModel.LogItemViewModels;
            return DownloadFile(
                url: url,
                fileName: assetsObject["name"].ToString(), contentType:
                assetsObject["content_type"].ToString(),
                downloader: downloader,
                saveTo: saveTo,
                proxy: _settingsViewModel.Proxy);
        }

        /// <summary>
        /// 通过网络获取文件资源
        /// </summary>
        /// <param name="url">网络资源地址</param>
        /// <param name="fileName">保存此文件使用的文件名</param>
        /// <param name="contentType">获取对象的物联网通用类型</param>
        /// <param name="downloader">下载方式，如为空则使用 CSharp 原生方式下载</param>
        /// <param name="saveTo">保存至的文件夹，如为空则使用当前位置</param>
        /// <param name="proxy">http proxy</param>
        /// <returns>操作成功返回 <see langword="true"/>，反之则返回 <see langword="false"/>。</returns>
        public static bool DownloadFile(string url, string fileName, string contentType = null,
            Downloader downloader = Downloader.Native, string saveTo = null, string proxy = "")
        {
            string fileDir = (saveTo == null) ? Directory.GetCurrentDirectory() : Path.GetFullPath(saveTo);
            string fileNameWithTemp = fileName + ".temp";
            string fullFilePath = Path.Combine(fileDir, fileName);
            string fullFilePathWithTemp = Path.Combine(fileDir, fileNameWithTemp);
            bool returned = false;
            try
            {
                switch (downloader)
                {
                    case Downloader.Native:
                        returned = DownloadFileForCSharpNative(url: url, filePath: fullFilePathWithTemp, contentType: contentType, proxy);
                        break;
                }

                OutputDownloadProgress(string.Empty);
            }
            catch (WebException)
            {
                returned = false;
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                returned = false;
            }

            if (returned)
            {
                File.Copy(fullFilePathWithTemp, fullFilePath, true);
            }

            // 删除临时文件
            if (File.Exists(fullFilePathWithTemp))
            {
                File.Delete(fullFilePathWithTemp);
            }

            return returned;
        }

        private static bool DownloadFileForAria2(string url, string saveTo, string fileName, string proxy = "")
        {
            var aria2FilePath = Path.GetFullPath(Directory.GetCurrentDirectory() + "/aria2c.exe");
            var aria2Args = "\"" + url + "\" --continue=true --dir=\"" + saveTo + "\" --out=\"" + fileName + "\" --user-agent=\"" + WebService.RequestUserAgent + "\"";

            if (proxy.Length > 0)
            {
                aria2Args += " --all-proxy=\"" + proxy + "\"";
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
                EnableRaisingEvents = true,
            };
            aria2Process.OutputDataReceived += new DataReceivedEventHandler((sender, e) =>
            {
                if (e.Data != null && e.Data.StartsWith("["))
                {
                    OutputDownloadProgress(e.Data);
                }
            });

            aria2Process.Start();
            aria2Process.BeginOutputReadLine();
            aria2Process.WaitForExit();
            var exit_code = aria2Process.ExitCode;
            aria2Process.Close();
            return exit_code == 0;
        }

        /// <summary>
        /// 使用 CSharp 原生方式下载文件
        /// </summary>
        /// <param name="url">下载地址</param>
        /// <param name="filePath">文件路径</param>
        /// <param name="contentType">HTTP ContentType</param>
        /// <param name="proxy">http proxy</param>
        /// <returns>是否成功</returns>
        private static bool DownloadFileForCSharpNative(string url, string filePath, string contentType = null, string proxy = "")
        {
            bool downloaded = false;

            // 创建 Http 请求
            var httpWebRequest = WebRequest.Create(url) as HttpWebRequest;

            // 设定相关属性
            httpWebRequest.Method = "GET";
            httpWebRequest.UserAgent = WebService.RequestUserAgent;
            httpWebRequest.Accept = contentType;
            if (!string.IsNullOrEmpty(proxy))
            {
                httpWebRequest.Proxy = new WebProxy(proxy);
            }

            // 获取输入输出流
            using (var response = httpWebRequest.GetResponse())
            {
                using var responseStream = response.GetResponseStream();
                using var fileStream = new FileStream(filePath, FileMode.Create, FileAccess.Write);

                // 记录初始化
                long value = 0;
                int valueInOneSecond = 0;
                long fileMaximum = response.ContentLength;
                DateTime beforDT = DateTime.Now;
                OutputDownloadProgress();

                // 输入输出初始化
                byte[] buffer = new byte[81920];
                int byteLen = responseStream.Read(buffer, 0, buffer.Length);

                while (byteLen > 0)
                {
                    // 记录
                    valueInOneSecond += byteLen;
                    double ts = DateTime.Now.Subtract(beforDT).TotalSeconds;
                    if (ts > 1)
                    {
                        beforDT = DateTime.Now;
                        value += valueInOneSecond;
                        OutputDownloadProgress(value, fileMaximum, valueInOneSecond, ts);
                        valueInOneSecond = 0;
                    }

                    // 输入输出
                    fileStream.Write(buffer, 0, byteLen);
                    byteLen = responseStream.Read(buffer, 0, buffer.Length);
                }

                downloaded = true;
            }

            return downloaded;
        }

        private static System.Collections.ObjectModel.ObservableCollection<LogItemViewModel> _logItemViewModels = null;

        private static void OutputDownloadProgress(long value = 0, long maximum = 1, int len = 0, double ts = 1)
        {
            OutputDownloadProgress(
                string.Format("[{0:F}MiB/{1:F}MiB({2}%) {3:F} KiB/s]",
                    value / 1048576.0,
                    maximum / 1048576.0,
                    100 * value / maximum,
                    len / ts / 1024.0));
        }

        private static void OutputDownloadProgress(string output, bool downloading = true)
        {
            if (_logItemViewModels == null)
            {
                return;
            }

            var log = new LogItemViewModel(downloading ? Localization.GetString("NewVersionFoundDescDownloading") + "\n" + output : output, UILogColor.Download);

            Application.Current.Dispatcher.Invoke(() =>
            {
                if (_logItemViewModels.Count > 0 && _logItemViewModels[0].Color == UILogColor.Download)
                {
                    if (!string.IsNullOrEmpty(output))
                    {
                        _logItemViewModels[0] = log;
                    }
                    else
                    {
                        _logItemViewModels.RemoveAt(0);
                    }
                }
                else if (!string.IsNullOrEmpty(output))
                {
                    _logItemViewModels.Clear();
                    _logItemViewModels.Add(log);
                }
            });
        }

        private bool isDebugVersion(string version = null)
        {
            version ??= _curVersion;
            return version == "DEBUG VERSION";
        }

        private bool isStdVersion(string version = null)
        {
            // 正式版：vX.X.X
            // DevBuild (CI)：yyyy-MM-dd-HH-mm-ss-{CommitHash[..7]}
            // DevBuild (Local)：yyyy-MM-dd-HH-mm-ss-{CommitHash[..7]}-Local
            // Release (Local Commit)：v.{CommitHash[..7]}-Local
            // Release (Local Tag)：{Tag}-Local
            // Debug (Local)：DEBUG VERSION
            // Script Compiled：c{CommitHash[..7]}
            version ??= _curVersion;

            if (isDebugVersion(version))
            {
                return false;
            }
            else if (version.StartsWith("c") || version.StartsWith("20") || version.Contains("Local"))
            {
                return false;
            }
            else if (!Semver.SemVersion.TryParse(version, Semver.SemVersionStyles.AllowLowerV, out var semVersion))
            {
                return false;
            }
            else if (isNightlyVersion(semVersion))
            {
                return false;
            }

            return true;
        }

        private bool isNightlyVersion(Semver.SemVersion version)
        {
            if (!version.IsPrerelease)
            {
                return false;
            }

            // v{Major}.{Minor}.{Patch}-{Prerelease}.{CommitDistance}.g{CommitHash}
            // v4.6.7-beta.2.1.g1234567
            // v4.6.8-5.g1234567
            var lastId = version.PrereleaseIdentifiers.LastOrDefault().ToString();
            return lastId.StartsWith("g") && lastId.Length >= 7;
        }

        /// <summary>
        /// 复制文件夹内容并覆盖已存在的相同名字的文件
        /// </summary>
        /// <param name="sourcePath">源文件夹</param>
        /// <param name="targetPath">目标文件夹</param>
        public static void CopyFilesRecursively(string sourcePath, string targetPath)
        {
            Directory.CreateDirectory(targetPath);

            // Now Create all of the directories
            foreach (string dirPath in Directory.GetDirectories(sourcePath, "*", SearchOption.AllDirectories))
            {
                Directory.CreateDirectory(dirPath.Replace(sourcePath, targetPath));
            }

            // Copy all the files & Replaces any files with the same name
            foreach (string newPath in Directory.GetFiles(sourcePath, "*.*", SearchOption.AllDirectories))
            {
                File.Copy(newPath, newPath.Replace(sourcePath, targetPath), true);
            }
        }

        /// <summary>
        /// The event handler of opening hyperlink.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments.</param>
        public void OpenHyperlink(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
        {
            Process.Start(e.Parameter.ToString());
        }
    }
}
