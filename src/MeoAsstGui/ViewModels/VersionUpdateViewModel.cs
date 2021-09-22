using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using System;
using System.IO;
using System.Net;
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

        [DllImport("MeoAssistance.dll")] static private extern IntPtr AsstGetVersion();

        private string _curVersion = Marshal.PtrToStringAnsi(AsstGetVersion());
        private string _latestVersion;

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
        private string RequestApi()
        {
            HttpWebRequest httpWebRequest = (HttpWebRequest)HttpWebRequest.Create(
                "https://api.github.com/repos/MistEO/MeoAssistance/releases/latest");
            httpWebRequest.Method = "GET";
            httpWebRequest.UserAgent = "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36";
            httpWebRequest.Accept = "application/vnd.github.v3+json";
            httpWebRequest.Timeout = 20000;
            try
            {
                HttpWebResponse httpWebResponse = (HttpWebResponse)httpWebRequest.GetResponse();
                StreamReader streamReader = new StreamReader(httpWebResponse.GetResponseStream(), Encoding.UTF8);
                string responseContent = streamReader.ReadToEnd();
                streamReader.Close();
                httpWebResponse.Close();
                return responseContent;
            }
            catch (Exception)
            {
                return string.Empty;
            }
            return string.Empty;
        }

        private string _latestUrl = "https://github.com/MistEO/MeoAssistance/releases/latest";

        public bool CheckUpdate()
        {
            string response = RequestApi();
            if (response.Length == 0)
            {
                return false;
            }
            JObject json = (JObject)JsonConvert.DeserializeObject(response);

            _latestVersion = json["tag_name"].ToString();
            if (string.Compare(_latestVersion, _curVersion) <= 0
                || ViewStatusStorage.Get("VersionUpdate.Ignore", string.Empty) == _latestVersion)
            {
                return false;
            }

            UpdateTag = "新版本：" + json["name"].ToString();
            UpdateInfo = json["body"].ToString();
            _latestUrl = json["html_url"].ToString();

            return true;
        }

        public void Download()
        {
            System.Diagnostics.Process.Start(_latestUrl);
            RequestClose();
        }

        public void Close()
        {
            RequestClose();
        }

        public void Ignore()
        {
            ViewStatusStorage.Set("VersionUpdate.Ignore", _latestVersion);
            RequestClose();
        }
    }
}