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
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MeoAsstGui
{
    using TaskMsg = MainWindow.TaskMsg;
    /// <summary>
    /// RecruitWindow.xaml 的交互逻辑
    /// </summary>
    public partial class RecruitWindow : Window
    {
        [DllImport("MeoAssistance.dll")] static private extern bool AsstCatchEmulator(IntPtr ptr);
        [DllImport("MeoAssistance.dll")] static private extern bool AsstRunOpenRecruit(IntPtr ptr, int[] required_level, bool set_time);

        private IntPtr p_asst;
        public RecruitWindow(IntPtr ptr)
        {
            InitializeComponent();
            p_asst = ptr;
        }
        public void proc_msg(TaskMsg msg, JObject detail)
        {
            switch (msg)
            {
                case TaskMsg.TextDetected:
                case TaskMsg.RecruitTagsDetected:
                case TaskMsg.OcrResultError:
                case TaskMsg.RecruitSpecialTag:
                case TaskMsg.RecruitResult:
                    break;
            }
        }

        private void button_start_Click(object sender, RoutedEventArgs e)
        {
            info.Content = "正在计算……";

            if (!AsstCatchEmulator(p_asst))
            {
                return;
            }
            List<int> level_list = new List<int>();

            if (checkBox_level_3.IsChecked == true)
            {
                level_list.Add(3);
            }
            if (checkBox_level_4.IsChecked == true)
            {
                level_list.Add(4);
            }
            if (checkBox_level_5.IsChecked == true)
            {
                level_list.Add(5);
            }
            if (checkBox_level_6.IsChecked == true)
            {
                level_list.Add(6);
            }
            bool set_time = checkBox_time.IsChecked == true ? true : false;

            AsstRunOpenRecruit(p_asst, level_list.ToArray(), set_time);
        }
    }
}
