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

using System.Threading.Tasks;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class RootViewModel : Conductor<Screen>.Collection.OneActive
    {
        private IContainer _container;
        private IWindowManager _windowManager;

        public RootViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
        }

        protected override void OnViewLoaded()
        {
            CheckAndUpdateNow();
            InitProxy();
            InitViewModels();
            ShowUpdateOrDownload();
        }

        private async void InitProxy()
        {
            var task = Task.Run(() =>
            {
                var p = _container.Get<AsstProxy>();
                p.Init();
            });

            await task;
        }

        private void InitViewModels()
        {
            var tvm = _container.Get<TaskQueueViewModel>();
            var rvm = _container.Get<RecruitViewModel>();
            //var ivm = _container.Get<InfrastViewModel>();
            var svm = _container.Get<SettingsViewModel>();

            Items.Add(tvm);
            Items.Add(rvm);
            //Items.Add(ivm);
            Items.Add(svm);
            ActiveItem = tvm;
        }

        private bool CheckAndUpdateNow()
        {
            var vuvm = _container.Get<VersionUpdateViewModel>();
            return vuvm.CheckAndUpdateNow();
        }

        private async void ShowUpdateOrDownload()
        {
            var vuvm = _container.Get<VersionUpdateViewModel>();

            if (vuvm.IsFirstBootAfterUpdate)
            {
                _windowManager.ShowWindow(vuvm);
            }
            else
            {
                var task = Task.Run(() =>
                {
                    if (!vuvm.CheckAndDownloadUpdate())
                    {
                        vuvm.ResourceOTA();
                    }
                });

                await task;
            }
        }
    }
}
