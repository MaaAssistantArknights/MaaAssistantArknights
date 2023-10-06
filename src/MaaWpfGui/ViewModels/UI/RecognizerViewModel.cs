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
using HandyControl.Tools.Extension;
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
        // UI 绑定的方法
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
                string localized_name = name;

                if (!(ConfigurationHelper.GetValue(ConfigurationKeys.Localization, "zh-cn") == "zh-cn"))
                {
                    string[] zh_cn = {
"Lancet-2",
"Castle-3",
"THRM-EX",
"正义骑士号",
"泰拉大陆调查团",
"U-Official",
"Friston-3",
"12F",
"黑角",
"杜林",
"夜刀",
"巡林者",
"芙蓉",
"炎熔",
"米格鲁",
"芬",
"克洛丝",
"翎羽",
"玫兰莎",
"卡缇",
"史都华德",
"安德切尔",
"安赛尔",
"香草",
"梓兰",
"泡普卡",
"空爆",
"月见夜",
"斑点",
"远山",
"深海色",
"末药",
"白雪",
"流星",
"艾丝黛尔",
"杜宾",
"梅",
"猎蜂",
"夜烟",
"清道夫",
"蛇屠箱",
"桃金娘",
"断罪者",
"调香师",
"地灵",
"慕斯",
"嘉维尔",
"红云",
"霜叶",
"古米",
"讯使",
"角峰",
"杰西卡",
"暗索",
"砾",
"格雷伊",
"波登可",
"坚雷",
"芳汀",
"孑",
"阿消",
"缠丸",
"红豆",
"苏苏洛",
"刻刀",
"安比尔",
"卡达",
"宴",
"杰克",
"伊桑",
"酸糖",
"泡泡",
"清流",
"松果",
"豆苗",
"深靛",
"罗比菈塔",
"休谟斯",
"布丁",
"褐果",
"铅踝",
"石英",
"罗小黑",
"阿米娅",
"空",
"德克萨斯",
"芙兰卡",
"雷蛇",
"赫默",
"凛冬",
"白面鸮",
"蓝毒",
"炎客",
"星源",
"拉普兰德",
"幽灵鲨",
"红",
"普罗旺斯",
"临光",
"摩根",
"因陀罗",
"达格达",
"守林人",
"火神",
"夜魔",
"天火",
"华法琳",
"崖心",
"初雪",
"真理",
"可颂",
"白金",
"卡夫卡",
"狮蝎",
"安哲拉",
"陨星",
"格拉尼",
"吽",
"暴行",
"食铁兽",
"梅尔",
"槐琥",
"柏喙",
"巫恋",
"苇草",
"鞭刃",
"星极",
"微风",
"送葬人",
"断崖",
"和弦",
"暴雨",
"惊蛰",
"诗怀雅",
"拜松",
"格劳克斯",
"铸铁",
"稀音",
"爱丽丝",
"寒檀",
"月禾",
"蜜蜡",
"亚叶",
"奥斯塔",
"锡兰",
"贾维",
"布洛卡",
"熔泉",
"四月",
"灰喉",
"贝娜",
"莱恩哈特",
"石棉",
"慑砂",
"雪雉",
"薄绿",
"极境",
"图耶",
"苦艾",
"特米米",
"燧石",
"羽毛笔",
"极光",
"灰毫",
"掠风",
"絮雨",
"蜜莓",
"罗宾",
"乌有",
"闪击",
"霜华",
"战车",
"洋灰",
"雪绒",
"桑葚",
"赤冬",
"夜半",
"绮良",
"龙舌兰",
"青枳",
"蚀清",
"夏栎",
"火哨",
"野鬃",
"晓歌",
"隐现",
"郁金香",
"炎狱炎熔",
"寒芒克洛丝",
"濯尘芙蓉",
"承曦格雷伊",
"火龙S黑角",
"玫拉",
"耶拉",
"子月",
"空构",
"风丸",
"谜图",
"九色鹿",
"暮落",
"但书",
"见行者",
"洛洛",
"埃拉托",
"海蒂",
"车尔尼",
"至简",
"海沫",
"明椒",
"截云",
"铎铃",
"凛视",
"苍苔",
"冰酿",
"凯尔希",
"陈",
"煌",
"能天使",
"推进之王",
"W",
"伊芙利特",
"星熊",
"闪灵",
"银灰",
"夜莺",
"艾雅法拉",
"赫拉格",
"早露",
"塞雷娅",
"灵知",
"莫斯提马",
"风笛",
"阿",
"麦哲伦",
"缪尔赛思",
"傀影",
"斯卡蒂",
"山",
"安洁莉娜",
"棘刺",
"菲亚梅塔",
"泥岩",
"老鲤",
"空弦",
"黑",
"史尔特尔",
"铃兰",
"嵯峨",
"澄闪",
"迷迭香",
"温蒂",
"森蚺",
"焰尾",
"瑕光",
"卡涅利安",
"伺夜",
"远牙",
"水月",
"灰烬",
"异客",
"歌蕾蒂娅",
"琴柳",
"帕拉斯",
"浊心斯卡蒂",
"假日威龙陈",
"耀骑士临光",
"纯烬艾雅法拉",
"焰影苇草",
"归溟幽灵鲨",
"百炼嘉维尔",
"缄默德克萨斯",
"麒麟R夜刀",
"淬羽赫默",
"圣约送葬人",
"琳琅诗怀雅",
"涤火杰西卡",
"提丰",
"刻俄柏",
"年",
"夕",
"令",
"重岳",
"艾丽妮",
"霍尔海雅",
"号角",
"流明",
"黑键",
"多萝西",
"鸿雪",
"玛恩纳",
"斥罪",
"白铁",
"林",
"仇白",
"伊内丝",
};

                    switch (ConfigurationHelper.GetValue(ConfigurationKeys.Localization, "zh-cn"))
                    {
                        case "en-us":
                            string[] en_us = {
"Lancet-2",
"Castle-3",
"THRM-EX",
"'Justice Knight'",
"Terra Research Commission",
"U-Official",
"Friston-3",
"12F",
"Noir Corne",
"Durin",
"Yato",
"Rangers",
"Hibiscus",
"Lava",
"Beagle",
"Fang",
"Kroos",
"Plume",
"Melantha",
"Cardigan",
"Steward",
"Adnachiel",
"Ansel",
"Vanilla",
"Orchid",
"Popukar",
"Catapult",
"Midnight",
"Spot",
"Gitano",
"Deepcolor",
"Myrrh",
"Shirayuki",
"Meteor",
"Estelle",
"Dobermann",
"May",
"Beehunter",
"Haze",
"Scavenger",
"Cuora",
"Myrtle",
"Conviction",
"Perfumer",
"Earthspirit",
"Mousse",
"Gavial",
"Vermeil",
"Frostleaf",
"Gummy",
"Courier",
"Matterhorn",
"Jessica",
"Rope",
"Gravel",
"Greyy",
"Podenco",
"Dur-nar",
"Arene",
"Jaye",
"Shaw",
"Matoimaru",
"Vigna",
"Sussurro",
"Cutter",
"Ambriel",
"Click",
"Utage",
"Jackie",
"Ethan",
"Aciddrop",
"Bubble",
"Purestream",
"Pinecone",
"Beanstalk",
"Indigo",
"Roberta",
"Humus",
"Pudding",
"Chestnut",
"Totter",
"Quartz",
"Luo Xiaohei",
"Amiya",
"Sora",
"Texas",
"Franka",
"Liskarm",
"Silence",
"Zima",
"Ptilopsis",
"Blue Poison",
"Flamebringer",
"Astgenne",
"Lappland",
"Specter",
"Projekt Red",
"Provence",
"Nearl",
"Morgan",
"Indra",
"Dagda",
"Firewatch",
"Vulcan",
"Nightmare",
"Skyfire",
"Warfarin",
"Cliffheart",
"Pramanix",
"Istina",
"Croissant",
"Platinum",
"Kafka",
"Manticore",
"Andreana",
"Meteorite",
"Grani",
"Hung",
"Savage",
"FEater",
"Mayer",
"Waai Fu",
"Bibeak",
"Shamare",
"Reed",
"Whislash",
"Astesia",
"Breeze",
"Executor",
"Ayerscarpe",
"Harmonie",
"Heavyrain",
"Leizi",
"Swire",
"Bison",
"Glaucus",
"Sideroca",
"Scene",
"Iris",
"Santalla",
"Tsukinogi",
"Beeswax",
"Folinic",
"Aosta",
"Ceylon",
"Chiave",
"Broca",
"Toddifons",
"April",
"GreyThroat",
"Bena",
"Leonhardt",
"Asbestos",
"Sesa",
"Snowsant",
"Mint",
"Elysium",
"Tuye",
"Absinthe",
"Tomimi",
"Flint",
"La Pluma",
"Aurora",
"Ashlock",
"Windflit",
"Whisperain",
"Honeyberry",
"Robin",
"Mr. Nothing",
"Blitz",
"Frost",
"Tachanka",
"Cement",
"Qanipalaat",
"Mulberry",
"Akafuyu",
"Blacknight",
"Kirara",
"Tequila",
"Poncirus",
"Corroserum",
"Quercus",
"Firewhistle",
"Wild Mane",
"Cantabile",
"Insider",
"Tulip",
"Lava the Purgatory",
"Kroos the Keen Glint",
"Hibiscus the Purifier",
"Greyy the Lightningbearer",
"Rathalos S Noir Corne",
"Melanite",
"Kjera",
"Lunacub",
"Spuria",
"Kazemaru",
"Puzzle",
"Nine-Colored Deer",
"Shalem",
"Proviso",
"Enforcer",
"Rockrock",
"Erato",
"Heidi",
"Czerny",
"Minimalist",
"Highmore",
"Paprika",
"Jieyun",
"Wind Chimes",
"Valarqvin",
"Bryophyta",
"Coldshot",
"Kal'tsit",
"Ch'en",
"Blaze",
"Exusiai",
"Siege",
"W",
"Ifrit",
"Hoshiguma",
"Shining",
"SilverAsh",
"Nightingale",
"Eyjafjalla",
"Hellagur",
"Rosa",
"Saria",
"Gnosis",
"Mostima",
"Bagpipe",
"Aak",
"Magallan",
"Muelsyse",
"Phantom",
"Skadi",
"Mountain",
"Angelina",
"Thorns",
"Fiammetta",
"Mudrock",
"Lee",
"Archetto",
"Schwarz",
"Surtr",
"Suzuran",
"Saga",
"Goldenglow",
"Rosmontis",
"Weedy",
"Eunectes",
"Flametail",
"Blemishine",
"Carnelian",
"Vigil",
"Fartooth",
"Mizuki",
"Ash",
"Passenger",
"Gladiia",
"Saileach",
"Pallas",
"Skadi the Corrupting Heart",
"Ch'en the Holungday",
"Nearl the Radiant Knight",
"Eyjafjalla the Hvít Aska",
"Reed the Flame Shadow",
"Specter the Unchained",
"Gavial the Invincible",
"Texas the Omertosa",
"Kirin R Yato",
"Silence the Paradigmatic",
"Executor the Ex Foedere",
"Swire the Elegant Wit",
"Jessica the Liberated",
"Typhon",
"Ceobe",
"Nian",
"Dusk",
"Ling",
"Chongyue",
"Irene",
"Ho'olheyak",
"Horn",
"Lumen",
"Ebenholz",
"Dorothy",
"Pozëmka",
"Młynar",
"Penance",
"Stainless",
"Lin",
"Qiubai",
"Ines",

};
                            for (var i = 0; i < zh_cn.Length; i++)
                            {
                                if (name == zh_cn[i])
                                {
                                    if (en_us[i].IsNullOrEmpty())
                                    {
                                        break;
                                    }

                                    localized_name = en_us[i];
                                }
                            }

                            break;

                        case "ja-jp":
                            string[] ja_jp = {"Lancet-2",
"Castle-3",
"THRM-EX",
"ジャスティスナイト",
"テラ大陸調査団",
"",
"",
"12F",
"ノイルホーン",
"ドゥリン",
"ヤトウ",
"レンジャー",
"ハイビスカス",
"ラヴァ",
"ビーグル",
"フェン",
"クルース",
"プリュム",
"メランサ",
"カーディ",
"スチュワード",
"アドナキエル",
"アンセル",
"バニラ",
"オーキッド",
"ポプカル",
"カタパルト",
"ミッドナイト",
"スポット",
"ギターノ",
"ディピカ",
"ミルラ",
"シラユキ",
"メテオ",
"エステル",
"ドーベルマン",
"メイ",
"ビーハンター",
"ヘイズ",
"スカベンジャー",
"クオーラ",
"テンニンカ",
"コンビクション",
"パフューマー",
"アーススピリット",
"ムース",
"ガヴィル",
"ヴァーミル",
"フロストリーフ",
"グム",
"クーリエ",
"マッターホルン",
"ジェシカ",
"ロープ",
"グラベル",
"グレイ",
"ポデンコ",
"ジュナー",
"アレーン",
"ジェイ",
"ショウ",
"マトイマル",
"ヴィグナ",
"ススーロ",
"カッター",
"アンブリエル",
"カシャ",
"ウタゲ",
"ジャッキー",
"イーサン",
"アシッドドロップ",
"バブル",
"セイリュウ",
"パインコーン",
"ビーンストーク",
"インディゴ",
"ロベルタ",
"",
"プリン",
"チェストナット",
"トター",
"クォーツ",
"羅小黒",
"アーミヤ",
"ソラ",
"テキサス",
"フランカ",
"リスカム",
"サイレンス",
"ズィマー",
"フィリオプシス",
"アズリウス",
"エンカク",
"アステジーニ",
"ラップランド",
"スペクター",
"レッド",
"プロヴァンス",
"ニアール",
"",
"インドラ",
"ダグザ",
"ファイヤーウォッチ",
"ヴァルカン",
"ナイトメア",
"スカイフレア",
"ワルファリン",
"クリフハート",
"プラマニクス",
"イースチナ",
"クロワッサン",
"プラチナ",
"カフカ",
"マンティコア",
"アンドレアナ",
"メテオリーテ",
"グラニ",
"ウン",
"サベージ",
"エフイーター",
"メイヤー",
"ワイフー",
"バイビーク",
"シャマレ",
"リード",
"ウィスラッシュ",
"アステシア",
"ブリーズ",
"イグゼキュター",
"エアースカーペ",
"ハーモニー",
"ヘビーレイン",
"レイズ",
"スワイヤー",
"バイソン",
"グラウコス",
"シデロカ",
"シーン",
"アイリス",
"",
"ツキノギ",
"ビーズワクス",
"フォリニック",
"アオスタ",
"セイロン",
"キアーベ",
"ブローカ",
"トギフォンス",
"エイプリル",
"グレースロート",
"ベナ",
"レオンハルト",
"アスベストス",
"シェーシャ",
"スノーズント",
"ミント",
"エリジウム",
"トゥイエ",
"アブサント",
"トミミ",
"フリント",
"ラ・プルマ",
"オーロラ",
"アッシュロック",
"ウインドフリット",
"ウィスパーレイン",
"ハニーベリー",
"ロビン",
"ウユウ",
"Blitz",
"Frost",
"Tachanka",
"",
"カニパラート",
"マルベリー",
"アカフユ",
"ブラックナイト",
"キララ",
"テキーラ",
"",
"コロセラム",
"クエルクス",
"ファイヤーホイッスル",
"ワイルドメイン",
"カンタービレ",
"",
"チューリップ",
"炎獄ラヴァ",
"寒芒クルース",
"濯塵ハイビスカス",
"承曦グレイ",
"レウスSノイルホーン",
"",
"イェラ",
"ルナカブ",
"",
"カゼマル",
"パズル",
"九色鹿",
"シャレム",
"プロヴァイゾ",
"エンフォーサー",
"ロックロック",
"エラト",
"ハイディ",
"ツェルニー",
"ミニマリスト",
"ハイモア",
"パプリカ",
"ジエユン",
"ウィンドチャイム",
"",
"",
"",
"ケルシー",
"チェン",
"ブレイズ",
"エクシア",
"シージ",
"W",
"イフリータ",
"ホシグマ",
"シャイニング",
"シルバーアッシュ",
"ナイチンゲール",
"エイヤフィヤトラ",
"ヘラグ",
"ロサ",
"サリア",
"ノーシス",
"モスティマ",
"バグパイプ",
"ア",
"マゼラン",
"",
"ファントム",
"スカジ",
"マウンテン",
"アンジェリーナ",
"ソーンズ",
"フィアメッタ",
"マドロック",
"リー",
"アルケット",
"シュヴァルツ",
"スルト",
"スズラン",
"サガ",
"ゴールデングロー",
"ロスモンティス",
"ウィーディ",
"ユーネクテス",
"フレイムテイル",
"ブレミシャイン",
"カーネリアン",
"ヴィジェル",
"ファートゥース",
"ミヅキ",
"Ash",
"パッセンジャー",
"グレイディーア",
"サイラッハ",
"パラス",
"濁心スカジ",
"遊龍チェン",
"耀騎士ニアール",
"",
"焔影リード",
"帰溟スペクター",
"百錬ガヴィル",
"血掟テキサス",
"キリンRヤトウ",
"",
"",
"",
"",
"",
"ケオベ",
"ニェン",
"シー",
"リィン",
"チョンユエ",
"アイリーニ",
"",
"ホルン",
"ルーメン",
"エーベンホルツ",
"ドロシー",
"パゼオンカ",
"ムリナール",
"ペナンス",
"ステインレス",
"リン",
"チューバイ",
"",
};
                            for (var i = 0; i < zh_cn.Length; i++)
                            {
                                if (name == zh_cn[i])
                                {
                                    if (ja_jp[i].IsNullOrEmpty())
                                    {
                                        break;
                                    }

                                    localized_name = ja_jp[i];
                                }
                            }

                            break;

                        case "ko-kr":
                            string[] ko_kr = {"Lancet-2",
"Castle-3",
"THRM-EX",
"저스티스 나이트",
"",
"",
"",
"12F",
"느와르 코르네",
"두린",
"야토",
"레인저",
"히비스커스",
"라바",
"비글",
"팽",
"크루스",
"플룸",
"멜란사",
"카디건",
"스튜어드",
"아드나키엘",
"안셀",
"바닐라",
"오키드",
"포푸카",
"캐터펄트",
"미드나이트",
"스팟",
"기타노",
"딥컬러",
"미르",
"시라유키",
"메테오",
"에스텔",
"도베르만",
"메이",
"비헌터",
"헤이즈",
"스캐빈저",
"쿠오라",
"머틀",
"컨빅션",
"퍼퓨머",
"어스스피릿",
"무스",
"가비알",
"버메일",
"프로스트리프",
"굼",
"쿠리어",
"마터호른",
"제시카",
"로프",
"그라벨",
"그레이",
"포덴코",
"듀나",
"아렌",
"제이",
"쇼",
"마토이마루",
"비그나",
"수수로",
"커터",
"엠브리엘",
"클릭",
"우타게",
"재키",
"에단",
"애시드드롭",
"버블",
"퓨어스트림",
"파인콘",
"빈스토크",
"인디고",
"로베르타",
"",
"푸딩",
"체스트넛",
"토터",
"쿼츠",
"나소흑",
"아미야",
"소라",
"텍사스",
"프란카",
"리스캄",
"사일런스",
"지마",
"프틸롭시스",
"블루포이즌",
"플레임브링어",
"아스트젠",
"라플란드",
"스펙터",
"레드",
"프로방스",
"니어",
"",
"인드라",
"다그다",
"파이어워치",
"벌컨",
"나이트메어",
"스카이파이어",
"와파린",
"클리프하트",
"프라마닉스",
"이스티나",
"크루아상",
"플래티넘",
"카프카",
"맨티코어",
"안드레아나",
"메테오라이트",
"그라니",
"훔",
"새비지",
"에프이터",
"메이어",
"와이후",
"바이비크",
"샤마르",
"리드",
"위슬래시",
"아스테시아",
"브리즈",
"이그제큐터",
"에이어스카르페",
"아르모니",
"헤비레인",
"레이즈",
"스와이어",
"바이슨",
"글라우쿠스",
"시데로카",
"씬",
"아이리스",
"",
"츠키노기",
"비즈왁스",
"폴리닉",
"아오스타",
"실론",
"키아베",
"브로카",
"토디폰스",
"에이프릴",
"그레이스롯",
"베나",
"레온하르트",
"아스베스토스",
"세사",
"스노우상트",
"민트",
"엘리시움",
"투예",
"압생트",
"토미미",
"플린트",
"라 플루마",
"오로라",
"애쉬락",
"윈드플릿",
"위스퍼레인",
"허니베리",
"로빈",
"우요우",
"Blitz",
"Frost",
"Tachanka",
"",
"카니팔라트",
"멀베리",
"아카후유",
"블랙나이트",
"키라라",
"테킬라",
"",
"코로세럼",
"퀘르쿠스",
"파이어휘슬",
"와일드메인",
"칸타빌레",
"",
"튤립",
"라바 더 퍼거토리",
"크루스 더 킨 글린트",
"히비스커스 더 퓨리파이어",
"그레이 더 라이트닝베어러",
"",
"",
"쉐라",
"루나컵",
"",
"카제마루",
"퍼즐",
"나인컬러드 디어",
"샬렘",
"프로바이조",
"인포서",
"락락",
"에라토",
"하이디",
"체르니",
"미니멀리스트",
"하이모어",
"파프리카",
"지에윈",
"윈드차임스",
"",
"",
"",
"켈시",
"첸",
"블레이즈",
"엑시아",
"시즈",
"W",
"이프리트",
"호시구마",
"샤이닝",
"실버애쉬",
"나이팅게일",
"에이야퍄들라",
"헬라그",
"로사",
"사리아",
"노시스",
"모스티마",
"백파이프",
"아",
"마젤란",
"",
"팬텀",
"스카디",
"마운틴",
"안젤리나",
"쏜즈",
"피아메타",
"머드락",
"리",
"아르케토",
"슈바르츠",
"수르트",
"스즈란",
"사가",
"골든글로우",
"로즈몬티스",
"위디",
"유넥티스",
"플레임테일",
"블레미샤인",
"카넬리안",
"비질",
"파투스",
"미즈키",
"Ash",
"패신저",
"글래디아",
"사일라흐",
"팔라스",
"스카디 더 커럽팅 하트",
"첸 더 홀룽데이",
"니어 더 래디언트 나이트",
"",
"리드 더 플레임 섀도우",
"스펙터 디 언체인드",
"가비알 디 인빈서블",
"텍사스 디 오메르토사",
"",
"",
"",
"",
"",
"",
"케오베",
"니엔",
"시",
"링",
"총웨",
"아이린",
"",
"혼",
"루멘",
"에벤홀츠",
"도로시",
"파죰카",
"무에나",
"페넌스",
"스테인리스",
"린",
"치우바이",
"",
};
                            for (var i = 0; i < zh_cn.Length; i++)
                            {
                                if (name == zh_cn[i])
                                {
                                    if (ko_kr[i].IsNullOrEmpty())
                                    {
                                        break;
                                    }

                                    localized_name = ko_kr[i];
                                }
                            }

                            break;

                        case "zh-tw":
                            // string[] zh_tw = { };
                            // No operator list
                            break;

                        default: break;
                    }
                }

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
                string localized_name = name;

                if (!(ConfigurationHelper.GetValue(ConfigurationKeys.Localization, "zh-cn") == "zh-cn"))
                {
                    string[] zh_cn = {
"Lancet-2",
"Castle-3",
"THRM-EX",
"正义骑士号",
"泰拉大陆调查团",
"U-Official",
"Friston-3",
"12F",
"黑角",
"杜林",
"夜刀",
"巡林者",
"芙蓉",
"炎熔",
"米格鲁",
"芬",
"克洛丝",
"翎羽",
"玫兰莎",
"卡缇",
"史都华德",
"安德切尔",
"安赛尔",
"香草",
"梓兰",
"泡普卡",
"空爆",
"月见夜",
"斑点",
"远山",
"深海色",
"末药",
"白雪",
"流星",
"艾丝黛尔",
"杜宾",
"梅",
"猎蜂",
"夜烟",
"清道夫",
"蛇屠箱",
"桃金娘",
"断罪者",
"调香师",
"地灵",
"慕斯",
"嘉维尔",
"红云",
"霜叶",
"古米",
"讯使",
"角峰",
"杰西卡",
"暗索",
"砾",
"格雷伊",
"波登可",
"坚雷",
"芳汀",
"孑",
"阿消",
"缠丸",
"红豆",
"苏苏洛",
"刻刀",
"安比尔",
"卡达",
"宴",
"杰克",
"伊桑",
"酸糖",
"泡泡",
"清流",
"松果",
"豆苗",
"深靛",
"罗比菈塔",
"休谟斯",
"布丁",
"褐果",
"铅踝",
"石英",
"罗小黑",
"阿米娅",
"空",
"德克萨斯",
"芙兰卡",
"雷蛇",
"赫默",
"凛冬",
"白面鸮",
"蓝毒",
"炎客",
"星源",
"拉普兰德",
"幽灵鲨",
"红",
"普罗旺斯",
"临光",
"摩根",
"因陀罗",
"达格达",
"守林人",
"火神",
"夜魔",
"天火",
"华法琳",
"崖心",
"初雪",
"真理",
"可颂",
"白金",
"卡夫卡",
"狮蝎",
"安哲拉",
"陨星",
"格拉尼",
"吽",
"暴行",
"食铁兽",
"梅尔",
"槐琥",
"柏喙",
"巫恋",
"苇草",
"鞭刃",
"星极",
"微风",
"送葬人",
"断崖",
"和弦",
"暴雨",
"惊蛰",
"诗怀雅",
"拜松",
"格劳克斯",
"铸铁",
"稀音",
"爱丽丝",
"寒檀",
"月禾",
"蜜蜡",
"亚叶",
"奥斯塔",
"锡兰",
"贾维",
"布洛卡",
"熔泉",
"四月",
"灰喉",
"贝娜",
"莱恩哈特",
"石棉",
"慑砂",
"雪雉",
"薄绿",
"极境",
"图耶",
"苦艾",
"特米米",
"燧石",
"羽毛笔",
"极光",
"灰毫",
"掠风",
"絮雨",
"蜜莓",
"罗宾",
"乌有",
"闪击",
"霜华",
"战车",
"洋灰",
"雪绒",
"桑葚",
"赤冬",
"夜半",
"绮良",
"龙舌兰",
"青枳",
"蚀清",
"夏栎",
"火哨",
"野鬃",
"晓歌",
"隐现",
"郁金香",
"炎狱炎熔",
"寒芒克洛丝",
"濯尘芙蓉",
"承曦格雷伊",
"火龙S黑角",
"玫拉",
"耶拉",
"子月",
"空构",
"风丸",
"谜图",
"九色鹿",
"暮落",
"但书",
"见行者",
"洛洛",
"埃拉托",
"海蒂",
"车尔尼",
"至简",
"海沫",
"明椒",
"截云",
"铎铃",
"凛视",
"苍苔",
"冰酿",
"凯尔希",
"陈",
"煌",
"能天使",
"推进之王",
"W",
"伊芙利特",
"星熊",
"闪灵",
"银灰",
"夜莺",
"艾雅法拉",
"赫拉格",
"早露",
"塞雷娅",
"灵知",
"莫斯提马",
"风笛",
"阿",
"麦哲伦",
"缪尔赛思",
"傀影",
"斯卡蒂",
"山",
"安洁莉娜",
"棘刺",
"菲亚梅塔",
"泥岩",
"老鲤",
"空弦",
"黑",
"史尔特尔",
"铃兰",
"嵯峨",
"澄闪",
"迷迭香",
"温蒂",
"森蚺",
"焰尾",
"瑕光",
"卡涅利安",
"伺夜",
"远牙",
"水月",
"灰烬",
"异客",
"歌蕾蒂娅",
"琴柳",
"帕拉斯",
"浊心斯卡蒂",
"假日威龙陈",
"耀骑士临光",
"纯烬艾雅法拉",
"焰影苇草",
"归溟幽灵鲨",
"百炼嘉维尔",
"缄默德克萨斯",
"麒麟R夜刀",
"淬羽赫默",
"圣约送葬人",
"琳琅诗怀雅",
"涤火杰西卡",
"提丰",
"刻俄柏",
"年",
"夕",
"令",
"重岳",
"艾丽妮",
"霍尔海雅",
"号角",
"流明",
"黑键",
"多萝西",
"鸿雪",
"玛恩纳",
"斥罪",
"白铁",
"林",
"仇白",
"伊内丝",};

                    switch (ConfigurationHelper.GetValue(ConfigurationKeys.Localization, "zh-cn"))
                    {
                        case "en-us":
                            string[] en_us = {
"Lancet-2",
"Castle-3",
"THRM-EX",
"'Justice Knight'",
"Terra Research Commission",
"U-Official",
"Friston-3",
"12F",
"Noir Corne",
"Durin",
"Yato",
"Rangers",
"Hibiscus",
"Lava",
"Beagle",
"Fang",
"Kroos",
"Plume",
"Melantha",
"Cardigan",
"Steward",
"Adnachiel",
"Ansel",
"Vanilla",
"Orchid",
"Popukar",
"Catapult",
"Midnight",
"Spot",
"Gitano",
"Deepcolor",
"Myrrh",
"Shirayuki",
"Meteor",
"Estelle",
"Dobermann",
"May",
"Beehunter",
"Haze",
"Scavenger",
"Cuora",
"Myrtle",
"Conviction",
"Perfumer",
"Earthspirit",
"Mousse",
"Gavial",
"Vermeil",
"Frostleaf",
"Gummy",
"Courier",
"Matterhorn",
"Jessica",
"Rope",
"Gravel",
"Greyy",
"Podenco",
"Dur-nar",
"Arene",
"Jaye",
"Shaw",
"Matoimaru",
"Vigna",
"Sussurro",
"Cutter",
"Ambriel",
"Click",
"Utage",
"Jackie",
"Ethan",
"Aciddrop",
"Bubble",
"Purestream",
"Pinecone",
"Beanstalk",
"Indigo",
"Roberta",
"Humus",
"Pudding",
"Chestnut",
"Totter",
"Quartz",
"Luo Xiaohei",
"Amiya",
"Sora",
"Texas",
"Franka",
"Liskarm",
"Silence",
"Zima",
"Ptilopsis",
"Blue Poison",
"Flamebringer",
"Astgenne",
"Lappland",
"Specter",
"Projekt Red",
"Provence",
"Nearl",
"Morgan",
"Indra",
"Dagda",
"Firewatch",
"Vulcan",
"Nightmare",
"Skyfire",
"Warfarin",
"Cliffheart",
"Pramanix",
"Istina",
"Croissant",
"Platinum",
"Kafka",
"Manticore",
"Andreana",
"Meteorite",
"Grani",
"Hung",
"Savage",
"FEater",
"Mayer",
"Waai Fu",
"Bibeak",
"Shamare",
"Reed",
"Whislash",
"Astesia",
"Breeze",
"Executor",
"Ayerscarpe",
"Harmonie",
"Heavyrain",
"Leizi",
"Swire",
"Bison",
"Glaucus",
"Sideroca",
"Scene",
"Iris",
"Santalla",
"Tsukinogi",
"Beeswax",
"Folinic",
"Aosta",
"Ceylon",
"Chiave",
"Broca",
"Toddifons",
"April",
"GreyThroat",
"Bena",
"Leonhardt",
"Asbestos",
"Sesa",
"Snowsant",
"Mint",
"Elysium",
"Tuye",
"Absinthe",
"Tomimi",
"Flint",
"La Pluma",
"Aurora",
"Ashlock",
"Windflit",
"Whisperain",
"Honeyberry",
"Robin",
"Mr. Nothing",
"Blitz",
"Frost",
"Tachanka",
"Cement",
"Qanipalaat",
"Mulberry",
"Akafuyu",
"Blacknight",
"Kirara",
"Tequila",
"Poncirus",
"Corroserum",
"Quercus",
"Firewhistle",
"Wild Mane",
"Cantabile",
"Insider",
"Tulip",
"Lava the Purgatory",
"Kroos the Keen Glint",
"Hibiscus the Purifier",
"Greyy the Lightningbearer",
"Rathalos S Noir Corne",
"Melanite",
"Kjera",
"Lunacub",
"Spuria",
"Kazemaru",
"Puzzle",
"Nine-Colored Deer",
"Shalem",
"Proviso",
"Enforcer",
"Rockrock",
"Erato",
"Heidi",
"Czerny",
"Minimalist",
"Highmore",
"Paprika",
"Jieyun",
"Wind Chimes",
"Valarqvin",
"Bryophyta",
"Coldshot",
"Kal'tsit",
"Ch'en",
"Blaze",
"Exusiai",
"Siege",
"W",
"Ifrit",
"Hoshiguma",
"Shining",
"SilverAsh",
"Nightingale",
"Eyjafjalla",
"Hellagur",
"Rosa",
"Saria",
"Gnosis",
"Mostima",
"Bagpipe",
"Aak",
"Magallan",
"Muelsyse",
"Phantom",
"Skadi",
"Mountain",
"Angelina",
"Thorns",
"Fiammetta",
"Mudrock",
"Lee",
"Archetto",
"Schwarz",
"Surtr",
"Suzuran",
"Saga",
"Goldenglow",
"Rosmontis",
"Weedy",
"Eunectes",
"Flametail",
"Blemishine",
"Carnelian",
"Vigil",
"Fartooth",
"Mizuki",
"Ash",
"Passenger",
"Gladiia",
"Saileach",
"Pallas",
"Skadi the Corrupting Heart",
"Ch'en the Holungday",
"Nearl the Radiant Knight",
"Eyjafjalla the Hvít Aska",
"Reed the Flame Shadow",
"Specter the Unchained",
"Gavial the Invincible",
"Texas the Omertosa",
"Kirin R Yato",
"Silence the Paradigmatic",
"Executor the Ex Foedere",
"Swire the Elegant Wit",
"Jessica the Liberated",
"Typhon",
"Ceobe",
"Nian",
"Dusk",
"Ling",
"Chongyue",
"Irene",
"Ho'olheyak",
"Horn",
"Lumen",
"Ebenholz",
"Dorothy",
"Pozëmka",
"Młynar",
"Penance",
"Stainless",
"Lin",
"Qiubai",
"Ines",

};
                            for (var i = 0; i < zh_cn.Length; i++)
                            {
                                if (name == zh_cn[i])
                                {
                                    if (en_us[i].IsNullOrEmpty())
                                    {
                                        break;
                                    }

                                    localized_name = en_us[i];
                                }
                            }

                            break;

                        case "ja-jp":
                            string[] ja_jp = {"Lancet-2",
"Castle-3",
"THRM-EX",
"ジャスティスナイト",
"テラ大陸調査団",
"",
"",
"12F",
"ノイルホーン",
"ドゥリン",
"ヤトウ",
"レンジャー",
"ハイビスカス",
"ラヴァ",
"ビーグル",
"フェン",
"クルース",
"プリュム",
"メランサ",
"カーディ",
"スチュワード",
"アドナキエル",
"アンセル",
"バニラ",
"オーキッド",
"ポプカル",
"カタパルト",
"ミッドナイト",
"スポット",
"ギターノ",
"ディピカ",
"ミルラ",
"シラユキ",
"メテオ",
"エステル",
"ドーベルマン",
"メイ",
"ビーハンター",
"ヘイズ",
"スカベンジャー",
"クオーラ",
"テンニンカ",
"コンビクション",
"パフューマー",
"アーススピリット",
"ムース",
"ガヴィル",
"ヴァーミル",
"フロストリーフ",
"グム",
"クーリエ",
"マッターホルン",
"ジェシカ",
"ロープ",
"グラベル",
"グレイ",
"ポデンコ",
"ジュナー",
"アレーン",
"ジェイ",
"ショウ",
"マトイマル",
"ヴィグナ",
"ススーロ",
"カッター",
"アンブリエル",
"カシャ",
"ウタゲ",
"ジャッキー",
"イーサン",
"アシッドドロップ",
"バブル",
"セイリュウ",
"パインコーン",
"ビーンストーク",
"インディゴ",
"ロベルタ",
"",
"プリン",
"チェストナット",
"トター",
"クォーツ",
"羅小黒",
"アーミヤ",
"ソラ",
"テキサス",
"フランカ",
"リスカム",
"サイレンス",
"ズィマー",
"フィリオプシス",
"アズリウス",
"エンカク",
"アステジーニ",
"ラップランド",
"スペクター",
"レッド",
"プロヴァンス",
"ニアール",
"",
"インドラ",
"ダグザ",
"ファイヤーウォッチ",
"ヴァルカン",
"ナイトメア",
"スカイフレア",
"ワルファリン",
"クリフハート",
"プラマニクス",
"イースチナ",
"クロワッサン",
"プラチナ",
"カフカ",
"マンティコア",
"アンドレアナ",
"メテオリーテ",
"グラニ",
"ウン",
"サベージ",
"エフイーター",
"メイヤー",
"ワイフー",
"バイビーク",
"シャマレ",
"リード",
"ウィスラッシュ",
"アステシア",
"ブリーズ",
"イグゼキュター",
"エアースカーペ",
"ハーモニー",
"ヘビーレイン",
"レイズ",
"スワイヤー",
"バイソン",
"グラウコス",
"シデロカ",
"シーン",
"アイリス",
"",
"ツキノギ",
"ビーズワクス",
"フォリニック",
"アオスタ",
"セイロン",
"キアーベ",
"ブローカ",
"トギフォンス",
"エイプリル",
"グレースロート",
"ベナ",
"レオンハルト",
"アスベストス",
"シェーシャ",
"スノーズント",
"ミント",
"エリジウム",
"トゥイエ",
"アブサント",
"トミミ",
"フリント",
"ラ・プルマ",
"オーロラ",
"アッシュロック",
"ウインドフリット",
"ウィスパーレイン",
"ハニーベリー",
"ロビン",
"ウユウ",
"Blitz",
"Frost",
"Tachanka",
"",
"カニパラート",
"マルベリー",
"アカフユ",
"ブラックナイト",
"キララ",
"テキーラ",
"",
"コロセラム",
"クエルクス",
"ファイヤーホイッスル",
"ワイルドメイン",
"カンタービレ",
"",
"チューリップ",
"炎獄ラヴァ",
"寒芒クルース",
"濯塵ハイビスカス",
"承曦グレイ",
"レウスSノイルホーン",
"",
"イェラ",
"ルナカブ",
"",
"カゼマル",
"パズル",
"九色鹿",
"シャレム",
"プロヴァイゾ",
"エンフォーサー",
"ロックロック",
"エラト",
"ハイディ",
"ツェルニー",
"ミニマリスト",
"ハイモア",
"パプリカ",
"ジエユン",
"ウィンドチャイム",
"",
"",
"",
"ケルシー",
"チェン",
"ブレイズ",
"エクシア",
"シージ",
"W",
"イフリータ",
"ホシグマ",
"シャイニング",
"シルバーアッシュ",
"ナイチンゲール",
"エイヤフィヤトラ",
"ヘラグ",
"ロサ",
"サリア",
"ノーシス",
"モスティマ",
"バグパイプ",
"ア",
"マゼラン",
"",
"ファントム",
"スカジ",
"マウンテン",
"アンジェリーナ",
"ソーンズ",
"フィアメッタ",
"マドロック",
"リー",
"アルケット",
"シュヴァルツ",
"スルト",
"スズラン",
"サガ",
"ゴールデングロー",
"ロスモンティス",
"ウィーディ",
"ユーネクテス",
"フレイムテイル",
"ブレミシャイン",
"カーネリアン",
"ヴィジェル",
"ファートゥース",
"ミヅキ",
"Ash",
"パッセンジャー",
"グレイディーア",
"サイラッハ",
"パラス",
"濁心スカジ",
"遊龍チェン",
"耀騎士ニアール",
"",
"焔影リード",
"帰溟スペクター",
"百錬ガヴィル",
"血掟テキサス",
"キリンRヤトウ",
"",
"",
"",
"",
"",
"ケオベ",
"ニェン",
"シー",
"リィン",
"チョンユエ",
"アイリーニ",
"",
"ホルン",
"ルーメン",
"エーベンホルツ",
"ドロシー",
"パゼオンカ",
"ムリナール",
"ペナンス",
"ステインレス",
"リン",
"チューバイ",
"",
};
                            for (var i = 0; i < zh_cn.Length; i++)
                            {
                                if (name == zh_cn[i])
                                {
                                    if (ja_jp[i].IsNullOrEmpty())
                                    {
                                        break;
                                    }

                                    localized_name = ja_jp[i];
                                }
                            }

                            break;

                        case "ko-kr":
                            string[] ko_kr = {"Lancet-2",
"Castle-3",
"THRM-EX",
"저스티스 나이트",
"",
"",
"",
"12F",
"느와르 코르네",
"두린",
"야토",
"레인저",
"히비스커스",
"라바",
"비글",
"팽",
"크루스",
"플룸",
"멜란사",
"카디건",
"스튜어드",
"아드나키엘",
"안셀",
"바닐라",
"오키드",
"포푸카",
"캐터펄트",
"미드나이트",
"스팟",
"기타노",
"딥컬러",
"미르",
"시라유키",
"메테오",
"에스텔",
"도베르만",
"메이",
"비헌터",
"헤이즈",
"스캐빈저",
"쿠오라",
"머틀",
"컨빅션",
"퍼퓨머",
"어스스피릿",
"무스",
"가비알",
"버메일",
"프로스트리프",
"굼",
"쿠리어",
"마터호른",
"제시카",
"로프",
"그라벨",
"그레이",
"포덴코",
"듀나",
"아렌",
"제이",
"쇼",
"마토이마루",
"비그나",
"수수로",
"커터",
"엠브리엘",
"클릭",
"우타게",
"재키",
"에단",
"애시드드롭",
"버블",
"퓨어스트림",
"파인콘",
"빈스토크",
"인디고",
"로베르타",
"",
"푸딩",
"체스트넛",
"토터",
"쿼츠",
"나소흑",
"아미야",
"소라",
"텍사스",
"프란카",
"리스캄",
"사일런스",
"지마",
"프틸롭시스",
"블루포이즌",
"플레임브링어",
"아스트젠",
"라플란드",
"스펙터",
"레드",
"프로방스",
"니어",
"",
"인드라",
"다그다",
"파이어워치",
"벌컨",
"나이트메어",
"스카이파이어",
"와파린",
"클리프하트",
"프라마닉스",
"이스티나",
"크루아상",
"플래티넘",
"카프카",
"맨티코어",
"안드레아나",
"메테오라이트",
"그라니",
"훔",
"새비지",
"에프이터",
"메이어",
"와이후",
"바이비크",
"샤마르",
"리드",
"위슬래시",
"아스테시아",
"브리즈",
"이그제큐터",
"에이어스카르페",
"아르모니",
"헤비레인",
"레이즈",
"스와이어",
"바이슨",
"글라우쿠스",
"시데로카",
"씬",
"아이리스",
"",
"츠키노기",
"비즈왁스",
"폴리닉",
"아오스타",
"실론",
"키아베",
"브로카",
"토디폰스",
"에이프릴",
"그레이스롯",
"베나",
"레온하르트",
"아스베스토스",
"세사",
"스노우상트",
"민트",
"엘리시움",
"투예",
"압생트",
"토미미",
"플린트",
"라 플루마",
"오로라",
"애쉬락",
"윈드플릿",
"위스퍼레인",
"허니베리",
"로빈",
"우요우",
"Blitz",
"Frost",
"Tachanka",
"",
"카니팔라트",
"멀베리",
"아카후유",
"블랙나이트",
"키라라",
"테킬라",
"",
"코로세럼",
"퀘르쿠스",
"파이어휘슬",
"와일드메인",
"칸타빌레",
"",
"튤립",
"라바 더 퍼거토리",
"크루스 더 킨 글린트",
"히비스커스 더 퓨리파이어",
"그레이 더 라이트닝베어러",
"",
"",
"쉐라",
"루나컵",
"",
"카제마루",
"퍼즐",
"나인컬러드 디어",
"샬렘",
"프로바이조",
"인포서",
"락락",
"에라토",
"하이디",
"체르니",
"미니멀리스트",
"하이모어",
"파프리카",
"지에윈",
"윈드차임스",
"",
"",
"",
"켈시",
"첸",
"블레이즈",
"엑시아",
"시즈",
"W",
"이프리트",
"호시구마",
"샤이닝",
"실버애쉬",
"나이팅게일",
"에이야퍄들라",
"헬라그",
"로사",
"사리아",
"노시스",
"모스티마",
"백파이프",
"아",
"마젤란",
"",
"팬텀",
"스카디",
"마운틴",
"안젤리나",
"쏜즈",
"피아메타",
"머드락",
"리",
"아르케토",
"슈바르츠",
"수르트",
"스즈란",
"사가",
"골든글로우",
"로즈몬티스",
"위디",
"유넥티스",
"플레임테일",
"블레미샤인",
"카넬리안",
"비질",
"파투스",
"미즈키",
"Ash",
"패신저",
"글래디아",
"사일라흐",
"팔라스",
"스카디 더 커럽팅 하트",
"첸 더 홀룽데이",
"니어 더 래디언트 나이트",
"",
"리드 더 플레임 섀도우",
"스펙터 디 언체인드",
"가비알 디 인빈서블",
"텍사스 디 오메르토사",
"",
"",
"",
"",
"",
"",
"케오베",
"니엔",
"시",
"링",
"총웨",
"아이린",
"",
"혼",
"루멘",
"에벤홀츠",
"도로시",
"파죰카",
"무에나",
"페넌스",
"스테인리스",
"린",
"치우바이",
"",
};
                            for (var i = 0; i < zh_cn.Length; i++)
                            {
                                if (name == zh_cn[i])
                                {
                                    if (ko_kr[i].IsNullOrEmpty())
                                    {
                                        break;
                                    }

                                    localized_name = ko_kr[i];
                                }
                            }

                            break;

                        case "zh-tw":
                            // string[] zh_tw = { };
                            // No operator list
                            break;

                        default: break;
                    }
                }

                operHaveNames += PadRightEx(localized_name, 12) + "\t";
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
