// <copyright file="AsstProxy.cs" company="MaaAssistantArknights">
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
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;
using HandyControl.Data;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;

namespace MaaWpfGui.Main
{
#pragma warning disable SA1135 // Using directives should be qualified

    using AsstHandle = IntPtr;
    using AsstInstanceOptionKey = Int32;

    using AsstTaskId = Int32;

#pragma warning restore SA1135 // Using directives should be qualified

#pragma warning disable SA1121 // Use built-in type alias

    /// <summary>
    /// MaaCore 代理类。
    /// </summary>
    public class AsstProxy
    {
        private readonly RunningState runningState;

        private delegate void CallbackDelegate(int msg, IntPtr json_buffer, IntPtr custom_arg);

        private delegate void ProcCallbackMsg(AsstMsg msg, JObject details);

        private static unsafe byte[] EncodeNullTerminatedUTF8(string s)
        {
            var enc = Encoding.UTF8.GetEncoder();
            fixed (char* c = s)
            {
                var len = enc.GetByteCount(c, s.Length, true);
                var buf = new byte[len + 1];
                fixed (byte* ptr = buf)
                {
                    enc.Convert(c, s.Length, ptr, len, true, out _, out _, out var completed);
                }

                return buf;
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern unsafe bool AsstLoadResource(byte* dirname);

        private static unsafe bool AsstLoadResource(string dirname)
        {
            fixed (byte* ptr = EncodeNullTerminatedUTF8(dirname))
            {
                return AsstLoadResource(ptr);
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern AsstHandle AsstCreate();

        [DllImport("MaaCore.dll")]
        private static extern AsstHandle AsstCreateEx(CallbackDelegate callback, IntPtr custom_arg);

        [DllImport("MaaCore.dll")]
        private static extern void AsstDestroy(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        private static extern unsafe bool AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, byte* value);

        private static unsafe bool AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, string value)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUTF8(value))
            {
                return AsstSetInstanceOption(handle, key, ptr1);
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern unsafe bool AsstConnect(AsstHandle handle, byte* adb_path, byte* address, byte* config);

        private static unsafe bool AsstConnect(AsstHandle handle, string adb_path, string address, string config)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUTF8(adb_path),
                ptr2 = EncodeNullTerminatedUTF8(address),
                ptr3 = EncodeNullTerminatedUTF8(config))
            {
                return AsstConnect(handle, ptr1, ptr2, ptr3);
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern unsafe AsstTaskId AsstAppendTask(AsstHandle handle, byte* type, byte* task_params);

        private static unsafe AsstTaskId AsstAppendTask(AsstHandle handle, string type, string task_params)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUTF8(type),
                ptr2 = EncodeNullTerminatedUTF8(task_params))
            {
                return AsstAppendTask(handle, ptr1, ptr2);
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern unsafe bool AsstSetTaskParams(AsstHandle handle, AsstTaskId id, byte* task_params);

        private static unsafe bool AsstSetTaskParams(AsstHandle handle, AsstTaskId id, string task_params)
        {
            fixed (byte* ptr1 = EncodeNullTerminatedUTF8(task_params))
            {
                return AsstSetTaskParams(handle, id, ptr1);
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern bool AsstStart(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        private static extern bool AsstRunning(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        private static extern bool AsstStop(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        private static extern unsafe void AsstLog(byte* level, byte* message);

        /// <summary>
        /// 记录日志。
        /// </summary>
        /// <param name="message">日志内容。</param>
        public static unsafe void AsstLog(string message)
        {
            var level = new ReadOnlySpan<byte>(new byte[] { (byte)'G', (byte)'U', (byte)'I', 0 });
            fixed (byte* ptr1 = level, ptr2 = EncodeNullTerminatedUTF8(message))
            {
                AsstLog(ptr1, ptr2);
            }
        }

        [DllImport("MaaCore.dll")]
        private static extern unsafe ulong AsstGetImage(AsstHandle handle, byte* buff, ulong buff_size);

        [DllImport("MaaCore.dll")]
        private static extern unsafe ulong AsstGetNullSize();

        public static unsafe BitmapImage AsstGetImage(AsstHandle handle)
        {
            byte[] buff = new byte[1280 * 720 * 3];
            ulong read_size = 0;
            fixed (byte* ptr = buff)
            {
                read_size = AsstGetImage(handle, ptr, (ulong)buff.Length);
            }

            if (read_size == AsstGetNullSize())
            {
                return null;
            }

            try
            {
                // buff is a png data
                var image = new BitmapImage();
                image.BeginInit();
                image.StreamSource = new MemoryStream(buff, 0, (int)read_size);
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

        private readonly CallbackDelegate _callback;

        /// <summary>
        /// Initializes a new instance of the <see cref="AsstProxy"/> class.
        /// </summary>
        public AsstProxy()
        {
            _callback = CallbackFunction;
            runningState = RunningState.Instance;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="AsstProxy"/> class.
        /// </summary>
        ~AsstProxy()
        {
            if (_handle != IntPtr.Zero)
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
            static bool LoadResIfExists(string path)
            {
                string resource = "\\resource";
                if (!Directory.Exists(path + resource))
                {
                    return true;
                }

                return AsstLoadResource(path);
            }

            string clientType = Instances.SettingsViewModel.ClientType;
            string mainRes = Directory.GetCurrentDirectory();
            string globalResource = mainRes + "\\resource\\global\\" + clientType;
            string mainCache = Directory.GetCurrentDirectory() + "\\cache";
            string globalCache = mainCache + "\\resource\\global\\" + clientType;

            string officialClientType = "Official";
            string bilibiliClientType = "Bilibili";

            bool loaded;
            if (clientType == string.Empty || clientType == officialClientType || clientType == bilibiliClientType)
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
        }

        /// <summary>
        /// 初始化。
        /// </summary>
        public void Init()
        {
            bool loaded = LoadResource();

            _handle = AsstCreateEx(_callback, IntPtr.Zero);

            if (loaded == false || _handle == IntPtr.Zero)
            {
                Execute.OnUIThread(() =>
                {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("ResourceBroken"), LocalizationHelper.GetString("Error"), iconKey: ResourceToken.FatalGeometry, iconBrushKey: ResourceToken.DangerBrush);
                    Application.Current.Shutdown();
                });
            }

            Instances.TaskQueueViewModel.SetInited();
            runningState.SetIdle(true);
            this.AsstSetInstanceOption(InstanceOptionKey.TouchMode, Instances.SettingsViewModel.TouchMode);
            this.AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, Instances.SettingsViewModel.DeploymentWithPause ? "1" : "0");
            this.AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, Instances.SettingsViewModel.AdbLiteEnabled ? "1" : "0");
            Execute.OnUIThread(async () =>
            {
                if (Instances.SettingsViewModel.RunDirectly)
                {
                    // 如果是直接运行模式，就先让按钮显示为运行
                    runningState.SetIdle(false);
                }

                await Task.Run(() => Instances.SettingsViewModel.TryToStartEmulator());

                // 一般是点了“停止”按钮了
                if (Instances.TaskQueueViewModel.Stopping)
                {
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                if (Instances.SettingsViewModel.RunDirectly)
                {
                    // 重置按钮状态，不影响LinkStart判断
                    runningState.SetIdle(true);
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
        /// If <paramref name="ptr"/> is <see cref="IntPtr.Zero"/>, the function returns <c>0</c>.
        /// </returns>
        [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int strlen(IntPtr ptr);

        private static string PtrToStringCustom(IntPtr ptr, Encoding enc)
        {
            if (ptr == IntPtr.Zero)
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

        private void CallbackFunction(int msg, IntPtr json_buffer, IntPtr custom_arg)
        {
            string json_str = PtrToStringCustom(json_buffer, Encoding.UTF8);

            // Console.WriteLine(json_str);
            JObject json = (JObject)JsonConvert.DeserializeObject(json_str);
            ProcCallbackMsg dlg = ProcMsg;
            Execute.OnUIThread(() =>
            {
                dlg((AsstMsg)msg, json);
            });
        }

        private IntPtr _handle;

        private void ProcMsg(AsstMsg msg, JObject details)
        {
            switch (msg)
            {
                case AsstMsg.InternalError:
                    break;

                case AsstMsg.InitFailed:
                    MessageBoxHelper.Show(LocalizationHelper.GetString("InitializationError"), LocalizationHelper.GetString("Error"), iconKey: ResourceToken.FatalGeometry, iconBrushKey: ResourceToken.DangerBrush);
                    Application.Current.Shutdown();
                    break;

                case AsstMsg.ConnectionInfo:
                    ProcConnectInfo(details);
                    break;

                case AsstMsg.TaskChainStart:
                    Instances.TaskQueueViewModel.Running = true;
                    goto case AsstMsg.TaskChainExtraInfo;  // fallthrough
                case AsstMsg.AllTasksCompleted:
                case AsstMsg.TaskChainError:
                case AsstMsg.TaskChainCompleted:
                case AsstMsg.TaskChainStopped:
                    Instances.TaskQueueViewModel.Running = false;
                    goto case AsstMsg.TaskChainExtraInfo;  // fallthrough
                case AsstMsg.TaskChainExtraInfo:
                    ProcTaskChainMsg(msg, details);
                    break;

                case AsstMsg.SubTaskError:
                case AsstMsg.SubTaskStart:
                case AsstMsg.SubTaskCompleted:
                case AsstMsg.SubTaskExtraInfo:
                    ProcSubTaskMsg(msg, details);
                    break;
            }
        }

        private bool connected = false;

        public bool Connected
        {
            get => connected;
            set => connected = value;
        }

        private string connectedAdb;
        private string connectedAddress;

        private void ProcConnectInfo(JObject details)
        {
            var what = details["what"].ToString();
            switch (what)
            {
                case "Connected":
                    connected = true;
                    connectedAdb = details["details"]["adb"].ToString();
                    connectedAddress = details["details"]["address"].ToString();
                    Instances.SettingsViewModel.ConnectAddress = connectedAddress;
                    break;

                case "UnsupportedResolution":
                    connected = false;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ResolutionNotSupported"), UiLogColor.Error);
                    break;

                case "ResolutionError":
                    connected = false;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ResolutionAcquisitionFailure"), UiLogColor.Error);
                    break;

                case "Reconnecting":
                    Instances.TaskQueueViewModel.AddLog($"{LocalizationHelper.GetString("TryToReconnect")}({Convert.ToUInt32(details["details"]["times"]) + 1})", UiLogColor.Error);
                    break;

                case "Reconnected":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReconnectSuccess"));
                    break;

                case "Disconnect":
                    connected = false;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReconnectFailed"), UiLogColor.Error);
                    if (runningState.GetIdle())
                    {
                        break;
                    }

                    AsstStop();

                    Execute.OnUIThread(async () =>
                    {
                        if (Instances.SettingsViewModel.RetryOnDisconnected)
                        {
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TryToStartEmulator"), UiLogColor.Error);
                            Instances.TaskQueueViewModel.KillEmulator();
                            await Task.Delay(3000);
                            await Instances.TaskQueueViewModel.Stop();
                            Instances.TaskQueueViewModel.SetStopped();
                            Instances.TaskQueueViewModel.LinkStart();
                        }
                    });

                    break;

                case "ScreencapFailed":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ScreencapFailed"), UiLogColor.Error);
                    break;

                case "TouchModeNotAvailable":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TouchModeNotAvailable"), UiLogColor.Error);
                    break;
            }
        }

        private void ProcTaskChainMsg(AsstMsg msg, JObject details)
        {
            string taskChain = details["taskchain"].ToString();

            if (taskChain == "CloseDown")
            {
                return;
            }

            if (taskChain == "Recruit")
            {
                if (msg == AsstMsg.TaskChainError)
                {
                    Instances.RecognizerViewModel.RecruitInfo = LocalizationHelper.GetString("IdentifyTheMistakes");
                    using var toast = new ToastNotification(LocalizationHelper.GetString("IdentifyTheMistakes"));
                    toast.Show();
                }
            }

            bool isCoplitTaskChain = taskChain == "Copilot" || taskChain == "VideoRecognition";

            switch (msg)
            {
                case AsstMsg.TaskChainStopped:
                    Instances.TaskQueueViewModel.SetStopped();
                    if (isCoplitTaskChain)
                    {
                        runningState.SetIdle(true);
                    }

                    break;

                case AsstMsg.TaskChainError:
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskError") + taskChain, UiLogColor.Error);
                        using var toast = new ToastNotification(LocalizationHelper.GetString("TaskError") + taskChain);
                        toast.Show();
                        if (isCoplitTaskChain)
                        {
                            runningState.SetIdle(true);
                            Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("CombatError"), UiLogColor.Error);
                        }

                        if (taskChain == "Fight" && (Instances.TaskQueueViewModel.Stage == "Annihilation"))
                        {
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AnnihilationTaskFailed"), UiLogColor.Warning);
                        }

                        break;
                    }

                case AsstMsg.TaskChainStart:
                    if (taskChain == "Fight")
                    {
                        Instances.TaskQueueViewModel.FightTaskRunning = true;
                    }
                    else if (taskChain == "Infrast")
                    {
                        Instances.TaskQueueViewModel.InfrastTaskRunning = true;
                    }

                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartTask") + taskChain);
                    break;

                case AsstMsg.TaskChainCompleted:
                    if (taskChain == "Infrast")
                    {
                        Instances.TaskQueueViewModel.IncreaseCustomInfrastPlanIndex();
                    }
                    else if (taskChain == "Mall")
                    {
                        if (Instances.TaskQueueViewModel.Stage != string.Empty && Instances.SettingsViewModel.CreditFightTaskEnabled)
                        {
                            Instances.SettingsViewModel.LastCreditFightTaskTime = DateTime.UtcNow.ToYJDate().ToFormattedString();
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString("CreditFight"));
                        }
                    }

                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + taskChain);
                    if (isCoplitTaskChain)
                    {
                        runningState.SetIdle(true);
                        Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("CompleteCombat"), UiLogColor.Info);
                    }

                    break;

                case AsstMsg.TaskChainExtraInfo:
                    break;

                case AsstMsg.AllTasksCompleted:
                    bool isMainTaskQueueAllCompleted = true;
                    var finished_tasks = details["finished_tasks"] as JArray;
                    if (finished_tasks.Count == 1)
                    {
                        var unique_finished_task = (AsstTaskId)finished_tasks[0];
                        if (unique_finished_task == (_latestTaskId.TryGetValue(TaskType.Copilot, out var copilotTaskId) ? copilotTaskId : 0)
                            || unique_finished_task == (_latestTaskId.TryGetValue(TaskType.RecruitCalc, out var recruitCalcTaskId) ? recruitCalcTaskId : 0)
                            || unique_finished_task == (_latestTaskId.TryGetValue(TaskType.CloseDown, out var closeDownTaskId) ? closeDownTaskId : 0)
                            || unique_finished_task == (_latestTaskId.TryGetValue(TaskType.Depot, out var depotTaskId) ? depotTaskId : 0)
                            || unique_finished_task == (_latestTaskId.TryGetValue(TaskType.OperBox, out var operBoxTaskId) ? operBoxTaskId : 0)
                            || unique_finished_task == (_latestTaskId.TryGetValue(TaskType.Gacha, out var gachaTaskId) ? gachaTaskId : 0))
                        {
                            isMainTaskQueueAllCompleted = false;
                        }
                    }

                    bool buy_wine = false;
                    if (_latestTaskId.ContainsKey(TaskType.Mall) && Instances.SettingsViewModel.DidYouBuyWine())
                    {
                        buy_wine = true;
                    }

                    _latestTaskId.Clear();

                    Instances.TaskQueueViewModel.ResetFightVariables();
                    runningState.SetIdle(true);
                    Instances.RecognizerViewModel.GachaDone = true;

                    if (isMainTaskQueueAllCompleted)
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AllTasksComplete"));
                        using (var toast = new ToastNotification(LocalizationHelper.GetString("AllTasksComplete")))
                        {
                            toast.Show();
                        }

                        // Instances.TaskQueueViewModel.CheckAndShutdown();
                        Instances.TaskQueueViewModel.CheckAfterCompleted();
                    }

                    if (buy_wine)
                    {
                        Instances.SettingsViewModel.Cheers = true;
                    }

                    break;
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
            }
        }

        private void ProcSubTaskError(JObject details)
        {
            string subTask = details["subtask"].ToString();

            switch (subTask)
            {
                case "StartGameTask":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FailedToOpenClient"), UiLogColor.Error);
                    break;

                case "AutoRecruitTask":
                    {
                        var why_str = details.TryGetValue("why", out var why) ? why.ToString() : LocalizationHelper.GetString("ErrorOccurred");
                        Instances.TaskQueueViewModel.AddLog(why_str + "，" + LocalizationHelper.GetString("HasReturned"), UiLogColor.Error);
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
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TheEX"), UiLogColor.Error);
                    break;
            }
        }

        private void ProcSubTaskStart(JObject details)
        {
            string subTask = details["subtask"].ToString();

            if (subTask == "ProcessTask")
            {
                string taskName = details["details"]["task"].ToString();
                int execTimes = (int)details["details"]["exec_times"];

                switch (taskName)
                {
                    case "StartButton2":
                    case "AnnihilationConfirm":
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MissionStart") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                        break;

                    case "MedicineConfirm":
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MedicineUsed") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                        break;

                    case "StoneConfirm":
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StoneUsed") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
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

                    case "StageTraderInvestConfirm":
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("HasInvested") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
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

                    case "StageEncounterEnter":
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Encounter"));
                        break;

                    // case "StageBoonsEnter":
                    //    Instances.TaskQueueViewModel.AddLog("古堡馈赠");
                    //    break;
                    case "StageCambatDpsEnter":
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CambatDps"));
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
            }
            else if (subTask == "CombatRecordRecognitionTask")
            {
                string what = details["what"]?.ToString();
                if (!string.IsNullOrEmpty(what))
                {
                    Instances.CopilotViewModel.AddLog(what);
                }
            }
        }

        private void ProcSubTaskCompleted(JObject details)
        {
            // DoNothing
            _ = details;
        }

        private void ProcSubTaskExtraInfo(JObject details)
        {
            string taskChain = details["taskchain"].ToString();

            if (taskChain == "Recruit")
            {
                ProcRecruitCalcMsg(details);
            }
            else if (taskChain == "VideoRecognition")
            {
                ProcVideoRecMsg(details);
            }

            var subTaskDetails = details["details"];
            if (taskChain == "Depot")
            {
                Instances.RecognizerViewModel.DepotParse((JObject)subTaskDetails);
            }

            if (taskChain == "OperBox")
            {
                Instances.RecognizerViewModel.OperBoxParse((JObject)subTaskDetails);
            }

            string what = details["what"].ToString();

            switch (what)
            {
                case "StageDrops":
                    {
                        string all_drops = string.Empty;
                        JArray statistics = (JArray)subTaskDetails["stats"];
                        foreach (var item in statistics)
                        {
                            string itemName = item["itemName"].ToString();
                            int totalQuantity = (int)item["quantity"];
                            int addQuantity = (int)item["addQuantity"];
                            all_drops += $"{itemName} : {totalQuantity}";
                            if (addQuantity > 0)
                            {
                                all_drops += $" (+{addQuantity})";
                            }

                            all_drops += "\n";
                        }

                        all_drops = all_drops.EndsWith("\n") ? all_drops.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TotalDrop") + "\n" + all_drops);
                    }

                    break;

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
                        JArray tags = (JArray)subTaskDetails["tags"];
                        string log_content = string.Empty;
                        foreach (var tag_name in tags)
                        {
                            string tag_str = tag_name.ToString();
                            log_content += tag_str + "\n";
                        }

                        log_content = log_content.EndsWith("\n") ? log_content.TrimEnd('\n') : LocalizationHelper.GetString("Error");
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RecruitingResults") + "\n" + log_content);
                    }

                    break;

                case "RecruitSpecialTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        if (special == "支援机械" && Instances.SettingsViewModel.NotChooseLevel1 == false)
                        {
                            break;
                        }

                        using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                        toast.AppendContentText(special).ShowRecruit();
                    }

                    break;

                case "RecruitRobotTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                        toast.AppendContentText(special).ShowRecruitRobot();
                    }

                    break;

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
                    }

                    break;

                case "RecruitTagsSelected":
                    {
                        JArray selected = (JArray)subTaskDetails["tags"];
                        string selected_log = string.Empty;
                        foreach (var tag in selected)
                        {
                            selected_log += tag + "\n";
                        }

                        selected_log = selected_log.EndsWith("\n") ? selected_log.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");

                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Choose") + " Tags：\n" + selected_log);
                    }

                    break;

                case "RecruitTagsRefreshed":
                    {
                        int refresh_count = (int)subTaskDetails["count"];
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Refreshed") + refresh_count + LocalizationHelper.GetString("UnitTime"));
                        break;
                    }

                case "NotEnoughStaff":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NotEnoughStaff"), UiLogColor.Error);
                    }

                    break;

                /* Roguelike */
                case "StageInfo":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartCombat") + subTaskDetails["name"]);
                    }

                    break;

                case "StageInfoError":
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageInfoError"), UiLogColor.Error);
                    }

                    break;

                case "PenguinId":
                    {
                        string id = subTaskDetails["id"].ToString();
                        Instances.SettingsViewModel.PenguinId = id;
                    }

                    break;

                case "BattleFormation":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormation") + "\n" + JsonConvert.SerializeObject(subTaskDetails["formation"]));
                    break;

                case "BattleFormationSelected":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormationSelected") + subTaskDetails["selected"]);
                    break;

                case "CopilotAction":
                    {
                        string doc = subTaskDetails["doc"].ToString();
                        if (doc.Length != 0)
                        {
                            string color = subTaskDetails["doc_color"].ToString();
                            Instances.CopilotViewModel.AddLog(doc, color.Length == 0 ? UiLogColor.Message : color);
                        }

                        Instances.CopilotViewModel.AddLog(
                            string.Format(LocalizationHelper.GetString("CurrentSteps"),
                                subTaskDetails["action"].ToString(),
                                subTaskDetails["target"].ToString()));
                    }

                    break;

                case "SSSStage":
                    {
                        Instances.CopilotViewModel.AddLog("CurrentStage: " + subTaskDetails["stage"].ToString(), UiLogColor.Info);
                    }

                    break;

                case "SSSSettlement":
                    {
                        Instances.CopilotViewModel.AddLog(details["why"].ToString(), UiLogColor.Info);
                    }

                    break;

                case "SSSGamePass":
                    {
                        Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("SSSGamePass"), UiLogColor.RareOperator);
                    }

                    break;

                case "UnsupportedLevel":
                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("UnsupportedLevel"), UiLogColor.Error);
                    break;

                case "CustomInfrastRoomOperators":
                    string nameStr = string.Empty;
                    foreach (var name in subTaskDetails["names"])
                    {
                        nameStr += name.ToString() + ", ";
                    }

                    if (nameStr != string.Empty)
                    {
                        nameStr = nameStr.Remove(nameStr.Length - 2);
                    }

                    Instances.TaskQueueViewModel.AddLog(nameStr.ToString());
                    break;

                /* 生息演算 */
                case "ReclamationReport":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AlgorithmFinish") + "\n" +
                        LocalizationHelper.GetString("AlgorithmBadge") + ": " + $"{(int)subTaskDetails["total_badges"]}(+{(int)subTaskDetails["badges"]})" + "\n" +
                        LocalizationHelper.GetString("AlgorithmConstructionPoint") + ": " + $"{(int)subTaskDetails["total_construction_points"]}(+{(int)subTaskDetails["construction_points"]})");
                    break;

                case "ReclamationProcedureStart":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MissionStart") + $" {(int)subTaskDetails["times"]} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                    break;

                case "ReclamationSmeltGold":
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AlgorithmDoneSmeltGold") + $" {(int)subTaskDetails["times"]} " + LocalizationHelper.GetString("UnitTime"));
                    break;
            }
        }

        private void ProcRecruitCalcMsg(JObject details)
        {
            Instances.RecognizerViewModel.procRecruitMsg(details);
        }

        private void ProcVideoRecMsg(JObject details)
        {
            string what = details["what"].ToString();

            switch (what)
            {
                case "Finished":
                    var filename = details["details"]["filename"].ToString();
                    Instances.CopilotViewModel.AddLog("Save to: " + filename, UiLogColor.Info);

                    // string p = @"C:\tmp\this path contains spaces, and,commas\target.txt";
                    string args = string.Format("/e, /select, \"{0}\"", filename);

                    ProcessStartInfo info = new ProcessStartInfo
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

        /// <summary>
        /// 连接模拟器。
        /// </summary>
        /// <param name="error">具体的连接错误。</param>
        /// <returns>是否成功。</returns>
        public bool AsstConnect(ref string error)
        {
            if (Instances.SettingsViewModel.AutoDetectConnection)
            {
                string bsHvAddress = Instances.SettingsViewModel.TryToSetBlueStacksHyperVAddress();
                if (bsHvAddress != null)
                {
                    Instances.SettingsViewModel.ConnectAddress = bsHvAddress;
                }
                else if (!Instances.SettingsViewModel.DetectAdbConfig(ref error))
                {
                    return false;
                }
            }

            if (connected && connectedAdb == Instances.SettingsViewModel.AdbPath &&
                connectedAddress == Instances.SettingsViewModel.ConnectAddress)
            {
                return true;
            }

            if (!LoadResource())
            {
                error = "Load Resource Failed";
                return false;
            }

            bool ret = AsstConnect(_handle, Instances.SettingsViewModel.AdbPath, Instances.SettingsViewModel.ConnectAddress, Instances.SettingsViewModel.ConnectConfig);

            // 尝试默认的备选端口
            if (!ret && Instances.SettingsViewModel.AutoDetectConnection)
            {
                foreach (var address in Instances.SettingsViewModel.DefaultAddress[Instances.SettingsViewModel.ConnectConfig])
                {
                    if (runningState.GetIdle())
                    {
                        break;
                    }

                    ret = AsstConnect(_handle, Instances.SettingsViewModel.AdbPath, address, Instances.SettingsViewModel.ConnectConfig);
                    if (ret)
                    {
                        Instances.SettingsViewModel.ConnectAddress = address;
                        break;
                    }
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

        private AsstTaskId AsstAppendTaskWithEncoding(string type, JObject task_params = null)
        {
            task_params ??= new JObject();
            return AsstAppendTask(_handle, type, JsonConvert.SerializeObject(task_params));
        }

        private bool AsstSetTaskParamsWithEncoding(AsstTaskId id, JObject task_params = null)
        {
            if (id == 0)
            {
                return false;
            }

            task_params ??= new JObject();
            return AsstSetTaskParams(_handle, id, JsonConvert.SerializeObject(task_params));
        }

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
        }

        private readonly Dictionary<TaskType, AsstTaskId> _latestTaskId = new Dictionary<TaskType, AsstTaskId>();

        private JObject SerializeFightTaskParams(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity)
        {
            var task_params = new JObject
            {
                ["stage"] = stage,
                ["medicine"] = max_medicine,
                ["stone"] = max_stone,
                ["times"] = max_times,
                ["report_to_penguin"] = true,
            };
            if (drops_item_quantity != 0 && !string.IsNullOrWhiteSpace(drops_item_id))
            {
                task_params["drops"] = new JObject
                {
                    [drops_item_id] = drops_item_quantity,
                };
            }

            task_params["client_type"] = Instances.SettingsViewModel.ClientType;
            task_params["penguin_id"] = Instances.SettingsViewModel.PenguinId;
            task_params["DrGrandet"] = Instances.SettingsViewModel.IsDrGrandet;
            task_params["expiring_medicine"] = Instances.SettingsViewModel.UseExpiringMedicine ? 9999 : 0;
            task_params["server"] = Instances.SettingsViewModel.ServerType;
            return task_params;
        }

        /// <summary>
        /// 刷理智。
        /// </summary>
        /// <param name="stage">关卡名。</param>
        /// <param name="max_medicine">最大使用理智药数量。</param>
        /// <param name="max_stone">最大吃石头数量。</param>
        /// <param name="max_times">指定次数。</param>
        /// <param name="drops_item_id">指定掉落 ID。</param>
        /// <param name="drops_item_quantity">指定掉落数量。</param>
        /// <param name="is_main_fight">是否是主任务，决定c#侧是否记录任务id</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendFight(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity, bool is_main_fight = true)
        {
            var task_params = SerializeFightTaskParams(stage, max_medicine, max_stone, max_times, drops_item_id, drops_item_quantity);
            AsstTaskId id = AsstAppendTaskWithEncoding("Fight", task_params);
            if (is_main_fight)
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
        /// <param name="max_medicine">最大使用理智药数量。</param>
        /// <param name="max_stone">最大吃石头数量。</param>
        /// <param name="max_times">指定次数。</param>
        /// <param name="drops_item_id">指定掉落 ID。</param>
        /// <param name="drops_item_quantity">指定掉落数量。</param>
        /// <param name="is_main_fight">是否是主任务，决定c#侧是否记录任务id</param>
        /// <returns>是否成功。</returns>
        public bool AsstSetFightTaskParams(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity, bool is_main_fight = true)
        {
            var type = is_main_fight ? TaskType.Fight : TaskType.FightRemainingSanity;
            if (!_latestTaskId.ContainsKey(type))
            {
                return false;
            }

            var id = _latestTaskId[type];
            if (id == 0)
            {
                return false;
            }

            var task_params = SerializeFightTaskParams(stage, max_medicine, max_stone, max_times, drops_item_id, drops_item_quantity);
            return AsstSetTaskParamsWithEncoding(id, task_params);
        }

        /// <summary>
        /// 领取日常奖励。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstAppendAward()
        {
            AsstTaskId id = AsstAppendTaskWithEncoding("Award");
            _latestTaskId[TaskType.Award] = id;
            return id != 0;
        }

        /// <summary>
        /// 开始唤醒。
        /// </summary>
        /// <param name="client_type">客户端版本。</param>
        /// <param name="enable">是否自动启动客户端。</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendStartUp(string client_type, bool enable)
        {
            var task_params = new JObject
            {
                ["client_type"] = client_type,
                ["start_game_enabled"] = enable,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("StartUp", task_params);
            _latestTaskId[TaskType.StartUp] = id;
            return id != 0;
        }

        public bool AsstAppendCloseDown()
        {
            AsstStop();
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

        /// <summary>
        /// 领取信用及商店购物。
        /// </summary>
        /// <param name="credit_fight">是否信用战斗。</param>
        /// <param name="with_shopping">是否购物。</param>
        /// <param name="first_list">优先购买列表。</param>
        /// <param name="blacklist">黑名单列表。</param>
        /// <param name="force_shopping_if_credit_full">是否在信用溢出时无视黑名单</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendMall(bool credit_fight, bool with_shopping, string[] first_list, string[] blacklist, bool force_shopping_if_credit_full)
        {
            var task_params = new JObject
            {
                ["credit_fight"] = credit_fight,
                ["shopping"] = with_shopping,
                ["buy_first"] = new JArray { first_list },
                ["blacklist"] = new JArray { blacklist },
                ["force_shopping_if_credit_full"] = force_shopping_if_credit_full,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("Mall", task_params);
            _latestTaskId[TaskType.Mall] = id;
            return id != 0;
        }

        /// <summary>
        /// 公开招募。
        /// </summary>
        /// <param name="max_times">加急次数，仅在 <paramref name="use_expedited"/> 为 <see langword="true"/> 时有效。</param>
        /// <param name="select_level">会去点击标签的 Tag 等级。</param>
        /// <param name="confirm_level">会去点击确认的 Tag 等级。若仅公招计算，可设置为空数组。</param>
        /// <param name="need_refresh">是否刷新三星 Tags。</param>
        /// <param name="use_expedited">是否使用加急许可。</param>
        /// <param name="skip_robot">是否在识别到小车词条时跳过。</param>
        /// <param name="is_level3_use_short_time">三星Tag是否使用短时间（7:40）</param>
        /// <param name="is_level3_use_short_time2">三星Tag是否使用短时间（1:00）</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendRecruit(int max_times, int[] select_level, int[] confirm_level, bool need_refresh, bool use_expedited,
            bool skip_robot, bool is_level3_use_short_time, bool is_level3_use_short_time2 = false)
        {
            var task_params = new JObject
            {
                ["refresh"] = need_refresh,
                ["select"] = new JArray(select_level),
                ["confirm"] = new JArray(confirm_level),
                ["times"] = max_times,
                ["set_time"] = true,
                ["expedite"] = use_expedited,
                ["expedite_times"] = max_times,
                ["skip_robot"] = skip_robot,
            };
            if (is_level3_use_short_time)
            {
                task_params["recruitment_time"] = new JObject
                {
                    ["3"] = 460, // 7:40
                };
            }
            else if (is_level3_use_short_time2)
            {
                task_params["recruitment_time"] = new JObject
                {
                    ["3"] = 60, // 1:00
                };
            }

            task_params["report_to_penguin"] = true;
            task_params["report_to_yituliu"] = true;
            task_params["penguin_id"] = Instances.SettingsViewModel.PenguinId;
            task_params["server"] = Instances.SettingsViewModel.ServerType;

            AsstTaskId id = AsstAppendTaskWithEncoding("Recruit", task_params);
            _latestTaskId[TaskType.Recruit] = id;
            return id != 0;
        }

        private JObject SerializeInfrastTaskParams(string[] order, string uses_of_drones, double dorm_threshold, bool dorm_filter_not_stationed_enabled, bool dorm_dorm_trust_enabled, bool originium_shard_auto_replenishment,
            bool is_custom, string filename, int plan_index)
        {
            var task_params = new JObject
            {
                ["facility"] = new JArray(order),
                ["drones"] = uses_of_drones,
                ["threshold"] = dorm_threshold,
                ["dorm_notstationed_enabled"] = dorm_filter_not_stationed_enabled,
                ["dorm_trust_enabled"] = dorm_dorm_trust_enabled,
                ["replenish"] = originium_shard_auto_replenishment,
                ["mode"] = is_custom ? 10000 : 0,
                ["filename"] = filename,
                ["plan_index"] = plan_index,
            };

            return task_params;
        }

        /// <summary>
        /// 基建换班。
        /// </summary>
        /// <param name="order">要换班的设施（有序）。</param>
        /// <param name="uses_of_drones">
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
        /// <param name="dorm_threshold">宿舍进驻心情阈值。</param>
        /// <param name="dorm_filter_not_stationed_enabled">宿舍是否使用未进驻筛选标签</param>
        /// <param name="dorm_dorm_trust_enabled">宿舍是否使用蹭信赖功能</param>
        /// <param name="originium_shard_auto_replenishment">制造站搓玉是否补货</param>
        /// <param name="is_custom">是否开启自定义配置</param>
        /// <param name="filename">自定义配置文件路径</param>
        /// <param name="plan_index">自定义配置计划编号</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendInfrast(string[] order, string uses_of_drones, double dorm_threshold,
            bool dorm_filter_not_stationed_enabled, bool dorm_dorm_trust_enabled, bool originium_shard_auto_replenishment,
            bool is_custom, string filename, int plan_index)
        {
            var task_params = SerializeInfrastTaskParams(
                order, uses_of_drones, dorm_threshold,
                dorm_filter_not_stationed_enabled, dorm_dorm_trust_enabled, originium_shard_auto_replenishment,
                is_custom, filename, plan_index);
            AsstTaskId id = AsstAppendTaskWithEncoding("Infrast", task_params);
            _latestTaskId[TaskType.Infrast] = id;
            return id != 0;
        }

        public bool AsstSetInfrastTaskParams(string[] order, string uses_of_drones, double dorm_threshold,
            bool dorm_filter_not_stationed_enabled, bool dorm_dorm_trust_enabled, bool originium_shard_auto_replenishment,
            bool is_custom, string filename, int plan_index)
        {
            var type = TaskType.Infrast;
            if (!_latestTaskId.ContainsKey(type))
            {
                return false;
            }

            var id = _latestTaskId[type];
            if (id == 0)
            {
                return false;
            }

            var task_params = SerializeInfrastTaskParams(
                order, uses_of_drones, dorm_threshold,
                dorm_filter_not_stationed_enabled, dorm_dorm_trust_enabled, originium_shard_auto_replenishment,
                is_custom, filename, plan_index);
            return AsstSetTaskParamsWithEncoding(id, task_params);
        }

        /// <summary>
        /// 无限刷肉鸽。
        /// </summary>
        /// <param name="mode">
        /// 模式。可用值包括：
        /// <list type="bullet">
        ///     <item>
        ///         <term><c>0</c></term>
        ///         <description>刷蜡烛，尽可能稳定的打更多层数。</description>
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
        /// <param name="investment_enabled">是否投资源石锭</param>
        /// <param name="invests">投资源石锭次数。</param>
        /// <param name="stop_when_full">投资满了自动停止任务。</param>
        /// <param name="squad"><paramref name="squad"/> TODO.</param>
        /// <param name="roles"><paramref name="roles"/> TODO.</param>
        /// <param name="core_char"><paramref name="core_char"/> TODO.</param>
        /// <param name="use_support">是否core_char使用好友助战</param>
        /// <param name="enable_nonfriend_support">是否允许使用非好友助战</param>
        /// <param name="theme">肉鸽名字。["Phantom", "Mizuki", "Sami"]</param>
        /// <param name="refresh_trader_with_dice">是否用骰子刷新商店购买特殊商品，目前支持水月肉鸽的指路鳞</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendRoguelike(int mode, int starts, bool investment_enabled, int invests, bool stop_when_full,
            string squad, string roles, string core_char, bool use_support, bool enable_nonfriend_support, string theme, bool refresh_trader_with_dice)
        {
            var task_params = new JObject
            {
                ["mode"] = mode,
                ["starts_count"] = starts,
                ["investment_enabled"] = investment_enabled,
                ["investments_count"] = invests,
                ["stop_when_investment_full"] = stop_when_full,
                ["theme"] = theme,
            };

            if (mode == 1)
            {
                task_params["investment_enabled"] = true;
            }

            if (squad.Length > 0)
            {
                task_params["squad"] = squad;
            }

            if (roles.Length > 0)
            {
                task_params["roles"] = roles;
            }

            if (core_char.Length > 0)
            {
                task_params["core_char"] = core_char;
            }

            task_params["use_support"] = use_support;
            task_params["use_nonfriend_support"] = enable_nonfriend_support;
            task_params["refresh_trader_with_dice"] = refresh_trader_with_dice;

            AsstTaskId id = AsstAppendTaskWithEncoding("Roguelike", task_params);
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
            _latestTaskId[TaskType.Recruit] = id;
            return id != 0;
        }

        /// <summary>
        /// 公招识别。
        /// </summary>
        /// <param name="select_level">会去点击标签的 Tag 等级。</param>
        /// <param name="set_time">是否设置 9 小时。</param>
        /// <returns>是否成功。</returns>
        public bool AsstStartRecruitCalc(int[] select_level, bool set_time)
        {
            var task_params = new JObject
            {
                ["refresh"] = false,
                ["select"] = new JArray(select_level),
                ["confirm"] = new JArray(),
                ["times"] = 0,
                ["set_time"] = set_time,
                ["expedite"] = false,
                ["expedite_times"] = 0,
                ["report_to_penguin"] = true,
                ["report_to_yituliu"] = true,
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

            task_params["recruitment_time"] = new JObject { { "3", recruitmentTime } };
            task_params["penguin_id"] = Instances.SettingsViewModel.PenguinId;
            task_params["yituliu_id"] = Instances.SettingsViewModel.PenguinId; // 一图流说随便传个uuid就行，让client自己生成，所以先直接嫖一下企鹅的（
            task_params["server"] = Instances.SettingsViewModel.ServerType;

            AsstTaskId id = AsstAppendTaskWithEncoding("Recruit", task_params);
            _latestTaskId[TaskType.RecruitCalc] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 仓库识别。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartDepot()
        {
            var task_params = new JObject();
            AsstTaskId id = AsstAppendTaskWithEncoding("Depot", task_params);
            _latestTaskId[TaskType.Depot] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 干员识别。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartOperBox()
        {
            var task_params = new JObject();
            AsstTaskId id = AsstAppendTaskWithEncoding("OperBox", task_params);
            _latestTaskId[TaskType.OperBox] = id;
            return id != 0 && AsstStart();
        }

        public bool AsstStartGacha(bool once = true)
        {
            var task_params = new JObject
            {
                ["task_names"] = new JArray { once ? "GachaOnce" : "GachaTenTimes" },
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("Custom", task_params);
            _latestTaskId[TaskType.Gacha] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 自动抄作业。
        /// </summary>
        /// <param name="filename">作业 JSON 的文件路径，绝对、相对路径均可。</param>
        /// <param name="formation">是否进行 “快捷编队”。</param>
        /// <param name="type">任务类型</param>
        /// <param name="loop_times">任务重复执行次数</param>
        /// <returns>是否成功。</returns>
        public bool AsstStartCopilot(string filename, bool formation, string type, int loop_times)
        {
            var task_params = new JObject
            {
                ["filename"] = filename,
                ["formation"] = formation,
                ["loop_times"] = loop_times,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding(type, task_params);
            _latestTaskId[TaskType.Copilot] = id;
            return id != 0 && AsstStart();
        }

        public bool AsstStartVideoRec(string filename)
        {
            var task_params = new JObject
            {
                ["filename"] = filename,
            };
            AsstTaskId id = AsstAppendTaskWithEncoding("VideoRecognition", task_params);
            _latestTaskId[TaskType.Copilot] = id;
            return id != 0 && AsstStart();
        }

        /// <summary>
        /// 启动。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStart()
        {
            return AsstStart(_handle);
        }

        /// <summary>
        /// 运行中。
        /// </summary>
        /// <returns>是否正在运行。</returns>
        public bool AsstRunning()
        {
            return AsstRunning(_handle);
        }

        /// <summary>
        /// 停止。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStop()
        {
            bool ret = AsstStop(_handle);
            _latestTaskId.Clear();
            return ret;
        }

        /// <summary>
        /// 销毁。
        /// </summary>
        public void AsstDestroy()
        {
            AsstDestroy(_handle);
        }
    }

    /// <summary>
    /// MaaCore 消息。
    /// </summary>
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
