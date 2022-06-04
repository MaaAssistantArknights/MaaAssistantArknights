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
using System.Windows.Controls;

namespace MeoAsstGui
{
    /// <summary>
    /// ParamSettingUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class FightSettingsUserControl : UserControl
    {
        public FightSettingsUserControl()
        {
            InitializeComponent();
            InitDrops();
            ComboBoxCtr.ItemsSource = Drops;
        }

        public ObservableCollection<CombData> Drops { get; set; } = new ObservableCollection<CombData>();

        private void InitDrops()
        {
            for (int i = 0; i < 100; i++)
            {
                Drops.Add(new CombData { Display = i.ToString(), Value = (i * 2).ToString() });
            }
        }

        public ObservableCollection<CombData> DropsList { get; set; } = new ObservableCollection<CombData>();

        private void ComboBoxCtr_TextChanged(object sender, TextChangedEventArgs e)
        {
            string str = ComboBoxCtr.Text.ToString();
            DropsList.Clear();

            if (string.IsNullOrEmpty(str))
            {
                ComboBoxCtr.ItemsSource = Drops;
                return;
            }

            foreach (CombData drop in Drops)
            {
                var enumStr = drop.Display;
                if (enumStr.Contains(str))
                    DropsList.Add(drop);
            }

            if (DropsList.Count > 0)
            {
                ComboBoxCtr.ItemsSource = DropsList;
                ComboBoxCtr.IsDropDownOpen = true;
            }
        }
    }
}
