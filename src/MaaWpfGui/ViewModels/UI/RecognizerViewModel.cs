// <copyright file="RecognizerViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
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

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
using Stylet;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of recruit.
    /// </summary>
    public class RecognizerViewModel : Screen
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RecognizerViewModel"/> class.
        /// </summary>
        public RecognizerViewModel()
        {
            DisplayName = LocalizationHelper.GetString("Toolbox");
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
        }

        private static string PadRightEx(string str, int totalByteCount)
        {
            Encoding coding = Encoding.GetEncoding("gb2312");
            int dcount = 0;
            foreach (char ch in str.ToCharArray())
            {
                if (coding.GetByteCount(ch.ToString()) == 2)
                {
                    dcount++;
                }
            }

            string w = str.PadRight(totalByteCount - dcount);
            return w;
        }

        #region Recruit

        private string _recruitInfo = LocalizationHelper.GetString("RecruitmentRecognitionTip");

        /// <summary>
        /// Gets or sets the recruit info.
        /// </summary>
        public string RecruitInfo
        {
            get => _recruitInfo;
            set => SetAndNotify(ref _recruitInfo, value);
        }

        private string _recruitResult;

        /// <summary>
        /// Gets or sets the recruit result.
        /// </summary>
        public string RecruitResult
        {
            get => _recruitResult;
            set => SetAndNotify(ref _recruitResult, value);
        }

        private bool _chooseLevel3 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 3.
        /// </summary>
        public bool ChooseLevel3
        {
            get => _chooseLevel3;
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel3, value.ToString());
            }
        }

        private bool _chooseLevel4 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 4.
        /// </summary>
        public bool ChooseLevel4
        {
            get => _chooseLevel4;
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel4, value.ToString());
            }
        }

        private bool _chooseLevel5 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 5.
        /// </summary>
        public bool ChooseLevel5
        {
            get => _chooseLevel5;
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel5, value.ToString());
            }
        }

        private bool _chooseLevel6 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel6, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 6.
        /// </summary>
        public bool ChooseLevel6
        {
            get => _chooseLevel6;
            set
            {
                SetAndNotify(ref _chooseLevel6, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel6, value.ToString());
            }
        }

        private bool _autoSetTime = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoSetTime, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to set time automatically.
        /// </summary>
        public bool RecruitAutoSetTime
        {
            get => _autoSetTime;
            set
            {
                SetAndNotify(ref _autoSetTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AutoSetTime, value.ToString());
            }
        }

        private bool _isLevel3UseShortTime = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Level3UseShortTime, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to shorten the time for level 3.
        /// </summary>
        public bool IsLevel3UseShortTime
        {
            get => _isLevel3UseShortTime;
            set
            {
                SetAndNotify(ref _isLevel3UseShortTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Level3UseShortTime, value.ToString());
            }
        }

        private bool _recruitCaught = false;

        /// <summary>
        /// Starts calculation.
        /// </summary>
        public async void RecruitStartCalc()
        {
            string errMsg = string.Empty;
            RecruitInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            _recruitCaught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!_recruitCaught)
            {
                RecruitInfo = errMsg;
                return;
            }

            RecruitInfo = LocalizationHelper.GetString("Identifying");
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

            Instances.AsstProxy.AsstStartRecruitCalc(levelList.ToArray(), RecruitAutoSetTime);
        }

        #endregion Recruit

        #region Depot

        private string _depotInfo = LocalizationHelper.GetString("DepotRecognitionTip");

        /// <summary>
        /// Gets or sets the depot info.
        /// </summary>
        public string DepotInfo
        {
            get => _depotInfo;
            set => SetAndNotify(ref _depotInfo, value);
        }

        private string _depotResult;

        /// <summary>
        /// Gets or sets the depot result.
        /// </summary>
        public string DepotResult
        {
            get => _depotResult;
            set => SetAndNotify(ref _depotResult, value);
        }

        /// <summary>
        /// Gets or sets the ArkPlanner result.
        /// </summary>
        public string ArkPlannerResult { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the Lolicon result.
        /// </summary>
        public string LoliconResult { get; set; } = string.Empty;

        /// <summary>
        /// parse of depot recognition result
        /// </summary>
        /// <param name="details">detailed json-style parameters</param>
        /// <returns>Success or not</returns>
        public bool DepotParse(JObject details)
        {
            string result = string.Empty;
            int count = 0;
            foreach (var item in details["arkplanner"]["object"]["items"].Cast<JObject>())
            {
                result += PadRightEx((string)item["name"], 12) + " : " + ((string)item["have"]).PadRight(5) + "\t";
                if (++count == 3)
                {
                    result += "\n";
                    count = 0;
                }
            }

            DepotResult = result;

            ArkPlannerResult = (string)details["arkplanner"]["data"];
            LoliconResult = (string)details["lolicon"]["data"];

            bool done = (bool)details["done"];
            if (done)
            {
                DepotInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("DepotRecognitionTip");
                DepotDone = true;
            }

            return true;
        }

        private bool _depotDone = true;

        /// <summary>
        /// Gets or sets a value indicating whether depot info is parsed.
        /// </summary>
        public bool DepotDone
        {
            get => _depotDone;
            set => SetAndNotify(ref _depotDone, value);
        }

        /// <summary>
        /// Export depot info to ArkPlanner.
        /// </summary>
        public void ExportToArkplanner()
        {
            Clipboard.SetDataObject(ArkPlannerResult);
            DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        /// <summary>
        /// Export depot info to Lolicon.
        /// </summary>
        public void ExportToLolicon()
        {
            Clipboard.SetDataObject(LoliconResult);
            DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        private void DepotClear()
        {
            DepotResult = string.Empty;
            ArkPlannerResult = string.Empty;
            LoliconResult = string.Empty;
            DepotDone = false;
        }

        /// <summary>
        /// Starts depot recognition.
        /// </summary>
        public async void StartDepot()
        {
            string errMsg = string.Empty;
            DepotInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                DepotInfo = errMsg;
                return;
            }

            DepotInfo = LocalizationHelper.GetString("Identifying");
            DepotClear();

            Instances.AsstProxy.AsstStartDepot();
        }

        #endregion Depot

        #region OperBox

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

        private string _operBoxInfo = LocalizationHelper.GetString("OperBoxRecognitionTip");

        public string OperBoxInfo
        {
            get => _operBoxInfo;
            set => SetAndNotify(ref _operBoxInfo, value);
        }

        private string _operBoxResult;

        public string OperBoxResult
        {
            get => _operBoxResult;
            set => SetAndNotify(ref _operBoxResult, value);
        }

        private bool _operBoxDone = true;

        /// <summary>
        /// Gets or sets a value indicating whether operBox info is parsed.
        /// </summary>
        public bool OperBoxDone
        {
            get => _operBoxDone;
            set => SetAndNotify(ref _operBoxDone, value);
        }

        private string _operBoxExportData = string.Empty;

        public bool OperBoxParse(JObject details)
        {
            JArray operBoxs = (JArray)details["all_opers"];

            List<string> operHave = new List<string>();
            List<string> operNotHave = new List<string>();

            foreach (JObject operBox in operBoxs.Cast<JObject>())
            {
                if ((bool)operBox["own"])
                {
                    /*已拥有干员*/
                    operHave.Add((string)operBox["name"]);
                }
                else
                {
                    /*未拥有干员,包含预备干员等*/
                    operNotHave.Add((string)operBox["name"]);
                }
            }

            /*移除未实装干员*/
            operNotHave = operNotHave.Except(second: VirtuallyOpers).ToList();

            int newline_flag = 0;
            string operNotHaveNames = "\t";

            foreach (var name in operNotHave)
            {
                operNotHaveNames += PadRightEx(name, 12) + "\t";
                if (newline_flag++ == 3)
                {
                    operNotHaveNames += "\n\t";
                    newline_flag = 0;
                }
            }

            newline_flag = 0;
            string operHaveNames = "\t";
            foreach (var name in operHave)
            {
                operHaveNames += PadRightEx(name, 12) + "\t";
                if (newline_flag++ == 3)
                {
                    operHaveNames += "\n\t";
                    newline_flag = 0;
                }
            }

            bool done = (bool)details["done"];
            if (done)
            {
                OperBoxInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("OperBoxRecognitionTip");
                OperBoxResult = string.Format(LocalizationHelper.GetString("OperBoxRecognitionResult"), operNotHave.Count, operNotHaveNames, operHave.Count, operHaveNames);
                _operBoxExportData = details["own_opers"].ToString();
                OperBoxDone = true;
            }
            else
            {
                OperBoxResult = operHaveNames;
            }

            return true;
        }

        public async void StartOperBox()
        {
            _operBoxExportData = string.Empty;
            OperBoxDone = false;

            string errMsg = string.Empty;
            OperBoxInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                OperBoxInfo = errMsg;
                return;
            }

            if (Instances.AsstProxy.AsstStartOperBox())
            {
                OperBoxInfo = LocalizationHelper.GetString("Identifying");
            }
        }

        public void ExportOperBox()
        {
            if (string.IsNullOrEmpty(_operBoxExportData))
            {
                return;
            }

            Clipboard.SetDataObject(_operBoxExportData);
            OperBoxInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        #endregion OperBox

        #region Gacha

        private bool _gachaDone = true;

        public bool GachaDone
        {
            get => _gachaDone;
            set
            {
                SetAndNotify(ref _gachaDone, value);
                if (value)
                {
                    _gachaImageTimer.Stop();
                    // 强制再刷一下
                    RefreshGachaImage(null, null);
                }
            }
        }

        private string _gachaInfo = LocalizationHelper.GetString("GachaInitTip");

        public string GachaInfo
        {
            get => _gachaInfo;
            set => SetAndNotify(ref _gachaInfo, value);
        }

        public void GachaOnce()
        {
            StartGacha(true);
        }

        public void GachaTenTimes()
        {
            StartGacha(false);
        }

        public async void StartGacha(bool once = true)
        {
            GachaDone = false;
            GachaImage = null;

            string errMsg = string.Empty;
            GachaInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                GachaInfo = errMsg;
                return;
            }

            if (!Instances.AsstProxy.AsstStartGacha(once))
            {
                return;
            }

            _gachaImageTimer.Interval = TimeSpan.FromMilliseconds(500);
            _gachaImageTimer.Tick += RefreshGachaImage;
            _gachaImageTimer.Start();
        }

        private readonly DispatcherTimer _gachaImageTimer = new DispatcherTimer();

        private static readonly object _lock = new object();

        private BitmapImage _gachaImage;

        public BitmapImage GachaImage
        {
            get => _gachaImage;
            set => SetAndNotify(ref _gachaImage, value);
        }

        private void RefreshGachaImage(object sender, EventArgs e)
        {
            lock (_lock)
            {
                var image = Instances.AsstProxy.AsstGetImage();
                if (!GachaImage.IsEqual(image))
                {
                    GachaImage = image;

                    var rd = new Random();
                    GachaInfo = LocalizationHelper.GetString("GachaTip" + rd.Next(1, 18).ToString());
                }
            }
        }

        #endregion Gacha
    }
}
