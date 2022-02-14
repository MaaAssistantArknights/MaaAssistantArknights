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
using System.Runtime.InteropServices;
using System.Windows;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class AsstProxy
    {
        private delegate void CallbackDelegate(int msg, IntPtr json_buffer, IntPtr custom_arg);

        private delegate void ProcCallbckMsg(AsstMsg msg, JObject details);

        [DllImport("MeoAssistant.dll")] private static extern IntPtr AsstCreate(string dirname);

        [DllImport("MeoAssistant.dll")] private static extern IntPtr AsstCreateEx(string dirname, CallbackDelegate callback, IntPtr custom_arg);

        [DllImport("MeoAssistant.dll")] private static extern void AsstDestroy(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstCatchDefault(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstCatchCustom(IntPtr ptr, string address);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendStartUp(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendFight(IntPtr ptr, string stage, int max_medicine, int max_stone, int max_times);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendAward(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendVisit(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendMall(IntPtr ptr, bool with_shopping);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendInfrast(IntPtr ptr, int work_mode, string[] order, int order_len, string uses_of_drones, double dorm_threshold);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendRecruit(IntPtr ptr, int max_times, int[] select_level, int required_len, int[] confirm_level, int confirm_len, bool need_refresh, bool use_expedited);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstAppendRoguelike(IntPtr ptr, int mode);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstStartRecruitCalc(IntPtr ptr, int[] select_level, int required_len, bool set_time);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstStart(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstStop(IntPtr ptr);

        [DllImport("MeoAssistant.dll")] private static extern bool AsstSetPenguinId(IntPtr p_asst, string id);

        //[DllImport("MeoAssistant.dll")] private static extern bool AsstSetParam(IntPtr p_asst, string type, string param, string value);

        private readonly CallbackDelegate _callback;

        public AsstProxy(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            _callback = CallbackFunction;
        }

        public void Init()
        {
            _ptr = AsstCreateEx(System.IO.Directory.GetCurrentDirectory(), _callback, IntPtr.Zero);
            if (_ptr == IntPtr.Zero)
            {
                Execute.OnUIThread(() =>
                {
                    _windowManager.ShowMessageBox("出现未知异常", "错误", icon: MessageBoxImage.Error);
                    Environment.Exit(0);
                });
            }
            var tvm = _container.Get<TaskQueueViewModel>();
            tvm.Idle = true;
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
        private IntPtr _ptr;

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

                case AsstMsg.ConnectionError:
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

        private void procTaskChainMsg(AsstMsg msg, JObject details)
        {
            string taskChain = details["taskchain"].ToString();

            if (taskChain == "RecruitCalc")
            {
                if (msg == AsstMsg.TaskChainError)
                {
                    var recruitModel = _container.Get<RecruitViewModel>();
                    recruitModel.RecruitInfo = "识别错误！";
                }
                return;
            }
            var mainModel = _container.Get<TaskQueueViewModel>();

            switch (msg)
            {
                case AsstMsg.TaskChainError:
                    mainModel.AddLog("任务出错：" + taskChain, "darkred");
                    break;

                case AsstMsg.TaskChainStart:
                    mainModel.AddLog("开始任务：" + taskChain);
                    break;

                case AsstMsg.TaskChainCompleted:
                    mainModel.AddLog("完成任务：" + taskChain);
                    break;

                case AsstMsg.TaskChainExtraInfo:
                    break;

                case AsstMsg.AllTasksCompleted:
                    mainModel.Idle = true;
                    mainModel.AddLog("任务已全部完成");
                    using (var toast = new ToastNotification("任务已全部完成！"))
                    {
                        toast.Show();
                    }
                    mainModel.CheckAndShutdown();
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
                case "AutoRecruitTask":
                    mainModel.AddLog("公招识别错误，已返回", "darkred");
                    break;

                //case "StageDropsTask":
                //    mainModel.AddLog("关卡识别错误", "darkred");
                //    break;

                case "ReportToPenguinStats":
                    mainModel.AddLog("上传企鹅数据错误", "darkred");
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
                        mainModel.AddLog("关卡：诡异行商");
                        break;

                    case "Roguelike1StageSafeHouseEnter":
                        mainModel.AddLog("关卡：安全的角落");
                        break;

                    case "Roguelike1StageEncounterEnter":
                        mainModel.AddLog("关卡：不期而遇/古堡馈赠");
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
                }
            }
        }

        private void procSubTaskCompleted(JObject details)
        {
        }

        private void procSubTaskExtraInfo(JObject details)
        {
            string taskChain = details["taskchain"].ToString();

            if (taskChain == "RecruitCalc")
            {
                procRecruitCalcMsg(details);
                return;
            }

            string what = details["what"].ToString();
            var subTaskDetails = details["details"];

            var mainModel = _container.Get<TaskQueueViewModel>();
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

                case "RecruitSpecialTag":
                    {
                        string special = subTaskDetails["tag"].ToString();
                        using (var toast = new ToastNotification("公招提示"))
                        {
                            toast.AppendContentText(special).ShowRecruit();
                        }
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
                        if (level >= 5)
                        {
                            using (var toast = new ToastNotification($"公招出 {level} 星了哦！"))
                            {
                                toast.AppendContentText(new string('★', level)).ShowRecruit(row: 2);
                            }
                        }
                    }
                    break;
            }
        }

        public bool AsstCatch()
        {
            var settings = _container.Get<SettingsViewModel>();
            settings.TryToSetBlueStacksHyperVAddress();
            if (settings.ConnectAddress.Length == 0)
            {
                return AsstCatchDefault(_ptr);
            }
            else
            {
                return AsstCatchCustom(_ptr, settings.ConnectAddress);
            }
        }

        public bool AsstAppendFight(string stage, int max_medicine, int max_stone, int max_times)
        {
            return AsstAppendFight(_ptr, stage, max_medicine, max_stone, max_times);
        }

        public bool AsstAppendAward()
        {
            return AsstAppendAward(_ptr);
        }

        public bool AsstAppendStartUp()
        {
            return AsstAppendStartUp(_ptr);
        }

        public bool AsstAppendVisit()
        {
            return AsstAppendVisit(_ptr);
        }

        public bool AsstAppendMall(bool with_shopping)
        {
            return AsstAppendMall(_ptr, with_shopping);
        }

        public bool AsstAppendRecruit(int max_times, int[] select_level, int required_len, int[] confirm_level, int confirm_len, bool need_refresh, bool use_expedited)
        {
            return AsstAppendRecruit(_ptr, max_times, select_level, required_len, confirm_level, confirm_len, need_refresh, use_expedited);
        }

        public bool AsstAppendInfrast(int work_mode, string[] order, int order_len, string uses_of_drones, double dorm_threshold)
        {
            return AsstAppendInfrast(_ptr, work_mode, order, order_len, uses_of_drones, dorm_threshold);
        }

        public bool AsstAppendRoguelike(int mode)
        {
            return AsstAppendRoguelike(_ptr, mode);
        }

        public bool AsstStart()
        {
            return AsstStart(_ptr);
        }

        public bool AsstStartRecruitCalc(int[] select_level, int required_len, bool set_time)
        {
            return AsstStartRecruitCalc(_ptr, select_level, required_len, set_time);
        }

        public bool AsstStop()
        {
            return AsstStop(_ptr);
        }

        public void AsstSetPenguinId(string id)
        {
            AsstSetPenguinId(_ptr, id);
        }

        //public void AsstSetParam(string type, string param, string value)
        //{
        //    AsstSetParam(_ptr, type, param, value);
        //}
    }

    public enum AsstMsg
    {
        /* Global Info */
        InternalError = 0,          // 内部错误
        InitFailed,                 // 初始化失败
        ConnectionError,            // 连接相关错误
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
