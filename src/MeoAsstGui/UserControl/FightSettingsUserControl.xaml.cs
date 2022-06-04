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
            BindingEnumData();
            ComboBoxCtr.ItemsSource = DropsList;
        }

        public ObservableCollection<Drops> DropsList { get; set; } = new ObservableCollection<Drops>();

        private void BindingEnumData()
        {
            foreach (Drops drop in Enum.GetValues(typeof(Drops)))
            {
                DropsList.Add(drop);
            }
        }

        public enum Drops
        {
            固源岩 = 0,
            晶体元件 = 1,
            全新装置 = 2,
            固源岩组 = 3
        }

        private void ComboBoxCtr_TextChanged(object sender, TextChangedEventArgs e)
        {
            {
                string str = ComboBoxCtr.Text.ToString();
                ComboBoxCtr.IsDropDownOpen = false;
                DropsList.Clear();

                if (string.IsNullOrEmpty(str))
                {
                    BindingEnumData();
                    return;
                }

                foreach (Drops drop in Enum.GetValues(typeof(Drops)))
                {
                    var enumStr = drop.ToString();
                    if (enumStr.Contains(str))
                        DropsList.Add(drop);
                }

                if (DropsList.Count > 0)
                {
                    ComboBoxCtr.ItemsSource = DropsList;
                    ComboBoxCtr.IsDropDownOpen = true;
                }
                else
                {
                    BindingEnumData();
                }
            }
        }
    }
}
