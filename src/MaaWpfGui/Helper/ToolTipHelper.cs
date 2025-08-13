// <copyright file="ToolTipHelper.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Documents;
using System.Windows.Media;
using MaaWpfGui.Extensions;

namespace MaaWpfGui.Helper
{
    public static class ToolTipHelper
    {
        private static ToolTip CreateTooltip(this object content, PlacementMode? placementMode = null)
        {
            var toolTip = new ToolTip
            {
                Content = content,
            };

            if (placementMode.HasValue)
            {
                toolTip.Placement = placementMode.Value;
            }

            if (placementMode == PlacementMode.Center)
            {
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
            }

            return toolTip;
        }

        /// <summary>
        /// 通过字符串创建 Tooltip。
        /// </summary>
        /// <param name="text">字符串</param>
        /// <param name="placementMode">位置模式</param>
        /// <param name="maxWidth">ToolTip 最大宽度</param>
        /// <returns>ToolTip</returns>
        public static ToolTip CreateTooltip(this string text, PlacementMode? placementMode = null, int maxWidth = 750)
        {
            var tb = new TextBlock
            {
                Text = text,
                TextWrapping = TextWrapping.Wrap,
                Margin = new(4),
                MaxWidth = maxWidth,
            };

            return tb.CreateTooltip(placementMode);
        }

        public static ToolTip CreateTooltip(this IEnumerable<Inline> inlines, PlacementMode? placementMode = null, int maxWidth = 750)
        {
            var tb = new TextBlock
            {
                TextWrapping = TextWrapping.Wrap,
                Margin = new(4),
                MaxWidth = maxWidth,
            };

            foreach (var inline in inlines)
            {
                tb.Inlines.Add(CloneInline(inline));
            }

            return tb.CreateTooltip(placementMode);

            Inline CloneInline(Inline inline)
            {
                switch (inline)
                {
                    case Run run:
                        var newRun = new Run(run.Text)
                        {
                            FontWeight = run.FontWeight,
                            FontStyle = run.FontStyle,
                        };

                        if (run.Tag is string resourceKey)
                        {
                            newRun.SetResourceReference(TextElement.ForegroundProperty, resourceKey);
                            newRun.Tag = resourceKey;
                        }
                        else
                        {
                            newRun.Foreground = run.Foreground;
                        }

                        return newRun;
                    case LineBreak:
                        return new LineBreak();
                    default:
                        return new Run();
                }
            }
        }

        #region 创建日志 Tooltip 的辅助方法

        /// <summary>
        /// 掉落识别物品的 Tooltip 创建方法。
        /// </summary>
        /// <param name="drops">掉落物列表</param>
        /// <returns>ToolTip</returns>
        public static ToolTip CreateMaterialDropTooltip(this IEnumerable<(string ItemId, string ItemName, int Total, int Add)> drops)
        {
            var row = new WrapPanel
            {
                Orientation = Orientation.Horizontal,
                HorizontalAlignment = HorizontalAlignment.Center,
                Margin = new(4),
                ItemWidth = 64,
                MaxWidth = (64 * 5) + (4 * 10),
            };

            foreach (var (itemId, _, total, add) in drops)
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
                    Text = total.FormatNumber(false),
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
                    textAdd.Inlines.Add(new Run($"(+{add.FormatNumber(false)})"));
                    itemStack.Children.Add(textAdd);
                }

                row.Children.Add(itemStack);
            }

            return row.CreateTooltip(PlacementMode.Center);
        }

        /// <summary>
        /// 截图测速 Tooltip 创建方法。
        /// </summary>
        /// <param name="methods">测速列表</param>
        /// <returns>ToolTip</returns>
        public static ToolTip? CreateScreencapTooltip(this IEnumerable<(string Method, string Cost)>? methods)
        {
            if (methods == null)
            {
                return null;
            }

            var panel = new StackPanel
            {
                Orientation = Orientation.Vertical,
                Margin = new(4),
            };

            foreach (var (method, cost) in methods)
            {
                var grid = new Grid();
                grid.ColumnDefinitions.Add(new() { Width = new(1, GridUnitType.Star) });
                grid.ColumnDefinitions.Add(new() { Width = new(1, GridUnitType.Auto) });

                var methodText = new TextBlock
                {
                    Text = method,
                    FontSize = 12,
                    Margin = new(2, 1, 6, 1),
                    HorizontalAlignment = HorizontalAlignment.Left,
                };
                Grid.SetColumn(methodText, 0);

                var costText = new TextBlock
                {
                    Text = cost + " ms",
                    FontSize = 12,
                    Margin = new(2, 1, 2, 1),
                    HorizontalAlignment = HorizontalAlignment.Right,
                };
                Grid.SetColumn(costText, 1);

                grid.Children.Add(methodText);
                grid.Children.Add(costText);
                panel.Children.Add(grid);
            }

            return panel.Children.Count == 0
                ? null
                : panel.CreateTooltip(PlacementMode.Center);
        }

        #endregion
    }
}
