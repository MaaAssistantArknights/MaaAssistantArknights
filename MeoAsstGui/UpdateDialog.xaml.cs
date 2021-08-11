using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;

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
            int len = strBuffer.Length > 128 ? 128 : strBuffer.Length;
            label_body.Content = strBuffer.Substring(0, len) + "\n......";

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