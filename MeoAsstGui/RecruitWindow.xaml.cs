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

namespace MeoAsstGui
{
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
            StringBuilder result_buff = new StringBuilder(16384);
            int maybe_level = 0;
            bool set_time = checkBox_time.IsChecked == true ? true : false;
            if (AsstRunOpenRecruit(p_asst, level_list.ToArray(), set_time, result_buff, 16384, ref maybe_level))
            {
                info.Content = result_buff;
                if (maybe_level > 4 || maybe_level == 1)
                {
                    MessageBox.Show("出 " + maybe_level + " 星了哦！", "公招提示");
                }
            }
            else
            {
                info.Content = "识别错误！";
                MessageBox.Show("识别错误！", "公招提示");
            }
        }
    }
}
