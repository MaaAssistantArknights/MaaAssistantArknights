// <copyright file="WeeklyScheduleItem.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels;

public class WeeklyScheduleItem(DayOfWeek dayOfWeek) : PropertyChangedBase
{
    public string Display => LocalizationHelper.CustomCultureInfo.DateTimeFormat.GetDayName(DayOfWeek);

    /// <summary>
    /// 语言切换后通知 Display 回读新文化的星期名，Value（勾选状态）保持不变。
    /// </summary>
    public void RefreshLocalization() => NotifyOfPropertyChange(nameof(Display));

    public DayOfWeek DayOfWeek { get; } = dayOfWeek;

    public bool Value { get => field; set => SetAndNotify(ref field, value); } = true;
}
