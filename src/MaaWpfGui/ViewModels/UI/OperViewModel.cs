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
    public class OperViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        private AsstProxy _asstProxy;

        /// <summary>
        /// 未实装干员，但在battle_data中，
        /// </summary>
        private static readonly string[] VirtuallyOpers =
        {
            "预备干员-近战",
            "预备干员-术师",
            "预备干员-后勤",
            "预备干员-狙击",
            "预备干员-重装",
            "郁金香",
            "Stormeye",
            "Touch",
            "Pith",
            "Sharp",
            "阿米娅-WARRIOR",
        };

        public OperViewModel(IContainer container, IWindowManager windowManager)
        {
            _windowManager = windowManager;
            _container = container;
            DisplayName = LocalizationHelper.GetString("OperRecognition");
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
            _asstProxy = _container.Get<AsstProxy>();
        }

        private string _operInfo = LocalizationHelper.GetString("OperRecognitionTip");

        public string OperInfo
        {
            get => _operInfo;
            set => SetAndNotify(ref _operInfo, value);
        }

        private string _operResult;

        public string OperResult
        {
            get => _operResult;
            set => SetAndNotify(ref _operResult, value);
        }

        public bool Parse(JObject details)
        {
            string result = string.Empty;
            /*已拥有干员*/
            JArray operOwn = (JArray)details["have"];
            /*未拥有干员,包含预备干员等*/
            JArray operNotOwn = (JArray)details["nhave"];
            /*JArray operAll = (JArray)details["all"];*/
            /*groups.Where(i => i.Type == JTokenType.String && (string)i == groupName).ToList().ForEach(i => i.Remove());*/
            /*移除未实装干员*/
            foreach (var name in VirtuallyOpers)
            {
                operNotOwn.Where(i => i.Type == JTokenType.String && (string)i == name).ToList().ForEach(i => i.Remove());
            }
            
            result = "已拥有：" + operOwn.Count.ToString() + "名，未拥有：" + operNotOwn.Count.ToString() + "名；" + "\n\n";
            result += "以下干员未拥有：\n\t";
            int count = 0;
            foreach (var name in operNotOwn)
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

            result += "\n\n以下干员已拥有：\n\n\t";
            count = 0;
            foreach (var name in operOwn)
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

            OperResult = result;
            bool done = (bool)details["done"];
            if (done)
            {
                OperInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("OperRecognitionTip");
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
            OperResult = string.Empty;
            Done = false;
        }

        public async void Start()
        {
            string errMsg = string.Empty;
            OperInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => _asstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                OperInfo = errMsg;
                return;
            }

            OperInfo = LocalizationHelper.GetString("Identifying");
            Clear();

            _asstProxy.AsstStartOper();
        }
    }
}

