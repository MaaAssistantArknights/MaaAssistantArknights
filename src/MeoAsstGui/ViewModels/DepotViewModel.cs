// <copyright file="DepotViewModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    /// <summary>
    /// The view model of recruit.
    /// </summary>
    public class DepotViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        /// <summary>
        /// Initializes a new instance of the <see cref="DepotViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        /// <param name="windowManager">The window manager.</param>
        public DepotViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = Localization.GetString("DepotRecognition");
        }

        private string _depotInfo = Localization.GetString("DepotRecognitionTip");

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
        public bool Parse(JObject details)
        {
            string result = string.Empty;
            int count = 0;
            foreach (var item in details["arkplanner"]["object"]["items"].Cast<JObject>())
            {
                result += (string)item["name"] + " : " + (int)item["have"] + "\t";
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
                DepotInfo = Localization.GetString("IdentificationCompleted") + "\n" + Localization.GetString("DepotRecognitionTip");
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

        /// <summary>
        /// Export depot info to ArkPlanner.
        /// </summary>
        public void ExportToArkplanner()
        {
            Clipboard.SetDataObject(ArkPlannerResult);
            DepotInfo = Localization.GetString("CopiedToClipboard");
        }

        /// <summary>
        /// Export depot info to Lolicon.
        /// </summary>
        public void ExportToLolicon()
        {
            Clipboard.SetDataObject(LoliconResult);
            DepotInfo = Localization.GetString("CopiedToClipboard");
        }

        private void Clear()
        {
            DepotResult = string.Empty;
            ArkPlannerResult = string.Empty;
            LoliconResult = string.Empty;
            Done = false;
        }

        /// <summary>
        /// Starts depot recognition.
        /// </summary>
        public async void Start()
        {
            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = string.Empty;
            DepotInfo = Localization.GetString("ConnectingToEmulator");
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg);
            });
            bool caught = await task;
            if (!caught)
            {
                DepotInfo = errMsg;
                return;
            }

            DepotInfo = Localization.GetString("Identifying");
            Clear();

            asstProxy.AsstStartDepot();
        }
    }
}
