using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;

namespace MeoAsstGui
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        [DllImport("MeoAssistance.dll")] static private extern IntPtr AsstCreate();

        [DllImport("MeoAssistance.dll")] static private extern IntPtr AsstCreateEx(CallbackDelegate callback, IntPtr custom_arg);

        [DllImport("MeoAssistance.dll")] static private extern void AsstDestory(IntPtr ptr);

        [DllImport("MeoAssistance.dll")] static private extern bool AsstCatchEmulator(IntPtr ptr);

        [DllImport("MeoAssistance.dll")] static private extern bool AsstStart(IntPtr ptr, string task);

        [DllImport("MeoAssistance.dll")] static private extern void AsstStop(IntPtr ptr);

        [DllImport("MeoAssistance.dll")] static private extern bool AsstSetParam(IntPtr p_asst, string type, string param, string value);

        private delegate void CallbackDelegate(int msg, IntPtr json_buffer, IntPtr custom_arg);

        private delegate void ProcCallbckMsg(AsstMsg msg, JObject detail);

        private static CallbackDelegate callback;

        private IntPtr p_asst;
        private RecruitWindow recruitWindow;

        private int retry_times = 0;
        private int retry_limits = 2;

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

        private void CallbackFunction(int msg, IntPtr json_buffer, IntPtr custom_arg)
        {
            string json_str = Marshal.PtrToStringAnsi(json_buffer);
            //Console.WriteLine(json_str);
            JObject json = (JObject)JsonConvert.DeserializeObject(json_str);
            ProcCallbckMsg dlg = new ProcCallbckMsg(proc_msg);
            this.Dispatcher.Invoke(dlg, msg, json);
        }

        private void proc_msg(AsstMsg msg, JObject detail)
        {
            switch (msg)
            {
                case AsstMsg.TaskCompleted:
                    {
                        if ((int)detail["algorithm"] > 0)   // JustReturn的执行完成不算
                        {
                            retry_times = 0;
                        }
                        string task_name = detail["name"].ToString();
                        if (task_name == "StartButton2")
                        {
                            exec_times.Content = "已开始行动 " + (int)detail["exec_times"] + " 次";
                        }
                        else if (task_name == "StoneConfirm")
                        {
                            stone_times.Content = "已碎石 " + (int)detail["exec_times"] + " 个";
                        }
                    }
                    break;

                case AsstMsg.TaskStart:
                    {
                        string task_chain = detail["task_chain"].ToString();
                        string task_type = detail["task_type"].ToString();
                        if (task_chain == "SanityBegin" || task_chain == "VisitBegin")
                        {
                            label_status.Content = "正在运行中……";
                        }
                    }
                    break;

                case AsstMsg.TaskStop:
                    {
                        string task_chain = detail["task_chain"].ToString();
                        if (task_chain != "SanityBegin" && task_chain != "VisitBegin")
                        {
                            break;
                        }
                        label_status.Content = "已刷完，自动停止";
                        if (checkBox_shutdown.IsChecked == true)
                        {
                            System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");

                            MessageBoxResult result = MessageBox.Show("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK);
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
                    recruitWindow.proc_msg(msg, detail);
                    break;

                case AsstMsg.TaskError:
                    {
                        string task_chain = detail["task_chain"].ToString();
                        if (task_chain == "SanityBegin")
                        {
                            // 出错了会重试两次，再不行就算了
                            if (retry_times >= retry_limits)
                            {
                                retry_times = 0;
                                label_status.Content = "出现错误，已停止运行";
                                break;
                            }
                            ++retry_times;
                            button_Click_startSanity(null, null);
                        }
                        else if (task_chain == "VisitBegin")
                        {
                            // 出错了会重试两次，再不行就算了
                            if (retry_times >= retry_limits)
                            {
                                retry_times = 0;
                                label_status.Content = "出现错误，已停止运行";
                                break;
                            }
                            ++retry_times;
                            button_Click_visit(null, null);
                        }
                    }
                    break;

                case AsstMsg.InitFaild:
                    MessageBox.Show("资源文件错误！请尝试重新解压或下载", "错误");
                    Close();
                    break;
            }
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        ~MainWindow()
        {
            AsstDestory(p_asst);
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            callback = CallbackFunction;
            p_asst = AsstCreateEx(callback, IntPtr.Zero);
            //update_times.Tick += new EventHandler(updateExecTimes);
            //update_times.Interval = TimeSpan.FromSeconds(1);

            Dispatcher.BeginInvoke(new Action(delegate
            {
                UpdateDialog updateDialog = new UpdateDialog();
                updateDialog.CheckUpdateAndShowDialog();
                updateDialog.Close();
            }));
        }

        private void button_Click_startSanity(object sender, RoutedEventArgs e)
        {
            bool catched = AsstCatchEmulator(p_asst);
            exec_times.Content = "";
            catch_status.Content = "捕获模拟器窗口：" + catched;
            if (AsstStart(p_asst, "SanityBegin"))
            {
                exec_times.Content = "";
            }
            if (checkBox_useStone.IsChecked == true)
            {
                stone_times.Content = "已碎石 0 个";
            }
        }

        private void button_Click_stop(object sender, RoutedEventArgs e)
        {
            AsstStop(p_asst);
            catch_status.Content = "";
            exec_times.Content = "";
            stone_times.Content = "";
            label_status.Content = "";
        }

        private void checkBox_useMedicine_Checked(object sender, RoutedEventArgs e)
        {
            if (checkBox_useMedicine.IsChecked == true)
            {
                AsstSetParam(p_asst, "task.type", "UseMedicine", "doNothing");
            }
            else
            {
                AsstSetParam(p_asst, "task.type", "UseMedicine", "stop");
            }
        }

        private void textBox_useStone_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (checkBox_useStone.IsChecked == true)
            {
                String text = textBox_useStone.Text != String.Empty ? textBox_useStone.Text : "0";
                AsstSetParam(p_asst, "task.maxTimes", "StoneConfirm", text);
            }
        }

        private void checkBox_useStone_Checked(object sender, RoutedEventArgs e)
        {
            if (checkBox_useStone.IsChecked == true)
            {
                AsstSetParam(p_asst, "task.type", "UseStone", "doNothing");
                String text = textBox_useStone.Text != String.Empty ? textBox_useStone.Text : "0";
                AsstSetParam(p_asst, "task.maxTimes", "StoneConfirm", text);
            }
            else
            {
                AsstSetParam(p_asst, "task.type", "UseStone", "stop");
                AsstSetParam(p_asst, "task.maxTimes", "StoneConfirm", "0");
            }
        }

        private void button_Click_visit(object sender, RoutedEventArgs e)
        {
            bool catched = AsstCatchEmulator(p_asst);
            catch_status.Content = "捕获模拟器窗口：" + catched;
            if (AsstStart(p_asst, "VisitBegin"))
            {
                exec_times.Content = "";
            }
        }

        private void checkBox_maxTimes_Checked(object sender, RoutedEventArgs e)
        {
            if (checkBox_maxTimes.IsChecked == true)
            {
                String text = textBox_maxTimes.Text != String.Empty ? textBox_maxTimes.Text : "0";
                AsstSetParam(p_asst, "task.maxTimes", "StartButton1", text);
            }
            else
            {
                AsstSetParam(p_asst, "task.maxTimes", "StartButton1", int.MaxValue.ToString());
            }
        }

        private void textBox_maxTimes_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (checkBox_maxTimes.IsChecked == true)
            {
                String text = textBox_maxTimes.Text != String.Empty ? textBox_maxTimes.Text : "0";
                AsstSetParam(p_asst, "task.maxTimes", "StartButton1", text);
            }
        }

        private void button_recruit_Click(object sender, RoutedEventArgs e)
        {
            recruitWindow = new RecruitWindow(p_asst);
            recruitWindow.Show();
        }
    }
}