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
#nullable enable
using System;
using System.Buffers;
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
using System.Windows.Threading;
using HandyControl.Data;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Helper.Instances.Data;
using AsstHandle = nint;
using AsstInstanceOptionKey = System.Int32;
using AsstTaskId = System.Int32;
using ToastNotification = MaaWpfGui.Helper.ToastNotification;

namespace MaaWpfGui.Main
{
    /// <summary>
    /// MaaCore 代理类。
    /// </summary>
    public class AsstProxy
    {
        private readonly RunningState _runningState;
        private static readonly ILogger _logger = Log.ForContext<AsstProxy>();

        public DateTimeOffset StartTaskTime { get; set; }

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

        private static void AsstSetConnectionExtrasMuMu12(string extras)
        {
            AsstSetConnectionExtras("MuMuEmulator12", extras);
        }

        private static void AsstSetConnectionExtrasLdPlayer(string extras)
        {
            AsstSetConnectionExtras("LDPlayer", extras);
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

        public static unsafe BitmapImage? AsstGetImage(AsstHandle handle)
        {
            var buffer = ArrayPool<byte>.Shared.Rent(1280 * 720 * 3);
            try
            {
                ulong readSize;
                fixed (byte* ptr = buffer)
                {
                    readSize = MaaService.AsstGetImage(handle, ptr, (ulong)buffer.Length);
                }

                if (readSize == MaaService.AsstGetNullSize())
                {
                    return null;
                }

                // buff is a png data
                var image = new BitmapImage();
                image.BeginInit();
                using var stream = new MemoryStream(buffer, 0, (int)readSize, false);
                image.StreamSource = stream;
                image.CacheOption = BitmapCacheOption.OnLoad;
                image.CreateOptions = BitmapCreateOptions.IgnoreColorProfile;
                image.EndInit();
                image.Freeze();
                return image;
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(buffer);
            }
        }

        public static async Task<BitmapImage?> AsstGetImageAsync(AsstHandle handle)
        {
            return await Task.Run(() => AsstGetImage(handle));
        }

        public BitmapImage? AsstGetImage()
        {
            return AsstGetImage(_handle);
        }

        public BitmapImage? AsstGetFreshImage()
        {
            MaaService.AsstAsyncScreencap(_handle, true);
            return AsstGetImage(_handle);
        }

        public async Task<BitmapImage?> AsstGetImageAsync()
        {
            return await AsstGetImageAsync(_handle);
        }

        public async Task<BitmapImage?> AsstGetFreshImageAsync()
        {
            MaaService.AsstAsyncScreencap(_handle, true);
            return await AsstGetImageAsync(_handle);
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

            string clientType = SettingsViewModel.GameSettings.ClientType;
            string mainRes = Directory.GetCurrentDirectory();
            string globalResource = mainRes + @"\resource\global\" + clientType;
            string mainCache = Directory.GetCurrentDirectory() + @"\cache";
            string globalCache = mainCache + @"\resource\global\" + clientType;

            bool loaded;
            if (clientType is "" or "Official" or "Bilibili")
            {
                // Read resources first, then read cache
                MoveTasks(mainCache);
                loaded = LoadResIfExists(mainRes);
                loaded &= LoadResIfExists(mainCache);
            }
            else
            {
                // Read resources first, then read cache
                MoveTasks(mainCache);
                MoveTasks(globalCache);
                loaded = LoadResIfExists(mainRes) && LoadResIfExists(mainCache);
                loaded &= LoadResIfExists(globalResource) && LoadResIfExists(globalCache);
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

            static void MoveTasks(string oldPath)
            {
                try
                {
                    string tasksJsonPath = Path.Combine(oldPath, @"resource\tasks.json");
                    string tasksFolderPath = Path.Combine(oldPath, @"resource\tasks");
                    string newTasksJsonPath = Path.Combine(tasksFolderPath, "tasks.json");

                    if (/*File.Exists(newTasksJsonPath) for 旧版本铲屎 || */!File.Exists(tasksJsonPath))
                    {
                        return;
                    }

                    if (!Directory.Exists(tasksFolderPath))
                    {
                        Directory.CreateDirectory(tasksFolderPath);
                        _logger.Information($"Created directory: {tasksFolderPath}");
                    }

                    File.Move(tasksJsonPath, newTasksJsonPath, true);
                    _logger.Information($"Moved {tasksJsonPath} to {newTasksJsonPath}");
                }
                catch (Exception ex)
                {
                    _logger.Error($"Failed to move tasks.json: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// 初始化。
        /// </summary>
        public void Init()
        {
            if (GpuOption.GetCurrent() is GpuOption.EnableOption x)
            {
                if (x.IsDeprecated && !GpuOption.AllowDeprecatedGpu)
                {
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("GpuDeprecatedMessage"), x.GpuInfo?.Description), UiLogColor.Warning);
                }

                _logger.Information("Using GPU {0} (Driver {1} {2})", x.GpuInfo?.Description, x.GpuInfo?.DriverVersion, x.GpuInfo?.DriverDate?.ToString("yyyy-MM-dd"));

                AsstSetStaticOption(AsstStaticOptionKey.GpuOCR, x.Index.ToString());
            }

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
            AsstSetInstanceOption(InstanceOptionKey.TouchMode, SettingsViewModel.ConnectSettings.TouchMode);
            AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, SettingsViewModel.GameSettings.DeploymentWithPause ? "1" : "0");
            AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, SettingsViewModel.ConnectSettings.AdbLiteEnabled ? "1" : "0");

            // TODO: 之后把这个 OnUIThread 拆出来
            // ReSharper disable once AsyncVoidLambda
            Execute.OnUIThread(
                async () =>
            {
                if (SettingsViewModel.StartSettings.RunDirectly)
                {
                    // 如果是直接运行模式，就先让按钮显示为运行
                    _runningState.SetIdle(false);
                }

                await Task.Run(() => SettingsViewModel.StartSettings.TryToStartEmulator(true));

                // 一般是点了“停止”按钮了
                if (Instances.TaskQueueViewModel.Stopping)
                {
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                // ReSharper disable once InvertIf
                if (SettingsViewModel.StartSettings.RunDirectly)
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

        private static string? PtrToStringCustom(AsstHandle ptr, Encoding enc)
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
            var jsonStr = PtrToStringCustom(jsonBuffer, Encoding.UTF8);

            // Console.WriteLine(json_str);
            var json = (JObject?)JsonConvert.DeserializeObject(jsonStr ?? string.Empty);
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
                case AsstMsg.AsyncCallInfo:
                case AsstMsg.Destroyed:
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
                    TaskQueueViewModel.InvokeProcSubTaskMsg(msg, details);
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
                    _connectedAdb = details["details"]!["adb"]!.ToString();
                    _connectedAddress = details["details"]!["address"]!.ToString();
                    SettingsViewModel.ConnectSettings.ConnectAddress = _connectedAddress;
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
                    Instances.TaskQueueViewModel.AddLog($"{LocalizationHelper.GetString("TryToReconnect")} ({Convert.ToUInt32(details!["details"]!["times"]) + 1})", UiLogColor.Error);
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
                        SettingsViewModel.ConnectSettings.ScreencapMethod = method;

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

                        var needToStop = false;
                        switch (SettingsViewModel.ConnectSettings.ConnectConfig)
                        {
                            case "MuMuEmulator12":
                                if (!SettingsViewModel.ConnectSettings.MuMuEmulator12Extras.Enable)
                                {
                                    break;
                                }

                                if (method != "MumuExtras")
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage"), UiLogColor.Error);
                                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage"), UiLogColor.Error, showTime: false);
                                    needToStop = true;
                                }
                                else if (timeCost < 100)
                                {
                                    color = UiLogColor.MuMuSpecialScreenshot;
                                }

                                break;
                            case "LDPlayer":
                                if (!SettingsViewModel.ConnectSettings.LdPlayerExtras.Enable)
                                {
                                    break;
                                }

                                if (method != "LDExtras")
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("LdExtrasNotEnabledMessage"), UiLogColor.Error);
                                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("LdExtrasNotEnabledMessage"), UiLogColor.Error, showTime: false);
                                    needToStop = true;
                                }
                                else if (timeCost < 100)
                                {
                                    color = UiLogColor.LdSpecialScreenshot;
                                }

                                break;
                        }

                        fastestScreencapStringBuilder.Insert(0, string.Format(LocalizationHelper.GetString("FastestWayToScreencap"), costString, method));
                        var fastestScreencapString = fastestScreencapStringBuilder.ToString();
                        SettingsViewModel.ConnectSettings.ScreencapTestCost = fastestScreencapString;
                        Instances.TaskQueueViewModel.AddLog(fastestScreencapString, color);
                        Instances.CopilotViewModel.AddLog(fastestScreencapString, color, showTime: false);

                        // 截图增强未生效禁止启动
                        if (needToStop)
                        {
                            Execute.OnUIThreadAsync(async () =>
                            {
                                Connected = false;
                                await Instances.TaskQueueViewModel.Stop();
                                Instances.TaskQueueViewModel.SetStopped();
                            });
                        }
                    }

                    break;

                case "ScreencapCost":
                    var screencapCostMin = details["details"]?["min"]?.ToString() ?? "???";
                    var screencapCostAvg = details["details"]?["avg"]?.ToString() ?? "???";
                    var screencapCostMax = details["details"]?["max"]?.ToString() ?? "???";
                    var currentTime = DateTimeOffset.Now.ToString("HH:mm:ss");
                    SettingsViewModel.ConnectSettings.ScreencapCost = string.Format(LocalizationHelper.GetString("ScreencapCost"), screencapCostMin, screencapCostAvg, screencapCostMax, currentTime);
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

        private DispatcherTimer? _toastNotificationTimer;

        private void OnToastNotificationTimerTick(object? sender, EventArgs e)
        {
            if (SanityReport is not null)
            {
                var sanityReport = LocalizationHelper.GetString("SanityReport");
                var recoveryTime = SanityReport.ReportTime.AddMinutes(SanityReport.SanityCurrent < SanityReport.SanityMax ? (SanityReport.SanityMax - SanityReport.SanityCurrent) * 6 : 0);
                sanityReport = sanityReport.Replace("{DateTime}", recoveryTime.ToString("yyyy-MM-dd HH:mm")).Replace("{TimeDiff}", (recoveryTime - DateTimeOffset.Now).ToString(@"h\h\ m\m"));
                ToastNotification.ShowDirect(sanityReport);
            }

            DisposeTimer();
        }

        public void DisposeTimer()
        {
            if (_toastNotificationTimer is null)
            {
                return;
            }

            _toastNotificationTimer.Stop();
            _toastNotificationTimer.Tick -= OnToastNotificationTimerTick;
            _toastNotificationTimer = null;
        }

        private void ProcTaskChainMsg(AsstMsg msg, JObject details)
        {
            string taskChain = details["taskchain"]?.ToString() ?? string.Empty;
            AsstTaskId taskId = details["taskid"]?.ToObject<AsstTaskId>() ?? 0;
            switch (taskChain)
            {
                case "CloseDown":
                    return;

                case "Recruit":
                    {
                        if (msg == AsstMsg.TaskChainError)
                        {
                            Instances.RecognizerViewModel.RecruitInfo = LocalizationHelper.GetString("IdentifyTheMistakes");
                            ToastNotification.ShowDirect(LocalizationHelper.GetString("IdentifyTheMistakes"));
                        }

                        break;
                    }
            }

            bool isCopilotTaskChain = taskChain is "Copilot" or "VideoRecognition";

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
                        // 对剿灭的特殊处理，如果刷完了剿灭还选了剿灭会因为找不到入口报错
                        if (taskChain == "Fight" && (TaskQueueViewModel.FightTask.Stage == "Annihilation"))
                        {
                            if (TaskQueueViewModel.FightTask.Stages.Any(stage => Instances.TaskQueueViewModel.IsStageOpen(stage) && (stage != "Annihilation")))
                            {
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AnnihilationTaskFailed"), UiLogColor.Warning);
                            }
                        }
                        else
                        {
                            var log = LocalizationHelper.GetString("TaskError") + taskChain;
                            Instances.TaskQueueViewModel.AddLog(log, UiLogColor.Error);
                            ToastNotification.ShowDirect(log);

                            if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenError)
                            {
                                ExternalNotificationService.Send(log, log);
                            }
                        }

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
                    {
                        // 判断 _latestTaskId 中是否有元素的值和 details["taskid"] 相等，如果有再判断这个 id 对应的任务是否在 _mainTaskTypes 中
                        if (_taskStatus.TryGetValue(taskId, out var taskType))
                        {
                            if (_mainTaskTypes.Contains(taskType))
                            {
                                Instances.TaskQueueViewModel.UpdateMainTasksProgress();
                            }
                        }

                        switch (taskChain)
                        {
                            case "Fight":
                                Instances.TaskQueueViewModel.FightTaskRunning = false;
                                break;

                            case "Infrast":
                                Instances.TaskQueueViewModel.InfrastTaskRunning = false;
                                InfrastSettingsUserControlModel.Instance.IncreaseCustomInfrastPlanIndex();
                                InfrastSettingsUserControlModel.Instance.RefreshCustomInfrastPlanIndexByPeriod();
                                break;
                        }

                        if (taskChain == "Fight" && SanityReport is not null)
                        {
                            var sanityLog = "\n" + string.Format(LocalizationHelper.GetString("CurrentSanity"), SanityReport.SanityCurrent, SanityReport.SanityMax);
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
                    }

                    break;
                case AsstMsg.TaskChainExtraInfo:
                    break;

                case AsstMsg.AllTasksCompleted:
                    bool isMainTaskQueueAllCompleted = false;
                    var taskList = details["finished_tasks"]?.ToObject<AsstTaskId[]>();
                    if (taskList?.Length > 0)
                    {
                        var latestMainTaskIds = _taskStatus.Where(i => _mainTaskTypes.Contains(i.Value)).Select(i => i.Key);
                        isMainTaskQueueAllCompleted = taskList.Any(i => latestMainTaskIds.Contains(i));
                    }

                    if (_taskStatus.ContainsValue(TaskType.Copilot))
                    {
                        if (SettingsViewModel.GameSettings.CopilotWithScript)
                        {
                            Task.Run(() => SettingsViewModel.GameSettings.RunScript("EndsWithScript", showLog: false));
                            if (!string.IsNullOrWhiteSpace(SettingsViewModel.GameSettings.EndsWithScript))
                            {
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("EndsWithScript"));
                            }
                        }
                    }

                    bool buyWine = _taskStatus.ContainsValue(TaskType.Mall) && Instances.SettingsViewModel.DidYouBuyWine();
                    _taskStatus.Clear();

                    TaskQueueViewModel.FightTask.ResetFightVariables();
                    TaskQueueViewModel.RecruitTask.ResetRecruitVariables();
                    Instances.TaskQueueViewModel.ResetTaskSelection();
                    _runningState.SetIdle(true);

                    if (isMainTaskQueueAllCompleted)
                    {
                        var dateTimeNow = DateTimeOffset.Now;
                        var diffTaskTime = (dateTimeNow - StartTaskTime).ToString(@"h\h\ m\m\ s\s");

                        var allTaskCompleteTitle = string.Format(LocalizationHelper.GetString("AllTasksComplete"), diffTaskTime);
                        var allTaskCompleteMessage = LocalizationHelper.GetString("AllTaskCompleteContent");
                        var sanityReport = LocalizationHelper.GetString("SanityReport");

                        var configurationPreset = ConfigurationHelper.GetCurrentConfiguration();

                        allTaskCompleteMessage = allTaskCompleteMessage
                            .Replace("{DateTime}", dateTimeNow.ToString("yyyy-MM-dd HH:mm:ss"))
                            .Replace("{Preset}", configurationPreset)
                            .Replace("{TimeDiff}", diffTaskTime);

                        var allTaskCompleteLog = string.Format(LocalizationHelper.GetString("AllTasksComplete"), diffTaskTime);

                        if (SanityReport is not null)
                        {
                            var recoveryTime = SanityReport.ReportTime.AddMinutes(SanityReport.SanityCurrent < SanityReport.SanityMax ? (SanityReport.SanityMax - SanityReport.SanityCurrent) * 6 : 0);
                            sanityReport = sanityReport.Replace("{DateTime}", recoveryTime.ToString("yyyy-MM-dd HH:mm")).Replace("{TimeDiff}", (recoveryTime - DateTimeOffset.Now).ToString(@"h\h\ m\m"));

                            allTaskCompleteLog = allTaskCompleteLog + Environment.NewLine + sanityReport;
                            Instances.TaskQueueViewModel.AddLog(allTaskCompleteLog);

                            if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenComplete)
                            {
                                var logs = SettingsViewModel.ExternalNotificationSettings.ExternalNotificationEnableDetails
                                    ? Instances.TaskQueueViewModel.LogItemViewModels.Aggregate(string.Empty, (current, logItem) => current + $"[{logItem.Time}][{logItem.Color}]{logItem.Content}\n")
                                    : string.Empty;
                                logs += allTaskCompleteMessage;

                                ExternalNotificationService.Send(allTaskCompleteTitle, logs + Environment.NewLine + sanityReport);
                            }

                            if (_toastNotificationTimer is not null)
                            {
                                DisposeTimer();
                            }

                            var interval = recoveryTime - DateTimeOffset.Now.AddMinutes(6);
                            if (interval > TimeSpan.Zero)
                            {
                                _toastNotificationTimer = new DispatcherTimer
                                {
                                    Interval = interval,
                                };
                                _toastNotificationTimer.Tick += OnToastNotificationTimerTick;
                                _toastNotificationTimer.Start();
                            }
                        }
                        else
                        {
                            Instances.TaskQueueViewModel.AddLog(allTaskCompleteLog);

                            if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenComplete)
                            {
                                var logs = SettingsViewModel.ExternalNotificationSettings.ExternalNotificationEnableDetails
                                    ? Instances.TaskQueueViewModel.LogItemViewModels.Aggregate(string.Empty, (current, logItem) => current + $"[{logItem.Time}][{logItem.Color}]{logItem.Content}\n")
                                    : string.Empty;
                                logs += allTaskCompleteMessage;

                                ExternalNotificationService.Send(allTaskCompleteTitle, logs);
                            }
                        }

