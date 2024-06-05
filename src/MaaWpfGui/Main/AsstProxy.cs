// <copyright file="AsstProxy.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;
using HandyControl.Data;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Services;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Helper.Instances.Data;
using AsstHandle = System.IntPtr;
using AsstInstanceOptionKey = System.Int32;

using AsstTaskId = System.Int32;

namespace MaaWpfGui.Main
{
    /// <summary>
    /// MaaCore 代理类。
    /// </summary>
    public class AsstProxy
    {
        private readonly RunningState _runningState;
        private static readonly ILogger _logger = Log.ForContext<AsstProxy>();

        private static unsafe byte[] EncodeNullTerminatedUtf8(string s)
        {
            var enc = Encoding.UTF8.GetEncoder();
            fixed (char* c = s)
            {
                var len = enc.GetByteCount(c, s.Length, true);
                var buf = new byte[len + 1];
                fixed (byte* ptr = buf)
                {
                    enc.Convert(c, s.Length, ptr, len, true, out _, out _, out _);
                }

                return buf;
            }
        }

        private static unsafe bool AsstLoadResource(string dirname)
        {
            fixed (byte* ptr = EncodeNullTerminatedUtf8(dirname))
            {
                _logger.Information($"AsstLoadResource dirname: {dirname}");
                var ret = MaaService.AsstLoadResource(ptr);
                _logger.Information($"AsstLoadResource ret: {ret}");
                return ret;
            }
        }

