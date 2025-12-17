// <copyright file="MiniGameRecastUserControl.xaml.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Windows.Controls;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;

namespace MaaWpfGui.Views.UserControl.TaskQueue
{
    /// <summary>
    /// User control for configuring roguelike coppers recast conditions.
    /// MiniGameRecastUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class MiniGameRecastUserControl : System.Windows.Controls.UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MiniGameRecastUserControl"/> class.
        /// </summary>
        public MiniGameRecastUserControl()
        {
            DataContext = MiniGameRecastUserControlModel.Instance;
            InitializeComponent();
        }

        /// <summary>
        /// Handles the CellEditEnding event to trigger condition updates.
        /// </summary>
        private void DataGrid_CellEditEnding(object sender, DataGridCellEditEndingEventArgs e)
        {
            MiniGameRecastUserControlModel.Instance.UpdateRecastCondition();
        }
    }
}
