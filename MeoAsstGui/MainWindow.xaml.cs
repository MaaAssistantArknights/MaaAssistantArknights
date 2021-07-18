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

        [DllImport("MeoAssistance.dll")] static public extern IntPtr CreateAsst();
        [DllImport("MeoAssistance.dll")] static public extern void DestoryAsst(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static public extern bool AsstCatchSimulator(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static public extern void AsstStart(IntPtr ptr, string task);
        [DllImport("MeoAssistance.dll")] static public extern void AsstStop(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static public extern bool AsstSetParam(IntPtr p_asst, string type, string param, string value);
        [DllImport("MeoAssistance.dll")] static public extern bool AsstGetParam(IntPtr p_asst, string type, string param, [In, Out] StringBuilder lp_string, int buffer_size);


        private IntPtr p_asst;
        private DispatcherTimer update_times = new DispatcherTimer();

        public MainWindow()
        {
            InitializeComponent();

            p_asst = CreateAsst();
            update_times.Tick += new EventHandler(updateExecTimes);
            update_times.Interval = TimeSpan.FromSeconds(1);
        }
        ~MainWindow()
        {
            DestoryAsst(p_asst);
        }

        private void button_Click_startSanity(object sender, RoutedEventArgs e)
        {
            bool catched = AsstCatchSimulator(p_asst);
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
            AsstSetParam(p_asst, "task.maxTimes", "StoneConfirm", textBox_useStone.Text);
        }

        private void checkBox_useStone_Checked(object sender, RoutedEventArgs e)
        {
            if (checkBox_useMedicine.IsChecked == true)
            {
                AsstSetParam(p_asst, "task.type", "UseStone", "doNothing");
            }
            else
            {
                AsstSetParam(p_asst, "task.type", "UseStone", "stop");
            }
        }

        private void button_Click_visit(object sender, RoutedEventArgs e)
        {
            bool catched = AsstCatchSimulator(p_asst);
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
        }
    }
}
