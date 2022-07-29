// <copyright file="AsstProxy.cs" company="MistEO">
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
// </copyright>

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    using AsstHandle = IntPtr;
    using TaskId = Int32;

    public class AsstProxy
    {
        private delegate void CallbackDelegate(int msg, IntPtr json_buffer, IntPtr custom_arg);

        private delegate void ProcCallbckMsg(AsstMsg msg, JObject details);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstLoadResource(byte[] dirname);

        private static bool AsstLoadResource(string dirname)
        {
            return AsstLoadResource(Encoding.UTF8.GetBytes(dirname));
        }

        [DllImport("MeoAssistant.dll")] private static extern AsstHandle AsstCreate();

        [DllImport("MeoAssistant.dll")] private static extern AsstHandle AsstCreateEx(CallbackDelegate callback, IntPtr custom_arg);

        [DllImport("MeoAssistant.dll")] private static extern void AsstDestroy(AsstHandle handle);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstConnect(AsstHandle handle, byte[] adb_path, byte[] address, byte[] config);

        private static bool AsstConnect(AsstHandle handle, string adb_path, string address, string config)
        {
            return AsstConnect(handle, Encoding.UTF8.GetBytes(adb_path), Encoding.UTF8.GetBytes(address), Encoding.UTF8.GetBytes(config));
        }

        [DllImport("MeoAssistant.dll")] private static extern TaskId AsstAppendTask(AsstHandle handle, byte[] type, byte[] task_params);

        private static TaskId AsstAppendTask(AsstHandle handle, string type, string task_params)
        {
            return AsstAppendTask(handle, Encoding.UTF8.GetBytes(type), Encoding.UTF8.GetBytes(task_params));
        }

        [DllImport("MeoAssistant.dll")] private static extern bool AsstSetTaskParams(AsstHandle handle, TaskId id, byte[] task_params);

        private static bool AsstSetTaskParams(AsstHandle handle, TaskId id, string task_params)
        {
            return AsstSetTaskParams(handle, id, Encoding.UTF8.GetBytes(task_params));
        }

        [DllImport("MeoAssistant.dll")] private static extern bool AsstStart(AsstHandle handle);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstStop(AsstHandle handle);

        [DllImport("MeoAssistant.dll")] private static extern void AsstLog(byte[] level, byte[] message);

        public static void AsstLog(string message)
        {
            AsstLog(Encoding.UTF8.GetBytes("GUI"), Encoding.UTF8.GetBytes(message));
        }

        private readonly CallbackDelegate _callback;

        public AsstProxy(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            _callback = CallbackFunction;
        }

        ~AsstProxy()
        {
            if (_handle != IntPtr.Zero)
            {
                AsstDestroy();
            }
        }

        private string _curResource = "_Unloaded";

        public bool LoadGlobalResource()
        {
            var settingsModel = _container.Get<SettingsViewModel>();
            if (settingsModel.ClientType == _curResource)
            {
                return true;
            }

            bool loaded = true;
            if (settingsModel.ClientType == String.Empty
                || settingsModel.ClientType == "Official" || settingsModel.ClientType == "Bilibili")
            {
                // The resources of Official and Bilibili are the same
                if (_curResource == "Official" || _curResource == "Bilibili")
                {
                    return true;
                }
                loaded = AsstLoadResource(System.IO.Directory.GetCurrentDirectory());
            }
            else if (_curResource == "Official" || _curResource == "Bilibili")
            {
                // Load basic resources for CN client first
                // Then load global incremental resources
                loaded = AsstLoadResource(System.IO.Directory.GetCurrentDirectory() + "\\resource\\global\\" + settingsModel.ClientType);
            }
            else
            {
                // Load basic resources for CN client first
                // Then load global incremental resources
                loaded = AsstLoadResource(System.IO.Directory.GetCurrentDirectory())
                    && AsstLoadResource(System.IO.Directory.GetCurrentDirectory() + "\\resource\\global\\" + settingsModel.ClientType);
            }

            if (loaded)
            {
                if (settingsModel.ClientType == String.Empty)
                {
                    _curResource = "Official";
                }
                else
                {
                    _curResource = settingsModel.ClientType;
                }
            }
            return loaded;
        }

        public void Init()
        {
            bool loaded = LoadGlobalResource();

            _handle = AsstCreateEx(_callback, IntPtr.Zero);

            if (loaded == false || _handle == IntPtr.Zero)
            {
                Execute.OnUIThread(() =>
                {
                    _windowManager.ShowMessageBox(Localization.GetString("UnknownAbnormal"), Localization.GetString("Error"), icon: MessageBoxImage.Error);
                    App.Current.Shutdown();
                });
            }
            var mainModel = _container.Get<TaskQueueViewModel>();
            mainModel.Idle = true;
            var settingsModel = _container.Get<SettingsViewModel>();
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

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true)]
        internal static extern int lstrlenA(IntPtr ptr);

        private static string PtrToStringCustom(IntPtr ptr, System.Text.Encoding enc)
        {
            if (IntPtr.Zero == ptr)
            {
                return null;
            }
            if (lstrlenA(ptr) == 0)
            {
                return string.Empty;
            }
            int len = lstrlenA(ptr);
            Byte[] bytes = new Byte[len + 1];
            Marshal.Copy(ptr, bytes, 0, len + 1);
            return enc.GetString(bytes);
        }

        private void CallbackFunction(int msg, IntPtr json_buffer, IntPtr custom_arg)
        {
            string json_str = PtrToStringCustom(json_buffer, System.Text.Encoding.UTF8);
            //Console.WriteLine(json_str);
            JObject json = (JObject)JsonConvert.DeserializeObject(json_str);
            ProcCallbckMsg dlg = new ProcCallbckMsg(procMsg);
            Execute.OnUIThread(() =>
            {
                dlg((AsstMsg)msg, json);
            });
        }

        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;
        private IntPtr _handle;

        private void procMsg(AsstMsg msg, JObject details)
        {
            switch (msg)
            {
                case AsstMsg.InternalError:
                    break;

                case AsstMsg.InitFailed:
                    _windowManager.ShowMessageBox(Localization.GetString("InitializationError"), Localization.GetString("Error"), icon: MessageBoxImage.Error);
                    App.Current.Shutdown();
                    break;

                case AsstMsg.ConnectionInfo:
                    procConnectInfo(details);
                    break;

                case AsstMsg.AllTasksCompleted:
                case AsstMsg.TaskChainError:
                case AsstMsg.TaskChainStart:
                case AsstMsg.TaskChainCompleted:
                case AsstMsg.TaskChainExtraInfo:
                    procTaskChainMsg(msg, details);
                    break;

                case AsstMsg.SubTaskError:
                case AsstMsg.SubTaskStart:
                case AsstMsg.SubTaskCompleted:
                case AsstMsg.SubTaskExtraInfo:
                    procSubTaskMsg(msg, details);
                    break;
            }
        }

        private bool connected = false;
        private string connected_adb;
        private string connected_address;

        private void procConnectInfo(JObject details)
        {
            var what = details["what"].ToString();
            var svm = _container.Get<SettingsViewModel>();
            var mainModel = _container.Get<TaskQueueViewModel>();
            switch (what)
            {
                case "Connected":
                    connected = true;
                    connected_adb = details["details"]["adb"].ToString();
                    connected_address = details["details"]["address"].ToString();
                    svm.ConnectAddress = connected_address;
                    break;

                case "UnsupportedResolution":
                    connected = false;
                    mainModel.AddLog(Localization.GetString("ResolutionNotSupported"), "darkred");
                    break;

                case "ResolutionError":
                    connected = false;
                    mainModel.AddLog(Localization.GetString("ResolutionAcquisitionFailure"), "darkred");
                    break;

                case "Reconnecting":
                    mainModel.AddLog(Localization.GetString("TryToReconnect"), "darkred");
                    break;

                case "Reconnected":
                    mainModel.AddLog(Localization.GetString("ReconnectSuccess"));
                    break;

                case "Disconnect":
                    connected = false;
                    mainModel.AddLog(Localization.GetString("ReconnectFailed"), "darkred");
                    AsstStop();
                    break;
            }
        }

        private void procTaskChainMsg(AsstMsg msg, JObject details)
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

            switch (msg)
            {
                case AsstMsg.TaskChainError:
                    mainModel.AddLog(Localization.GetString("TaskError") + taskChain, "darkred");
                    if (taskChain == "Copilot")
                    {
                        copilotModel.Idle = true;
                        copilotModel.AddLog(Localization.GetString("CombatError"), "darkred");
                    }
                    break;

                case AsstMsg.TaskChainStart:
                    if (taskChain == "Fight")
                    {
                        mainModel.FightTaskRunning = true;
                    }
                    mainModel.AddLog(Localization.GetString("StartTask") + taskChain);
                    break;

                case AsstMsg.TaskChainCompleted:
                    mainModel.AddLog(Localization.GetString("CompleteTask") + taskChain);
                    if (taskChain == "Copilot")
                    {
                        copilotModel.Idle = true;
                        copilotModel.AddLog(Localization.GetString("CompleteCombat"), "darkcyan");
                    }
                    break;

                case AsstMsg.TaskChainExtraInfo:
                    break;

                case AsstMsg.AllTasksCompleted:
                    bool isMainTaskQueueAllCompleted = true;
                    var runned_tasks = details["runned_tasks"] as JArray;
                    if (runned_tasks.Count == 1)
                    {
                        var unique_runned_task = (TaskId)runned_tasks[0];
                        if (unique_runned_task == (_latestTaskId.TryGetValue(TaskType.Copilot, out var copilotTaskId) ? copilotTaskId : 0)
                            || unique_runned_task == (_latestTaskId.TryGetValue(TaskType.RecruitCalc, out var recruitCalcTaskId) ? recruitCalcTaskId : 0)
                            || unique_runned_task == (_latestTaskId.TryGetValue(TaskType.CloseDown, out var closeDownTaskId) ? closeDownTaskId : 0))
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
                        //mainModel.CheckAndShutdown();
                        mainModel.CheckAfterCompleted();
                    }
                    if (buy_wine)
                    {
                        settingsModel.Cheers = true;
                    }

                    break;
            }
        }

        private void procSubTaskMsg(AsstMsg msg, JObject details)
        {
            // 下面几行注释暂时没用到，先注释起来...
            //string taskChain = details["taskchain"].ToString();
            //string classType = details["class"].ToString();

            //var mainModel = _container.Get<TaskQueueViewModel>();
            switch (msg)
            {
                case AsstMsg.SubTaskError:
                    procSubTaskError(details);
                    break;

                case AsstMsg.SubTaskStart:
                    procSubTaskStart(details);
                    break;

                case AsstMsg.SubTaskCompleted:
                    procSubTaskCompleted(details);
                    break;

                case AsstMsg.SubTaskExtraInfo:
                    procSubTaskExtraInfo(details);
                    break;
            }
        }

        private void procSubTaskError(JObject details)
        {
            string subTask = details["subtask"].ToString();

            var mainModel = _container.Get<TaskQueueViewModel>();

            switch (subTask)
            {
                case "StartGameTask":
                    mainModel.AddLog(Localization.GetString("FailedToOpenClient"), "darkred");
                    break;

                case "AutoRecruitTask":
                    {
                        var why_str = details.TryGetValue("why", out var why) ? why.ToString() : Localization.GetString("出现错误");
                        mainModel.AddLog(why_str + "，" + Localization.GetString("HasReturned"), "darkred");
                        break;
                    }

                case "RecognizeDrops":
                    mainModel.AddLog(Localization.GetString("DropRecognitionError"), "darkred");
                    break;

                case "ReportToPenguinStats":
                    {
                        var why = details["why"].ToString();
                        mainModel.AddLog(why + "，" + Localization.GetString("GiveUpUploadingPenguins"), "darkred");
                        break;
                    }

                case "CheckStageValid":
                    mainModel.AddLog(Localization.GetString("TheEX"), "darkred");
                    break;
            }
        }

        private void procSubTaskStart(JObject details)
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
                        mainModel.AddLog(Localization.GetString("OnTheMove") + $" {execTimes} " + Localization.GetString("UnitTime"), "darkcyan");
                        break;

                    case "MedicineConfirm":
                        mainModel.AddLog(Localization.GetString("MedicineUsed") + $" {execTimes} " + Localization.GetString("UnitTime"), "darkcyan");
                        break;

                    case "StoneConfirm":
                        mainModel.AddLog(Localization.GetString("StoneUsed") + $" {execTimes} " + Localization.GetString("UnitTime"), "darkcyan");
                        break;

                    case "AbandonAction":
                        mainModel.AddLog(Localization.GetString("ActingCommandError"), "darkred");
                        break;

                    case "RecruitRefreshConfirm":
                        mainModel.AddLog(Localization.GetString("LabelsRefreshed"), "darkcyan");
                        break;

                    case "RecruitConfirm":
                        mainModel.AddLog(Localization.GetString("RecruitConfirm"), "darkcyan");
                        break;

                    case "InfrastDormDoubleConfirmButton":
                        mainModel.AddLog(Localization.GetString("InfrastDormDoubleConfirmed"), "darkred");
                        break;

                    /* 肉鸽相关 */
                    case "Roguelike1Start":
                        mainModel.AddLog(Localization.GetString("BegunToExplore") + $" {execTimes} " + Localization.GetString("UnitTime"), "darkcyan");
                        break;

                    case "Roguelike1StageTraderInvestConfirm":
                        mainModel.AddLog(Localization.GetString("HasInvested") + $" {execTimes} " + Localization.GetString("UnitTime"), "darkcyan");
                        break;

                    case "Roguelike1ExitThenAbandon":
                        mainModel.AddLog(Localization.GetString("ExplorationAbandoned"));
                        break;

                    //case "Roguelike1StartAction":
                    //    mainModel.AddLog("开始战斗");
                    //    break;

                    case "Roguelike1MissionCompletedFlag":
                        mainModel.AddLog(Localization.GetString("FightCompleted"));
                        break;

                    case "Roguelike1MissionFailedFlag":
                        mainModel.AddLog(Localization.GetString("FightFailed"));
                        break;

                    case "Roguelike1StageTraderEnter":
                        mainModel.AddLog(Localization.GetString("Trader"));
                        break;

                    case "Roguelike1StageSafeHouseEnter":
                        mainModel.AddLog(Localization.GetString("SafeHouse"));
                        break;

                    case "Roguelike1StageEncounterEnter":
                        mainModel.AddLog(Localization.GetString("Encounter"));
                        break;

                    //case "Roguelike1StageBoonsEnter":
                    //    mainModel.AddLog("古堡馈赠");
                    //    break;

                    case "Roguelike1StageCambatDpsEnter":
                        mainModel.AddLog(Localization.GetString("CambatDps"));
                        break;

                    case "Roguelike1StageEmergencyDps":
                        mainModel.AddLog(Localization.GetString("EmergencyDps"));
                        break;

                    case "Roguelike1StageDreadfulFoe":
                        mainModel.AddLog(Localization.GetString("DreadfulFoe"));
                        break;

                    case "Roguelike1StageTraderInvestSystemFull":
                        mainModel.AddLog(Localization.GetString("UpperLimit"), "darkcyan");
                        break;

                    case "RestartGameAndContinueFighting":
                        mainModel.AddLog(Localization.GetString("GameCrash"), "darkgoldenrod");
                        break;

                    case "OfflineConfirm":
                        mainModel.AddLog(Localization.GetString("GameDrop"), "darkgoldenrod");
                        break;
                }
            }
        }

        private void procSubTaskCompleted(JObject details)
        {
        }

        private void procSubTaskExtraInfo(JObject details)
        {
            string taskChain = details["taskchain"].ToString();

            if (taskChain == "Recruit")
            {
                procRecruitCalcMsg(details);
            }

            string what = details["what"].ToString();
            var subTaskDetails = details["details"];

            var mainModel = _container.Get<TaskQueueViewModel>();
            var copilotModel = _container.Get<CopilotViewModel>();
            switch (what)
            {
                case "StageDrops":
                    {
                        string cur_drops = string.Empty;
                        JArray drops = (JArray)subTaskDetails["drops"];
                        foreach (var item in drops)
                        {
                            string itemName = item["itemName"].ToString();
                            int count = (int)item["quantity"];
                            cur_drops += $"{itemName} : {count}\n";
                        }
                        cur_drops = cur_drops.EndsWith("\n") ? cur_drops.TrimEnd('\n') : Localization.GetString("NoDrop");
                        mainModel.AddLog(Localization.GetString("Drop") + "\n" + cur_drops);

                        string all_drops = string.Empty;
                        JArray statistics = (JArray)subTaskDetails["stats"];
                        foreach (var item in statistics)
                        {
                            string itemName = item["itemName"].ToString();
                            int count = (int)item["quantity"];
                            all_drops += $"{itemName} : {count}\n";
                        }
                        all_drops = all_drops.EndsWith("\n") ? all_drops.TrimEnd('\n') : Localization.GetString("NoDrop");
                        mainModel.AddLog(Localization.GetString("TotalDrop") + "\n" + all_drops);
                    }
                    break;

                case "EnterFacility":
                    mainModel.AddLog(Localization.GetString("ThisFacility") + subTaskDetails["facility"] + " " + (int)subTaskDetails["index"]);
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
                            using (var toast = new ToastNotification(String.Format(Localization.GetString("RecruitmentOfStar"), level)))
                            {
                                toast.AppendContentText(new string('★', level)).ShowRecruit(row: 2);
                            }
                            mainModel.AddLog(level + " ★ Tags", "darkorange", "Bold");
                        }
                        else
                        {
                            mainModel.AddLog(level + " ★ Tags", "darkcyan");
                        }

                        bool robot = (bool)subTaskDetails["robot"];
                        if (robot)
                        {
                            using (var toast = new ToastNotification(Localization.GetString("RecruitmentOfBot")))
                            {
                                toast.AppendContentText(new string('★', 1)).ShowRecruitRobot(row: 2);
                            }
                            mainModel.AddLog(1 + " ★ Tag", "darkgray", "Bold");
                        }
                    }
                    break;

                case "RecruitTagsSelected":
                    {
                        JArray selected = (JArray)subTaskDetails["tags"];
                        string selected_log = string.Empty;
                        foreach (var tag in selected)
                        {
                            selected_log += tag.ToString() + "\n";
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
                        mainModel.AddLog(Localization.GetString("NotEnoughStaff"), "darkred");
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
                        mainModel.AddLog(Localization.GetString("StageInfoError"), "darkred");
                    }
                    break;

                case "PenguinId":
                    {
                        var settings = _container.Get<SettingsViewModel>();
                        if (settings.PenguinId == String.Empty)
                        {
                            string id = subTaskDetails["id"].ToString();
                            settings.PenguinId = id;
                            //AsstSetPenguinId(id);
                        }
                    }
                    break;

                case "BattleFormation":
                    copilotModel.AddLog(Localization.GetString("BattleFormation") + "\n" + JsonConvert.SerializeObject(subTaskDetails["formation"]));
                    break;

                case "BattleFormationSelected":
                    copilotModel.AddLog(Localization.GetString("BattleFormationSelected") + subTaskDetails["selected"].ToString());
                    break;

                case "BattleAction":
                    {
                        string doc = subTaskDetails["doc"].ToString();
                        if (doc.Length != 0)
                        {
                            string color = subTaskDetails["doc_color"].ToString();
                            copilotModel.AddLog(doc, color.Length == 0 ? "dark" : color);
                        }
                        var action = subTaskDetails["action"].ToString();
                        if (action.Length != 0)
                        {
                            copilotModel.AddLog(Localization.GetString("CurrentSteps") + action);
                        }
                    }
                    break;

                case "BattleActionDoc":
                    //{
                    //    string title_color = subTaskDetails["title_color"].ToString();
                    //    copilotModel.AddLog(subTaskDetails["title"].ToString(), title_color.Length == 0 ? "dark" : title_color);
                    //    string details_color = subTaskDetails["details_color"].ToString();
                    //    copilotModel.AddLog(subTaskDetails["details"].ToString(), details_color.Length == 0 ? "dark" : details_color);
                    //}
                    break;

                case "UnsupportedLevel":
                    mainModel.AddLog(Localization.GetString("UnsupportedLevel"), "darkred");
                    break;
            }
        }

        private void procRecruitCalcMsg(JObject details)
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
                        int level = (int)subTaskDetails["level"];
                        foreach (var combs in result_array)
                        {
                            int tag_level = (int)combs["level"];
                            resultContent += tag_level + "★Tags:  ";
                            foreach (var tag in (JArray)combs["tags"])
                            {
                                resultContent += tag.ToString() + "    ";
                            }
                            resultContent += "\n\t";
                            foreach (var oper in (JArray)combs["opers"])
                            {
                                resultContent += oper["level"].ToString() + " - " + oper["name"].ToString() + "    ";
                            }
                            resultContent += "\n\n";
                        }
                        recruitModel.RecruitResult = resultContent;
                    }
                    break;
            }
        }

        public bool AsstConnect(ref string error, bool firsttry = false)
        {
            if (!LoadGlobalResource())
            {
                error = "Load Global Resource Failed";
                return false;
            }

            var settings = _container.Get<SettingsViewModel>();
            if (connected
                && connected_adb == settings.AdbPath
                && connected_address == settings.ConnectAddress)
            {
                return true;
            }

            if (settings.AdbPath == String.Empty ||
                settings.ConnectAddress == String.Empty)
            {
                if (!settings.RefreshAdbConfig(ref error))
                {
                    return false;
                }
            }
            settings.TryToSetBlueStacksHyperVAddress();

            bool ret = AsstConnect(_handle, settings.AdbPath, settings.ConnectAddress, settings.ConnectConfig);

            // 尝试默认的备选端口
            if (!ret)
            {
                foreach (var address in settings.DefaultAddress[settings.ConnectConfig])
                {
                    ret = AsstConnect(_handle, settings.AdbPath, address, settings.ConnectConfig);
                    if (ret)
                    {
                        settings.ConnectAddress = address;
                        break;
                    }
                }
            }
            if (!ret)
            {
                if (firsttry)
                {
                    error = Localization.GetString("ConnectFailed") + "\n" + Localization.GetString("TryToStartEmulator");
                }
                else
                {
                    error = Localization.GetString("ConnectFailed") + "\n" + Localization.GetString("CheckSettings");
                }
            }
            return ret;
        }

        private TaskId AsstAppendTaskWithEncoding(string type, JObject task_params = null)
        {
            task_params = task_params ?? new JObject();
            return AsstAppendTask(_handle, type, JsonConvert.SerializeObject(task_params));
        }

        private bool AsstSetTaskParamsWithEncoding(TaskId id, JObject task_params = null)
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
            Recruit,
            Infrast,
            Visit,
            Mall,
            Award,
            Roguelike,
            RecruitCalc,
            Copilot
        };

        private Dictionary<TaskType, TaskId> _latestTaskId = new Dictionary<TaskType, TaskId>();

        private JObject serializeFightTaskParams(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity)
        {
            var task_params = new JObject();
            task_params["stage"] = stage;
            task_params["medicine"] = max_medicine;
            task_params["stone"] = max_stone;
            task_params["times"] = max_times;
            task_params["report_to_penguin"] = true;
            if (drops_item_quantity != 0)
            {
                task_params["drops"] = new JObject();
                task_params["drops"][drops_item_id] = drops_item_quantity;
            }
            var settings = _container.Get<SettingsViewModel>();
            task_params["client_type"] = settings.ClientType;
            task_params["penguin_id"] = settings.PenguinId;
            task_params["server"] = "CN";
            return task_params;
        }

        public bool AsstAppendFight(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity)
        {
            var task_params = serializeFightTaskParams(stage, max_medicine, max_stone, max_times, drops_item_id, drops_item_quantity);
            TaskId id = AsstAppendTaskWithEncoding("Fight", task_params);
            _latestTaskId[TaskType.Fight] = id;
            return id != 0;
        }

        public bool AsstSetFightTaskParams(string stage, int max_medicine, int max_stone, int max_times, string drops_item_id, int drops_item_quantity)
        {
            var task_params = serializeFightTaskParams(stage, max_medicine, max_stone, max_times, drops_item_id, drops_item_quantity);
            return AsstSetTaskParamsWithEncoding(_latestTaskId.TryGetValue(TaskType.Fight, out var task_id) ? task_id : 0, task_params);
        }

        public bool AsstAppendAward()
        {
            TaskId id = AsstAppendTaskWithEncoding("Award");
            _latestTaskId[TaskType.Award] = id;
            return id != 0;
        }

        public bool AsstAppendStartUp(string client_type, bool enable)
        {
            var task_params = new JObject();
            task_params["client_type"] = client_type;
            task_params["start_game_enabled"] = enable;
            TaskId id = AsstAppendTaskWithEncoding("StartUp", task_params);
            _latestTaskId[TaskType.StartUp] = id;
            return id != 0;
        }

        public bool AsstStartCloseDown()
        {
            AsstStop();
            TaskId id = AsstAppendTaskWithEncoding("CloseDown");
            _latestTaskId[TaskType.CloseDown] = id;
            return id != 0 && AsstStart();
        }

        public bool AsstAppendVisit()
        {
            TaskId id = AsstAppendTaskWithEncoding("Visit");
            _latestTaskId[TaskType.Visit] = id;
            return id != 0;
        }

        public bool AsstAppendMall(bool with_shopping, string[] firstlist, string[] blacklist)
        {
            var task_params = new JObject();
            task_params["shopping"] = with_shopping;
            task_params["buy_first"] = new JArray { firstlist };
            task_params["blacklist"] = new JArray { blacklist };
            TaskId id = AsstAppendTaskWithEncoding("Mall", task_params);
            _latestTaskId[TaskType.Mall] = id;
            return id != 0;
        }

        public bool AsstAppendRecruit(int max_times, int[] select_level, int required_len, int[] confirm_level, int confirm_len, bool need_refresh, bool use_expedited, bool skip_robot)
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
            TaskId id = AsstAppendTaskWithEncoding("Recruit", task_params);
            _latestTaskId[TaskType.Recruit] = id;
            return id != 0;
        }

        public bool AsstAppendInfrast(int work_mode, string[] order, int order_len, string uses_of_drones, double dorm_threshold)
        {
            var task_params = new JObject();
            //task_params["mode"] = 0;
            task_params["facility"] = new JArray(order);
            task_params["drones"] = uses_of_drones;
            task_params["threshold"] = dorm_threshold;
            task_params["replenish"] = true;
            TaskId id = AsstAppendTaskWithEncoding("Infrast", task_params);
            _latestTaskId[TaskType.Infrast] = id;
            return id != 0;
        }

        public bool AsstAppendRoguelike(int mode, int starts, int invests, bool stop_when_full,
            string squad, string roles, string core_char)
        {
            var task_params = new JObject();
            task_params["mode"] = mode;
            task_params["starts_count"] = starts;
            task_params["investments_count"] = invests;
            task_params["stop_when_investment_full"] = stop_when_full;
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
            TaskId id = AsstAppendTaskWithEncoding("Roguelike", task_params);
            _latestTaskId[TaskType.Roguelike] = id;
            return id != 0;
        }

        public bool AsstStartRecruitCalc(int[] select_level, int required_len, bool set_time)
        {
            var task_params = new JObject();
            task_params["refresh"] = false;
            task_params["select"] = new JArray(select_level);
            task_params["confirm"] = new JArray();
            task_params["times"] = 0;
            task_params["set_time"] = set_time;
            task_params["expedite"] = false;
            task_params["expedite_times"] = 0;
            TaskId id = AsstAppendTaskWithEncoding("Recruit", task_params);
            _latestTaskId[TaskType.RecruitCalc] = id;
            return id != 0 && AsstStart();
        }

        public bool AsstStartCopilot(string stage_name, string filename, bool formation)
        {
            var task_params = new JObject();
            task_params["stage_name"] = stage_name;
            task_params["filename"] = filename;
            task_params["formation"] = formation;
            TaskId id = AsstAppendTaskWithEncoding("Copilot", task_params);
            _latestTaskId[TaskType.Copilot] = id;
            return id != 0 && AsstStart();
        }

        public bool AsstStart()
        {
            return AsstStart(_handle);
        }

        public bool AsstStop()
        {
            bool ret = AsstStop(_handle);
            _latestTaskId.Clear();
            return ret;
        }

        public void AsstDestroy()
        {
            AsstDestroy(_handle);
        }
    }

    public enum AsstMsg
    {
        /* Global Info */
        InternalError = 0,          // 内部错误
        InitFailed,                 // 初始化失败
        ConnectionInfo,            // 连接相关错误
        AllTasksCompleted,          // 全部任务完成
        /* TaskChain Info */
        TaskChainError = 10000,     // 任务链执行/识别错误
        TaskChainStart,             // 任务链开始
        TaskChainCompleted,         // 任务链完成
        TaskChainExtraInfo,         // 任务链额外信息
        /* SubTask Info */
        SubTaskError = 20000,       // 原子任务执行/识别错误
        SubTaskStart,               // 原子任务开始
        SubTaskCompleted,           // 原子任务完成
        SubTaskExtraInfo            // 原子任务额外信息
    };
}
