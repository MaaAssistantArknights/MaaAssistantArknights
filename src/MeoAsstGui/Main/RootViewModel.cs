// <copyright file="RootViewModel.cs" company="MaaAssistantArknights">
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

using System.Threading.Tasks;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    /// <summary>
    /// The root view model.
    /// </summary>
    public class RootViewModel : Conductor<Screen>.Collection.OneActive
    {
        private readonly IContainer _container;
        private readonly IWindowManager _windowManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="RootViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        /// <param name="windowManager">The window manager.</param>
        public RootViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
        }

        /// <inheritdoc/>
        protected override void OnViewLoaded()
        {
            CheckAndUpdateNow();
            InitViewModels();
            InitProxy();
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
            var farming = _container.Get<TaskQueueViewModel>();
            var recruit = _container.Get<RecruitViewModel>();
            var settings = _container.Get<SettingsViewModel>();
            var copilot = _container.Get<CopilotViewModel>();
            var depot = _container.Get<DepotViewModel>();

            Items.Add(farming);
            Items.Add(copilot);
            Items.Add(recruit);
            Items.Add(depot);
            Items.Add(settings);

            settings.UpdateWindowTitle(); // 在标题栏上显示模拟器和IP端口 必须在 Items.Add(settings)之后执行。
            ActiveItem = farming;
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
                    return vuvm.CheckAndDownloadUpdate();
                });

                var ret = await task;
                if (ret == VersionUpdateViewModel.CheckUpdateRetT.OK)
                {
                    vuvm.AskToRestart();
                }
            }
        }

        private string _windowTitle = "MAA";

        /// <summary>
        /// Gets or sets the window title.
        /// </summary>
        public string WindowTitle
        {
            get => _windowTitle;
            set => SetAndNotify(ref _windowTitle, value);
        }

        /// <inheritdoc/>
        protected override void OnClose()
        {
            System.Windows.Application.Current.Shutdown();
        }
    }
}
