// <copyright file="RoguelikeSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Reflection;
using System.Windows.Data;

namespace MaaWpfGui.Views.UserControl
{
    /// <summary>
    /// RoguelikeSettingsUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class RoguelikeSettingsUserControl : System.Windows.Controls.UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RoguelikeSettingsUserControl"/> class.
        /// </summary>
        public RoguelikeSettingsUserControl()
        {
            InitializeComponent();
        }

        private static readonly MethodInfo _setText = typeof(HandyControl.Controls.NumericUpDown).GetMethod("SetText", BindingFlags.NonPublic | BindingFlags.Instance);

        private static readonly object[] _paras = { true };

        private void NumericUpDown_ValueChanged(object sender, HandyControl.Data.FunctionEventArgs<double> e)
        {
            _setText?.Invoke(sender, _paras);
        }
    }


    public class InvestmentButtonCheckedConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            string isEnabled = System.Convert.ToString(values[0]);
            string RoguelikeMode = System.Convert.ToString(values[1]);

            if (RoguelikeMode == "1" || RoguelikeMode == "4")
                return true;

            if (isEnabled == "True")
                return true;
                
            return false;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            bool isEnabled = (bool)value;
            return new object[] { isEnabled, isEnabled };
        }
    }

}
