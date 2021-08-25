using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Windows.Threading;
using System.Threading;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class AsstProxy
    {
        private delegate void CallbackDelegate(int msg, IntPtr json_buffer, IntPtr custom_arg);
        private delegate void ProcCallbckMsg(AsstMsg msg, JObject detail);

        [DllImport("MeoAssistance.dll")] static private extern IntPtr AsstCreate();
        [DllImport("MeoAssistance.dll")] static private extern IntPtr AsstCreateEx(CallbackDelegate callback, IntPtr custom_arg);
        [DllImport("MeoAssistance.dll")] static private extern void AsstDestory(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstCatchEmulator(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstStart(IntPtr ptr, string task);
        [DllImport("MeoAssistance.dll")] static private extern void AsstStop(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstSetParam(IntPtr p_asst, string type, string param, string value);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstRunOpenRecruit(IntPtr ptr, int[] required_level, bool set_time);

        private CallbackDelegate _callback;
        public AsstProxy(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            _callback = CallbackFunction;
        }

        public void Init()
        {
            _ptr = AsstCreateEx(_callback, IntPtr.Zero);
        }

        private void CallbackFunction(int msg, IntPtr json_buffer, IntPtr custom_arg)
        {
            string json_str = Marshal.PtrToStringAnsi(json_buffer);
            //Console.WriteLine(json_str);
            JObject json = (JObject)JsonConvert.DeserializeObject(json_str);
            ProcCallbckMsg dlg = new ProcCallbckMsg(proc_msg);
            Execute.OnUIThread(() =>
            {
                dlg((AsstMsg)msg, json);
            });
        }

        private IWindowManager _windowManager;
        private IContainer _container;
        private int _retryTimes;
        private int _retryLimit = 2;
        private IntPtr _ptr;
        private void proc_msg(AsstMsg msg, JObject detail)
        {
            var mfvm = _container.Get<MainFunctionViewModel>();
            switch (msg)
            {
                case AsstMsg.TaskCompleted:
                    {
                        if ((int)detail["algorithm"] > 0)   // JustReturn的执行完成不算
                        {
                            _retryTimes = 0;
                        }
                        string taskName = detail["name"].ToString();
                        if (taskName == "StartButton2")
                        {
                            mfvm.ExecInfo = "已开始行动 " + (int)detail["exec_times"] + " 次";
                        }
                        else if (taskName == "StoneConfirm")
                        {
                            mfvm.StoneInfo = "已碎石 " + (int)detail["exec_times"] + " 个";
                        }
                    }
                    break;
                case AsstMsg.TaskStart:
                    {
                        string taskChain = detail["task_chain"].ToString();
                        string taskType = detail["task_type"].ToString();
                        if (taskChain == "SanityBegin" || taskType == "VisitBegin")
                        {
                            mfvm.RunStatus = "正在运行中……";
                        }
                    }
                    break;
                case AsstMsg.TaskStop:
                    {
                        string taskChain = detail["task_chain"].ToString();
                        if (taskChain != "SanityBegin" && taskChain != "VisitBegin")
                        {
                            break;
                        }
                        mfvm.RunStatus = "已刷完，自动停止";
                        if (mfvm.Shutdown == true)
                        {
                            System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");

                            var result = _windowManager.ShowMessageBox("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK);
                            if (result == MessageBoxResult.OK)
                            {
                                System.Diagnostics.Process.Start("shutdown.exe", "-a");
                            }
                        }
                    }
                    break;
                case AsstMsg.TextDetected:
                    break;
                case AsstMsg.RecruitTagsDetected:
                case AsstMsg.OcrResultError:
                case AsstMsg.RecruitSpecialTag:
                case AsstMsg.RecruitResult:
                    recruit_proc_msg(msg, detail);
                    break;
                case AsstMsg.TaskError:
                    {
                        string task_chain = detail["task_chain"].ToString();
                        if (task_chain == "SanityBegin")
                        {
                            // 出错了会重试两次，再不行就算了
                            if (_retryTimes >= _retryLimit)
                            {
                                _retryTimes = 0;
                                mfvm.RunStatus = "出现错误，已停止运行";
                                break;
                            }
                            ++_retryTimes;
                            AsstStart("SanityBegin");

                        }
                        else if (task_chain == "VisitBegin")
                        {
                            // 出错了会重试两次，再不行就算了
                            if (_retryTimes >= _retryLimit)
                            {
                                _retryTimes = 0;
                                mfvm.RunStatus = "出现错误，已停止运行";
                                break;
                            }
                            ++_retryTimes;
                            AsstStart("VisitBegin");
                        }
                    }
                    break;
                case AsstMsg.InitFaild:
                    _windowManager.ShowMessageBox("资源文件错误！请尝试重新解压或下载", "错误");
                    Environment.Exit(0);
                    break;
            }
        }

        private void recruit_proc_msg(AsstMsg msg, JObject detail)
        {
            var rvm = _container.Get<RecruitViewModel>();
            switch (msg)
            {
                case AsstMsg.TextDetected:
                    break;
                case AsstMsg.RecruitTagsDetected:
                    JArray tags = (JArray)detail["tags"];
                    string info_content = "识别结果:    ";
                    foreach (var tag_name in tags)
                    {
                        info_content += tag_name.ToString() + "    ";
                    }
                    rvm.RecruitInfo = info_content;
                    break;
                case AsstMsg.OcrResultError:
                    rvm.RecruitInfo = "识别错误！";
                    break;
                case AsstMsg.RecruitSpecialTag:
                    _windowManager.ShowMessageBox("检测到特殊Tag:" + detail["tag"].ToString(), "提示");
                    break;
                case AsstMsg.RecruitResult:
                    string resultContent = "";
                    JArray result_array = (JArray)detail["result"];
                    int combs_level = 0;
                    foreach (var combs in result_array)
                    {
                        int tag_level = (int)combs["tag_level"];
                        if (tag_level > combs_level)
                        {
                            combs_level = tag_level;
                        }
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
                    rvm.RecruitResult = resultContent;
                    if (combs_level >= 5)
                    {
                        _windowManager.ShowMessageBox("出 " + combs_level + " 星了哦！", "提示");
                    }
                    break;
            }
        }

        public void AsstStop()
        {
            AsstStop(_ptr);
        }

        public bool AsstCatchEmulator()
        {
            if (_ptr == null)
            {
                return false;
            }
            return AsstCatchEmulator(_ptr);
        }

        public bool AsstStart(string task)
        {
            return AsstStart(_ptr, task);
        }

        public void AsstSetParam(string type, string param, string value)
        {
            AsstSetParam(_ptr, type, param, value);
        }

        public bool AsstRunOpenRecruit(int[] required_level, bool set_time)
        {
            return AsstRunOpenRecruit(_ptr, required_level, set_time);
        }
    }

    public enum AsstMsg
    {
        /* Error Msg */
        PtrIsNull,
        ImageIsEmpty,
        WindowMinimized,
        InitFaild,
        /* Info Msg */
        TaskStart,
        ImageFindResult,
        ImageMatched,
        TaskMatched,
        ReachedLimit,
        ReadyToSleep,
        EndOfSleep,
        AppendProcessTask,
        AppendTask,
        TaskCompleted,
        PrintWindow,
        TaskStop,
        TaskError,
        /* Open Recruit Msg */
        TextDetected,
        RecruitTagsDetected,
        OcrResultError,
        RecruitSpecialTag,
        RecruitResult
    };
}