                        using (var toast = new ToastNotification(allTaskCompleteTitle))
                        {
                            if (SanityReport is not null)
                            {
                                toast.AppendContentText(sanityReport);
                            }

                            toast.Show();
                        }

                        if (DateTime.UtcNow.ToYjDate().IsAprilFoolsDay())
                        {
                            if (Application.Current.MainWindow?.DataContext is RootViewModel rvm)
                            {
                                rvm.GifVisibility = true;
                                rvm.ChangeGif();
                            }
                        }

                        // Instances.TaskQueueViewModel.CheckAndShutdown();
                        Instances.TaskQueueViewModel.CheckAfterCompleted();
                    }
                    else if (isCopilotTaskChain)
                    {
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("CompleteTask") + taskChain);
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

                case AsstMsg.AsyncCallInfo:
                    break;

                case AsstMsg.Destroyed:
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

        private static void ProcSubTaskMsg(AsstMsg msg, JObject details)
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

                case "StopGameTask":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
                    break;

                case "AutoRecruitTask":
                    {
                        var whyStr = details.TryGetValue("why", out var why) ? why.ToString() : LocalizationHelper.GetString("ErrorOccurred");
                        Instances.TaskQueueViewModel.AddLog(whyStr + ", " + LocalizationHelper.GetString("HasReturned"), UiLogColor.Error);
                        break;
                    }

