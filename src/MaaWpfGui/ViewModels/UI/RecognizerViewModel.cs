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
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
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

        private static string PadRightEx(string str, int totalByteCount)
        {
            Encoding coding = Encoding.GetEncoding("gb2312");
            int count = str.ToCharArray().Count(ch => coding.GetByteCount(ch.ToString()) == 2);

            string w = str.PadRight(totalByteCount - count);
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
                if (value)
                {
                    IsLevel3UseShortTime2 = false;
                }

                SetAndNotify(ref _isLevel3UseShortTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Level3UseShortTime, value.ToString());
            }
        }

        private bool _isLevel3UseShortTime2 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Level3UseShortTime2, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to shorten the time for level 3.
        /// </summary>
        public bool IsLevel3UseShortTime2
        {
            get => _isLevel3UseShortTime2;
            set
            {
                if (value)
                {
                    IsLevel3UseShortTime = false;
                }

                SetAndNotify(ref _isLevel3UseShortTime2, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Level3UseShortTime2, value.ToString());
            }
        }

        private bool _recruitCaught;

        /// <summary>
        /// Starts calculation.
        /// </summary>
        // xaml 中绑定了 action
        // ReSharper disable once UnusedMember.Global
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

        private bool _recruitmentShowPotential = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RecruitmentShowPotential, bool.TrueString));

        public bool RecruitmentShowPotential
        {
            get => _recruitmentShowPotential;
            set
            {
                SetAndNotify(ref _recruitmentShowPotential, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RecruitmentShowPotential, value.ToString());
            }
        }

        public void ProcRecruitMsg(JObject details)
        {
            string what = details["what"].ToString();
            var subTaskDetails = details["details"];

            switch (what)
            {
                case "RecruitTagsDetected":
                    {
                        JArray tags = (JArray)subTaskDetails["tags"];
                        string infoContent = LocalizationHelper.GetString("RecruitTagsDetected");
                        tags ??= new JArray();
                        infoContent = tags.Select(tagName => tagName.ToString()).Aggregate(infoContent, (current, tagStr) => current + (tagStr + "    "));

                        RecruitInfo = infoContent;
                    }

                    break;

                case "RecruitResult":
                    {
                        string resultContent = string.Empty;
                        JArray resultArray = (JArray)subTaskDetails["result"];
                        /* int level = (int)subTaskDetails["level"]; */
                        foreach (var combs in resultArray ?? new JArray())
                        {
                            int tagLevel = (int)combs["level"];
                            resultContent += tagLevel + " ★ Tags:  ";
                            resultContent = (((JArray)combs["tags"]) ?? new JArray()).Aggregate(resultContent, (current, tag) => current + (tag + "    "));

                            resultContent += "\n\t";
                            foreach (var oper in (JArray)combs["opers"] ?? new JArray())
                            {
                                int operLevel = (int)oper["level"];
                                string operId = oper["id"]?.ToString();
                                string operName = oper["name"]?.ToString();

                                string potential = string.Empty;

                                if (RecruitmentShowPotential && OperBoxPotential != null && operId != null
                                    && (tagLevel >= 4 || operLevel == 1))
                                {
                                    if (OperBoxPotential.ContainsKey(operId))
                                    {
                                        potential = " ( " + OperBoxPotential[operId] + " )";
                                    }
                                    else
                                    {
                                        potential = " ( !!! NEW !!! )";
                                    }
                                }

                                resultContent += operLevel + " - " + operName + potential + "    ";
                            }

                            resultContent += "\n\n";
                        }

                        RecruitResult = resultContent;
                    }

                    break;
            }
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

        private ObservableCollection<DepotResultDate> _depotResult = new ObservableCollection<DepotResultDate>();

        /// <summary>
        /// Gets or sets the depot result.
        /// </summary>
        public ObservableCollection<DepotResultDate> DepotResult
        {
            get => _depotResult;
            set => SetAndNotify(ref _depotResult, value);
        }

        public class DepotResultDate
        {
            public string Name { get; set; }

            public int Count { get; set; }
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
            if (details == null) { return false; }
            DepotResult.Clear();
            foreach (var item in details["arkplanner"]?["object"]?["items"]?.Cast<JObject>()!)
            {
                DepotResultDate result = new DepotResultDate() { Name = (string)item["name"], Count = (int)item["have"] };
                DepotResult.Add(result);
            }

            ArkPlannerResult = (string)details["arkplanner"]["data"];
            LoliconResult = (string)details["lolicon"]["data"];

            bool done = (bool)details["done"];
            if (!done)
            {
                return true;
            }

            DepotInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("DepotRecognitionTip");
            DepotDone = true;

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
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void ExportToArkplanner()
        {
            Clipboard.SetDataObject(ArkPlannerResult);
            DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        /// <summary>
        /// Export depot info to Lolicon.
        /// </summary>
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void ExportToLolicon()
        {
            Clipboard.SetDataObject(LoliconResult);
            DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        private void DepotClear()
        {
            DepotResult.Clear();
            ArkPlannerResult = string.Empty;
            LoliconResult = string.Empty;
            DepotDone = false;
        }

        /// <summary>
        /// Starts depot recognition.
        /// </summary>
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
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
        private static readonly HashSet<string> _virtuallyOpers = new HashSet<string>
        {
            "char_504_rguard",
            "char_505_rcast",
            "char_506_rmedic",
            "char_507_rsnipe",
            "char_514_rdfend",
            "char_513_apionr",
            "char_511_asnipe",
            "char_510_amedic",
            "char_509_acast",
            "char_508_aguard",
            "char_1001_amiya2",
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

        public string OperBoxExportData { get; set; } = string.Empty;

        private JArray _operBoxDataArray = (JArray)JsonConvert.DeserializeObject(ConfigurationHelper.GetValue(ConfigurationKeys.OperBoxData, string.Empty));

        public JArray OperBoxDataArray
        {
            get => _operBoxDataArray;
            set
            {
                SetAndNotify(ref _operBoxDataArray, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.OperBoxData, value.ToString());
                _operBoxPotential = null;   // reset
            }
        }

        private Dictionary<string, int> _operBoxPotential;

        public Dictionary<string, int> OperBoxPotential
        {
            get
            {
                if (OperBoxDataArray == null)
                {
                    return null;
                }

                if (_operBoxPotential != null)
                {
                    return _operBoxPotential;
                }

                _operBoxPotential = new Dictionary<string, int>();
                foreach (JObject operBoxData in OperBoxDataArray.Cast<JObject>())
                {
                    var id = (string)operBoxData["id"];
                    var potential = (int)operBoxData["potential"];
                    if (id != null)
                    {
                        _operBoxPotential[id] = potential;
                    }
                }

                return _operBoxPotential;
            }
        }

        public bool OperBoxParse(JObject details)
        {
            JArray operBoxes = (JArray)details["all_opers"];

            List<Tuple<string, int>> operHave = new List<Tuple<string, int>>();
            List<Tuple<string, int>> operNotHave = new List<Tuple<string, int>>();

            foreach (JObject operBox in operBoxes.Cast<JObject>())
            {
                var tuple = new Tuple<string, int>((string)operBox["name"], (int)operBox["rarity"]);

                if (_virtuallyOpers.Contains((string)operBox["id"]))
                {
                    continue;
                }

                if ((bool)operBox["own"])
                {
                    /*已拥有干员*/
                    operHave.Add(tuple);
                }
                else
                {
                    /*未拥有干员,包含预备干员等*/
                    operNotHave.Add(tuple);
                }
            }

            operHave.Sort((x, y) => y.Item2.CompareTo(x.Item2));
            operNotHave.Sort((x, y) => y.Item2.CompareTo(x.Item2));

            int newlineFlag = 0;
            string operNotHaveNames = "\t";

            foreach (var name in operNotHave.Select(tuple => tuple.Item1))
            {
                operNotHaveNames += PadRightEx(name, 12) + "\t";
                if (newlineFlag++ != 3)
                {
                    continue;
                }

                operNotHaveNames += "\n\t";
                newlineFlag = 0;
            }

            newlineFlag = 0;
            string operHaveNames = "\t";
            foreach (var name in operHave.Select(tuple => tuple.Item1))
            {
                operHaveNames += PadRightEx(name, 12) + "\t";
                if (newlineFlag++ != 3)
                {
                    continue;
                }

                operHaveNames += "\n\t";
                newlineFlag = 0;
            }

            bool done = (bool)details["done"];
            if (done)
            {
                OperBoxInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("OperBoxRecognitionTip");
                OperBoxResult = string.Format(LocalizationHelper.GetString("OperBoxRecognitionResult"), operNotHave.Count, operNotHaveNames, operHave.Count, operHaveNames);
                OperBoxExportData = details["own_opers"].ToString();
                OperBoxDataArray = (JArray)details["own_opers"];
                OperBoxDone = true;
            }
            else
            {
                OperBoxResult = operHaveNames;
            }

            return true;
        }

        /// <summary>
        /// 开始识别干员
        /// </summary>
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public async void StartOperBox()
        {
            OperBoxExportData = string.Empty;
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

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void ExportOperBox()
        {
            if (string.IsNullOrEmpty(OperBoxExportData))
            {
                return;
            }

            Clipboard.SetDataObject(OperBoxExportData);
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
                bool stop = value && !_gachaDone;
                SetAndNotify(ref _gachaDone, value);

                if (!stop)
                {
                    return;
                }

                _gachaImageTimer.Stop();

                // 强制再刷一下
                RefreshGachaImage(null, null);
            }
        }

        private string _gachaInfo = LocalizationHelper.GetString("GachaInitTip");

        public string GachaInfo
        {
            get => _gachaInfo;
            set => SetAndNotify(ref _gachaInfo, value);
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void GachaOnce()
        {
            StartGacha();
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
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
                if (GachaImage.IsEqual(image))
                {
                    return;
                }

                GachaImage = image;

                var rd = new Random();
                GachaInfo = LocalizationHelper.GetString("GachaTip" + rd.Next(1, 18).ToString());
            }
        }

        private bool _gachaShowDisclaimer = !Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, bool.FalseString));

        public bool GachaShowDisclaimer
        {
            get => _gachaShowDisclaimer;
            set
            {
                SetAndNotify(ref _gachaShowDisclaimer, value);
            }
        }

        private bool _gachaShowDisclaimerNoMore = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, bool.FalseString));

        public bool GachaShowDisclaimerNoMore
        {
            get => _gachaShowDisclaimerNoMore;
            set
            {
                SetAndNotify(ref _gachaShowDisclaimerNoMore, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, value.ToString());
            }
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void GachaAgreeDisclaimer()
        {
            GachaShowDisclaimer = false;
        }

        #endregion Gacha
    }
}
