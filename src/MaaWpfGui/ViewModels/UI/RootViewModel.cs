// <copyright file="RootViewModel.cs" company="MaaAssistantArknights">
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

using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Main;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The root view model.
    /// </summary>
    public class RootViewModel : Conductor<Screen>.Collection.OneActive
    {
        private readonly IWindowManager _windowManager;

        private readonly AsstProxy _asstProxy;
        private readonly TaskQueueViewModel _taskQueueViewModel;
        private readonly RecruitViewModel _recruitViewModel;
        private readonly SettingsViewModel _settingsViewModel;
        private readonly CopilotViewModel _copilotViewModel;
        private readonly DepotViewModel _depotViewModel;
        private readonly VersionUpdateViewModel _versionUpdateViewModel;

        /// <summary>
        /// Initializes a new instance of the <see cref="RootViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public RootViewModel(IContainer container)
        {
            _windowManager = container.Get<Helper.WindowManager>();

            _asstProxy = container.Get<AsstProxy>();
            _taskQueueViewModel = container.Get<TaskQueueViewModel>();
            _recruitViewModel = container.Get<RecruitViewModel>();
            _settingsViewModel = container.Get<SettingsViewModel>();
            _copilotViewModel = container.Get<CopilotViewModel>();
            _depotViewModel = container.Get<DepotViewModel>();
            _versionUpdateViewModel = container.Get<VersionUpdateViewModel>();
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
            await Task.Run(_asstProxy.Init);
        }

        private void InitViewModels()
        {
            Items.Add(_taskQueueViewModel);
            Items.Add(_copilotViewModel);
            Items.Add(_recruitViewModel);
            Items.Add(_depotViewModel);
            Items.Add(_settingsViewModel);

            _settingsViewModel.UpdateWindowTitle(); // 在标题栏上显示模拟器和IP端口 必须在 Items.Add(settings)之后执行。
            ActiveItem = _taskQueueViewModel;
        }

        private bool CheckAndUpdateNow()
        {
            return _versionUpdateViewModel.CheckAndUpdateNow();
        }

        private async void ShowUpdateOrDownload()
        {
            if (_versionUpdateViewModel.IsFirstBootAfterUpdate)
            {
                _versionUpdateViewModel.IsFirstBootAfterUpdate = false;
                _windowManager.ShowWindow(_versionUpdateViewModel);
            }
            else
            {
                var ret = await _versionUpdateViewModel.CheckAndDownloadUpdate();

                if (ret == VersionUpdateViewModel.CheckUpdateRetT.OK)
                {
                    _versionUpdateViewModel.AskToRestart();
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

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();

            // TrayIcon应该在显示rootViewModel之后再加载
            Bootstrapper.SetTrayIconInSettingsViewModel(_settingsViewModel);
        }

        /// <inheritdoc/>
        protected override void OnClose()
        {
            Application.Current.Shutdown();
        }
    }
}