                case "RecognizeDrops":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DropRecognitionError"), UiLogColor.Error);
                    break;

                case "ReportToPenguinStats":
                    {
                        var why = details["why"]!.ToString();
                        Instances.TaskQueueViewModel.AddLog(why + ", " + LocalizationHelper.GetString("GiveUpUploadingPenguins"), UiLogColor.Warning);
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
                                var missingOpersStr = "[" + string.Join("]; [", missingOpers.Select(opers =>
                                    string.Join(", ", opers.Select(oper => DataHelper.GetLocalizedCharacterName(oper))))) + "]";
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
                        string taskName = details!["details"]!["task"]!.ToString();
                        int execTimes = (int)details!["details"]!["exec_times"]!;

                        switch (taskName)
                        {
                            case "StartButton2":
                            case "AnnihilationConfirm":
                                StringBuilder missionStartLogBuilder = new();
                                if (FightTimes is null)
                                {
                                    missionStartLogBuilder.AppendLine(string.Format(LocalizationHelper.GetString("MissionStart.FightTask"), "???", "???"));
                                }
                                else
                                {
                                    var times = FightTimes.Series == 1 ? $"{FightTimes.TimesFinished + 1}" : $"{FightTimes.TimesFinished + 1}~{FightTimes.TimesFinished + FightTimes.Series}";
                                    missionStartLogBuilder.AppendLine(string.Format(LocalizationHelper.GetString("MissionStart.FightTask"), times, FightTimes.SanityCost));
                                }

                                if (SanityReport is not null)
                                {
                                    missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("CurrentSanity"), SanityReport.SanityCurrent, SanityReport.SanityMax);
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
                            case "ExitThenAbandon":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ExplorationAbandoned"), UiLogColor.Error);
                                break;

                            // case "StartAction":
                            //    Instances.TaskQueueViewModel.AddLog("开始战斗");
                            //    break;
                            case "MissionCompletedFlag":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightCompleted"), UiLogColor.SuccessIS);
                                break;

                            case "MissionFailedFlag":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightFailed"), UiLogColor.Error);
                                break;

                            case "StageTraderEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Trader"), UiLogColor.TraderIS);
                                break;

