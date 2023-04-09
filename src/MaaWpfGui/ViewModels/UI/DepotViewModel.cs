// <copyright file="DepotViewModel.cs" company="MaaAssistantArknights">
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
    /// <summary>
    /// The view model of recruit.
    /// </summary>
    public class DepotViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        private AsstProxy _asstProxy;

        /// <summary>
        /// Initializes a new instance of the <see cref="DepotViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        /// <param name="windowManager">The window manager.</param>
        public DepotViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = LocalizationHelper.GetString("DepotRecognition");
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
            _asstProxy = _container.Get<AsstProxy>();
        }

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
                DepotInfo = LocalizationHelper.GetString("IdentificationCompleted") + "\n" + LocalizationHelper.GetString("DepotRecognitionTip");
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
            string errMsg = string.Empty;
            DepotInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => _asstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                DepotInfo = errMsg;
                return;
            }

            DepotInfo = LocalizationHelper.GetString("Identifying");
            Clear();

            _asstProxy.AsstStartDepot();
        }
    }
}
