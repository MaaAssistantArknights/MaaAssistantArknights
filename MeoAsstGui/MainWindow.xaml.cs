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

        private IntPtr p_asst;

        public MainWindow()
        {
            InitializeComponent();

            p_asst = CreateAsst();
        }
        ~MainWindow()
        {
            DestoryAsst(p_asst);
        }

        private void button_Click_startSanity(object sender, RoutedEventArgs e)
        {
            bool catched = AsstCatchSimulator(p_asst);
            button_status.Content = "捕获模拟器窗口：" + catched;
            AsstStart(p_asst, "SanityBegin");
        }

        private void button_Click_stop(object sender, RoutedEventArgs e)
        {
            AsstStop(p_asst);
        }
    }
}
