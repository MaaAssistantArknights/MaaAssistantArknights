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

        private string _curResource = "";

        public bool LoadGlobalResource()
        {
            bool loaded = true;
            var settingsModel = _container.Get<SettingsViewModel>();
            if (_curResource.Length == 0
                && settingsModel.ClientType == "YoStarEN"
                && _curResource != settingsModel.ClientType)
            {
                loaded = AsstLoadResource(System.IO.Directory.GetCurrentDirectory() + "\\resource\\global\\YoStarEN");
                _curResource = "YoStarEN";
            }
            // 这种是手贱看国际服点了一下，又点回官服的
            else if (_curResource.Length != 0
                && (settingsModel.ClientType == "Official" || settingsModel.ClientType == "Bilibili" || settingsModel.ClientType == String.Empty))
            {
                loaded = AsstLoadResource(System.IO.Directory.GetCurrentDirectory());
                _curResource = "";
            }
            return loaded;
        }

        public void Init()
        {
            // basic resource for CN client
            bool loaded = AsstLoadResource(System.IO.Directory.GetCurrentDirectory());
            _curResource = "";

            loaded = LoadGlobalResource();

            _handle = AsstCreateEx(_callback, IntPtr.Zero);

            if (loaded == false || _handle == IntPtr.Zero)
            {
                Execute.OnUIThread(() =>
                {
                    _windowManager.ShowMessageBox("出现未知异常", "错误", icon: MessageBoxImage.Error);
                    Environment.Exit(0);
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
                    _windowManager.ShowMessageBox("初始化错误！请检查是否使用了中文路径", "错误", icon: MessageBoxImage.Error);
                    Environment.Exit(0);
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
                    mainModel.AddLog("分辨率不支持，请设置为 720p 或更高，且为 16:9 比例", "darkred");
                    break;

                case "ResolutionError":
                    connected = false;
                    mainModel.AddLog("分辨率获取失败，建议重启电脑，或更换模拟器后再试", "darkred");
                    break;

                case "Disconnect":
                case "CommandExecFailed":
                    connected = false;
                    mainModel.AddLog("错误！连接断开！", "darkred");
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
                    recruitModel.RecruitInfo = "识别错误！";
                }
            }
            var mainModel = _container.Get<TaskQueueViewModel>();
            var copilotModel = _container.Get<CopilotViewModel>();

            switch (msg)
            {
                case AsstMsg.TaskChainError:
                    mainModel.AddLog("任务出错：" + taskChain, "darkred");
                    if (taskChain == "Copilot")
                    {
                        copilotModel.Idle = true;
                        copilotModel.AddLog("战斗出错！", "darkred");
                    }
                    break;

                case AsstMsg.TaskChainStart:
                    mainModel.AddLog("开始任务：" + taskChain);
                    break;

                case AsstMsg.TaskChainCompleted:
                    mainModel.AddLog("完成任务：" + taskChain);
                    if (taskChain == "Copilot")
                    {
                        copilotModel.Idle = true;
                        copilotModel.AddLog("完成战斗", "darkcyan");
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
                    _latestTaskId.Clear();

                    mainModel.Idle = true;
                    mainModel.UseStone = false;
                    copilotModel.Idle = true;

                    if (isMainTaskQueueAllCompleted)
                    {
                        mainModel.AddLog("任务已全部完成");
                        using (var toast = new ToastNotification("任务已全部完成！"))
                        {
                            toast.Show();
                        }
                        //mainModel.CheckAndShutdown();
                        mainModel.CheckAfterCompleted();
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
                    mainModel.AddLog("打开客户端失败，请检查配置文件", "darkred");
                    break;

                case "AutoRecruitTask":
                    {
                        var why_str = details.TryGetValue("why", out var why) ? why.ToString() : "出现错误";
                        mainModel.AddLog(why_str + "，已返回", "darkred");
                        break;
                    }

                case "RecognizeDrops":
                    mainModel.AddLog("掉落识别错误", "darkred");
                    break;

                case "ReportToPenguinStats":
                    {
                        var why = details["why"].ToString();
                        mainModel.AddLog(why + "，放弃上传企鹅", "darkred");
                        break;
                    }

                case "CheckStageValid":
                    mainModel.AddLog("EX 关卡，已停止", "darkred");
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
                        mainModel.AddLog("已开始行动 " + execTimes + " 次", "darkcyan");
                        break;

                    case "MedicineConfirm":
                        mainModel.AddLog("已吃药 " + execTimes + " 个", "darkcyan");
                        break;

                    case "StoneConfirm":
                        mainModel.AddLog("已碎石 " + execTimes + " 颗", "darkcyan");
                        break;

                    case "AbandonAction":
                        mainModel.AddLog("代理指挥失误", "darkred");
                        break;

                    case "RecruitRefreshConfirm":
                        mainModel.AddLog("已刷新标签", "darkcyan");
                        break;

                    case "RecruitConfirm":
                        mainModel.AddLog("已确认招募", "darkcyan");
                        break;

                    case "InfrastDormDoubleConfirmButton":
                        mainModel.AddLog("干员冲突", "darkred");
                        break;

                    /* 肉鸽相关 */
                    case "Roguelike1Start":
                        mainModel.AddLog("已开始探索 " + execTimes + " 次", "darkcyan");
                        break;

                    case "Roguelike1StageTraderInvestConfirm":
                        mainModel.AddLog("已投资 " + execTimes + " 个源石锭", "darkcyan");
                        break;

                    case "Roguelike1ExitThenAbandon":
                        mainModel.AddLog("已放弃本次探索");
                        break;

                    //case "Roguelike1StartAction":
                    //    mainModel.AddLog("开始战斗");
                    //    break;

                    case "Roguelike1MissionCompletedFlag":
                        mainModel.AddLog("战斗完成");
                        break;

                    case "Roguelike1MissionFailedFlag":
                        mainModel.AddLog("战斗失败");
                        break;

                    case "Roguelike1StageTraderEnter":
                        mainModel.AddLog("关卡：诡意行商");
                        break;

                    case "Roguelike1StageSafeHouseEnter":
                        mainModel.AddLog("关卡：安全的角落");
                        break;

                    case "Roguelike1StageEncounterEnter":
                        mainModel.AddLog("关卡：不期而遇");
                        break;

                    //case "Roguelike1StageBoonsEnter":
                    //    mainModel.AddLog("古堡馈赠");
                    //    break;

                    case "Roguelike1StageCambatDpsEnter":
                        mainModel.AddLog("关卡：普通作战");
                        break;

                    case "Roguelike1StageEmergencyDps":
                        mainModel.AddLog("关卡：紧急作战");
                        break;

                    case "Roguelike1StageDreadfulFoe":
                        mainModel.AddLog("关卡：险路恶敌");
                        break;

                    case "Roguelike1StageTraderInvestSystemFull":
                        mainModel.AddLog("投资达到上限", "darkcyan");
                        break;

                    case "RestartGameAndContinueFighting":
                        mainModel.AddLog("游戏崩溃，重新启动", "darkgoldenrod");
                        break;

                    case "OfflineConfirm":
                        mainModel.AddLog("游戏掉线，重新连接", "darkgoldenrod");
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
                        cur_drops = cur_drops.EndsWith("\n") ? cur_drops.TrimEnd('\n') : "无";
                        mainModel.AddLog("当次掉落：\n" + cur_drops);

                        string all_drops = string.Empty;
                        JArray statistics = (JArray)subTaskDetails["stats"];
                        foreach (var item in statistics)
                        {
                            string itemName = item["itemName"].ToString();
                            int count = (int)item["quantity"];
                            all_drops += $"{itemName} : {count}\n";
                        }
                        all_drops = all_drops.EndsWith("\n") ? all_drops.TrimEnd('\n') : "无";
                        mainModel.AddLog("掉落统计：\n" + all_drops);
                    }
                    break;

                case "EnterFacility":
                    mainModel.AddLog("当前设施：" + subTaskDetails["facility"] + " " + (int)subTaskDetails["index"]);
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
                        log_content = log_content.EndsWith("\n") ? log_content.TrimEnd('\n') : "错误";
                        mainModel.AddLog("公招识别结果：\n" + log_content);
                    }
                    break;

                case "RecruitSpecialTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        using (var toast = new ToastNotification("公招提示"))
                        {
                            toast.AppendContentText(special).ShowRecruit();
                        }
                    }
                    break;

                case "RecruitRobotTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        using (var toast = new ToastNotification("公招提示"))
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
                            using (var toast = new ToastNotification($"公招出 {level} 星了哦！"))
                            {
                                toast.AppendContentText(new string('★', level)).ShowRecruit(row: 2);
                            }
                            mainModel.AddLog(level + " 星 Tags", "darkorange", "Bold");
                        }
                        else
                        {
                            mainModel.AddLog(level + " 星 Tags", "darkcyan");
                        }

                        bool robot = (bool)subTaskDetails["robot"];
                        if (robot)
                        {
                            using (var toast = new ToastNotification($"公招出小车了哦！"))
                            {
                                toast.AppendContentText(new string('★', 1)).ShowRecruitRobot(row: 2);
                            }
                            mainModel.AddLog(1 + " 星 Tag", "darkgray", "Bold");
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
                        selected_log = selected_log.EndsWith("\n") ? selected_log.TrimEnd('\n') : "无";

                        mainModel.AddLog("选择 Tags：\n" + selected_log);
                    }
                    break;

                case "RecruitTagsRefreshed":
                    {
                        int refresh_count = (int)subTaskDetails["count"];
                        mainModel.AddLog("当前槽位已刷新 " + refresh_count + " 次");
                        break;
                    }

                case "NotEnoughStaff":
                    {
                        mainModel.AddLog("可用干员不足", "darkred");
                    }
                    break;

                /* Roguelike */
                case "StageInfo":
                    {
                        mainModel.AddLog("开始战斗：" + subTaskDetails["name"]);
                    }
                    break;

                case "StageInfoError":
                    {
                        mainModel.AddLog("关卡识别错误", "darkred");
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
                    copilotModel.AddLog("开始编队\n" + JsonConvert.SerializeObject(subTaskDetails["formation"]));
                    break;

                case "BattleFormationSelected":
                    copilotModel.AddLog("选择干员：" + subTaskDetails["selected"].ToString());
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
                            copilotModel.AddLog("当前步骤：" + action);
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
                    copilotModel.AddLog("不支持的关卡\n请更新 MAA 软件版本，或检查作业文件", "darkred");
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
                        string info_content = "识别结果:    ";
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
                            resultContent += tag_level + "星Tags:  ";
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
                    error = "连接失败\n正在尝试启动模拟器";
                else error = "连接失败\n请检查连接设置";
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

        public bool AsstAppendRoguelike(int mode)
        {
            var task_params = new JObject();
            task_params["mode"] = mode;
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
            task_params["set_time"] = true;
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
