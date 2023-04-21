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
        private readonly AsstProxy _asstProxy;
        private readonly TaskQueueViewModel _taskQueueViewModel;
        private readonly RecognizerViewModel _recognizerViewModel;
        private readonly SettingsViewModel _settingsViewModel;
        private readonly CopilotViewModel _copilotViewModel;
        private readonly VersionUpdateViewModel _versionUpdateViewModel;

        /// <summary>
        /// Initializes a new instance of the <see cref="RootViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public RootViewModel(IContainer container)
        {
            _asstProxy = container.Get<AsstProxy>();
            _taskQueueViewModel = container.Get<TaskQueueViewModel>();
            _recognizerViewModel = container.Get<RecognizerViewModel>();
            _settingsViewModel = container.Get<SettingsViewModel>();
            _copilotViewModel = container.Get<CopilotViewModel>();
            _versionUpdateViewModel = container.Get<VersionUpdateViewModel>();
        }

        /// <inheritdoc/>
        protected override void OnViewLoaded()
        {
            CheckAndUpdateNow();
            InitViewModels();
            InitProxy();
        }

        private async void InitProxy()
        {
            await Task.Run(_asstProxy.Init);
        }

        private void InitViewModels()
        {
            Items.Add(_taskQueueViewModel);
            Items.Add(_copilotViewModel);
            Items.Add(_recognizerViewModel);
            Items.Add(_settingsViewModel);

            _settingsViewModel.UpdateWindowTitle(); // 在标题栏上显示模拟器和IP端口 必须在 Items.Add(settings)之后执行。
            ActiveItem = _taskQueueViewModel;
        }

        private bool CheckAndUpdateNow()
        {
            return _versionUpdateViewModel.CheckAndUpdateNow();
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
        }

        /// <inheritdoc/>
        protected override void OnClose()
        {
            Application.Current.Shutdown();
        }
    }
}
