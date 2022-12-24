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
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MaaWpfGui
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

        private readonly CallbackDelegate _callback;

        /// <summary>
        /// Initializes a new instance of the <see cref="AsstProxy"/> class.
        /// </summary>
        /// <param name="container">IoC 容器。</param>
        /// <param name="windowManager">当前窗口。</param>
        public AsstProxy(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            _callback = CallbackFunction;
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

        private string _curResource = "_Unloaded";

        private static readonly bool ForcedReloadResource = File.Exists("DEBUG") || File.Exists("DEBUG.txt");

        /// <summary>
        /// 加载全局资源。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool LoadResource()
        {
            var settingsModel = _container.Get<SettingsViewModel>();
            if (!ForcedReloadResource && settingsModel.ClientType == _curResource)
            {
                return true;
            }

            bool loaded = false;
            if (settingsModel.ClientType == string.Empty
                || settingsModel.ClientType == "Official" || settingsModel.ClientType == "Bilibili")
            {
                // The resources of Official and Bilibili are the same
                if (!ForcedReloadResource && (_curResource == "Official" || _curResource == "Bilibili"))
                {
                    return true;
                }

                loaded = AsstLoadResource(Directory.GetCurrentDirectory());
            }
            else if (_curResource == "Official" || _curResource == "Bilibili")
            {
                // Load basic resources for CN client first
                // Then load global incremental resources
                loaded = AsstLoadResource(Directory.GetCurrentDirectory() + "\\resource\\global\\" + settingsModel.ClientType);
            }
            else
            {
                // Load basic resources for CN client first
                // Then load global incremental resources
                loaded = AsstLoadResource(Directory.GetCurrentDirectory())
                    && AsstLoadResource(Directory.GetCurrentDirectory() + "\\resource\\global\\" + settingsModel.ClientType);
            }

            if (!loaded)
            {
                return false;
            }

            if (ForcedReloadResource)
            {
                Execute.OnUIThread(() =>
                {
                    using (var toast = new ToastNotification("Auto Reload"))
                    {
                        toast.Show();
                    }
                });
            }

            if (settingsModel.ClientType == string.Empty)
            {
                _curResource = "Official";
            }
            else
            {
                _curResource = settingsModel.ClientType;
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
                    _windowManager.ShowMessageBox(Localization.GetString("ResourceBroken"), Localization.GetString("Error"), icon: MessageBoxImage.Error);
                    Application.Current.Shutdown();
                });
            }

            var mainModel = _container.Get<TaskQueueViewModel>();
            mainModel.SetInited();
            mainModel.Idle = true;
            var settingsModel = _container.Get<SettingsViewModel>();
            settingsModel.UpdateInstanceSettings();
            Execute.OnUIThread(async () =>
            {
                var task = Task.Run(() =>
                {
                    settingsModel.TryToStartEmulator();
                });
                await task;
                if (settingsModel.RunDirectly)
                {
                    mainModel.LinkStart();
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

        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;
        private IntPtr _handle;

        private void ProcMsg(AsstMsg msg, JObject details)
        {
            switch (msg)
            {
                case AsstMsg.InternalError:
                    break;

                case AsstMsg.InitFailed:
                    _windowManager.ShowMessageBox(Localization.GetString("InitializationError"), Localization.GetString("Error"), icon: MessageBoxImage.Error);
                    Application.Current.Shutdown();
                    break;

                case AsstMsg.ConnectionInfo:
                    ProcConnectInfo(details);
                    break;

                case AsstMsg.AllTasksCompleted:
                case AsstMsg.TaskChainError:
                case AsstMsg.TaskChainStart:
                case AsstMsg.TaskChainCompleted:
                case AsstMsg.TaskChainExtraInfo:
                case AsstMsg.TaskChainStopped:
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
        private string connectedAdb;
        private string connectedAddress;

        private void ProcConnectInfo(JObject details)
        {
            var what = details["what"].ToString();
            var svm = _container.Get<SettingsViewModel>();
            var mainModel = _container.Get<TaskQueueViewModel>();
            switch (what)
            {
                case "Connected":
                    connected = true;
                    connectedAdb = details["details"]["adb"].ToString();
                    connectedAddress = details["details"]["address"].ToString();
                    svm.ConnectAddress = connectedAddress;
                    break;

                case "UnsupportedResolution":
                    connected = false;
                    mainModel.AddLog(Localization.GetString("ResolutionNotSupported"), UILogColor.Error);
                    break;

                case "ResolutionError":
                    connected = false;
                    mainModel.AddLog(Localization.GetString("ResolutionAcquisitionFailure"), UILogColor.Error);
                    break;

                case "Reconnecting":
                    mainModel.AddLog($"{Localization.GetString("TryToReconnect")}({Convert.ToUInt32(details["details"]["times"]) + 1})", UILogColor.Error);
                    break;

                case "Reconnected":
                    mainModel.AddLog(Localization.GetString("ReconnectSuccess"));
                    break;

                case "Disconnect":
                    connected = false;
                    mainModel.AddLog(Localization.GetString("ReconnectFailed"), UILogColor.Error);
                    if (mainModel.Idle)
                    {
                        break;
                    }

                    AsstStop();

                    var settingsModel = _container.Get<SettingsViewModel>();
                    Execute.OnUIThread(async () =>
                    {
                        if (settingsModel.RetryOnDisconnected)
                        {
                            mainModel.AddLog(Localization.GetString("TryToStartEmulator"), UILogColor.Error);
                            mainModel.KillEmulator();
                            await Task.Delay(3000);
                            mainModel.Stop();
                            mainModel.SetStopped();
                            mainModel.LinkStart();
                        }
                    });

                    break;

                case "ScreencapFailed":
                    mainModel.AddLog(Localization.GetString("ScreencapFailed"), UILogColor.Error);
                    break;

                case "TouchModeNotAvaiable":
                    mainModel.AddLog(Localization.GetString("TouchModeNotAvaiable"), UILogColor.Error);
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
                    var recruitModel = _container.Get<RecruitViewModel>();
                    recruitModel.RecruitInfo = Localization.GetString("IdentifyTheMistakes");
                }
            }

            var mainModel = _container.Get<TaskQueueViewModel>();
            var settingsModel = _container.Get<SettingsViewModel>();
            var copilotModel = _container.Get<CopilotViewModel>();

            bool isCoplitTaskChain = taskChain == "Copilot";

            switch (msg)
            {
                case AsstMsg.TaskChainStopped:
                    mainModel.SetStopped();
                    if (isCoplitTaskChain)
                    {
                        copilotModel.Idle = true;
                    }

                    break;

                case AsstMsg.TaskChainError:
                    mainModel.AddLog(Localization.GetString("TaskError") + taskChain, UILogColor.Error);
                    if (isCoplitTaskChain)
                    {
                        copilotModel.Idle = true;
                        copilotModel.AddLog(Localization.GetString("CombatError"), UILogColor.Error);
                    }

                    break;

                case AsstMsg.TaskChainStart:
                    if (taskChain == "Fight")
                    {
                        mainModel.FightTaskRunning = true;
                    }
                    else if (taskChain == "Infrast")
                    {
                        mainModel.InfrastTaskRunning = true;
                    }

                    mainModel.AddLog(Localization.GetString("StartTask") + taskChain);
                    break;

                case AsstMsg.TaskChainCompleted:
                    if (taskChain == "Infrast")
                    {
                        mainModel.IncreaseCustomInfrastPlanIndex();
                    }
                    else if (taskChain == "Mall")
                    {
                        if (settingsModel.CreditFightTaskEnabled)
                        {
                            settingsModel.LastCreditFightTaskTime = Utils.GetYJTimeDateString();
                        }
                    }

                    mainModel.AddLog(Localization.GetString("CompleteTask") + taskChain);
                    if (isCoplitTaskChain)
                    {
                        copilotModel.Idle = true;
                        copilotModel.AddLog(Localization.GetString("CompleteCombat"), UILogColor.Info);
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
                            || unique_finished_task == (_latestTaskId.TryGetValue(TaskType.Depot, out var depotTaskId) ? depotTaskId : 0))
                        {
                            isMainTaskQueueAllCompleted = false;
                        }
                    }

                    bool buy_wine = false;
                    if (_latestTaskId.ContainsKey(TaskType.Mall) && settingsModel.DidYouBuyWine())
                    {
                        buy_wine = true;
                    }

                    _latestTaskId.Clear();

                    mainModel.Idle = true;
                    mainModel.UseStone = false;
                    copilotModel.Idle = true;

                    if (isMainTaskQueueAllCompleted)
                    {
                        mainModel.AddLog(Localization.GetString("AllTasksComplete"));
                        using (var toast = new ToastNotification(Localization.GetString("AllTasksComplete")))
                        {
                            toast.Show();
                        }

                        // mainModel.CheckAndShutdown();
                        mainModel.CheckAfterCompleted();
                    }

                    if (buy_wine)
                    {
                        settingsModel.Cheers = true;
                    }

                    break;
            }
        }

        private void ProcSubTaskMsg(AsstMsg msg, JObject details)
        {
            // 下面几行注释暂时没用到，先注释起来...
            // string taskChain = details["taskchain"].ToString();
            // string classType = details["class"].ToString();

            // var mainModel = _container.Get<TaskQueueViewModel>();
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

            var mainModel = _container.Get<TaskQueueViewModel>();

            switch (subTask)
            {
                case "StartGameTask":
                    mainModel.AddLog(Localization.GetString("FailedToOpenClient"), UILogColor.Error);
                    break;

                case "AutoRecruitTask":
                    {
                        var why_str = details.TryGetValue("why", out var why) ? why.ToString() : Localization.GetString("ErrorOccurred");
                        mainModel.AddLog(why_str + "，" + Localization.GetString("HasReturned"), UILogColor.Error);
                        break;
                    }

                case "RecognizeDrops":
                    mainModel.AddLog(Localization.GetString("DropRecognitionError"), UILogColor.Error);
                    break;

                case "ReportToPenguinStats":
                    {
                        var why = details["why"].ToString();
                        mainModel.AddLog(why + "，" + Localization.GetString("GiveUpUploadingPenguins"), UILogColor.Error);
                        break;
                    }

                case "CheckStageValid":
                    mainModel.AddLog(Localization.GetString("TheEX"), UILogColor.Error);
                    break;
            }
        }

        private void ProcSubTaskStart(JObject details)
        {
            string subTask = details["subtask"].ToString();

            var mainModel = _container.Get<TaskQueueViewModel>();
            if (subTask == "ProcessTask")
            {
                string taskName = details["details"]["task"].ToString();
                int execTimes = (int)details["details"]["exec_times"];

                switch (taskName)
                {
                    case "StartButton2":
                    case "AnnihilationConfirm":
                        mainModel.AddLog(Localization.GetString("MissionStart") + $" {execTimes} " + Localization.GetString("UnitTime"), UILogColor.Info);
                        break;

                    case "MedicineConfirm":
                        mainModel.AddLog(Localization.GetString("MedicineUsed") + $" {execTimes} " + Localization.GetString("UnitTime"), UILogColor.Info);
                        break;

                    case "StoneConfirm":
                        mainModel.AddLog(Localization.GetString("StoneUsed") + $" {execTimes} " + Localization.GetString("UnitTime"), UILogColor.Info);
                        break;

                    case "AbandonAction":
                        mainModel.AddLog(Localization.GetString("ActingCommandError"), UILogColor.Error);
                        break;

                    case "RecruitRefreshConfirm":
                        mainModel.AddLog(Localization.GetString("LabelsRefreshed"), UILogColor.Info);
                        break;

                    case "RecruitConfirm":
                        mainModel.AddLog(Localization.GetString("RecruitConfirm"), UILogColor.Info);
                        break;

                    case "InfrastDormDoubleConfirmButton":
                        mainModel.AddLog(Localization.GetString("InfrastDormDoubleConfirmed"), UILogColor.Error);
                        break;

                    /* 肉鸽相关 */
                    case "StartExplore":
                        mainModel.AddLog(Localization.GetString("BegunToExplore") + $" {execTimes} " + Localization.GetString("UnitTime"), UILogColor.Info);
                        break;

                    case "StageTraderInvestConfirm":
                        mainModel.AddLog(Localization.GetString("HasInvested") + $" {execTimes} " + Localization.GetString("UnitTime"), UILogColor.Info);
                        break;

                    case "ExitThenAbandon":
                        mainModel.AddLog(Localization.GetString("ExplorationAbandoned"));
                        break;

                    // case "StartAction":
                    //    mainModel.AddLog("开始战斗");
                    //    break;
                    case "MissionCompletedFlag":
                        mainModel.AddLog(Localization.GetString("FightCompleted"));
                        break;

                    case "MissionFailedFlag":
                        mainModel.AddLog(Localization.GetString("FightFailed"));
                        break;

                    case "StageTraderEnter":
                        mainModel.AddLog(Localization.GetString("Trader"));
                        break;

                    case "StageSafeHouseEnter":
                        mainModel.AddLog(Localization.GetString("SafeHouse"));
                        break;

                    case "StageEncounterEnter":
                        mainModel.AddLog(Localization.GetString("Encounter"));
                        break;

                    // case "StageBoonsEnter":
                    //    mainModel.AddLog("古堡馈赠");
                    //    break;
                    case "StageCambatDpsEnter":
                        mainModel.AddLog(Localization.GetString("CambatDps"));
                        break;

                    case "StageEmergencyDps":
                        mainModel.AddLog(Localization.GetString("EmergencyDps"));
                        break;

                    case "StageDreadfulFoe":
                    case "StageDreadfulFoe-5Enter":
                        mainModel.AddLog(Localization.GetString("DreadfulFoe"));
                        break;

                    case "StageTraderInvestSystemFull":
                        mainModel.AddLog(Localization.GetString("UpperLimit"), UILogColor.Info);
                        break;

                    case "RestartGameAndContinue":
                        mainModel.AddLog(Localization.GetString("GameCrash"), UILogColor.Warning);
                        break;

                    case "OfflineConfirm":
                        mainModel.AddLog(Localization.GetString("GameDrop"), UILogColor.Warning);
                        break;

                    case "GamePass":
                        mainModel.AddLog(Localization.GetString("RoguelikeGamePass"), UILogColor.RareOperator);
                        break;
                }
            }
        }

        private void ProcSubTaskCompleted(JObject details)
        {
        }

        private void ProcSubTaskExtraInfo(JObject details)
        {
            string taskChain = details["taskchain"].ToString();

            if (taskChain == "Recruit")
            {
                ProcRecruitCalcMsg(details);
            }

            var subTaskDetails = details["details"];
            if (taskChain == "Depot")
            {
                var depotModel = _container.Get<DepotViewModel>();
                depotModel.Parse((JObject)subTaskDetails);
            }

            string what = details["what"].ToString();

            var mainModel = _container.Get<TaskQueueViewModel>();
            var copilotModel = _container.Get<CopilotViewModel>();
            var settingsModel = _container.Get<SettingsViewModel>();
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

                        all_drops = all_drops.EndsWith("\n") ? all_drops.TrimEnd('\n') : Localization.GetString("NoDrop");
                        mainModel.AddLog(Localization.GetString("TotalDrop") + "\n" + all_drops);
                    }

                    break;

                case "EnterFacility":
                    mainModel.AddLog(Localization.GetString("ThisFacility") + subTaskDetails["facility"] + " " + (int)subTaskDetails["index"]);
                    break;

                case "ProductIncorrect":
                    mainModel.AddLog(Localization.GetString("ProductIncorrect"), UILogColor.Error);
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

                        log_content = log_content.EndsWith("\n") ? log_content.TrimEnd('\n') : Localization.GetString("Error");
                        mainModel.AddLog(Localization.GetString("RecruitingResults") + "\n" + log_content);
                    }

                    break;

                case "RecruitSpecialTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        if (special == "支援机械" && settingsModel.NotChooseLevel1 == false)
                        {
                            break;
                        }

                        using (var toast = new ToastNotification(Localization.GetString("RecruitingTips")))
                        {
                            toast.AppendContentText(special).ShowRecruit();
                        }
                    }

                    break;

                case "RecruitRobotTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        using (var toast = new ToastNotification(Localization.GetString("RecruitingTips")))
                        {
                            toast.AppendContentText(special).ShowRecruitRobot();
                        }
                    }

                    break;

                case "RecruitResult":
                    {
                        int level = (int)subTaskDetails["level"];
                        if (level >= 5)
                        {
                            using (var toast = new ToastNotification(string.Format(Localization.GetString("RecruitmentOfStar"), level)))
                            {
                                toast.AppendContentText(new string('★', level)).ShowRecruit(row: 2);
                            }

                            mainModel.AddLog(level + " ★ Tags", UILogColor.RareOperator, "Bold");
                        }
                        else
                        {
                            mainModel.AddLog(level + " ★ Tags", UILogColor.Info);
                        }

                        /*
                        bool robot = (bool)subTaskDetails["robot"];
                        if (robot)
                        {
                            using (var toast = new ToastNotification(Localization.GetString("RecruitmentOfBot")))
                            {
                                toast.AppendContentText(new string('★', 1)).ShowRecruitRobot(row: 2);
                            }

                            mainModel.AddLog(1 + " ★ Tag", LogColor.RobotOperator, "Bold");
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

                        selected_log = selected_log.EndsWith("\n") ? selected_log.TrimEnd('\n') : Localization.GetString("NoDrop");

                        mainModel.AddLog(Localization.GetString("Choose") + " Tags：\n" + selected_log);
                    }

                    break;

                case "RecruitTagsRefreshed":
                    {
                        int refresh_count = (int)subTaskDetails["count"];
                        mainModel.AddLog(Localization.GetString("Refreshed") + refresh_count + Localization.GetString("UnitTime"));
                        break;
                    }

                case "NotEnoughStaff":
                    {
                        mainModel.AddLog(Localization.GetString("NotEnoughStaff"), UILogColor.Error);
                    }

                    break;

                /* Roguelike */
                case "StageInfo":
                    {
                        mainModel.AddLog(Localization.GetString("StartCombat") + subTaskDetails["name"]);
                    }

                    break;

                case "StageInfoError":
                    {
                        mainModel.AddLog(Localization.GetString("StageInfoError"), UILogColor.Error);
                    }

                    break;

                case "PenguinId":
                    {
                        var settings = _container.Get<SettingsViewModel>();
                        if (settings.PenguinId == string.Empty)
                        {
                            string id = subTaskDetails["id"].ToString();
                            settings.PenguinId = id;

                            // AsstSetPenguinId(id);
                        }
                    }

                    break;

                case "BattleFormation":
                    copilotModel.AddLog(Localization.GetString("BattleFormation") + "\n" + JsonConvert.SerializeObject(subTaskDetails["formation"]));
                    break;

                case "BattleFormationSelected":
                    copilotModel.AddLog(Localization.GetString("BattleFormationSelected") + subTaskDetails["selected"]);
                    break;

                case "CopilotAction":
                    {
                        string doc = subTaskDetails["doc"].ToString();
                        if (doc.Length != 0)
                        {
                            string color = subTaskDetails["doc_color"].ToString();
                            copilotModel.AddLog(doc, color.Length == 0 ? UILogColor.Message : color);
                        }

                        copilotModel.AddLog(
                            string.Format(Localization.GetString("CurrentSteps"),
                                subTaskDetails["action"].ToString(),
                                subTaskDetails["target"].ToString()));
                    }

                    break;

                case "UnsupportedLevel":
                    copilotModel.AddLog(Localization.GetString("UnsupportedLevel"), UILogColor.Error);
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

                    mainModel.AddLog(nameStr.ToString());
                    break;
            }
        }

        private void ProcRecruitCalcMsg(JObject details)
        {
            string what = details["what"].ToString();
            var subTaskDetails = details["details"];

            var recruitModel = _container.Get<RecruitViewModel>();

            switch (what)
            {
                case "RecruitTagsDetected":
                    {
                        JArray tags = (JArray)subTaskDetails["tags"];
                        string info_content = Localization.GetString("RecruitTagsDetected");
                        foreach (var tag_name in tags)
                        {
                            string tag_str = tag_name.ToString();
                            info_content += tag_str + "    ";
                        }

                        recruitModel.RecruitInfo = info_content;
                    }

                    break;

                case "RecruitResult":
                    {
                        string resultContent = string.Empty;
                        JArray result_array = (JArray)subTaskDetails["result"];
                        /* int level = (int)subTaskDetails["level"]; */
                        foreach (var combs in result_array)
                        {
                            int tag_level = (int)combs["level"];
                            resultContent += tag_level + " ★ Tags:  ";
                            foreach (var tag in (JArray)combs["tags"])
                            {
                                resultContent += tag + "    ";
                            }

                            resultContent += "\n\t";
                            foreach (var oper in (JArray)combs["opers"])
                            {
                                resultContent += oper["level"] + " - " + oper["name"] + "    ";
                            }

                            resultContent += "\n\n";
                        }

                        recruitModel.RecruitResult = resultContent;
                    }

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
            if (!LoadResource())
            {
                error = "Load Resource Failed";
                return false;
            }

            var settings = _container.Get<SettingsViewModel>();

            settings.TryToSetBlueStacksHyperVAddress();

            if (!settings.AutoDetectConnection
                && connected
                && connectedAdb == settings.AdbPath
                && connectedAddress == settings.ConnectAddress)
            {
                return true;
            }

            if (settings.AutoDetectConnection)
            {
                if (!settings.DetectAdbConfig(ref error))
                {
                    return false;
                }
            }

            bool ret = AsstConnect(_handle, settings.AdbPath, settings.ConnectAddress, settings.ConnectConfig);

            // 尝试默认的备选端口
            if (!ret && settings.AutoDetectConnection)
            {
                foreach (var address in settings.DefaultAddress[settings.ConnectConfig])
                {
                    if (settings.Idle)
                    {
                        break;
                    }

                    ret = AsstConnect(_handle, settings.AdbPath, address, settings.ConnectConfig);
                    if (ret)
                    {
                        settings.ConnectAddress = address;
                        break;
                    }
                }
            }

            if (ret)
            {
                if (!settings.AlwaysAutoDetectConnection)
                {
                    settings.AutoDetectConnection = false;
                }
            }
            else
            {
                error = Localization.GetString("ConnectFailed") + "\n" + Localization.GetString("CheckSettings");
            }

            return ret;
        }

        private AsstTaskId AsstAppendTaskWithEncoding(string type, JObject task_params = null)
        {
            task_params = task_params ?? new JObject();
            return AsstAppendTask(_handle, type, JsonConvert.SerializeObject(task_params));
        }

        private bool AsstSetTaskParamsWithEncoding(AsstTaskId id, JObject task_params = null)
        {
            if (id == 0)
            {
                return false;
            }

            task_params = task_params ?? new JObject();
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
            Depot,
        }

        private readonly Dictionary<TaskType, AsstTaskId> _latestTaskId = new Dictionary<TaskType, AsstTaskId>();

        private JObject SerializeFightTaskParams(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity)
        {
            var task_params = new JObject();
            task_params["stage"] = stage;
            task_params["medicine"] = max_medicine;
            task_params["stone"] = max_stone;
            task_params["times"] = max_times;
            task_params["report_to_penguin"] = true;
            if (drops_item_quantity != 0 && !string.IsNullOrWhiteSpace(drops_item_id))
            {
                task_params["drops"] = new JObject();
                task_params["drops"][drops_item_id] = drops_item_quantity;
            }

            var settings = _container.Get<SettingsViewModel>();
            task_params["client_type"] = settings.ClientType;
            task_params["penguin_id"] = settings.PenguinId;
            task_params["DrGrandet"] = settings.IsDrGrandet;
            task_params["server"] = settings.ServerType;
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
            var task_params = new JObject();
            task_params["client_type"] = client_type;
            task_params["start_game_enabled"] = enable;
            AsstTaskId id = AsstAppendTaskWithEncoding("StartUp", task_params);
            _latestTaskId[TaskType.StartUp] = id;
            return id != 0;
        }

        /// <summary>
        /// <c>CloseDown</c> 任务。
        /// </summary>
        /// <returns>是否成功。</returns>
        public bool AsstStartCloseDown()
        {
            AsstStop();
            AsstTaskId id = AsstAppendTaskWithEncoding("CloseDown");
            _latestTaskId[TaskType.CloseDown] = id;
            return id != 0 && AsstStart();
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
            var task_params = new JObject();
            task_params["credit_fight"] = credit_fight;
            task_params["shopping"] = with_shopping;
            task_params["buy_first"] = new JArray { first_list };
            task_params["blacklist"] = new JArray { blacklist };
            task_params["force_shopping_if_credit_full"] = force_shopping_if_credit_full;
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
        /// <returns>是否成功。</returns>
        public bool AsstAppendRecruit(int max_times, int[] select_level, int[] confirm_level, bool need_refresh, bool use_expedited, bool skip_robot, bool is_level3_use_short_time)
        {
            var task_params = new JObject();
            task_params["refresh"] = need_refresh;
            task_params["select"] = new JArray(select_level);
            task_params["confirm"] = new JArray(confirm_level);
            task_params["times"] = max_times;
            task_params["set_time"] = true;
            task_params["expedite"] = use_expedited;
            task_params["expedite_times"] = max_times;
            task_params["skip_robot"] = skip_robot;
            if (is_level3_use_short_time)
            {
                task_params["recruitment_time"] = new JObject();
                task_params["recruitment_time"]["3"] = 460; // 7:40
            }

            task_params["report_to_penguin"] = true;
            task_params["report_to_yituliu"] = true;
            var settings = _container.Get<SettingsViewModel>();
            task_params["penguin_id"] = settings.PenguinId;
            task_params["server"] = settings.ServerType;

            AsstTaskId id = AsstAppendTaskWithEncoding("Recruit", task_params);
            _latestTaskId[TaskType.Recruit] = id;
            return id != 0;
        }

        private JObject SerializeInfrastTaskParams(string[] order, string uses_of_drones, double dorm_threshold, bool dorm_filter_not_stationed_enabled, bool dorm_drom_trust_enabled, bool originium_shard_auto_replenishment,
            bool is_custom, string filename, int plan_index)
        {
            var task_params = new JObject();

            task_params["facility"] = new JArray(order);
            task_params["drones"] = uses_of_drones;
            task_params["threshold"] = dorm_threshold;
            task_params["dorm_notstationed_enabled"] = dorm_filter_not_stationed_enabled;
            task_params["drom_trust_enabled"] = dorm_drom_trust_enabled;
            task_params["replenish"] = originium_shard_auto_replenishment;
            task_params["mode"] = is_custom ? 10000 : 0;
            task_params["filename"] = filename;
            task_params["plan_index"] = plan_index;

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
        /// <param name="dorm_drom_trust_enabled">宿舍是否使用蹭信赖功能</param>
        /// <param name="originium_shard_auto_replenishment">制造站搓玉是否补货</param>
        /// <param name="is_custom"></param>
        /// <param name="filename"></param>
        /// <param name="plan_index"></param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendInfrast(string[] order, string uses_of_drones, double dorm_threshold,
            bool dorm_filter_not_stationed_enabled, bool dorm_drom_trust_enabled, bool originium_shard_auto_replenishment,
            bool is_custom, string filename, int plan_index)
        {
            var task_params = SerializeInfrastTaskParams(
                order, uses_of_drones, dorm_threshold,
                dorm_filter_not_stationed_enabled, dorm_drom_trust_enabled, originium_shard_auto_replenishment,
                is_custom, filename, plan_index);
            AsstTaskId id = AsstAppendTaskWithEncoding("Infrast", task_params);
            _latestTaskId[TaskType.Infrast] = id;
            return id != 0;
        }

        public bool AsstSetInfrastTaskParams(string[] order, string uses_of_drones, double dorm_threshold,
            bool dorm_filter_not_stationed_enabled, bool dorm_drom_trust_enabled, bool originium_shard_auto_replenishment,
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
                dorm_filter_not_stationed_enabled, dorm_drom_trust_enabled, originium_shard_auto_replenishment,
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
        /// <param name="theme">肉鸽名字。["Phantom", "Mizuki"]</param>
        /// <returns>是否成功。</returns>
        public bool AsstAppendRoguelike(int mode, int starts, bool investment_enabled, int invests, bool stop_when_full,
            string squad, string roles, string core_char, string theme)
        {
            var task_params = new JObject();
            task_params["mode"] = mode;
            task_params["starts_count"] = starts;
            task_params["investment_enabled"] = investment_enabled;
            task_params["investments_count"] = invests;
            task_params["stop_when_investment_full"] = stop_when_full;
            task_params["theme"] = theme;
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

            AsstTaskId id = AsstAppendTaskWithEncoding("Roguelike", task_params);
            _latestTaskId[TaskType.Roguelike] = id;
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
            var task_params = new JObject();
            task_params["refresh"] = false;
            task_params["select"] = new JArray(select_level);
            task_params["confirm"] = new JArray();
            task_params["times"] = 0;
            task_params["set_time"] = set_time;
            task_params["expedite"] = false;
            task_params["expedite_times"] = 0;

            task_params["report_to_penguin"] = true;
            task_params["report_to_yituliu"] = true;
            var settings = _container.Get<SettingsViewModel>();
            task_params["penguin_id"] = settings.PenguinId;
            task_params["yituliu_id"] = settings.PenguinId; // 一图流说随便传个uuid就行，让client自己生成，所以先直接嫖一下企鹅的（
            task_params["server"] = settings.ServerType;

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
        /// 自动抄作业。
        /// </summary>
        /// <param name="filename">作业 JSON 的文件路径，绝对、相对路径均可。</param>
        /// <param name="formation">是否进行 “快捷编队”。</param>
        /// <returns>是否成功。</returns>
        public bool AsstStartCopilot(string filename, bool formation)
        {
            var task_params = new JObject();
            task_params["filename"] = filename;
            task_params["formation"] = formation;
            AsstTaskId id = AsstAppendTaskWithEncoding("Copilot", task_params);
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
        TouchMode = 2,
        DeploymentWithPause = 3,
    }
}

#pragma warning restore SA1121 // Use built-in type alias
