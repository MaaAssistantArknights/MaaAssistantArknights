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
    /// UpdateDialog.xaml 的交互逻辑
    /// </summary>
    public partial class UpdateDialog : Window
    {
        [DllImport("MeoAssistance.dll")]
        static private extern bool CheckVersionUpdate(
            [In, Out] StringBuilder tag, int tag_bufsize,
            [In, Out] StringBuilder html_url, int html_bufsize,
            [In, Out] StringBuilder body, int body_bufsize);
        public UpdateDialog()
        {
            InitializeComponent();
        }

        private String m_htmlUrl;

        public void CheckUpdateAndShowDialog()
        {
            StringBuilder tag = new StringBuilder(64);
            StringBuilder html_url = new StringBuilder(512);
            StringBuilder body = new StringBuilder(4096);

            bool ret = CheckVersionUpdate(tag, 64, html_url, 512, body, 4096);
            if (!ret)
            {
                return;
            }
            label_tag.Content = "新版本：" + tag;
            byte[] buffer1 = Encoding.Default.GetBytes(body.ToString());
            byte[] buffer2 = Encoding.Convert(Encoding.UTF8, Encoding.Default, buffer1, 0, buffer1.Length);
            string strBuffer = Encoding.Default.GetString(buffer2, 0, buffer2.Length);
            strBuffer = strBuffer.Replace("\\r\\n", "\r\n");
            label_body.Content = strBuffer.Substring(0, 128) + "\n......";

            m_htmlUrl = html_url.ToString();
            ShowDialog();
        }

        private void button_cancer_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void button_confirm_Click(object sender, RoutedEventArgs e)
        {
            System.Diagnostics.Process.Start(m_htmlUrl);
            Close();
        }
    }
}
