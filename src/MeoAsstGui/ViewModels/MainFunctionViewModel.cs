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

        private string _catchStatus;

        public string CatchStatus
        {
            get { return _catchStatus; }
            set
            {
                SetAndNotify(ref _catchStatus, value);
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

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();

            CatchStatus = "";
            ExecInfo = "";
            MedicineInfo = "";
            StoneInfo = "";
            RunStatus = "";
        }

        public void StartSanity()
        {
            var asstProxy = _container.Get<AsstProxy>();
            bool catched = asstProxy.AsstCatchDefault();
            CatchStatus = "捕获模拟器窗口：" + catched;
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

        public void Visit()
        {
            var asstProxy = _container.Get<AsstProxy>();
            bool catched = asstProxy.AsstCatchDefault();
            CatchStatus = "捕获模拟器窗口：" + catched;
            if (!asstProxy.AsstStartVisit())
            {
                return;
            }
            ExecInfo = "";
        }
    }
}