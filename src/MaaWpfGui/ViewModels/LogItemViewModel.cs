// <copyright file="LogItemViewModel.cs" company="MaaAssistantArknights">
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

#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Documents;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    /// <summary>
    /// The view model of log item.
    /// </summary>
    public class LogItemViewModel : PropertyChangedBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogItemViewModel"/> class.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        /// <param name="dateFormat">The Date format string</param>
        /// <param name="showTime">The showtime bool.</param>
        /// <param name="toolTip">The toolTip</param>
        public LogItemViewModel(string content, string color = UiLogColor.Message, string weight = "Regular", string dateFormat = "", bool showTime = true, ToolTip? toolTip = null)
        {
            if (string.IsNullOrEmpty(dateFormat))
            {
                dateFormat = SettingsViewModel.GuiSettings.LogItemDateFormatString;
            }

            _time = DateTime.Now.ToString(dateFormat);
            _content = content;
            _color = color;
            _weight = weight;
            _showTime = showTime;
            _toolTip = toolTip;
        }

        private string _time;

        /// <summary>
        /// Gets or sets the time.
        /// </summary>
        public string Time
        {
            get => _time;
            set => SetAndNotify(ref _time, value);
        }

        private bool _showTime;

        public bool ShowTime
        {
            get => _showTime;
            set => SetAndNotify(ref _showTime, value);
        }

        private string _content;

        /// <summary>
        /// Gets or sets the content.
        /// </summary>
        public string Content
        {
            get => _content;
            set => SetAndNotify(ref _content, value);
        }

        private string _color;

        /// <summary>
        /// Gets or sets the font color.
        /// </summary>
        public string Color
        {
            get => _color;
            set => SetAndNotify(ref _color, value);
        }

        private string _weight;

        /// <summary>
        /// Gets or sets the font weight.
        /// </summary>
        public string Weight
        {
            get => _weight;
            set => SetAndNotify(ref _weight, value);
        }

        private ToolTip? _toolTip;

        /// <summary>
        /// Gets or sets the toolTip.
        /// </summary>
        public ToolTip? ToolTip
        {
            get => _toolTip;
            set => SetAndNotify(ref _toolTip, value);
        }

        #region 创建日志 Tooltip 的辅助方法

        private static ToolTip CreateTooltip(object content)
        {
            var toolTip = new ToolTip
            {
                Content = content,
                Placement = PlacementMode.Center,
            };
            toolTip.Opened += (_, _) =>
            {
                if (toolTip.PlacementTarget is FrameworkElement target)
                {
                    double offset = -(target.ActualHeight / 2) - (toolTip.ActualHeight / 2);
                    toolTip.VerticalOffset = offset;
                }
                else
                {
                    toolTip.VerticalOffset = -60;
                }
            };
            return toolTip;
        }

        public static ToolTip CreateMaterialDropTooltip(IEnumerable<(string ItemId, int Total, int Add)> drops)
        {
            var row = new WrapPanel
            {
                Orientation = Orientation.Horizontal,
                HorizontalAlignment = HorizontalAlignment.Center,
                Margin = new(4),
                ItemWidth = 64,
                MaxWidth = (64 * 5) + (4 * 10),
            };

            foreach (var (itemId, total, add) in drops
                         .OrderByDescending(x => x.Add)
                         .ThenByDescending(x => x.Total))
            {
                var image = new Image
                {
                    Source = ItemListHelper.GetItemImage(itemId),
                    Width = 32,
                    Height = 32,
                    Margin = new(2),
                    HorizontalAlignment = HorizontalAlignment.Center,
                };

                var text = new TextBlock
                {
                    Text = FormatNumber(total),
                    FontSize = 12,
                    FontWeight = FontWeights.Bold,
                    TextAlignment = TextAlignment.Center,
                    TextWrapping = TextWrapping.NoWrap,
                    Margin = new(2, 0, 2, 2),
                    HorizontalAlignment = HorizontalAlignment.Center,
                    MaxWidth = 64,
                };

                var itemStack = new StackPanel
                {
                    Orientation = Orientation.Vertical,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    Margin = new(4, 0, 4, 0),
                };
                itemStack.Children.Add(image);
                itemStack.Children.Add(text);

                if (add > 0)
                {
                    var textAdd = new TextBlock
                    {
                        FontSize = 10,
                        TextAlignment = TextAlignment.Center,
                        TextWrapping = TextWrapping.NoWrap,
                        Margin = new(2, 0, 2, 2),
                        HorizontalAlignment = HorizontalAlignment.Center,
                        MaxWidth = 64,
                    };
                    textAdd.Inlines.Add(new Run($"(+{FormatNumber(add)})"));
                    itemStack.Children.Add(textAdd);
                }

                row.Children.Add(itemStack);
                continue;

                static string FormatNumber(int n) => n switch
                {
                    >= 1_000_000_000 => $"{n / 1_000_000_000.0:#.#}B",
                    >= 1_000_000 => $"{n / 1_000_000.0:#.#}M",
                    >= 1_000 => $"{n / 1_000.0:#.#}k",
                    _ => $"{n}",
                };
            }

            return CreateTooltip(row);
        }

        #endregion
    }
}
