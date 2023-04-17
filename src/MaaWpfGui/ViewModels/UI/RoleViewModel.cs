using System;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.ViewModels.UI
{
    public class RoleViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        private AsstProxy _asstProxy;

        public RoleViewModel(IContainer container, IWindowManager windowManager)
        {
            _windowManager = windowManager;
            _container = container;
            DisplayName = LocalizationHelper.GetString("RoleRecognition");
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
            _asstProxy = _container.Get<AsstProxy>();
        }

        private string _roleInfo = LocalizationHelper.GetString("RoleRecognitionTip");

        public string RoleInfo
        {
            get => _roleInfo;
            set => SetAndNotify(ref _roleInfo, value);
        }

        private string _roleResult;

        public string RoleResult
        {
            get => _roleResult;
            set => SetAndNotify(ref _roleResult, value);
        }

        public bool Parse(JObject details)
        {
            string result = string.Empty;
            /*foreach (var item in details.Cast<JProperty>())
            {
                result += (string)item["name"] + "\n";
            }*/

            /*result += details.Cast<JObject>();*/
            JArray allRoles = (JArray)details["allroles"];
            JArray nRoles = (JArray)details["nroles"];
            JArray hRoles = (JArray)details["hroles"];

            /*string[] allroles = details["allroles"].Value<List<string>>().ToArray();
            string[] noroles = details["nroles"].Value<List<string>>().ToArray();*/
            result = "干员总计：" + (int)details["total"] + "名，已拥有：" + (int)details["have"] + "名，未拥有：" + nRoles.Count.ToString() + "名；" + "\n\n\t";
            int count = 0;
            foreach (var name in nRoles)
            {
                if (count++ < 3)
                {
                    result += name + ",  ";
                }
                else
                {
                    result += name + "\n\t";
                    count = 0;
                }
            }

            RoleResult = result;
            bool done = (bool)details["done"];
            if (done)
            {
                RoleInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("RoleRecognitionTip");
                Done = true;
            }

            return true;
        }

        private bool _done = false;

        /// <summary>
        /// Gets or sets a value indicating whether depot info is parsed.
        /// </summary>
        public bool Done
        {
            get => _done;
            set => SetAndNotify(ref _done, value);
        }

        private void Clear()
        {
            RoleResult = string.Empty;
            Done = false;
        }

        public async void Start()
        {
            string errMsg = string.Empty;
            RoleInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => _asstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                RoleInfo = errMsg;
                return;
            }

            RoleInfo = LocalizationHelper.GetString("Identifying");
            Clear();

            _asstProxy.AsstStartRole();
        }
    }
}

