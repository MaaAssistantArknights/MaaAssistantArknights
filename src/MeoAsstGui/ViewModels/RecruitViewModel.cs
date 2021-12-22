// MeoAsstGui - A part of the MeoAssistantArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY

using System.Collections.Generic;
using System.Threading.Tasks;
using Stylet;
using StyletIoC;

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
            DisplayName = "公招识别";
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

        private bool _chooseLevel3 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel3", bool.FalseString));

        public bool ChooseLevel3
        {
            get { return _chooseLevel3; }
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel3", value.ToString());
            }
        }

        private bool _chooseLevel4 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel4", bool.TrueString));

        public bool ChooseLevel4
        {
            get { return _chooseLevel4; }
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel4", value.ToString());
            }
        }

        private bool _chooseLevel5 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel5", bool.TrueString));

        public bool ChooseLevel5
        {
            get { return _chooseLevel5; }
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel5", value.ToString());
            }
        }

        private bool _chooseLevel6 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel6", bool.TrueString));

        public bool ChooseLevel6
        {
            get { return _chooseLevel6; }
            set
            {
                SetAndNotify(ref _chooseLevel6, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel6", value.ToString());
            }
        }

        private bool _autoSetTime = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.AutoSetTime", bool.TrueString));

        public bool AutoSetTime
        {
            get { return _autoSetTime; }
            set
            {
                SetAndNotify(ref _autoSetTime, value);
                ViewStatusStorage.Set("Recruit.AutoSetTime", value.ToString());
            }
        }

        private bool _catched = false;

        public async void StartCalc()
        {
            var asstProxy = _container.Get<AsstProxy>();
            if (!_catched)
            {
                RecruitInfo = "正在捕获模拟器窗口……";
                var task = Task.Run(() =>
                {
                    return asstProxy.AsstCatch();
                });
                _catched = await task;
            }
            if (!_catched)
            {
                RecruitInfo = "捕获模拟器窗口失败，若是第一次运行，请尝试使用管理员权限";
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

            asstProxy.AsstStartRecruitCalc(levelList.ToArray(), levelList.Count, AutoSetTime);
            asstProxy.AsstStart();
        }
    }
}