                            case "StageSafeHouseEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("SafeHouse"), UiLogColor.SafehouseIS);
                                break;

                            case "StageFilterTruthEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FilterTruth"), UiLogColor.TruthIS);
                                break;

                            // case "StageBoonsEnter":
                            //    Instances.TaskQueueViewModel.AddLog("古堡馈赠");
                            //    break;
                            case "StageCombatDpsEnter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CombatDps"), UiLogColor.CombatIS);
                                break;

                            case "StageEmergencyDps":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("EmergencyDps"), UiLogColor.EmergencyIS);
                                break;

                            case "StageDreadfulFoe":
                            case "StageDreadfulFoe-5Enter":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DreadfulFoe"), UiLogColor.BossIS);
                                break;

                            case "StageTraderInvestSystemFull":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("UpperLimit"), UiLogColor.Info);
                                break;

                            case "OfflineConfirm":
                                if (SettingsViewModel.GameSettings.AutoRestartOnDrop)
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("GameDrop"), UiLogColor.Warning);
                                }
                                else
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("GameDropNoRestart"), UiLogColor.Warning);
                                    ToastNotification.ShowDirect(LocalizationHelper.GetString("GameDropNoRestart"));
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

                            case "DeepExplorationNotUnlockedComplain":
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DeepExplorationNotUnlockedComplain"), UiLogColor.Warning);
                                break;
                        }

                        break;
                    }

                case "CombatRecordRecognitionTask":
                    {
                        var what = details["what"]?.ToString();
                        if (!string.IsNullOrEmpty(what))
                        {
                            Instances.CopilotViewModel.AddLog(what);
                        }

                        break;
                    }
            }
        }

        private static void ProcSubTaskCompleted(JObject details)
        {
            string subTask = details["subtask"]?.ToString() ?? string.Empty;
            switch (subTask)
            {
                case "ProcessTask":
                    var taskchain = details["taskchain"]?.ToString();
                    switch (taskchain)
                    {
                        case "Roguelike":
                            {
                                var taskName = details!["details"]!["task"]!.ToString();
                                int execTimes = (int)details!["details"]!["exec_times"]!;

                                if (taskName == "StartExplore")
                                {
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("BegunToExplore") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                                }

                                break;
                            }

                        case "Mall":
                            {
                                var taskName = details["details"]!["task"]!.ToString();
                                switch (taskName)
                                {
                                    case "EndOfActionThenStop":
                                        TaskQueueViewModel.MallTask.LastCreditFightTaskTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString("CreditFight"));
                                        break;
                                    case "VisitLimited" or "VisitNextBlack":
                                        TaskQueueViewModel.MallTask.LastCreditVisitFriendsTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString("Visiting"));
                                        break;
                                }

                                break;
                            }
                    }

                    break;
            }
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
                    Instances.RecognizerViewModel.DepotParse((JObject?)subTaskDetails);
                    break;

                case "OperBox":
                    Instances.RecognizerViewModel.OperBoxParse((JObject?)subTaskDetails);
                    break;
            }

            string what = details["what"]?.ToString() ?? string.Empty;
            switch (what)
            {
                case "StageDrops":
                    {
                        string allDrops = string.Empty;
                        var statistics = subTaskDetails!["stats"] ?? new JArray();
                        var stageInfo = subTaskDetails!["stage"] ?? new JObject();
                        int curTimes = (int)(subTaskDetails["cur_times"] ?? -1);

                        foreach (var item in statistics)
                        {
                            var itemName = item["itemName"]?.ToString();
                            if (itemName == "furni")
                            {
                                itemName = LocalizationHelper.GetString("FurnitureDrop");
                            }

                            int totalQuantity = (int)(item["quantity"] ?? -1);
                            int addQuantity = (int)(item["addQuantity"] ?? -1);

                            allDrops += $"{itemName} : {totalQuantity:#,#}";
                            if (addQuantity > 0)
                            {
                                allDrops += $" (+{addQuantity:#,#})";
                            }

                            allDrops += "\n";
                        }

                        var stageCode = stageInfo["stageCode"]?.ToString();
                        allDrops = allDrops.EndsWith('\n') ? allDrops.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");
                        Instances.TaskQueueViewModel.AddLog(
                            $"{stageCode} {LocalizationHelper.GetString("TotalDrop")}\n" +
                            $"{allDrops}{(curTimes >= 0
                                ? $"\n{LocalizationHelper.GetString("CurTimes")} : {curTimes}"
                                : string.Empty)}");
                        break;
                    }

                case "EnterFacility":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ThisFacility") + subTaskDetails!["facility"] + " " + (int)(subTaskDetails["index"] ?? -1));
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
                        var tags = subTaskDetails!["tags"] ?? new JArray();
                        string logContent = tags.Select(tagName => tagName.ToString())
                            .Aggregate(string.Empty, (current, tagStr) => current + (tagStr + "\n"));

                        logContent = logContent.EndsWith('\n') ? logContent.TrimEnd('\n') : LocalizationHelper.GetString("Error");
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RecruitingResults") + "\n" + logContent);

                        break;
                    }

                case "RecruitSpecialTag":
                    {
                        string special = subTaskDetails!["tag"]!.ToString();
                        if (special == "支援机械" && TaskQueueViewModel.RecruitTask.NotChooseLevel1 == false)
                        {
                            break;
                        }

                        using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                        toast.AppendContentText(special).ShowRecruit();

                        break;
                    }

                case "RecruitRobotTag":
                    {
                        string special = subTaskDetails!["tag"]!.ToString();
                        using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                        toast.AppendContentText(special).ShowRecruitRobot();

                        break;
                    }

                case "RecruitResult":
                    {
                        int level = (int)subTaskDetails!["level"]!;
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

                case "RecruitSupportOperator":
                    {
                        var name = subTaskDetails!["name"]!.ToString();
                        Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RecruitSupportOperator"), name), UiLogColor.Info);
                        break;
                    }

                case "RecruitTagsSelected":
                    {
                        var selected = subTaskDetails!["tags"] ?? new JArray();
                        string selectedLog = selected.Aggregate(string.Empty, (current, tag) => current + (tag + "\n"));

                        selectedLog = selectedLog.EndsWith('\n') ? selectedLog.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");

                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Choose") + " Tags：\n" + selectedLog);

                        break;
                    }

                case "RecruitTagsRefreshed":
                    {
                        int refreshCount = (int)subTaskDetails!["count"]!;
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Refreshed") + refreshCount + LocalizationHelper.GetString("UnitTime"));
                        break;
                    }

                case "RecruitNoPermit":
                    {
                        bool continueRefresh = (bool)subTaskDetails!["continue"]!;
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString(continueRefresh ? "ContinueRefresh" : "NoRecruitmentPermit"));
                        break;
                    }

                case "NotEnoughStaff":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NotEnoughStaff"), UiLogColor.Error);
                    break;

                case "CreditFullOnlyBuyDiscount":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CreditFullOnlyBuyDiscount") + subTaskDetails!["credit"], UiLogColor.Message);
                        break;
                    }

                /* Roguelike */
                case "StageInfo":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartCombat") + subTaskDetails!["name"]);
                        if (SettingsViewModel.GameSettings.RoguelikeDelayAbortUntilCombatComplete)
                        {
                            Instances.TaskQueueViewModel.RoguelikeInCombatAndShowWait = true;
                        }

                        break;
                    }

                case "StageInfoError":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageInfoError"), UiLogColor.Error);
                    break;

                case "PenguinId":
                    {
                        string id = subTaskDetails!["id"]?.ToString() ?? string.Empty;
                        SettingsViewModel.GameSettings.PenguinId = id;

                        break;
                    }

                case "BattleFormation":
                    Instances.CopilotViewModel.AddLog(
                        LocalizationHelper.GetString("BattleFormation") +
                        "\n[" +
                        string.Join(
                            ", ",
                            (subTaskDetails!["formation"]?.ToObject<List<string?>>() ?? [])
                                .Select(oper => DataHelper.GetLocalizedCharacterName(oper) ?? oper)
                                .Where(oper => !string.IsNullOrEmpty(oper))) + "]");
                    break;

                case "BattleFormationSelected":
                    var oper_name = DataHelper.GetLocalizedCharacterName(subTaskDetails!["selected"]?.ToString());
                    var group_name = subTaskDetails!["group_name"]?.ToString();
                    if (group_name is not null && oper_name != group_name)
                    {
                        oper_name = $"{group_name} => {oper_name}";
                    }

                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormationSelected") + oper_name);
                    break;

                case "CopilotAction":
                    {
                        var doc = subTaskDetails!["doc"]?.ToString();
                        if (!string.IsNullOrEmpty(doc))
                        {
                            var color = subTaskDetails["doc_color"]?.ToString();
                            Instances.CopilotViewModel.AddLog(doc, string.IsNullOrEmpty(color) ? UiLogColor.Message : color);
                        }

                        var target = subTaskDetails["target"]?.ToString();
                        Instances.CopilotViewModel.AddLog(
                            string.Format(
                                LocalizationHelper.GetString("CurrentSteps"),
                                subTaskDetails["action"],
                                DataHelper.GetLocalizedCharacterName(target) ?? target));

                        break;
                    }

                case "CopilotListLoadTaskFileSuccess":
                    Instances.CopilotViewModel.AddLog($"Parse {subTaskDetails!["file_name"]}[{subTaskDetails["stage_name"]}] Success");
                    break;

                case "SSSStage":
                    Instances.CopilotViewModel.AddLog("CurrentStage: " + subTaskDetails!["stage"], UiLogColor.Info);
                    break;

                case "SSSSettlement":
                    Instances.CopilotViewModel.AddLog($"{details["why"]}", UiLogColor.Info);
                    break;

                case "SSSGamePass":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("SSSGamePass"), UiLogColor.RareOperator);
                    break;

                case "UnsupportedLevel":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("UnsupportedLevel") + subTaskDetails!["level"], UiLogColor.Error);
                    _ = ResourceUpdater.ResourceUpdateAndReloadAsync();
                    break;

                case "CustomInfrastRoomGroupsMatch":
                    // 选用xxx组编组
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomGroupsMatch") + subTaskDetails!["group"]);
                    break;

                case "CustomInfrastRoomGroupsMatchFailed":
                    // 干员编组匹配失败
                    var groups = (JArray?)subTaskDetails!["groups"];
                    if (groups != null)
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomGroupsMatchFailed") + string.Join(", ", groups));
                    }

                    break;

                case "CustomInfrastRoomOperators":
                    string nameStr = (subTaskDetails!["names"] ?? new JArray())
                        .Aggregate(string.Empty, (current, name) => current + DataHelper.GetLocalizedCharacterName(name.ToString()) + ", ");

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
                        var operatorName = DataHelper.GetLocalizedCharacterName(subTaskDetails!["operator"]?.ToString()) ?? "UnKnown";
                        var skillName = subTaskDetails["skill"]?.ToString() ?? "UnKnown";
                        Instances.TaskQueueViewModel.AddLog(
                            $"[{operatorName}] {skillName}\n" +
                            $"{LocalizationHelper.GetString("TrainingLevel")}: {(int)(subTaskDetails["level"] ?? -1)} {LocalizationHelper.GetString("TrainingCompleted")}",
                            UiLogColor.Info);
                        break;
                    }

                case "InfrastTrainingTimeLeft":
                    {
                        var operatorName = DataHelper.GetLocalizedCharacterName(subTaskDetails!["operator"]?.ToString()) ?? "UnKnown";
                        var skillName = subTaskDetails["skill"]?.ToString() ?? "UnKnown";
                        Instances.TaskQueueViewModel.AddLog(
                            $"[{operatorName}] {skillName}\n" +
                            $"{LocalizationHelper.GetString("TrainingLevel")}: {(int)(subTaskDetails["level"] ?? -1)}\n" +
                            $"{LocalizationHelper.GetString("TrainingTimeLeft")}: {subTaskDetails["hh"]:D2}:{subTaskDetails["mm"]:D2}:{subTaskDetails["ss"]:D2}",
                            UiLogColor.Info);
                        break;
                    }

                /* 生息演算 */
                case "ReclamationReport":
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetString("AlgorithmFinish") + "\n" +
                        LocalizationHelper.GetString("AlgorithmBadge") + ": " + $"{(int)(subTaskDetails!["total_badges"] ?? -1)}(+{(int)(subTaskDetails["badges"] ?? -1)})" + "\n" +
                        LocalizationHelper.GetString("AlgorithmConstructionPoint") + ": " + $"{(int)(subTaskDetails["total_construction_points"] ?? -1)}(+{(int)(subTaskDetails["construction_points"] ?? -1)})");
                    break;

                case "ReclamationProcedureStart":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MissionStart") + $" {(int)(subTaskDetails!["times"] ?? -1)} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                    break;

                case "ReclamationSmeltGold":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AlgorithmDoneSmeltGold") + $" {(int)(subTaskDetails!["times"] ?? -1)} " + LocalizationHelper.GetString("UnitTime"));
                    break;

                case "SanityBeforeStage":
                    {
                        SanityReport = null;
                        if (subTaskDetails?.ToObject<FightSettingsUserControlModel.SanityInfo>() is { SanityMax: > 0 } report)
                        {
                            SanityReport = report;
                        }

                        break;
                    }

                case "FightTimes":
                    {
                        FightTimes = null;
                        if ((subTaskDetails?.Children())?.Any() is true)
                        {
                            FightTimes = subTaskDetails.ToObject<FightSettingsUserControlModel.FightTimes>()!;
                        }

                        break;
                    }

                case "UseMedicine":
                    var medicineReport = (JObject?)subTaskDetails;
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
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageQueue") + $" {subTaskDetails!["stage_code"]} " + LocalizationHelper.GetString("UnableToAgent"), UiLogColor.Info);
                    break;

                case "StageQueueMissionCompleted":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageQueue") + $" {subTaskDetails!["stage_code"]} - {subTaskDetails["stars"]} ★", UiLogColor.Info);
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

        public bool AsstSetStaticOption(AsstStaticOptionKey key, string value)
        {
            return MaaService.AsstSetStaticOption(key, value);
        }

        private static readonly bool _forcedReloadResource = File.Exists("DEBUG") || File.Exists("DEBUG.txt");

        /// <summary>
        /// 使用 TCP 或 adb devices 命令检查连接。TCP 检测相比 adb devices 更快，但不支持实体机。
        /// </summary>
        /// <param name="adbPath">adb path，用于实体机检测</param>
        /// <param name="address">连接地址</param>
        /// <returns>设备是否在线</returns>
        private static bool CheckConnection(string adbPath, string? address)
        {
            if (string.IsNullOrEmpty(address))
            {
                return false;
            }

            // 实体机可能设备名 -> [host]
            if (!address.Contains(':') && !address.Contains('-'))
            {
                return WinAdapter.GetAdbAddresses(adbPath).Contains(address);
            }

            // normal -> [host]:[port]
            // LdPlayer -> emulator-[port]
            string[] hostAndPort = address.Split([':', '-'], StringSplitOptions.RemoveEmptyEntries);

            if (hostAndPort.Length != 2 || !int.TryParse(hostAndPort[1], out var port))
            {
                return false;
            }

            string host = hostAndPort[0];
            if (host.StartsWith("emulator"))
            {
                host = "127.0.0.1";
                port += 1;
            }

            using var client = new TcpClient();
            try
            {
                IAsyncResult result = client.BeginConnect(host, port, null, null);
                bool success = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(0.5));

                if (success)
                {
                    client.EndConnect(result);
                    return true;
                }

                client.Close();
                return false;
            }
            catch
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
            switch (SettingsViewModel.ConnectSettings.ConnectConfig)
            {
                case "MuMuEmulator12":
                    AsstSetConnectionExtrasMuMu12(SettingsViewModel.ConnectSettings.MuMuEmulator12Extras.Config);
                    break;
                case "LDPlayer":
                    AsstSetConnectionExtrasLdPlayer(SettingsViewModel.ConnectSettings.LdPlayerExtras.Config);
                    break;
            }

            if (SettingsViewModel.ConnectSettings.AutoDetectConnection)
            {
                if (!AutoDetectConnection(ref error))
                {
                    return false;
                }
            }

            if (Connected && _connectedAdb == SettingsViewModel.ConnectSettings.AdbPath &&
                _connectedAddress == SettingsViewModel.ConnectSettings.ConnectAddress)
            {
                var actualConnectionStatus = CheckConnection(_connectedAdb, _connectedAddress);
                if (!actualConnectionStatus)
                {
                    Connected = false;
                    _logger.Information($"Connection lost to {_connectedAdb} {_connectedAddress}");
                    error = "Connection lost";
                }
                else
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

                    ToastNotification.ShowDirect("Auto Reload");

                    return true;
                }
            }

            bool ret = AsstConnect(_handle, SettingsViewModel.ConnectSettings.AdbPath, SettingsViewModel.ConnectSettings.ConnectAddress, SettingsViewModel.ConnectSettings.ConnectConfig);

            // 尝试默认的备选端口
            if (!ret && SettingsViewModel.ConnectSettings.AutoDetectConnection)
            {
                if (SettingsViewModel.ConnectSettings.DefaultAddress.TryGetValue(SettingsViewModel.ConnectSettings.ConnectConfig, out var value))
                {
                    foreach (var address in value
                                 .TakeWhile(_ => !_runningState.GetIdle()))
                    {
                        ret = AsstConnect(_handle, SettingsViewModel.ConnectSettings.AdbPath, address, SettingsViewModel.ConnectSettings.ConnectConfig);
                        if (!ret)
                        {
                            continue;
                        }

                        SettingsViewModel.ConnectSettings.ConnectAddress = address;
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

            if (!ret)
            {
                error = LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("CheckSettings");
            }
            else if (SettingsViewModel.ConnectSettings.AutoDetectConnection && !SettingsViewModel.ConnectSettings.AlwaysAutoDetectConnection)
            {
                SettingsViewModel.ConnectSettings.AutoDetectConnection = false;
            }

            return ret;
        }

        private static bool AutoDetectConnection(ref string error)
        {
            var adbPath = SettingsViewModel.ConnectSettings.AdbPath;
            bool adbResult = !string.IsNullOrEmpty(adbPath) &&
                             File.Exists(adbPath) &&
                             Path.GetFileName(adbPath).Contains("adb", StringComparison.InvariantCultureIgnoreCase) &&
                             CheckConnection(adbPath, SettingsViewModel.ConnectSettings.ConnectAddress);

            if (adbResult)
            {
                error = string.Empty;
                return true;
            }

            // 蓝叠的特殊处理
            {
                string bsHvAddress = SettingsViewModel.ConnectSettings.TryToSetBlueStacksHyperVAddress() ?? string.Empty;
                bool bsResult = CheckConnection(adbPath, bsHvAddress);
                if (bsResult)
                {
                    error = string.Empty;
                    if (string.IsNullOrEmpty(SettingsViewModel.ConnectSettings.AdbPath) && SettingsViewModel.ConnectSettings.DetectAdbConfig(ref error))
                    {
                        return string.IsNullOrEmpty(error);
                    }

                    SettingsViewModel.ConnectSettings.ConnectAddress = bsHvAddress;
                    return true;
                }
            }

            if (SettingsViewModel.ConnectSettings.DetectAdbConfig(ref error))
            {
                // https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8547
                // DetectAdbConfig 会把 ConnectAddress 变成第一个不是 emulator 开头的地址，可能会存在多开问题
                error = string.Empty;
                return true;
            }

            return false;
        }

        private AsstTaskId AsstAppendTaskWithEncoding(AsstTaskType type, JObject? taskParams = null)
        {
            taskParams ??= [];
            return AsstAppendTask(_handle, type.ToString(), JsonConvert.SerializeObject(taskParams));
        }

        private bool AsstSetTaskParamsWithEncoding(AsstTaskId id, JObject? taskParams = null)
        {
            if (id == 0)
            {
                return false;
            }

            taskParams ??= [];
            return AsstSetTaskParams(_handle, id, JsonConvert.SerializeObject(taskParams));
        }

        [SuppressMessage("ReSharper", "UnusedMember.Local")]
        public enum TaskType
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
            Reclamation,
            MiniGame,
            Custom,
        }

        private readonly HashSet<TaskType> _mainTaskTypes =
        [
            TaskType.StartUp,
            TaskType.Fight,
            TaskType.FightRemainingSanity,
            TaskType.Recruit,
            TaskType.Infrast,
            TaskType.Mall,
            TaskType.Award,
            TaskType.Roguelike,
            TaskType.Reclamation,
        ];

        private readonly Dictionary<AsstTaskId, TaskType> _taskStatus = [];

        public IReadOnlyDictionary<AsstTaskId, TaskType> TaskStatus => new Dictionary<AsstTaskId, TaskType>(_taskStatus);

        public bool AsstAppendCloseDown(string clientType)
        {
            if (!AsstStop())
            {
                _logger.Warning("Failed to stop Asst");
            }

            var (type, param) = new AsstCloseDownTask() { ClientType = clientType }.Serialize();
            return AsstAppendTaskWithEncoding(TaskType.CloseDown, type, param);
        }

        /// <summary>
        /// <c>CloseDown</c> 任务。
        /// </summary>
        /// <param name="clientType">客户端版本。</param>
        /// <returns>是否成功。</returns>
        public bool AsstStartCloseDown(string clientType)
        {
            return AsstAppendCloseDown(clientType) && AsstStart();
        }

        public bool AsstBackToHome()
        {
            return MaaService.AsstBackToHome(_handle);
        }

        /// <summary>
        /// 仓库识别。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartDepot()
        {
            return AsstAppendTaskWithEncoding(TaskType.Depot, AsstTaskType.Depot) && AsstStart();
        }

        /// <summary>
        /// 干员识别。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartOperBox()
        {
            return AsstAppendTaskWithEncoding(TaskType.OperBox, AsstTaskType.OperBox) && AsstStart();
        }

        public bool AsstStartGacha(bool once = true)
        {
            var task = new AsstCustomTask()
            {
                CustomTasks = [once ? "GachaOnce" : "GachaTenTimes"],
            };
            var (type, param) = task.Serialize();
            return AsstAppendTaskWithEncoding(TaskType.Gacha, type, param) && AsstStart();
        }

        public bool AsstMiniGame(string taskName)
        {
            var task = new AsstCustomTask()
            {
                CustomTasks = [taskName],
            };
            var (type, param) = task.Serialize();
            return AsstAppendTaskWithEncoding(TaskType.MiniGame, type, param) && AsstStart();
        }

        public bool AsstStartVideoRec(string filename)
        {
            var taskParams = new JObject
            {
                ["filename"] = filename,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding(AsstTaskType.VideoRecognition, taskParams);
            _taskStatus.Add(id, TaskType.Copilot);
            return id != 0 && AsstStart();
        }

        public bool AsstAppendTaskWithEncoding(TaskType wpfTasktype, (AsstTaskType Type, JObject? TaskParams) task)
        {
            return AsstAppendTaskWithEncoding(wpfTasktype, task.Type, task.TaskParams);
        }

        public bool AsstAppendTaskWithEncoding(TaskType wpfTasktype, AsstTaskType type, JObject? taskParams = null)
        {
            taskParams ??= [];
            AsstTaskId id = AsstAppendTask(_handle, type.ToString(), JsonConvert.SerializeObject(taskParams));
            if (id == 0)
            {
                return false;
            }

            _taskStatus.Add(id, wpfTasktype);
            return true;
        }

        public bool AsstSetTaskParamsEncoded(AsstTaskId id, JObject? taskParams = null)
        {
            if (id == 0)
            {
                return false;
            }

            taskParams ??= [];
            return AsstSetTaskParams(_handle, id, JsonConvert.SerializeObject(taskParams));
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
                _taskStatus.Clear();
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

        /// <summary>
        /// 外部异步调用信息
        /// </summary>
        AsyncCallInfo,

        /// <summary>
        /// 实例已销毁
        /// </summary>
        Destroyed,

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

    public enum AsstStaticOptionKey
    {
        /// <summary>
        /// 无效
        /// </summary>
        Invalid,

        /// <summary>
        /// 用CPU进行OCR
        /// </summary>
        CpuOCR,

        /// <summary>
        /// 用GPU进行OCR
        /// </summary>
        GpuOCR,
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
