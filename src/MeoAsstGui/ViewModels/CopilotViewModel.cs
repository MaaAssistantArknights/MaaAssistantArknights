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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Notification.Wpf.Constants;
using Notification.Wpf.Controls;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class CopilotViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        public CopilotViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "自动战斗";
        }

        private string _filename;

        public string Filename
        {
            get => _filename;
            set => SetAndNotify(ref _filename, value);
        }

        public void SelectFile()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();

            dialog.Filter = "作业文件|*.json";

            if (dialog.ShowDialog() == true)
            {
                Filename = dialog.FileName;
            }
        }

        private bool _form = true;

        public bool Form
        {
            get => _form;
            set => SetAndNotify(ref _form, value);
        }

        private bool _catched = false;

        public async void Start()
        {
            var asstProxy = _container.Get<AsstProxy>();
            if (!_catched)
            {
                var task = Task.Run(() =>
                {
                    return asstProxy.AsstConnect();
                });
                _catched = await task;
            }
            if (!_catched)
            {
                return;
            }
            if (Filename.Length == 0 || !File.Exists(Filename))
            {
                return;
            }

            JObject data;

            try
            {
                string jsonStr = File.ReadAllText(Filename);

                // 文件存在但为空，会读出来一个null，感觉c#这库有bug，如果是null 就赋值一个空JObject
                data = (JObject)JsonConvert.DeserializeObject(jsonStr) ?? new JObject();
            }
            catch (Exception)
            {
                return;
            }

            asstProxy.AsstStartCopilot(data["stage_name"].ToString(), Filename, Form);
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
        }
    }
}
