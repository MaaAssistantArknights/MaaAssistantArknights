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
using MaaWpfGui.Helper;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The root view model.
    /// </summary>
    public class RootViewModel : Conductor<Screen>.Collection.OneActive
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RootViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public RootViewModel(IContainer container)
        {
            Instances.Instantiate(container);
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
            await Task.Run(Instances.AsstProxy.Init);
        }

        private void InitViewModels()
        {
            Items.Add(Instances.TaskQueueViewModel);
            Items.Add(Instances.CopilotViewModel);
            Items.Add(Instances.RecognizerViewModel);
            Items.Add(Instances.SettingsViewModel);

            Instances.SettingsViewModel.UpdateWindowTitle(); // 在标题栏上显示模拟器和IP端口 必须在 Items.Add(settings)之后执行。
            ActiveItem = Instances.TaskQueueViewModel;
        }

        private bool CheckAndUpdateNow()
        {
            return Instances.VersionUpdateViewModel.CheckAndUpdateNow();
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
