// <copyright file="RecruitViewModel.cs" company="MaaAssistantArknights">
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
// </copyright>

using System.Collections.Generic;
using System.Threading.Tasks;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    /// <summary>
    /// The view model of recruit.
    /// </summary>
    public class RecruitViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        /// <summary>
        /// Initializes a new instance of the <see cref="RecruitViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        /// <param name="windowManager">The window manager.</param>
        public RecruitViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = Localization.GetString("Recognition");
        }

        private string _recruitInfo = Localization.GetString("RecognitionTip");

        /// <summary>
        /// Gets or sets the recruit info.
        /// </summary>
        public string RecruitInfo
        {
            get { return _recruitInfo; }
            set { SetAndNotify(ref _recruitInfo, value); }
        }

        private string _recruitResult;

        /// <summary>
        /// Gets or sets the recruit result.
        /// </summary>
        public string RecruitResult
        {
            get { return _recruitResult; }
            set { SetAndNotify(ref _recruitResult, value); }
        }

        private bool _chooseLevel3 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel3", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 3.
        /// </summary>
        public bool ChooseLevel3
        {
            get
            {
                return _chooseLevel3;
            }

            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel3", value.ToString());
            }
        }

        private bool _chooseLevel4 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel4", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 4.
        /// </summary>
        public bool ChooseLevel4
        {
            get
            {
                return _chooseLevel4;
            }

            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel4", value.ToString());
            }
        }

        private bool _chooseLevel5 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel5", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 5.
        /// </summary>
        public bool ChooseLevel5
        {
            get
            {
                return _chooseLevel5;
            }

            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel5", value.ToString());
            }
        }

        private bool _chooseLevel6 = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.ChooseLevel6", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 6.
        /// </summary>
        public bool ChooseLevel6
        {
            get
            {
                return _chooseLevel6;
            }

            set
            {
                SetAndNotify(ref _chooseLevel6, value);
                ViewStatusStorage.Set("Recruit.ChooseLevel6", value.ToString());
            }
        }

        private bool _autoSetTime = System.Convert.ToBoolean(ViewStatusStorage.Get("Recruit.AutoSetTime", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to set time automatically.
        /// </summary>
        public bool AutoSetTime
        {
            get
            {
                return _autoSetTime;
            }

            set
            {
                SetAndNotify(ref _autoSetTime, value);
                ViewStatusStorage.Set("Recruit.AutoSetTime", value.ToString());
            }
        }

        private bool _caught = false;

        /// <summary>
        /// Starts calculation.
        /// </summary>
        public async void StartCalc()
        {
            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = string.Empty;
            RecruitInfo = Localization.GetString("ConnectingToEmulator");
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg);
            });
            _caught = await task;
            if (!_caught)
            {
                RecruitInfo = errMsg;
                return;
            }

            RecruitInfo = Localization.GetString("Identifying");
            RecruitResult = string.Empty;

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
        }
    }
}