        private static unsafe bool AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, string value)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUtf8(value))
            {
                return MaaService.AsstSetInstanceOption(handle, key, ptr1);
            }
        }

        private static unsafe bool AsstConnect(AsstHandle handle, string adbPath, string address, string config)
        {
            _logger.Information($"handle: {(long)handle}, adbPath: {adbPath}, address: {address}, config: {config}");

            fixed (byte* ptr1 = EncodeNullTerminatedUtf8(adbPath),
                ptr2 = EncodeNullTerminatedUtf8(address),
                ptr3 = EncodeNullTerminatedUtf8(config))
            {
                bool ret = MaaService.AsstConnect(handle, ptr1, ptr2, ptr3);
                _logger.Information($"handle: {(long)handle}, adbPath: {adbPath}, address: {address}, config: {config}, return: {ret}");
                return ret;
            }
        }

        private static unsafe void AsstSetConnectionExtras(string name, string extras)
        {
            _logger.Information($"name: {name}, extras: {extras}");

            fixed (byte* ptr1 = EncodeNullTerminatedUtf8(name),
                ptr2 = EncodeNullTerminatedUtf8(extras))
            {
                MaaService.AsstSetConnectionExtras(ptr1, ptr2);
            }
        }

        private static void AsstSetConnectionExtrasMuMu12(string extras, ref string error)
        {
            AsstSetConnectionExtras("MuMuEmulator12", extras);
        }

        private static unsafe AsstTaskId AsstAppendTask(AsstHandle handle, string type, string taskParams)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUtf8(type),
                ptr2 = EncodeNullTerminatedUtf8(taskParams))
            {
                return MaaService.AsstAppendTask(handle, ptr1, ptr2);
            }
        }

        private static unsafe bool AsstSetTaskParams(AsstHandle handle, AsstTaskId id, string taskParams)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUtf8(taskParams))
            {
                return MaaService.AsstSetTaskParams(handle, id, ptr1);
            }
        }

        // 现在拆分了 core 和 UI 的日志，所以这个函数暂时没用到
        /*
        [DllImport("MaaCore.dll")]
        private static extern unsafe void AsstLog(byte* level, byte* message);

        /// <summary>
        /// 记录日志。
        /// </summary>
        /// <param name="message">日志内容。</param>
        public static unsafe void AsstLog(string message)
        {
            var level = new ReadOnlySpan<byte>(new byte[] { (byte)'G', (byte)'U', (byte)'I', 0 });
            fixed (byte* ptr1 = level, ptr2 = EncodeNullTerminatedUtf8(message))
            {
                AsstLog(ptr1, ptr2);
            }
        }
        */

        public static unsafe BitmapImage AsstGetImage(AsstHandle handle)
        {
            byte[] buff = new byte[1280 * 720 * 3];
            ulong readSize;
            fixed (byte* ptr = buff)
            {
                readSize = MaaService.AsstGetImage(handle, ptr, (ulong)buff.Length);
            }

            if (readSize == MaaService.AsstGetNullSize())
            {
                return null;
            }

            try
            {
                // buff is a png data
                var image = new BitmapImage();
                image.BeginInit();
                image.StreamSource = new MemoryStream(buff, 0, (int)readSize);
                image.EndInit();
                return image;
            }
            catch (Exception)
            {
                return null;
            }
        }

        public BitmapImage AsstGetImage()
        {
            return AsstGetImage(_handle);
        }

        private readonly MaaService.CallbackDelegate _callback;

        /// <summary>
        /// Initializes a new instance of the <see cref="AsstProxy"/> class.
        /// </summary>
        public AsstProxy()
        {
            _callback = CallbackFunction;
            _runningState = RunningState.Instance;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="AsstProxy"/> class.
        /// </summary>
        ~AsstProxy()
        {
            if (_handle != AsstHandle.Zero)
            {
                AsstDestroy();
            }
        }

        /// <summary>
        /// 加载全局资源。新版 core 全惰性加载资源，所以可以无脑调用
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool LoadResource()
        {
            _logger.Information("Load Resource");

            string clientType = Instances.SettingsViewModel.ClientType;
            string mainRes = Directory.GetCurrentDirectory();
            string globalResource = mainRes + @"\resource\global\" + clientType;
            string mainCache = Directory.GetCurrentDirectory() + @"\cache";
            string globalCache = mainCache + @"\resource\global\" + clientType;

            bool loaded;
            if (clientType is "" or "Official" or "Bilibili")
            {
                // Read resources first, then read cache
                loaded = LoadResIfExists(mainRes);
                loaded &= LoadResIfExists(mainCache);
            }
            else
            {
                // Read resources first, then read cache
                loaded = LoadResIfExists(mainRes) && LoadResIfExists(globalResource);
                loaded &= LoadResIfExists(mainCache) && LoadResIfExists(globalCache);
            }

            return loaded;

            static bool LoadResIfExists(string path)
            {
                const string Resource = @"\resource";
                if (!Directory.Exists(path + Resource))
                {
                    _logger.Warning($"Resource not found: {path + Resource}");
                    return true;
                }

                _logger.Information($"Load resource: {path + Resource}");
                return AsstLoadResource(path);
            }
        }

        /// <summary>
        /// 初始化。
        /// </summary>
        public void Init()
        {
            bool loaded = LoadResource();

            _handle = MaaService.AsstCreateEx(_callback, AsstHandle.Zero);

            if (loaded == false || _handle == AsstHandle.Zero)
            {
                Execute.OnUIThreadAsync(
                    () =>
                {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("ResourceBroken"), LocalizationHelper.GetString("Error"), iconKey: ResourceToken.FatalGeometry, iconBrushKey: ResourceToken.DangerBrush);
                    Bootstrapper.Shutdown();
                });
            }

            Instances.TaskQueueViewModel.SetInited();
            _runningState.SetIdle(true);
            AsstSetInstanceOption(InstanceOptionKey.TouchMode, Instances.SettingsViewModel.TouchMode);
            AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, Instances.SettingsViewModel.DeploymentWithPause ? "1" : "0");
            AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, Instances.SettingsViewModel.AdbLiteEnabled ? "1" : "0");

            // TODO: 之后把这个 OnUIThread 拆出来
            // ReSharper disable once AsyncVoidLambda
            Execute.OnUIThread(
                async () =>
            {
                if (Instances.SettingsViewModel.RunDirectly)
                {
                    // 如果是直接运行模式，就先让按钮显示为运行
                    _runningState.SetIdle(false);
                }

                await Task.Run(() => Instances.SettingsViewModel.TryToStartEmulator());

                // 一般是点了“停止”按钮了
                if (Instances.TaskQueueViewModel.Stopping)
                {
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                // ReSharper disable once InvertIf
                if (Instances.SettingsViewModel.RunDirectly)
                {
                    // 重置按钮状态，不影响LinkStart判断
                    _runningState.SetIdle(true);
                    Instances.TaskQueueViewModel.LinkStart();
                }
            });
        }

        /// <summary>
        /// Determines the length of the specified string (not including the terminating null character).
        /// </summary>
        /// <param name="ptr">The null-terminated string to be checked.</param>
        /// <returns>
        /// The function returns the length of the string, in characters.
        /// If <paramref name="ptr"/> is <see cref="AsstHandle.Zero"/>, the function returns <c>0</c>.
        /// </returns>
        [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int strlen(AsstHandle ptr);

        private static string PtrToStringCustom(AsstHandle ptr, Encoding enc)
        {
            if (ptr == AsstHandle.Zero)
            {
                return null;
            }

            int len = strlen(ptr);

            if (len == 0)
            {
                return string.Empty;
            }

            byte[] bytes = new byte[len];
            Marshal.Copy(ptr, bytes, 0, len);
            return enc.GetString(bytes);
        }

        private void CallbackFunction(int msg, AsstHandle jsonBuffer, AsstHandle customArg)
        {
            string jsonStr = PtrToStringCustom(jsonBuffer, Encoding.UTF8);

            // Console.WriteLine(json_str);
            var json = (JObject)JsonConvert.DeserializeObject(jsonStr);
            MaaService.ProcCallbackMsg dlg = ProcMsg;
            Execute.OnUIThread(
                () =>
            {
                dlg((AsstMsg)msg, json);
            });
        }

        private AsstHandle _handle;

        private void ProcMsg(AsstMsg msg, JObject details)
        {
            switch (msg)
            {
                case AsstMsg.InternalError:
                    break;

                case AsstMsg.InitFailed:
                    MessageBoxHelper.Show(LocalizationHelper.GetString("InitializationError"), LocalizationHelper.GetString("Error"), iconKey: ResourceToken.FatalGeometry, iconBrushKey: ResourceToken.DangerBrush);
                    Bootstrapper.Shutdown();
                    break;

                case AsstMsg.ConnectionInfo:
                    ProcConnectInfo(details);
                    break;

                case AsstMsg.TaskChainStart:
                    Instances.TaskQueueViewModel.Running = true;
                    goto case AsstMsg.TaskChainExtraInfo; // fallthrough
                case AsstMsg.AllTasksCompleted:
                case AsstMsg.TaskChainError:
                case AsstMsg.TaskChainCompleted:
                case AsstMsg.TaskChainStopped:
                    Instances.TaskQueueViewModel.Running = false;
                    goto case AsstMsg.TaskChainExtraInfo; // fallthrough
                case AsstMsg.TaskChainExtraInfo:
                    ProcTaskChainMsg(msg, details);
                    break;

                case AsstMsg.SubTaskError:
                case AsstMsg.SubTaskStart:
                case AsstMsg.SubTaskCompleted:
                case AsstMsg.SubTaskExtraInfo:
                    ProcSubTaskMsg(msg, details);
                    break;

                case AsstMsg.SubTaskStopped:
                    break;

                default:
                    throw new ArgumentOutOfRangeException(nameof(msg), msg, null);
            }
        }

        public bool Connected { get; set; }

        private string _connectedAdb;
        private string _connectedAddress;

        private void ProcConnectInfo(JObject details)
        {
            var what = details["what"]?.ToString() ?? string.Empty;
            switch (what)
            {
                case "Connected":
                    Connected = true;
                    _connectedAdb = details["details"]?["adb"]?.ToString();
                    _connectedAddress = details["details"]?["address"]?.ToString();
                    Instances.SettingsViewModel.ConnectAddress = _connectedAddress;
                    break;

                case "UnsupportedResolution":
                    Connected = false;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ResolutionNotSupported"), UiLogColor.Error);
                    break;

                case "ResolutionError":
                    Connected = false;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ResolutionAcquisitionFailure"), UiLogColor.Error);
                    break;

                case "Reconnecting":
                    Instances.TaskQueueViewModel.AddLog($"{LocalizationHelper.GetString("TryToReconnect")} ({Convert.ToUInt32(details["details"]["times"]) + 1})", UiLogColor.Error);
                    break;

                case "Reconnected":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReconnectSuccess"));
                    break;

                case "Disconnect":
                    Connected = false;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReconnectFailed"), UiLogColor.Error);
                    if (_runningState.GetIdle())
                    {
                        break;
                    }

                    if (!AsstStop())
                    {
                        _logger.Warning("Failed to stop Asst");
                    }

                    // TODO: 之后把这个 OnUIThread 拆出来
                    // ReSharper disable once AsyncVoidLambda
                    Execute.OnUIThread(
                        async () =>
                    {
                        if (!Instances.SettingsViewModel.RetryOnDisconnected)
                        {
                            return;
                        }

                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TryToStartEmulator"), UiLogColor.Error);
                        TaskQueueViewModel.KillEmulator();
                        await Task.Delay(3000);
                        await Instances.TaskQueueViewModel.Stop();
                        Instances.TaskQueueViewModel.SetStopped();
                        Instances.TaskQueueViewModel.LinkStart();
                    });

                    break;

                case "ScreencapFailed":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ScreencapFailed"), UiLogColor.Error);
                    break;

                case "TouchModeNotAvailable":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TouchModeNotAvailable"), UiLogColor.Error);
                    Connected = false;
                    break;

                case "FastestWayToScreencap":
                    {
                        string costString = details["details"]?["cost"]?.ToString() ?? "???";
                        string method = details["details"]?["method"]?.ToString() ?? "???";

                        StringBuilder fastestScreencapStringBuilder = new();
                        string color = UiLogColor.Trace;
                        if (int.TryParse(costString, out var timeCost))
                        {
                            switch (timeCost)
                            {
                                case > 800:
                                    costString = timeCost.ToString("#,#");
                                    color = UiLogColor.Warning;
                                    break;

                                case > 400:
                                    color = UiLogColor.Warning;
                                    break;
                            }
                        }
                        else
                        {
                            color = UiLogColor.Error;
                        }

                        fastestScreencapStringBuilder.Insert(0, string.Format(LocalizationHelper.GetString("FastestWayToScreencap"), costString, method));
                        Instances.TaskQueueViewModel.AddLog(fastestScreencapStringBuilder.ToString(), color);
                        Instances.CopilotViewModel.AddLog(fastestScreencapStringBuilder.ToString(), color, showTime: false);

                        switch (Instances.SettingsViewModel.ConnectConfig)
                        {
                            case "MuMuEmulator12":
                                if (Instances.SettingsViewModel.MuMuEmulator12Extras.Enable && method != "MumuExtras")
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage"), UiLogColor.Warning);
                                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage"), UiLogColor.Warning, showTime: false);
                                }

                                break;
                        }
                    }

                    break;

                case "ScreencapCost":
                    var screencapCostMin = details["details"]?["min"]?.ToString() ?? "???";
                    var screencapCostAvg = details["details"]?["avg"]?.ToString() ?? "???";
                    var screencapCostMax = details["details"]?["max"]?.ToString() ?? "???";
                    var currentTime = DateTimeOffset.Now.ToString("HH:mm:ss");
                    Instances.SettingsViewModel.ScreencapCost = string.Format(LocalizationHelper.GetString("ScreencapCost"), screencapCostMin, screencapCostAvg, screencapCostMax, currentTime);
                    if (!HasPrintedScreencapWarning && int.TryParse(screencapCostAvg, out var screencapCostAvgInt))
                    {
                        static void AddLog(string message, string color)
                        {
                            Instances.TaskQueueViewModel.AddLog(message, color);
                            Instances.CopilotViewModel.AddLog(message, color, showTime: false);
                            HasPrintedScreencapWarning = true;
                        }

                        switch (screencapCostAvgInt)
                        {
                            case >= 800:
                                AddLog(string.Format(LocalizationHelper.GetString("FastestWayToScreencapErrorTip"), screencapCostAvgInt), UiLogColor.Warning);
                                break;

                            case >= 400:
                                AddLog(string.Format(LocalizationHelper.GetString("FastestWayToScreencapWarningTip"), screencapCostAvgInt), UiLogColor.Warning);
                                break;
                        }
                    }

                    break;
            }
        }

        private void ProcTaskChainMsg(AsstMsg msg, JObject details)
        {
            string taskChain = details["taskchain"]?.ToString() ?? string.Empty;
            switch (taskChain)
            {
                case "CloseDown":
                    return;

                case "Recruit":
                    {
                        if (msg == AsstMsg.TaskChainError)
                        {
                            Instances.RecognizerViewModel.RecruitInfo = LocalizationHelper.GetString("IdentifyTheMistakes");
                            using var toast = new ToastNotification(LocalizationHelper.GetString("IdentifyTheMistakes"));
                            toast.Show();
                        }

                        break;
                    }
            }

            bool isCopilotTaskChain = taskChain == "Copilot" || taskChain == "VideoRecognition";

            switch (msg)
            {
                case AsstMsg.TaskChainStopped:
                    Instances.TaskQueueViewModel.SetStopped();
                    if (isCopilotTaskChain)
                    {
                        _runningState.SetIdle(true);
                    }

                    break;

                case AsstMsg.TaskChainError:
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskError") + taskChain, UiLogColor.Error);
                        using var toast = new ToastNotification(LocalizationHelper.GetString("TaskError") + taskChain);
                        toast.Show();
                        if (isCopilotTaskChain)
                        {
                            // 如果启用战斗列表，需要中止掉剩余的任务
                            if (Instances.CopilotViewModel.UseCopilotList)
                            {
                                if (!AsstStop(false))
                                {
                                    _logger.Warning("Failed to stop Asst");
                                }
                            }

                            _runningState.SetIdle(true);
                            Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("CombatError"), UiLogColor.Error);
                        }

                        if (taskChain == "Fight" && (Instances.TaskQueueViewModel.Stage == "Annihilation"))
                        {
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AnnihilationTaskFailed"), UiLogColor.Warning);
                        }

                        break;
                    }

                case AsstMsg.TaskChainStart:
                    Instances.TaskQueueViewModel.FightTaskRunning = taskChain switch
                    {
                        "Fight" => true,
                        _ => Instances.TaskQueueViewModel.FightTaskRunning,
                    };

                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartTask") + taskChain);
                    break;

                case AsstMsg.TaskChainCompleted:
                    switch (taskChain)
                    {
                        case "Fight":
                            Instances.TaskQueueViewModel.FightTaskRunning = false;
                            break;

                        case "Infrast":
                            Instances.TaskQueueViewModel.InfrastTaskRunning = false;
                            Instances.TaskQueueViewModel.IncreaseCustomInfrastPlanIndex();
                            Instances.TaskQueueViewModel.RefreshCustomInfrastPlanIndexByPeriod();
                            break;

                        case "Mall":
                            {
                                if (Instances.TaskQueueViewModel.Stage != string.Empty && Instances.SettingsViewModel.CreditFightTaskEnabled)
                                {
                                    Instances.SettingsViewModel.LastCreditFightTaskTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString("CreditFight"));
                                }

                                break;
                            }
                    }

                    if (taskChain == "Fight" && SanityReport.HasSanityReport)
                    {
                        var sanityLog = "\n" + string.Format(LocalizationHelper.GetString("CurrentSanity"), SanityReport.Sanity[0], SanityReport.Sanity[1]);
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + taskChain + sanityLog);
                    }
                    else
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + taskChain);
                    }

                    if (isCopilotTaskChain)
                    {
                        if (!Instances.CopilotViewModel.UseCopilotList || Instances.CopilotViewModel.CopilotItemViewModels.All(model => !model.IsChecked))
                        {
                            _runningState.SetIdle(true);
                        }

                        if (Instances.CopilotViewModel.UseCopilotList)
                        {
                            Instances.CopilotViewModel.CopilotTaskSuccess();
                        }

                        Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("CompleteCombat"), UiLogColor.Info);
                    }

                    break;

                case AsstMsg.TaskChainExtraInfo:
                    break;

                case AsstMsg.AllTasksCompleted:
                    bool isMainTaskQueueAllCompleted = false;
                    var taskList = details["finished_tasks"]?.ToObject<AsstTaskId[]>();
                    if (taskList?.Length > 0)
                    {
                        // 有非主界面任务时，不执行任务链结束后操作
                        var minorTaskTypes = new HashSet<TaskType>
                        {
                            TaskType.Copilot,
                            TaskType.RecruitCalc,
                            TaskType.Depot,
                            TaskType.OperBox,
                            TaskType.Gacha,
                        };

                        // 仅有一个任务且为 CloseDown 时，不执行任务链结束后操作
                        if (taskList.Length == 1)
                        {
                            minorTaskTypes.Add(TaskType.CloseDown);
                        }

                        var latestMinorTaskIds = _latestTaskId.Where(i => minorTaskTypes.Contains(i.Key)).Select(i => i.Value);
                        isMainTaskQueueAllCompleted = taskList.All(i => !latestMinorTaskIds.Contains(i));
                    }

                    if (_latestTaskId.ContainsKey(TaskType.Copilot))
                    {
                        if (Instances.SettingsViewModel.CopilotWithScript)
                            {
                                Task.Run(() => Instances.SettingsViewModel.RunScript("EndsWithScript", showLog: false));
                                if (!string.IsNullOrWhiteSpace(Instances.SettingsViewModel.EndsWithScript))
                                {
                                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("EndsWithScript"));
                                }
                            }
                    }

                    bool buyWine = _latestTaskId.ContainsKey(TaskType.Mall) && Instances.SettingsViewModel.DidYouBuyWine();
                    _latestTaskId.Clear();

                    Instances.TaskQueueViewModel.ResetFightVariables();
                    Instances.TaskQueueViewModel.ResetTaskSelection();
                    _runningState.SetIdle(true);
                    Instances.RecognizerViewModel.GachaDone = true;

                    if (isMainTaskQueueAllCompleted)
                    {
                        var allTaskCompleteTitle = LocalizationHelper.GetString("AllTasksComplete");
                        var allTaskCompleteMessage = LocalizationHelper.GetString("AllTaskCompleteContent");
                        var sanityReport = LocalizationHelper.GetString("SanityReport");

                        var configurationPreset = ConfigurationHelper.GetCurrentConfiguration();

                        allTaskCompleteMessage = allTaskCompleteMessage
                            .Replace("{DateTime}", DateTimeOffset.Now.ToString("yyyy-MM-dd HH:mm:ss"))
                            .Replace("{Preset}", configurationPreset);
                        if (SanityReport.HasSanityReport)
                        {
                            var recoveryTime = SanityReport.ReportTime.AddMinutes(SanityReport.Sanity[0] < SanityReport.Sanity[1] ? (SanityReport.Sanity[1] - SanityReport.Sanity[0]) * 6 : 0);
                            sanityReport = sanityReport.Replace("{DateTime}", recoveryTime.ToString("yyyy-MM-dd HH:mm")).Replace("{TimeDiff}", (recoveryTime - DateTimeOffset.Now).ToString(@"h\h\ m\m"));

                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AllTasksComplete") + Environment.NewLine + sanityReport);
                            ExternalNotificationService.Send(allTaskCompleteTitle, allTaskCompleteMessage + Environment.NewLine + sanityReport);
                        }
                        else
                        {
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AllTasksComplete"));
                            ExternalNotificationService.Send(allTaskCompleteTitle, allTaskCompleteMessage);
                        }

                        using (var toast = new ToastNotification(allTaskCompleteTitle))
                        {
                            if (SanityReport.HasSanityReport)
                            {
                                toast.AppendContentText(sanityReport);
                            }

                            toast.Show();
                        }

                        // Instances.TaskQueueViewModel.CheckAndShutdown();
                        Instances.TaskQueueViewModel.CheckAfterCompleted();
                    }

                    if (buyWine)
                    {
                        Instances.SettingsViewModel.LastBuyWineTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                        var result = MessageBoxHelper.Show(
                            LocalizationHelper.GetString("DrunkAndStaggering"),
                            LocalizationHelper.GetString("Burping"),
                            iconKey: "DrunkAndStaggeringGeometry",
                            iconBrushKey: "PallasBrush");
                        if (result == MessageBoxResult.OK)
                        {
                            Instances.SettingsViewModel.Cheers = true;
                            Bootstrapper.ShutdownAndRestartWithoutArgs();
                        }
                    }

                    break;

                case AsstMsg.InternalError:
                    break;

                case AsstMsg.InitFailed:
                    break;

                case AsstMsg.ConnectionInfo:
                    break;

                case AsstMsg.SubTaskError:
                    break;

                case AsstMsg.SubTaskStart:
                    break;

                case AsstMsg.SubTaskCompleted:
                    break;

                case AsstMsg.SubTaskExtraInfo:
                    break;

                case AsstMsg.SubTaskStopped:
                    break;

                default:
                    throw new ArgumentOutOfRangeException(nameof(msg), msg, null);
            }
        }

        private void ProcSubTaskMsg(AsstMsg msg, JObject details)
        {
            // 下面几行注释暂时没用到，先注释起来...
            // string taskChain = details["taskchain"].ToString();
            // string classType = details["class"].ToString();
            switch (msg)
            {
                case AsstMsg.SubTaskError:
                    ProcSubTaskError(details);
                    break;

                case AsstMsg.SubTaskStart:
                    ProcSubTaskStart(details);
                    break;

                case AsstMsg.SubTaskCompleted:
                    ProcSubTaskCompleted(details);
                    break;

                case AsstMsg.SubTaskExtraInfo:
                    ProcSubTaskExtraInfo(details);
                    break;

                case AsstMsg.InternalError:
                    break;

                case AsstMsg.InitFailed:
                    break;

                case AsstMsg.ConnectionInfo:
                    break;

                case AsstMsg.AllTasksCompleted:
                    break;

                case AsstMsg.TaskChainError:
                    break;

                case AsstMsg.TaskChainStart:
                    break;

                case AsstMsg.TaskChainCompleted:
                    break;

                case AsstMsg.TaskChainExtraInfo:
                    break;

                case AsstMsg.TaskChainStopped:
                    break;

                case AsstMsg.SubTaskStopped:
                    break;

                default:
                    throw new ArgumentOutOfRangeException(nameof(msg), msg, null);
            }
        }

        private static void ProcSubTaskError(JObject details)
        {
            string subTask = details["subtask"]?.ToString() ?? string.Empty;
            switch (subTask)
            {
                case "StartGameTask":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FailedToOpenClient"), UiLogColor.Error);
                    break;

                case "AutoRecruitTask":
                    {
                        var whyStr = details.TryGetValue("why", out var why) ? why.ToString() : LocalizationHelper.GetString("ErrorOccurred");
                        Instances.TaskQueueViewModel.AddLog(whyStr + "，" + LocalizationHelper.GetString("HasReturned"), UiLogColor.Error);
                        break;
                    }

                case "RecognizeDrops":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DropRecognitionError"), UiLogColor.Error);
                    break;

                case "ReportToPenguinStats":
                    {
                        var why = details["why"].ToString();
                        Instances.TaskQueueViewModel.AddLog(why + "，" + LocalizationHelper.GetString("GiveUpUploadingPenguins"), UiLogColor.Error);
                        break;
                    }

                case "CheckStageValid":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TheEx"), UiLogColor.Error);
                    break;

                case "BattleFormationTask":
                    {
                        var why = details.TryGetValue("why", out var whyObj) ? whyObj.ToString() : string.Empty;
                        if (why == "OperatorMissing")
                        {
                            var missingOpers = details["details"]?["opers"]?.ToObject<List<List<string>>>();
                            if (missingOpers is not null)
                            {
                                var missingOpersStr = "[" + string.Join("]; [", missingOpers.Select(opers => string.Join(", ", opers))) + "]";
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MissingOperators") + missingOpersStr, UiLogColor.Error);
                            }
                            else
                            {
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MissingOperators"), UiLogColor.Error);
                            }
                        }

                        break;
                    }
            }
        }

        private static void ProcSubTaskStart(JObject details)
        {
            string subTask = details["subtask"]?.ToString() ?? string.Empty;
            switch (subTask)
            {
                case "ProcessTask":
                    {
                        string taskName = details["details"]?["task"]?.ToString();
                        int execTimes = (int)details["details"]["exec_times"];

                        switch (taskName)
                        {
                            case "StartButton2":
                            case "AnnihilationConfirm":
                                StringBuilder missionStartLogBuilder = new();
                                missionStartLogBuilder.AppendLine(LocalizationHelper.GetString("MissionStart") + $" {execTimes} {LocalizationHelper.GetString("UnitTime")}");
                                if (SanityReport.HasSanityReport)
                                {
                                    missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("CurrentSanity"), SanityReport.Sanity[0], SanityReport.Sanity[1]);
                                }

                                if (ExpiringMedicineUsedTimes > 0)
                                {
                                    missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("MedicineUsedTimesWithExpiring"), MedicineUsedTimes, ExpiringMedicineUsedTimes);
                                }
                                else if (MedicineUsedTimes > 0)
                                {
                                    missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("MedicineUsedTimes"), MedicineUsedTimes);
                                }

                                if (StoneUsedTimes > 0)
                                {
                                    missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("StoneUsedTimes"), StoneUsedTimes);
                                }

                                Instances.TaskQueueViewModel.AddLog(missionStartLogBuilder.ToString().TrimEnd(), UiLogColor.Info);
                                break;

                            case "StoneConfirm":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StoneUsed") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                                StoneUsedTimes++;
                                break;

                            case "AbandonAction":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ActingCommandError"), UiLogColor.Error);
                                break;

                            case "RecruitRefreshConfirm":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("LabelsRefreshed"), UiLogColor.Info);
                                break;

                            case "RecruitConfirm":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RecruitConfirm"), UiLogColor.Info);
                                break;

                            case "InfrastDormDoubleConfirmButton":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("InfrastDormDoubleConfirmed"), UiLogColor.Error);
                                break;

                            /* 肉鸽相关 */
                            case "StartExplore":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("BegunToExplore") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                                break;

                            case "ExitThenAbandon":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ExplorationAbandoned"));
                                break;

                            // case "StartAction":
                            //    Instances.TaskQueueViewModel.AddLog("开始战斗");
                            //    break;
                            case "MissionCompletedFlag":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightCompleted"));
                                break;

                            case "MissionFailedFlag":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightFailed"));
                                break;

                            case "StageTraderEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Trader"));
                                break;

                            case "StageSafeHouseEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("SafeHouse"));
                                break;

                            // case "StageBoonsEnter":
                            //    Instances.TaskQueueViewModel.AddLog("古堡馈赠");
                            //    break;
                            case "StageCombatDpsEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CombatDps"));
                                break;

                            case "StageEmergencyDps":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("EmergencyDps"));
                                break;

                            case "StageDreadfulFoe":
                            case "StageDreadfulFoe-5Enter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DreadfulFoe"));
                                break;

                            case "StageTraderInvestSystemFull":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("UpperLimit"), UiLogColor.Info);
                                break;

                            case "OfflineConfirm":
                                if (Instances.SettingsViewModel.AutoRestartOnDrop)
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("GameDrop"), UiLogColor.Warning);
                                }
                                else
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("GameDropNoRestart"), UiLogColor.Warning);
                                    using var toast = new ToastNotification(LocalizationHelper.GetString("GameDropNoRestart"));
                                    toast.Show();
                                    _ = Instances.TaskQueueViewModel.Stop();
                                }

                                break;

                            case "GamePass":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeGamePass"), UiLogColor.RareOperator);
                                break;

                            case "BattleStartAll":
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MissionStart"), UiLogColor.Info);
                                break;

                            case "StageTraderSpecialShoppingAfterRefresh":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeSpecialItemBought"), UiLogColor.RareOperator);
                                break;
                        }

                        break;
                    }

                case "CombatRecordRecognitionTask":
                    {
                        string what = details["what"]?.ToString();
                        if (!string.IsNullOrEmpty(what))
                        {
                            Instances.CopilotViewModel.AddLog(what);
                        }

                        break;
                    }
            }
        }

        private void ProcSubTaskCompleted(JObject details)
        {
            // DoNothing
            _ = details;
        }

        private static void ProcSubTaskExtraInfo(JObject details)
        {
            string taskChain = details["taskchain"]?.ToString() ?? string.Empty;
            switch (taskChain)
            {
                case "Recruit":
                    ProcRecruitCalcMsg(details);
                    break;

                case "VideoRecognition":
                    ProcVideoRecMsg(details);
                    break;
            }

            var subTaskDetails = details["details"];
            switch (taskChain)
            {
                case "Depot":
                    Instances.RecognizerViewModel.DepotParse((JObject)subTaskDetails);
                    break;

                case "OperBox":
                    Instances.RecognizerViewModel.OperBoxParse((JObject)subTaskDetails);
                    break;
            }

            string what = details["what"]?.ToString() ?? string.Empty;
            switch (what)
            {
                case "StageDrops":
                    {
                        string allDrops = string.Empty;
                        var statistics = subTaskDetails["stats"] ?? new JArray();
                        int curTimes = (int)(subTaskDetails["cur_times"] ?? -1);

                        foreach (var item in statistics)
                        {
                            string itemName = item["itemName"]?.ToString();
                            if (itemName == "furni")
                            {
                                itemName = LocalizationHelper.GetString("FurnitureDrop");
                            }

                            int totalQuantity = (int)item["quantity"];
                            int addQuantity = (int)item["addQuantity"];

                            allDrops += $"{itemName} : {totalQuantity:#,#}";
                            if (addQuantity > 0)
                            {
                                allDrops += $" (+{addQuantity:#,#})";
                            }

                            allDrops += "\n";
                        }

                        allDrops = allDrops.EndsWith('\n') ? allDrops.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");
                        Instances.TaskQueueViewModel.AddLog(
                            LocalizationHelper.GetString("TotalDrop") + "\n" + allDrops +
                            (curTimes >= 0 ? $"\n{LocalizationHelper.GetString("CurTimes")} : {curTimes}" : string.Empty));
                        break;
                    }

                case "EnterFacility":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ThisFacility") + subTaskDetails["facility"] + " " + (int)subTaskDetails["index"]);
                    break;

                case "ProductIncorrect":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ProductIncorrect"), UiLogColor.Error);
                    break;

                case "ProductUnknown":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ProductUnknown"), UiLogColor.Error);
                    break;

                case "ProductChanged":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ProductChanged"), UiLogColor.Info);
                    break;

                case "RecruitTagsDetected":
                    {
                        var tags = subTaskDetails["tags"] ?? new JArray();
                        string logContent = tags.Select(tagName => tagName.ToString())
                            .Aggregate(string.Empty, (current, tagStr) => current + (tagStr + "\n"));

                        logContent = logContent.EndsWith('\n') ? logContent.TrimEnd('\n') : LocalizationHelper.GetString("Error");
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RecruitingResults") + "\n" + logContent);

                        break;
                    }

                case "RecruitSpecialTag":
                    {
                        string special = subTaskDetails["tag"]?.ToString();
                        if (special == "支援机械" && Instances.SettingsViewModel.NotChooseLevel1 == false)
                        {
                            break;
                        }

                        using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                        toast.AppendContentText(special).ShowRecruit();

                        break;
                    }

                case "RecruitRobotTag":
                    {
                        string special = subTaskDetails["tag"]?.ToString();
                        using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                        toast.AppendContentText(special).ShowRecruitRobot();

                        break;
                    }

                case "RecruitResult":
                    {
                        int level = (int)subTaskDetails["level"];
                        if (level >= 5)
                        {
                            using (var toast = new ToastNotification(string.Format(LocalizationHelper.GetString("RecruitmentOfStar"), level)))
                            {
                                toast.AppendContentText(new string('★', level)).ShowRecruit(row: 2);
                            }

                            Instances.TaskQueueViewModel.AddLog(level + " ★ Tags", UiLogColor.RareOperator, "Bold");
                        }
                        else
                        {
                            Instances.TaskQueueViewModel.AddLog(level + " ★ Tags", UiLogColor.Info);
                        }

                        /*
                        bool robot = (bool)subTaskDetails["robot"];
                        if (robot)
                        {
                            using (var toast = new ToastNotification(Localization.GetString("RecruitmentOfBot")))
                            {
                                toast.AppendContentText(new string('★', 1)).ShowRecruitRobot(row: 2);
                            }

                            Instances.TaskQueueViewModel.AddLog(1 + " ★ Tag", LogColor.RobotOperator, "Bold");
                        }
                        */

                        break;
                    }

                case "RecruitTagsSelected":
                    {
                        var selected = subTaskDetails["tags"] ?? new JArray();
                        string selectedLog = selected.Aggregate(string.Empty, (current, tag) => current + (tag + "\n"));

                        selectedLog = selectedLog.EndsWith('\n') ? selectedLog.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");

                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Choose") + " Tags：\n" + selectedLog);

                        break;
                    }

                case "RecruitTagsRefreshed":
                    {
                        int refreshCount = (int)subTaskDetails["count"];
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Refreshed") + refreshCount + LocalizationHelper.GetString("UnitTime"));
                        break;
                    }

                case "RecruitNoPermit":
                    {
                        bool continueRefresh = (bool)subTaskDetails["continue"];
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString(continueRefresh ? "ContinueRefresh" : "NoRecruitmentPermit"));
                        break;
                    }

                case "NotEnoughStaff":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NotEnoughStaff"), UiLogColor.Error);
                    break;

                case "CreditFullOnlyBuyDiscount":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CreditFullOnlyBuyDiscount") + subTaskDetails["credit"], UiLogColor.Message);
                        break;
                    }

                case "RoguelikeInvestment":
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeInvestment"), subTaskDetails["count"], subTaskDetails["total"], subTaskDetails["deposit"]), UiLogColor.Info);
                    break;

                case "RoguelikeSettlement":
                    {// 肉鸽结算
                        var report = subTaskDetails;
                        bool pass = (bool)report["game_pass"];
                        StringBuilder roguelikeInfo = new();
                        roguelikeInfo.AppendFormat(
                            LocalizationHelper.GetString("RoguelikeSettlement"),
                            pass ? "✓" : "✗",
                            report["floor"],
                            report["step"],
                            report["combat"],
                            report["emergency"],
                            report["boss"],
                            report["recruit"],
                            report["collection"],
                            report["difficulty"],
                            report["score"],
                            report["exp"],
                            report["skill"]);

                        Instances.TaskQueueViewModel.AddLog(roguelikeInfo.ToString(), UiLogColor.Message);
                        break;
                    }

                /* Roguelike */
                case "StageInfo":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartCombat") + subTaskDetails["name"]);
                        if (Instances.SettingsViewModel.RoguelikeDelayAbortUntilCombatComplete)
                        {
                            Instances.TaskQueueViewModel.RoguelikeInCombatAndShowWait = true;
                        }

                        break;
                    }

                case "StageInfoError":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageInfoError"), UiLogColor.Error);
                    break;

                case "RoguelikeCombatEnd":
                    // 肉鸽战斗结束，无论成功与否
                    Instances.TaskQueueViewModel.RoguelikeInCombatAndShowWait = false;
                    break;

                case "RoguelikeEvent":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeEvent") + $" {subTaskDetails["name"]}");
                    break;

                case "FoldartalGainOcrNextLevel":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FoldartalGainOcrNextLevel") + $" {subTaskDetails["foldartal"]}");
                    break;

                case "PenguinId":
                    {
                        string id = subTaskDetails["id"]?.ToString();
                        Instances.SettingsViewModel.PenguinId = id;

                        break;
                    }

                case "BattleFormation":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormation") + "\n" + JsonConvert.SerializeObject(subTaskDetails["formation"]));
                    break;

                case "BattleFormationSelected":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormationSelected") + subTaskDetails["selected"]);
                    break;

                case "CopilotAction":
                    {
                        string doc = subTaskDetails["doc"]?.ToString();
                        if (!string.IsNullOrEmpty(doc))
                        {
                            string color = subTaskDetails["doc_color"]?.ToString();
                            Instances.CopilotViewModel.AddLog(doc, string.IsNullOrEmpty(color) ? UiLogColor.Message : color);
                        }

                        Instances.CopilotViewModel.AddLog(
                            string.Format(
                                LocalizationHelper.GetString("CurrentSteps"),
                                subTaskDetails["action"],
                                subTaskDetails["target"]));

                        break;
                    }

                case "CopilotListLoadTaskFileSuccess":
                    Instances.CopilotViewModel.AddLog($"Parse {subTaskDetails["file_name"]}[{subTaskDetails["stage_name"]}] Success");
                    break;

                case "SSSStage":
                    Instances.CopilotViewModel.AddLog("CurrentStage: " + subTaskDetails["stage"], UiLogColor.Info);
                    break;

                case "SSSSettlement":
                    Instances.CopilotViewModel.AddLog(details["why"].ToString(), UiLogColor.Info);
                    break;

                case "SSSGamePass":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("SSSGamePass"), UiLogColor.RareOperator);
                    break;

                case "UnsupportedLevel":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("UnsupportedLevel") + subTaskDetails["level"], UiLogColor.Error);
                    break;

                case "CustomInfrastRoomGroupsMatch":
                    // 选用xxx组编组
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomGroupsMatch") + subTaskDetails["group"]);
                    break;

                case "CustomInfrastRoomGroupsMatchFailed":
                    // 干员编组匹配失败
                    JArray groups = (JArray)subTaskDetails["groups"];
                    if (groups != null)
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomGroupsMatchFailed") + string.Join(", ", groups));
                    }

                    break;

                case "CustomInfrastRoomOperators":
                    string nameStr = (subTaskDetails["names"] ?? new JArray())
                        .Aggregate(string.Empty, (current, name) => current + name + ", ");

                    if (nameStr != string.Empty)
                    {
                        nameStr = nameStr.Remove(nameStr.Length - 2);
                    }

                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomOperators") + nameStr);
                    break;

                case "InfrastTrainingIdle":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TrainingIdle"));
                    break;

                case "InfrastTrainingCompleted":
                    {
                        var operatorName = DataHelper.GetLocalizedCharacterName(subTaskDetails["operator"]?.ToString()) ?? "UnKnown";
                        var skillName = subTaskDetails["skill"]?.ToString() ?? "UnKnown";
                        Instances.TaskQueueViewModel.AddLog(
                            $"[{operatorName}]{skillName}\n" +
                            $"{LocalizationHelper.GetString("TrainingLevel")}: {(int)subTaskDetails["level"]} {LocalizationHelper.GetString("TrainingCompleted")}",
                            UiLogColor.Info);
                        break;
                    }

                case "InfrastTrainingTimeLeft":
                    {
                        var operatorName = DataHelper.GetLocalizedCharacterName(subTaskDetails["operator"]?.ToString()) ?? "UnKnown";
                        var skillName = subTaskDetails["skill"]?.ToString() ?? "UnKnown";
                        Instances.TaskQueueViewModel.AddLog(
                            $"[{operatorName}]{skillName}\n" +
                            $"{LocalizationHelper.GetString("TrainingLevel")}: {(int)subTaskDetails["level"]}\n" +
                            $"{LocalizationHelper.GetString("TrainingTimeLeft")}: {subTaskDetails["hh"]}:{subTaskDetails["mm"]}:{subTaskDetails["ss"]}",
                            UiLogColor.Info);
                        break;
                    }

                /* 生息演算 */
                case "ReclamationReport":
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetString("AlgorithmFinish") + "\n" +
                        LocalizationHelper.GetString("AlgorithmBadge") + ": " + $"{(int)subTaskDetails["total_badges"]}(+{(int)subTaskDetails["badges"]})" + "\n" +
                        LocalizationHelper.GetString("AlgorithmConstructionPoint") + ": " + $"{(int)subTaskDetails["total_construction_points"]}(+{(int)subTaskDetails["construction_points"]})");
                    break;

                case "ReclamationProcedureStart":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MissionStart") + $" {(int)subTaskDetails["times"]} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                    break;

                case "ReclamationSmeltGold":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AlgorithmDoneSmeltGold") + $" {(int)subTaskDetails["times"]} " + LocalizationHelper.GetString("UnitTime"));
                    break;

                case "SanityBeforeStage":
                    SanityReport.HasSanityReport = false;
                    var sanityReport = (JObject)subTaskDetails;
                    if (sanityReport is null || !sanityReport.ContainsKey("current_sanity") || !sanityReport.ContainsKey("max_sanity") || !sanityReport.ContainsKey("report_time"))
                    {
                        break;
                    }

                    int sanityCur = sanityReport.TryGetValue("current_sanity", out var sanityCurToken) ? (int)sanityCurToken : -1;
                    int sanityMax = sanityReport.TryGetValue("max_sanity", out var sanityMaxToken) ? (int)sanityMaxToken : -1;
                    var reportTime = sanityReport.TryGetValue("report_time", out var reportTimeToken) ? (string)reportTimeToken : string.Empty;
                    if (sanityCur < 0 || sanityMax < 1 || reportTime?.Length < 12)
                    {
                        break;
                    }

                    SanityReport.Sanity[0] = sanityCur;
                    SanityReport.Sanity[1] = sanityMax;
                    try
                    {
                        SanityReport.ReportTime = DateTimeOffset.Parse(reportTime ?? string.Empty);
                    }
                    catch (FormatException)
                    {
                        _logger.Error("SanityReport analyze failed: report time format is invalid, {time}.", reportTime);
                        break;
                    }

                    SanityReport.HasSanityReport = true;
                    break;

                case "UseMedicine":
                    var medicineReport = (JObject)subTaskDetails;
                    if (medicineReport is null || !medicineReport.ContainsKey("is_expiring") || !medicineReport.ContainsKey("count"))
                    {
                        break;
                    }

                    var isExpiringMedicine = medicineReport.TryGetValue("is_expiring", out var isExpiringMedicineToken) && (bool)isExpiringMedicineToken;
                    int medicineCount = medicineReport.TryGetValue("count", out var medicineCountToken) ? (int)medicineCountToken : -1;

                    if (medicineCount == -1)
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MedicineUsed") + " Unknown times", UiLogColor.Error);
                        break;
                    }

                    string medicineLog;
                    if (!isExpiringMedicine)
                    {
                        MedicineUsedTimes += medicineCount;
                        medicineLog = LocalizationHelper.GetString("MedicineUsed") + $" {MedicineUsedTimes}(+{medicineCount})";
                    }
                    else
                    {
                        ExpiringMedicineUsedTimes += medicineCount;
                        medicineLog = LocalizationHelper.GetString("ExpiringMedicineUsed") + $" {ExpiringMedicineUsedTimes}(+{medicineCount})";
                    }

                    Instances.TaskQueueViewModel.AddLog(medicineLog, UiLogColor.Info);
                    break;

                case "StageQueueUnableToAgent":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageQueue") + $" {subTaskDetails["stage_code"]} " + LocalizationHelper.GetString("UnableToAgent"), UiLogColor.Info);
                    break;

                case "StageQueueMissionCompleted":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageQueue") + $" {subTaskDetails["stage_code"]} - {subTaskDetails["stars"]} ★", UiLogColor.Info);
                    break;

                case "AccountSwitch":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AccountSwitch") + $" {subTaskDetails["current_account"]} -->> {subTaskDetails["account_name"]}", UiLogColor.Info);
                    break;
            }
        }

        private static void ProcRecruitCalcMsg(JObject details)
        {
            Instances.RecognizerViewModel.ProcRecruitMsg(details);
        }

        private static void ProcVideoRecMsg(JObject details)
        {
            string what = details["what"]?.ToString() ?? string.Empty;
            switch (what)
            {
                case "Finished":
                    var filename = details["details"]?["filename"];
                    Instances.CopilotViewModel.AddLog("Save to: " + filename, UiLogColor.Info, showTime: false);

                    // string p = @"C:\tmp\this path contains spaces, and,commas\target.txt";
                    string args = $"/e, /select, \"{filename}\"";

                    ProcessStartInfo info = new()
                    {
                        FileName = "explorer",
                        Arguments = args,
                    };
                    Process.Start(info);
                    break;
            }
        }

        public bool AsstSetInstanceOption(InstanceOptionKey key, string value)
        {
            return AsstSetInstanceOption(_handle, (AsstInstanceOptionKey)key, value);
        }

        private static readonly bool _forcedReloadResource = File.Exists("DEBUG") || File.Exists("DEBUG.txt");

        /// <summary>
        /// 检查端口是否有效。
        /// </summary>
        /// <param name="address">连接地址。</param>
        /// <returns>是否有效。</returns>
        private static bool IfPortEstablished(string address)
        {
            if (string.IsNullOrEmpty(address) || !address.Contains(':'))
            {
                return false;
            }

            // normal -> [host]:[port]
            string[] hostAndPort = address.Split(':');
            if (hostAndPort.Length != 2)
            {
                return false;
            }

            string host = hostAndPort[0].Equals("emulator") ? "127.0.0.1" : hostAndPort[0];
            if (!int.TryParse(hostAndPort[1], out int port))
            {
                return false;
            }

            using var client = new TcpClient();
            try
            {
                IAsyncResult result = client.BeginConnect(host, port, null, null);
                bool success = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(.5));

                if (success)
                {
                    client.EndConnect(result);
                    return true;
                }

                client.Close();
                return false;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// 连接模拟器。
        /// </summary>
        /// <param name="error">具体的连接错误。</param>
        /// <returns>是否成功。</returns>
        public bool AsstConnect(ref string error)
        {
            switch (Instances.SettingsViewModel.ConnectConfig)
            {
                case "MuMuEmulator12":
                    AsstSetConnectionExtrasMuMu12(Instances.SettingsViewModel.MuMuEmulator12Extras.Config, ref error);
                    break;
            }

            if (Instances.SettingsViewModel.AutoDetectConnection)
            {
                string bsHvAddress = Instances.SettingsViewModel.TryToSetBlueStacksHyperVAddress();

                if (string.Equals(Instances.SettingsViewModel.ConnectAddress, bsHvAddress))
                {
                    // 防止bsHvAddress和connectAddress重合
                    bsHvAddress = string.Empty;
                }

                // tcp连接测试端口是否有效，超时时间500ms
                // 如果是本地设备，没有冒号
                bool adbResult =
                    (!Instances.SettingsViewModel.ConnectAddress.Contains(':') &&
                    !string.IsNullOrEmpty(Instances.SettingsViewModel.ConnectAddress)) ||
                    IfPortEstablished(Instances.SettingsViewModel.ConnectAddress);
                bool bsResult = IfPortEstablished(bsHvAddress);

                if (adbResult)
                {
                    error = string.Empty;
                }
                else if (bsResult)
                {
                    if (string.IsNullOrEmpty(Instances.SettingsViewModel.AdbPath))
                    {
                        Instances.SettingsViewModel.DetectAdbConfig(ref error);
                    }

                    Instances.SettingsViewModel.ConnectAddress = bsHvAddress;
                    if (string.IsNullOrEmpty(error))
                    {
                        error = string.Empty;
                    }
                }
                else if (Instances.SettingsViewModel.DetectAdbConfig(ref error))
                {
                    // https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8547
                    // DetectAdbConfig 会把 ConnectAddress 变成第一个不是 emulator 开头的地址，可能会存在多开问题
                    error = string.Empty;
                }
                else
                {
                    return false;
                }
            }

            if (Connected && _connectedAdb == Instances.SettingsViewModel.AdbPath &&
                _connectedAddress == Instances.SettingsViewModel.ConnectAddress)
            {
                _logger.Information($"Already connected to {_connectedAdb} {_connectedAddress}");
                if (!_forcedReloadResource)
                {
                    return true;
                }

                _logger.Information("Forced reload resource");
                if (!LoadResource())
                {
                    error = "Load Resource Failed";
                    return false;
                }

                Execute.OnUIThreadAsync(
                    () =>
                {
                    using var toast = new ToastNotification("Auto Reload");
                    toast.Show();
                });

                return true;
            }

            bool ret = AsstConnect(_handle, Instances.SettingsViewModel.AdbPath, Instances.SettingsViewModel.ConnectAddress, Instances.SettingsViewModel.ConnectConfig);

            // 尝试默认的备选端口
            if (!ret && Instances.SettingsViewModel.AutoDetectConnection)
            {
                if (Instances.SettingsViewModel.DefaultAddress.TryGetValue(Instances.SettingsViewModel.ConnectConfig, out var value))
                {
                    foreach (var address in value
                                 .TakeWhile(_ => !_runningState.GetIdle()))
                    {
                        ret = AsstConnect(_handle, Instances.SettingsViewModel.AdbPath, address, Instances.SettingsViewModel.ConnectConfig);
                        if (!ret)
                        {
                            continue;
                        }

                        Instances.SettingsViewModel.ConnectAddress = address;
                        break;
                    }
                }
                else
                {
                    Execute.OnUIThreadAsync(
                        () =>
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AutoDetectConnectionNotSupported"), UiLogColor.Error);
                    });
                }
            }

            if (ret)
            {
                if (!Instances.SettingsViewModel.AlwaysAutoDetectConnection)
                {
                    Instances.SettingsViewModel.AutoDetectConnection = false;
                }
            }
            else
            {
                error = LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("CheckSettings");
            }

            return ret;
        }

        private AsstTaskId AsstAppendTaskWithEncoding(string type, JObject taskParams = null)
        {
            taskParams ??= new();
            return AsstAppendTask(_handle, type, JsonConvert.SerializeObject(taskParams));
        }

        private bool AsstSetTaskParamsWithEncoding(AsstTaskId id, JObject taskParams = null)
        {
            if (id == 0)
            {
                return false;
            }

            taskParams ??= new();
            return AsstSetTaskParams(_handle, id, JsonConvert.SerializeObject(taskParams));
        }

        [SuppressMessage("ReSharper", "UnusedMember.Local")]
        private enum TaskType
        {
            StartUp,
            CloseDown,
            Fight,
            FightRemainingSanity,
            Recruit,
            Infrast,
            Mall,
            Award,
            Roguelike,
            RecruitCalc,
            Copilot,
            VideoRec,
            Depot,
            OperBox,
            Gacha,
            ReclamationAlgorithm,
            ReclamationAlgorithm2,
        }

        private readonly Dictionary<TaskType, AsstTaskId> _latestTaskId = new();

        private static JObject SerializeFightTaskParams(
            string stage,
            int maxMedicine,
            int maxStone,
            int maxTimes,
            int series,
            string dropsItemId,
            int dropsItemQuantity,
            bool isMainFight = true)
        {
            var taskParams = new JObject
            {
                ["stage"] = stage,
                ["medicine"] = maxMedicine,
                ["stone"] = maxStone,
                ["times"] = maxTimes,
                ["series"] = series,
                ["report_to_penguin"] = Instances.SettingsViewModel.EnablePenguin,
                ["report_to_yituliu"] = Instances.SettingsViewModel.EnableYituliu,
            };
            if (dropsItemQuantity != 0 && !string.IsNullOrWhiteSpace(dropsItemId))
            {
                taskParams["drops"] = new JObject
                {
                    [dropsItemId] = dropsItemQuantity,
                };
            }

            taskParams["client_type"] = Instances.SettingsViewModel.ClientType;
            taskParams["penguin_id"] = Instances.SettingsViewModel.PenguinId;
            taskParams["DrGrandet"] = Instances.SettingsViewModel.IsDrGrandet;
            taskParams["expiring_medicine"] = isMainFight && Instances.SettingsViewModel.UseExpiringMedicine ? 9999 : 0;
            taskParams["server"] = Instances.SettingsViewModel.ServerType;
            return taskParams;
        }

        /// <summary>
        /// 刷理智。
        /// </summary>
        /// <param name="stage">关卡名。</param>
        /// <param name="maxMedicine">最大使用理智药数量。</param>
        /// <param name="maxStone">最大吃石头数量。</param>
        /// <param name="maxTimes">指定次数。</param>
        /// <param name="series">连战次数。</param>
        /// <param name="dropsItemId">指定掉落 ID。</param>
        /// <param name="dropsItemQuantity">指定掉落数量。</param>
        /// <param name="isMainFight">是否是主任务，决定c#侧是否记录任务id</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendFight(string stage, int maxMedicine, int maxStone, int maxTimes, int series, string dropsItemId, int dropsItemQuantity, bool isMainFight = true)
        {
            var taskParams = SerializeFightTaskParams(stage, maxMedicine, maxStone, maxTimes, series, dropsItemId, dropsItemQuantity, isMainFight);
            AsstTaskId id = AsstAppendTaskWithEncoding("Fight", taskParams);
            if (isMainFight)
            {
                _latestTaskId[TaskType.Fight] = id;
            }
            else
            {
                _latestTaskId[TaskType.FightRemainingSanity] = id;
            }

            return id != 0;
        }

        /// <summary>
        /// 设置刷理智任务参数。
        /// </summary>
        /// <param name="stage">关卡名。</param>
        /// <param name="maxMedicine">最大使用理智药数量。</param>
        /// <param name="maxStone">最大吃石头数量。</param>
        /// <param name="maxTimes">指定次数。</param>
        /// <param name="series">连战次数。</param>
        /// <param name="dropsItemId">指定掉落 ID。</param>
        /// <param name="dropsItemQuantity">指定掉落数量。</param>
        /// <param name="isMainFight">是否是主任务，决定c#侧是否记录任务id</param>
        /// <returns>是否成功。</returns>
        public bool AsstSetFightTaskParams(string stage, int maxMedicine, int maxStone, int maxTimes, int series, string dropsItemId, int dropsItemQuantity, bool isMainFight = true)
        {
            var type = isMainFight ? TaskType.Fight : TaskType.FightRemainingSanity;
            if (!_latestTaskId.ContainsKey(type))
            {
                return false;
            }

            var id = _latestTaskId[type];
            if (id == 0)
            {
                return false;
            }

            var taskParams = SerializeFightTaskParams(stage, maxMedicine, maxStone, maxTimes, series, dropsItemId, dropsItemQuantity);
            return AsstSetTaskParamsWithEncoding(id, taskParams);
        }

        /// <summary>
        /// 领取奖励。
        /// </summary>
        /// <param name="award">是否领取每日/每周任务奖励</param>
        /// <param name="mail">是否领取所有邮件奖励</param>
        /// <param name="recruit">是否进行每日免费单抽</param>
        /// <param name="orundum">是否领取幸运墙合成玉奖励</param>
        /// <param name="mining">是否领取限时开采许可合成玉奖励</param>
        /// <param name="specialaccess">是否领取五周年赠送月卡奖励</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendAward(bool award, bool mail, bool recruit, bool orundum, bool mining, bool specialaccess)
        {
            var taskParams = new JObject
            {
                ["award"] = award,
                ["mail"] = mail,
                ["recruit"] = recruit,
                ["orundum"] = orundum,
                ["mining"] = mining,
                ["specialaccess"] = specialaccess,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("Award", taskParams);
            _latestTaskId[TaskType.Award] = id;
            return id != 0;
        }

        /// <summary>
        /// 开始唤醒。
        /// </summary>
        /// <param name="clientType">客户端版本。</param>
        /// <param name="enable">是否自动启动客户端。</param>
        /// <param name="accountName">需要切换到的登录名，留空以禁用</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendStartUp(string clientType, bool enable, string accountName)
        {
            var taskParams = new JObject
            {
                ["client_type"] = clientType,
                ["start_game_enabled"] = enable,
                ["account_name"] = accountName,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("StartUp", taskParams);
            _latestTaskId[TaskType.StartUp] = id;
            return id != 0;
        }

        public bool AsstAppendCloseDown()
        {
            if (!AsstStop())
            {
                _logger.Warning("Failed to stop Asst");
            }

            AsstTaskId id = AsstAppendTaskWithEncoding("CloseDown");
            _latestTaskId[TaskType.CloseDown] = id;
            return id != 0;
        }

        /// <summary>
        /// <c>CloseDown</c> 任务。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartCloseDown()
        {
            return AsstAppendCloseDown() && AsstStart();
        }

        public bool AsstBackToHome()
        {
            return MaaService.AsstBackToHome(_handle);
        }

        /// <summary>
        /// 领取信用及商店购物。
        /// </summary>
        /// <param name="creditFight">是否信用战斗。</param>
        /// <param name="selectFormation">信用战斗选择编队</param>
        /// <param name="visitFriends">是否访问好友。</param>
        /// <param name="withShopping">是否购物。</param>
        /// <param name="firstList">优先购买列表。</param>
        /// <param name="blacklist">黑名单列表。</param>
        /// <param name="forceShoppingIfCreditFull">是否在信用溢出时无视黑名单</param>
        /// <param name="onlyBuyDiscount">只购买折扣信用商品(未打折的白名单物品仍会购买)。</param>
        /// <param name="reserveMaxCredit">设置300以下信用点停止购买商品。</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendMall(bool creditFight, int selectFormation, bool visitFriends, bool withShopping, string[] firstList, string[] blacklist, bool forceShoppingIfCreditFull, bool onlyBuyDiscount, bool reserveMaxCredit)
        {
            var taskParams = new JObject
            {
                ["credit_fight"] = creditFight,
                ["select_formation"] = selectFormation,
                ["visit_friends"] = visitFriends,
                ["shopping"] = withShopping,
                ["buy_first"] = new JArray(firstList),
                ["blacklist"] = new JArray(blacklist),
                ["force_shopping_if_credit_full"] = forceShoppingIfCreditFull,
                ["only_buy_discount"] = onlyBuyDiscount,
                ["reserve_max_credit"] = reserveMaxCredit,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("Mall", taskParams);
            _latestTaskId[TaskType.Mall] = id;
            return id != 0;
        }

        /// <summary>
        /// 公开招募。
        /// </summary>
        /// <param name="maxTimes">加急次数，仅在 <paramref name="useExpedited"/> 为 <see langword="true"/> 时有效。</param>
        /// <param name="firstTags">首选 Tags，仅在 Tag 等级为 3 时有效。</param>
        /// <param name="selectLevel">会去点击标签的 Tag 等级。</param>
        /// <param name="confirmLevel">会去点击确认的 Tag 等级。若仅公招计算，可设置为空数组。</param>
        /// <param name="needRefresh">是否刷新三星 Tags。</param>
        /// <param name="needForceRefresh">无招募许可时是否继续尝试刷新 Tags。</param>
        /// <param name="useExpedited">是否使用加急许可。</param>
        /// <param name="selectExtraTagsMode">
        /// 公招选择额外tag的模式。可用值包括：
        /// <list type="bullet">
        ///     <item>
        ///         <term><c>0</c></term>
        ///         <description>默认不选择额外tag。</description>
        ///     </item>
        ///     <item>
        ///         <term><c>1</c></term>
        ///         <description>选满至3个tag。</description>
        ///     </item>
        ///     <item>
        ///         <term><c>2</c></term>
        ///         <description>尽可能多选且只选四星以上的tag。</description>
        ///     </item>
        /// </list>
        /// </param>
        /// <param name="skipRobot">是否在识别到小车词条时跳过。</param>
        /// <param name="isLevel3UseShortTime">三星Tag是否使用短时间（7:40）</param>
        /// <param name="isLevel3UseShortTime2">三星Tag是否使用短时间（1:00）</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendRecruit(
            int maxTimes,
            string[] firstTags,
            int[] selectLevel,
            int[] confirmLevel,
            bool needRefresh,
            bool needForceRefresh,
            bool useExpedited,
            int selectExtraTagsMode,
            bool skipRobot,
            bool isLevel3UseShortTime,
            bool isLevel3UseShortTime2 = false)
        {
            var taskParams = new JObject
            {
                ["refresh"] = needRefresh,
                ["force_refresh"] = needForceRefresh,
                ["select"] = new JArray(selectLevel),
                ["confirm"] = new JArray(confirmLevel),
                ["times"] = maxTimes,
                ["set_time"] = true,
                ["expedite"] = useExpedited,
                ["extra_tags_mode"] = selectExtraTagsMode,
                ["expedite_times"] = maxTimes,
                ["skip_robot"] = skipRobot,
                ["first_tags"] = new JArray(firstTags),
            };
            if (isLevel3UseShortTime)
            {
                taskParams["recruitment_time"] = new JObject
                {
                    ["3"] = 460, // 7:40
                };
            }
            else if (isLevel3UseShortTime2)
            {
                taskParams["recruitment_time"] = new JObject
                {
                    ["3"] = 60, // 1:00
                };
            }

            taskParams["report_to_penguin"] = Instances.SettingsViewModel.EnablePenguin;
            taskParams["report_to_yituliu"] = Instances.SettingsViewModel.EnableYituliu;
            taskParams["penguin_id"] = Instances.SettingsViewModel.PenguinId;
            taskParams["server"] = Instances.SettingsViewModel.ServerType;

            AsstTaskId id = AsstAppendTaskWithEncoding("Recruit", taskParams);
            _latestTaskId[TaskType.Recruit] = id;
            return id != 0;
        }

        private static JObject SerializeInfrastTaskParams(
            IEnumerable<string> order,
            string usesOfDrones,
            bool continueTraining,
            double dormThreshold,
            bool dormFilterNotStationedEnabled,
            bool dormDormTrustEnabled,
            bool originiumShardAutoReplenishment,
            bool isCustom,
            string filename,
            int planIndex)
        {
            var taskParams = new JObject
            {
                ["facility"] = new JArray(order.ToArray<object>()),
                ["drones"] = usesOfDrones,
                ["continue_training"] = continueTraining,
                ["threshold"] = dormThreshold,
                ["dorm_notstationed_enabled"] = dormFilterNotStationedEnabled,
                ["dorm_trust_enabled"] = dormDormTrustEnabled,
                ["replenish"] = originiumShardAutoReplenishment,
                ["mode"] = isCustom ? 10000 : 0,
                ["filename"] = filename,
                ["plan_index"] = planIndex,
            };

            return taskParams;
        }

        /// <summary>
        /// 基建换班。
        /// </summary>
        /// <param name="order">要换班的设施（有序）。</param>
        /// <param name="usesOfDrones">
        /// 无人机用途。可用值包括：
        /// <list type="bullet">
        /// <item><c>_NotUse</c></item>
        /// <item><c>Money</c></item>
        /// <item><c>SyntheticJade</c></item>
        /// <item><c>CombatRecord</c></item>
        /// <item><c>PureGold</c></item>
        /// <item><c>OriginStone</c></item>
        /// <item><c>Chip</c></item>
        /// </list>
        /// </param>
        /// <param name="continueTraining">训练室是否尝试连续专精</param>
        /// <param name="dormThreshold">宿舍进驻心情阈值。</param>
        /// <param name="dormFilterNotStationedEnabled">宿舍是否使用未进驻筛选标签</param>
        /// <param name="dormDormTrustEnabled">宿舍是否使用蹭信赖功能</param>
        /// <param name="originiumShardAutoReplenishment">制造站搓玉是否补货</param>
        /// <param name="isCustom">是否开启自定义配置</param>
        /// <param name="filename">自定义配置文件路径</param>
        /// <param name="planIndex">自定义配置计划编号</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendInfrast(
            IEnumerable<string> order,
            string usesOfDrones,
            bool continueTraining,
            double dormThreshold,
            bool dormFilterNotStationedEnabled,
            bool dormDormTrustEnabled,
            bool originiumShardAutoReplenishment,
            bool isCustom,
            string filename,
            int planIndex)
        {
            var taskParams = SerializeInfrastTaskParams(
                order,
                usesOfDrones,
                continueTraining,
                dormThreshold,
                dormFilterNotStationedEnabled,
                dormDormTrustEnabled,
                originiumShardAutoReplenishment,
                isCustom,
                filename,
                planIndex);
            AsstTaskId id = AsstAppendTaskWithEncoding("Infrast", taskParams);
            _latestTaskId[TaskType.Infrast] = id;
            return id != 0;
        }

        public bool AsstSetInfrastTaskParams(
            IEnumerable<string> order,
            string usesOfDrones,
            bool continueTraining,
            double dormThreshold,
            bool dormFilterNotStationedEnabled,
            bool dormDormTrustEnabled,
            bool originiumShardAutoReplenishment,
            bool isCustom,
            string filename,
            int planIndex)
        {
            const TaskType Type = TaskType.Infrast;
            if (!_latestTaskId.TryGetValue(Type, out var id))
            {
                return false;
            }

            if (id == 0)
            {
                return false;
            }

            var taskParams = SerializeInfrastTaskParams(
                order,
                usesOfDrones,
                continueTraining,
                dormThreshold,
                dormFilterNotStationedEnabled,
                dormDormTrustEnabled,
                originiumShardAutoReplenishment,
                isCustom,
                filename,
                planIndex);
            return AsstSetTaskParamsWithEncoding(id, taskParams);
        }

        /// <summary>
        /// 无限刷肉鸽。
        /// </summary>
        /// <param name="mode">
        /// 模式。可用值包括：
        /// <list type="bullet">
        ///     <item>
        ///         <term><c>0</c></term>
        ///         <description>刷蜡烛，尽可能稳定地打更多层数。</description>
        ///     </item>
        ///     <item>
        ///         <term><c>1</c></term>
        ///         <description>刷源石锭，第一层投资完就退出。</description>
        ///     </item>
        ///     <item>
        ///         <term><c>2</c></term>
        ///         <description><b>【即将弃用】</b>两者兼顾，投资过后再退出，没有投资就继续往后打。</description>
        ///     </item>
        ///     <item>
        ///         <term><c>3</c></term>
        ///         <description><b>【开发中】</b>尝试通关，尽可能打的远。</description>
        ///     </item>
        /// </list>
        /// </param>
        /// <param name="starts">开始探索次数。</param>
        /// <param name="investmentEnabled">是否投资源石锭</param>
        /// <param name="investmentWithMoreScore">投资时候刷更多分</param>
        /// <param name="invests">投资源石锭次数。</param>
        /// <param name="stopWhenFull">投资满了自动停止任务。</param>
        /// <param name="squad">开局分队</param>
        /// <param name="roles"> 开局职业组</param>
        /// <param name="coreChar">开局干员名</param>
        /// <param name="startWithEliteTwo">是否凹开局直升</param>
        /// <param name="onlyStartWithEliteTwo">是否只凹开局直升，不进行作战</param>
        /// <param name="roguelike3FirstFloorFoldartal">凹第一层远见板子</param>
        /// <param name="roguelike3StartFloorFoldartal">需要凹的板子</param>
        /// <param name="roguelike3NewSquad2StartingFoldartal">是否在萨米肉鸽生活队凹开局板子</param>
        /// <param name="roguelike3NewSquad2StartingFoldartals">需要凹的板子，用;连接的字符串</param>
        /// <param name="useSupport">是否core_char使用好友助战</param>
        /// <param name="enableNonFriendSupport">是否允许使用非好友助战</param>
        /// <param name="theme">肉鸽主题["Phantom", "Mizuki", "Sami"]</param>
        /// <param name="refreshTraderWithDice">是否用骰子刷新商店购买特殊商品，目前支持水月肉鸽的指路鳞</param>
        /// <param name="team_complete">是否跳过阵容完备度检测</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendRoguelike(
            int mode,
            int starts,
            bool investmentEnabled,
            bool investmentWithMoreScore,
            int invests,
            bool stopWhenFull,
            string squad,
            string roles,
            string coreChar,
            bool startWithEliteTwo,
            bool onlyStartWithEliteTwo,
            bool roguelike3FirstFloorFoldartal,
            string roguelike3StartFloorFoldartal,
            bool roguelike3NewSquad2StartingFoldartal,
            string roguelike3NewSquad2StartingFoldartals,
            bool useSupport,
            bool enableNonFriendSupport,
            string theme,
            bool refreshTraderWithDice,
            bool team_complete)
        {
            var taskParams = new JObject
            {
                ["mode"] = mode,
                ["starts_count"] = starts,
                ["theme"] = theme,
                ["investment_enabled"] = false,
            };

            if (mode == 1 || investmentEnabled)
            {
                taskParams["investment_enabled"] = true;
                taskParams["investment_with_more_score"] = investmentWithMoreScore;
                taskParams["investments_count"] = invests;
                taskParams["stop_when_investment_full"] = stopWhenFull;
            }

            if (squad.Length > 0)
            {
                taskParams["squad"] = squad;
            }

            if (roles.Length > 0)
            {
                taskParams["roles"] = roles;
            }

            if (coreChar.Length > 0)
            {
                taskParams["core_char"] = coreChar;
            }

            taskParams["start_with_elite_two"] = mode == 4 && theme != "Phantom" && startWithEliteTwo;
            taskParams["only_start_with_elite_two"] = mode == 4 && theme != "Phantom" && startWithEliteTwo && onlyStartWithEliteTwo;
            if (mode == 4 && theme == "Sami" && roguelike3FirstFloorFoldartal && roguelike3StartFloorFoldartal.Length > 0)
            {
                taskParams["first_floor_foldartal"] = roguelike3StartFloorFoldartal;
            }

            if (mode == 4 && theme == "Sami" && roguelike3NewSquad2StartingFoldartal && roguelike3NewSquad2StartingFoldartals.Length > 0)
            {
                taskParams["start_foldartal_list"] = new JArray(roguelike3NewSquad2StartingFoldartals.Trim().Split(';', '；').Where(i => !string.IsNullOrEmpty(i)).Take(3));
            }

            taskParams["use_support"] = useSupport;
            taskParams["use_nonfriend_support"] = enableNonFriendSupport;
            taskParams["refresh_trader_with_dice"] = theme == "Mizuki" && refreshTraderWithDice;
            taskParams["skip_team_completeness_check"] = team_complete;

            AsstTaskId id = AsstAppendTaskWithEncoding("Roguelike", taskParams);
            _latestTaskId[TaskType.Roguelike] = id;
            return id != 0;
        }

        /// <summary>
        /// 自动生息演算。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstAppendReclamation()
        {
            AsstTaskId id = AsstAppendTaskWithEncoding("ReclamationAlgorithm");
            _latestTaskId[TaskType.ReclamationAlgorithm] = id;
            return id != 0;
        }

        /// <summary>
        /// 自动生息演算。
        /// </summary>
        /// <param name="mode">是否通过制造刷点数。</param>
        /// <param name="product">制造产物。</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendReclamation2(int mode = 0, string product = "")
        {
            var taskParams = new JObject
            {
                ["theme"] = 1,
                ["mode"] = mode,
                ["product"] = product,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("ReclamationAlgorithm", taskParams);
            _latestTaskId[TaskType.ReclamationAlgorithm2] = id;
            return id != 0;
        }

        /// <summary>
        /// 公招识别。
        /// </summary>
        /// <param name="selectLevel">会去点击标签的 Tag 等级。</param>
        /// <param name="setTime">是否设置 9 小时。</param>
        /// <returns>是否成功。</returns>
        public bool AsstStartRecruitCalc(int[] selectLevel, bool setTime)
        {
            var taskParams = new JObject
            {
                ["refresh"] = false,
                ["select"] = new JArray(selectLevel),
                ["confirm"] = new JArray(),
                ["times"] = 0,
                ["set_time"] = setTime,
                ["expedite"] = false,
                ["expedite_times"] = 0,
                ["report_to_penguin"] = false,
                ["report_to_yituliu"] = false,
            };
            int recruitmentTime;
            if (Instances.RecognizerViewModel.IsLevel3UseShortTime)
            {
                recruitmentTime = 460;
            }
            else if (Instances.RecognizerViewModel.IsLevel3UseShortTime2)
            {
                recruitmentTime = 60;
            }
            else
            {
                recruitmentTime = 540;
            }

            taskParams["recruitment_time"] = new JObject
            {
                {
                    "3", recruitmentTime
                },
            };
            taskParams["penguin_id"] = Instances.SettingsViewModel.PenguinId;
            taskParams["yituliu_id"] = Instances.SettingsViewModel.PenguinId; // 一图流说随便传个uuid就行，让client自己生成，所以先直接嫖一下企鹅的（
            taskParams["server"] = Instances.SettingsViewModel.ServerType;

            AsstTaskId id = AsstAppendTaskWithEncoding("Recruit", taskParams);
            _latestTaskId[TaskType.RecruitCalc] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 仓库识别。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartDepot()
        {
            var taskParams = new JObject();
            AsstTaskId id = AsstAppendTaskWithEncoding("Depot", taskParams);
            _latestTaskId[TaskType.Depot] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 干员识别。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartOperBox()
        {
            var taskParams = new JObject();
            AsstTaskId id = AsstAppendTaskWithEncoding("OperBox", taskParams);
            _latestTaskId[TaskType.OperBox] = id;
            return id != 0 && AsstStart();
        }

        public bool AsstStartGacha(bool once = true)
        {
            var taskParams = new JObject
            {
                ["task_names"] = new JArray
                {
                    once ? "GachaOnce" : "GachaTenTimes",
                },
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("Custom", taskParams);
            _latestTaskId[TaskType.Gacha] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 自动抄作业。
        /// </summary>
        /// <param name="filename">作业 JSON 的文件路径，绝对、相对路径均可。</param>
        /// <param name="formation">是否进行 “快捷编队”。</param>
        /// <param name="addTrust">是否追加信赖干员</param>
        /// <param name="addUserAdditional">是否追加自定干员</param>
        /// <param name="userAdditional">自定干员列表</param>
        /// <param name="needNavigate">是否导航至关卡（启用自动战斗序列）</param>
        /// <param name="navigateName">关卡名</param>
        /// <param name="isRaid">是不是突袭</param>
        /// <param name="type">任务类型</param>
        /// <param name="loopTimes">任务重复执行次数</param>
        /// <param name="useSanityPotion">是否使用理智药</param>
        /// <param name="asstStart">是否启动战斗</param>
        /// <returns>是否成功。</returns>
        public bool AsstStartCopilot(string filename, bool formation, bool addTrust, bool addUserAdditional, JArray userAdditional, bool needNavigate, string navigateName, bool isRaid, string type, int loopTimes, bool useSanityPotion, bool asstStart = true)
        {
            var taskParams = new JObject
            {
                ["filename"] = filename,
                ["formation"] = formation,
                ["add_trust"] = addTrust,
                ["need_navigate"] = needNavigate,
                ["is_raid"] = isRaid,
                ["loop_times"] = loopTimes,
                ["use_sanity_potion"] = useSanityPotion,
            };

            if (addUserAdditional)
            {
                taskParams["user_additional"] = userAdditional;
            }

            if (needNavigate)
            {
                taskParams["navigate_name"] = navigateName;
            }

            AsstTaskId id = AsstAppendTaskWithEncoding(type, taskParams);
            _latestTaskId[TaskType.Copilot] = id;
            return id != 0 && (!asstStart || AsstStart());
        }

        public bool AsstStartVideoRec(string filename)
        {
            var taskParams = new JObject
            {
                ["filename"] = filename,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("VideoRecognition", taskParams);
            _latestTaskId[TaskType.Copilot] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 启动。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStart()
        {
            return MaaService.AsstStart(_handle);
        }

        /// <summary>
        /// 运行中。
        /// </summary>
        /// <returns>是否正在运行。</returns>
        public bool AsstRunning()
        {
            return MaaService.AsstRunning(_handle);
        }

        /// <summary>
        /// 停止。
        /// </summary>
        /// <param name="clearTask">是否清理_latestTaskId</param>
        /// <returns>是否成功。</returns>
        public bool AsstStop(bool clearTask = true)
        {
            bool ret = MaaService.AsstStop(_handle);
            if (clearTask)
            {
                _latestTaskId.Clear();
            }

            return ret;
        }

        /// <summary>
        /// 销毁。
        /// </summary>
        public void AsstDestroy()
        {
            MaaService.AsstDestroy(_handle);
        }
    }

    /// <summary>
    /// MaaCore 消息。
    /// </summary>
    [SuppressMessage("ReSharper", "UnusedMember.Global")]
    public enum AsstMsg
    {
        /* Global Info */

        /// <summary>
        /// 内部错误。
        /// </summary>
        InternalError = 0,

        /// <summary>
        /// 初始化失败。
        /// </summary>
        InitFailed,

        /// <summary>
        /// 连接相关错误。
        /// </summary>
        ConnectionInfo,

        /// <summary>
        /// 全部任务完成。
        /// </summary>
        AllTasksCompleted,

        /* TaskChain Info */

        /// <summary>
        /// 任务链执行/识别错误。
        /// </summary>
        TaskChainError = 10000,

        /// <summary>
        /// 任务链开始。
        /// </summary>
        TaskChainStart,

        /// <summary>
        /// 任务链完成。
        /// </summary>
        TaskChainCompleted,

        /// <summary>
        /// 任务链额外信息。
        /// </summary>
        TaskChainExtraInfo,

        /// <summary>
        /// 任务链手动停止
        /// </summary>
        TaskChainStopped,

        /* SubTask Info */

        /// <summary>
        /// 原子任务执行/识别错误。
        /// </summary>
        SubTaskError = 20000,

        /// <summary>
        /// 原子任务开始。
        /// </summary>
        SubTaskStart,

        /// <summary>
        /// 原子任务完成。
        /// </summary>
        SubTaskCompleted,

        /// <summary>
        /// 原子任务额外信息。
        /// </summary>
        SubTaskExtraInfo,

        /// <summary>
        /// 原子任务手动停止
        /// </summary>
        SubTaskStopped,
    }

    public enum InstanceOptionKey
    {
        /* Deprecated */ // MinitouchEnabled = 1,

        /// <summary>
        /// Indicates the touch mode.
        /// </summary>
        TouchMode = 2,

        /// <summary>
        /// Indicates whether the deployment should be paused.
        /// </summary>
        DeploymentWithPause = 3,

        /// <summary>
        /// Indicates whether AdbLite is used.
        /// </summary>
        AdbLiteEnabled = 4,

        /// <summary>
        /// Indicates whether the ADB server process should be killed when the instance is exited.
        /// </summary>
        KillAdbOnExit = 5,
    }
}
