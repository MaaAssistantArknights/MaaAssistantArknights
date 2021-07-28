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

namespace MeoAsstGui
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {

        [DllImport("MeoAssistance.dll")] static private extern IntPtr CreateAsst();
        [DllImport("MeoAssistance.dll")] static private extern void DestoryAsst(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstCatchEmulator(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern void AsstStart(IntPtr ptr, string task);
        [DllImport("MeoAssistance.dll")] static private extern void AsstStop(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstSetParam(IntPtr p_asst, string type, string param, string value);
        [DllImport("MeoAssistance.dll")]
        static private extern bool AsstGetParam(
            IntPtr p_asst, string type, string param,
            [In, Out] StringBuilder lp_string, int buffer_size);


        private IntPtr p_asst;
        UpdateDialog updateDialog;
        private DispatcherTimer update_times = new DispatcherTimer();

        public MainWindow()
        {
            InitializeComponent();
        }
        ~MainWindow()
        {
            DestoryAsst(p_asst);
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            p_asst = CreateAsst();
            update_times.Tick += new EventHandler(updateExecTimes);
            update_times.Interval = TimeSpan.FromSeconds(1);

            updateDialog = new UpdateDialog();
            updateDialog.CheckUpdateAndShowDialog();
            updateDialog.Close();
        }
        private void button_Click_startSanity(object sender, RoutedEventArgs e)
        {
            bool catched = AsstCatchEmulator(p_asst);
            catch_status.Content = "捕获模拟器窗口：" + catched;
            AsstStart(p_asst, "SanityBegin");
            update_times.Start();
        }

        private void button_Click_stop(object sender, RoutedEventArgs e)
        {
            AsstStop(p_asst);
            catch_status.Content = "";
            update_times.Stop();
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
            AsstStart(p_asst, "VisitBegin");
        }

        private void updateExecTimes(object sender, EventArgs e)
        {
            StringBuilder buff_start = new StringBuilder(16);
            AsstGetParam(p_asst, "task.execTimes", "StartButton2", buff_start, 16);
            exec_times.Content = "已开始行动 " + buff_start + " 次";

            if (checkBox_useStone.IsChecked == true)
            {
                StringBuilder buff_stone = new StringBuilder(16);
                AsstGetParam(p_asst, "task.execTimes", "StoneConfirm", buff_stone, 16);
                stone_times.Content = "已碎石 " + buff_stone + " 个";
            }


            StringBuilder buff_running = new StringBuilder(4);
            AsstGetParam(p_asst, "status", "running", buff_running, 4);
            if (int.Parse(buff_running.ToString()) == 0)
            {
                update_times.Stop();
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
            else
            {
                label_status.Content = "正在运行中……";
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

        private void checkBox_shutdown_Checked(object sender, RoutedEventArgs e)
        {

        }
    }
}
