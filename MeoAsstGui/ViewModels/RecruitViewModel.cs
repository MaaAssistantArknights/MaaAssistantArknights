using Stylet;
using StyletIoC;
using System.Collections.Generic;

namespace MeoAsstGui
{
    public class RecruitViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public RecruitViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "全自动公招";
        }

        private string _recruitInfo = "识别结果";

        public string RecruitInfo
        {
            get { return _recruitInfo; }
            set
            {
                SetAndNotify(ref _recruitInfo, value);
            }
        }

        private string _recruitResult;

        public string RecruitResult
        {
            get { return _recruitResult; }
            set
            {
                SetAndNotify(ref _recruitResult, value);
            }
        }

        private bool _chooseLevel3 = false;

        public bool ChooseLevel3
        {
            get { return _chooseLevel3; }
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
            }
        }

        private bool _chooseLevel4 = true;

        public bool ChooseLevel4
        {
            get { return _chooseLevel4; }
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
            }
        }

        private bool _chooseLevel5 = true;

        public bool ChooseLevel5
        {
            get { return _chooseLevel5; }
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
            }
        }

        private bool _chooseLevel6 = true;

        public bool ChooseLevel6
        {
            get { return _chooseLevel6; }
            set
            {
                SetAndNotify(ref _chooseLevel6, value);
            }
        }

        private bool _autoSetTime = true;

        public bool AutoSetTime
        {
            get { return _autoSetTime; }
            set
            {
                SetAndNotify(ref _autoSetTime, value);
            }
        }

        public void StartCalc()
        {
            var asstProxy = _container.Get<AsstProxy>();
            if (!asstProxy.AsstCatchEmulator())
            {
                return;
            }
            RecruitInfo = "正在识别……";
            RecruitResult = "";

            var levelList = new List<int>();

            if (ChooseLevel3)
            {
                levelList.Add(3);
            }
            if (ChooseLevel4)
            {
                levelList.Add(4);
            }
            if (ChooseLevel5)
            {
                levelList.Add(5);
            }
            if (ChooseLevel6)
            {
                levelList.Add(6);
            }

            asstProxy.AsstRunOpenRecruit(levelList.ToArray(), levelList.Count, AutoSetTime);
        }
    }
}
