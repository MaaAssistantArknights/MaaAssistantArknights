using Stylet;
using System.Runtime.InteropServices;
using System.Text;

namespace MeoAsstGui
{
    public class VersionUpdateViewModel : Screen
    {
        private IWindowManager _windowManager;

        public VersionUpdateViewModel(IWindowManager windowManager)
        {
            _windowManager = windowManager;
        }

        [DllImport("MeoAssistance.dll")]
        static private extern bool CheckVersionUpdate(
            [In, Out] StringBuilder tag, int tag_bufsize,
            [In, Out] StringBuilder html_url, int html_bufsize,
            [In, Out] StringBuilder body, int body_bufsize);

        private string _updateTag;

        public string UpdateTag
        {
            get
            {
                return _updateTag;
            }
            set
            {
                SetAndNotify(ref _updateTag, value);
            }
        }

        private string _updateInfo;

        public string UpdateInfo
        {
            get
            {
                return _updateInfo;
            }
            set
            {
                SetAndNotify(ref _updateInfo, value);
            }
        }

        private string _updateUrl = "https://github.com/MistEO/MeoAssistance/releases/latest";

        public bool CheckUpdate()
        {
            StringBuilder tag = new StringBuilder(64);
            StringBuilder html_url = new StringBuilder(512);
            StringBuilder body = new StringBuilder(4096);

            bool ret = CheckVersionUpdate(tag, 64, html_url, 512, body, 4096);
            if (!ret)
            {
                return false;
            }

            UpdateTag = "新版本：" + tag;
            UpdateInfo = body.ToString();
            _updateUrl = html_url.ToString();

            return true;
        }

        public void Download()
        {
            System.Diagnostics.Process.Start(_updateUrl);
            RequestClose();
        }

        public void Close()
        {
            RequestClose();
        }
    }
}