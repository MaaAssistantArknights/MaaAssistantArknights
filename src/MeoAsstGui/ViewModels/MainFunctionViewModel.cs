using System.Threading.Tasks;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class MainFunctionViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        public MainFunctionViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "刷理智";

            var asstProxy = _container.Get<AsstProxy>();
            if (UseMedicine)
            {
                asstProxy.AsstSetParam("task.action", "UseMedicine", "doNothing");
                asstProxy.AsstSetParam("task.maxTimes", "MedicineConfirm", int.MaxValue.ToString());
            }
            else
            {
                asstProxy.AsstSetParam("task.action", "UseMedicine", "stop");
                asstProxy.AsstSetParam("task.maxTimes", "MedicineConfirm", "0");
            }
        }

        private string _execInfo;

        public string ExecInfo
        {
            get { return _execInfo; }
            set
            {
                SetAndNotify(ref _execInfo, value);
            }
        }

        private string _medicineInfo;

        public string MedicineInfo
        {
            get { return _medicineInfo; }
            set
            {
                SetAndNotify(ref _medicineInfo, value);
            }
        }

        private string _stoneInfo;

        public string StoneInfo
        {
            get { return _stoneInfo; }
            set
            {
                SetAndNotify(ref _stoneInfo, value);
            }
        }

        private string _runStatus;

        public string RunStatus
        {
            get { return _runStatus; }
            set
            {
                SetAndNotify(ref _runStatus, value);
            }
        }

        private bool _shutdown;

        public bool Shutdown
        {
            get { return _shutdown; }
            set
            {
                SetAndNotify(ref _shutdown, value);
            }
        }

        private bool _useMedicine = System.Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.UseMedicine", bool.TrueString));

        public bool UseMedicine
        {
            get { return _useMedicine; }
            set
            {
                SetAndNotify(ref _useMedicine, value);
                ViewStatusStorage.Set("MainFunction.UseMedicine", value.ToString());
                var asstProxy = _container.Get<AsstProxy>();
                if (value)
                {
                    asstProxy.AsstSetParam("task.action", "UseMedicine", "doNothing");
                    int count;
                    if (MedicineNumber == "ALL")
                    {
                        count = int.MaxValue;
                    }
                    else if (!int.TryParse(MedicineNumber, out count))
                    {
                        count = 0;
                    }
                    asstProxy.AsstSetParam("task.maxTimes", "MedicineConfirm", count.ToString());
                }
                else
                {
                    asstProxy.AsstSetParam("task.action", "UseMedicine", "stop");
                    asstProxy.AsstSetParam("task.maxTimes", "MedicineConfirm", "0");
                }
            }
        }

        private bool _useStone;

        public bool UseStone
        {
            get { return _useStone; }
            set
            {
                SetAndNotify(ref _useStone, value);
                var asstProxy = _container.Get<AsstProxy>();
                if (value)
                {
                    asstProxy.AsstSetParam("task.action", "UseStone", "doNothing");
                    int count;
                    if (StoneNumber == "ALL")
                    {
                        count = int.MaxValue;
                    }
                    else if (!int.TryParse(StoneNumber, out count))
                    {
                        count = 0;
                    }
                    asstProxy.AsstSetParam("task.maxTimes", "StoneConfirm", count.ToString());
                }
                else
                {
                    asstProxy.AsstSetParam("task.action", "UseStone", "stop");
                    asstProxy.AsstSetParam("task.maxTimes", "StoneConfirm", "0");
                }
            }
        }

        private string _stoneNumber = "0";

        public string StoneNumber
        {
            get { return _stoneNumber; }
            set
            {
                SetAndNotify(ref _stoneNumber, value);
                if (UseStone)
                {
                    int.TryParse(value, out var count);
                    var asstProxy = _container.Get<AsstProxy>();
                    asstProxy.AsstSetParam("task.maxTimes", "StoneConfirm", count.ToString());
                }
            }
        }

        private string _medicineNumber = "ALL";

        public string MedicineNumber
        {
            get { return _medicineNumber; }
            set
            {
                SetAndNotify(ref _medicineNumber, value);
                if (UseMedicine)
                {
                    int.TryParse(value, out var count);
                    var asstProxy = _container.Get<AsstProxy>();
                    asstProxy.AsstSetParam("task.maxTimes", "MedicineConfirm", count.ToString());
                }
            }
        }

        private bool _hasTimesLimited;

        public bool HasTimesLimited
        {
            get { return _hasTimesLimited; }
            set
            {
                SetAndNotify(ref _hasTimesLimited, value);
                var asstProxy = _container.Get<AsstProxy>();
                if (value)
                {
                    int.TryParse(MaxTimes, out var count);
                    asstProxy.AsstSetParam("task.maxTimes", "StartButton1", count.ToString());
                }
                else
                {
                    asstProxy.AsstSetParam("task.maxTimes", "StartButton1", int.MaxValue.ToString());
                }
            }
        }

        private string _maxTimes = "5";

        public string MaxTimes
        {
            get { return _maxTimes; }
            set
            {
                SetAndNotify(ref _maxTimes, value);
                if (HasTimesLimited)
                {
                    var asstProxy = _container.Get<AsstProxy>();
                    int.TryParse(MaxTimes, out var count);
                    asstProxy.AsstSetParam("task.maxTimes", "StartButton1", count.ToString());
                }
            }
        }

        private bool _creditShopping = System.Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.CreditShopping", bool.FalseString));

        public bool CreditShopping
        {
            get { return _creditShopping; }
            set
            {
                SetAndNotify(ref _creditShopping, value);
                ViewStatusStorage.Set("MainFunction.CreditShopping", value.ToString());
            }
        }

        private bool _creditShoppingCheckBoxIsEnable = true;

        public bool CreditShoppingCheckBoxIsEnable
        {
            get { return _creditShoppingCheckBoxIsEnable; }
            set
            {
                SetAndNotify(ref _creditShoppingCheckBoxIsEnable, value);
            }
        }

        private string _stageDropsInfo = "";

        public string StageDropsInfo
        {
            get { return _stageDropsInfo; }
            set
            {
                SetAndNotify(ref _stageDropsInfo, value);
            }
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();

            ExecInfo = "";
            MedicineInfo = "";
            StoneInfo = "";
            RunStatus = "";
            StageDropsInfo = "";
            CreditShoppingCheckBoxIsEnable = true;
        }

        public async void StartSanity()
        {
            RunStatus = "正在捕获模拟器窗口……";
            var asstProxy = _container.Get<AsstProxy>();
            var task = Task.Run(() =>
            {
                return asstProxy.AsstCatchDefault();
            });
            bool catched = await task;
            if (!catched)
            {
                RunStatus = "捕获模拟器窗口失败，若是第一次运行，请尝试使用管理员权限";
                return;
            }
            if (!asstProxy.AsstStartSanity())
            {
                return;
            }
            ExecInfo = "";
            if (UseMedicine)
            {
                MedicineInfo = "已吃药 0 个";
            }
            if (UseStone)
            {
                StoneInfo = "已碎石 0 颗";
            }
        }

        public async void Visit()
        {
            RunStatus = "正在捕获模拟器窗口……";
            var asstProxy = _container.Get<AsstProxy>();
            var task = Task.Run(() =>
            {
                return asstProxy.AsstCatchDefault();
            });
            bool catched = await task;
            if (!catched)
            {
                RunStatus = "捕获模拟器窗口失败，若是第一次运行，请尝试使用管理员权限";
                return;
            }
            if (!asstProxy.AsstStartVisit(CreditShopping))
            {
                return;
            }
            CreditShoppingCheckBoxIsEnable = false;
            ExecInfo = "";
        }
    }
}
