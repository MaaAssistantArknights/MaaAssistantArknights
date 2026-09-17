// <copyright file="CardLogHelper.cs" company="MaaAssistantArknights">
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
using System.Buffers;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.ViewModels.UI;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 卡片样式日志（<see cref="LogCardItemViewModel"/> 集合）的公共逻辑：
/// 卡片创建/合并、缩略图抓取与数量裁剪，以及日志分区分隔符。信息流与自动战斗共用，
/// 保证两处日志的卡片结构（分块、时间列、截图）完全一致。
/// </summary>
public static class CardLogHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(CardLogHelper));

    private static readonly Random _logRandom = new();

    private static readonly SemaphoreSlim _logThumbnailSemaphore = new(1, 1);

    private const int LogThumbnailWidth = 640;
    private const int LogThumbnailHeight = 360;

    private static int MaxLogItemsWithThumbnails => SettingsViewModel.GuiSettings.MaxNumberOfLogThumbnails;

    /// <summary>
    /// 日志分区分隔符（<see cref="LogCardItemViewModel.IsDivider"/>）在日志前 / 后插入卡片切割。
    /// </summary>
    public enum SplitMode
    {
        /// <summary>
        /// 不拆分日志卡片
        /// </summary>
        None = 0,

        /// <summary>
        /// 插入日志前拆分卡片
        /// </summary>
        Before = 1,

        /// <summary>
        /// 插入日志后拆分卡片
        /// </summary>
        After = 2,

        /// <summary>
        /// 插入日志前后都拆分卡片
        /// </summary>
        Both = 3,
    }

    /// <summary>
    /// 结束当前卡片（日志分区边界）：把末尾卡片标记为已封闭，下一条日志会新开一张卡片。
    /// 不预先创建空卡片——否则末尾会留下一张只有时间戳、没有内容的空卡片。
    /// </summary>
    /// <param name="cards">目标卡片集合。</param>
    public static void SealTrailingCard(ObservableCollection<LogCardItemViewModel> cards)
    {
        if (cards.Count > 0)
        {
            cards[^1].Sealed = true;
        }
    }

    /// <summary>
    /// 把一条日志追加到末尾卡片；末尾没有可写卡片时（集合为空 / 已封闭 / 是分隔符）新开一张。
    /// </summary>
    /// <param name="cards">目标卡片集合。</param>
    /// <param name="content">日志内容。</param>
    /// <param name="color">字体颜色。</param>
    /// <param name="weight">字体粗细。</param>
    /// <param name="toolTip">日志条目的 ToolTip。</param>
    /// <returns>新建的日志条目，调用方可加入纯文本日志集合以复用同一条目（含愚人节逐字动画）。</returns>
    public static LogItemViewModel AppendToTrailingCard(ObservableCollection<LogCardItemViewModel> cards, string content, string color, string weight, ToolTip? toolTip)
    {
        if (cards.Count == 0 || cards[^1].Sealed || cards[^1].IsDivider)
        {
            cards.Add(new LogCardItemViewModel());
        }

        var lastCard = cards[^1];
        var log = new LogItemViewModel(content, color, weight, toolTip: toolTip);

        var isAprilFools = DateTime.UtcNow.ToYjDate().IsAprilFoolsDay();
        if (isAprilFools)
        {
            log.Content = "thinking 🤔";
        }

        lastCard.Items.Add(log);

        if (isAprilFools)
        {
            Execute.OnUIThread(async () => {
                await Task.Delay(_logRandom.Next(800, 1500));
                log.Content = string.Empty;
                foreach (var ch in content)
                {
                    log.Content += ch;
                    await Task.Delay(_logRandom.Next(10, 35));
                }
            });
        }

        return log;
    }

    /// <summary>
    /// 插入一条日志分区分隔符（独立卡片，渲染为 <c>LogSectionDivider</c>）。
    /// </summary>
    /// <param name="cards">目标卡片集合。</param>
    /// <param name="header">分隔符标题，为空时仅显示一条通栏横线。</param>
    public static void AddDivider(ObservableCollection<LogCardItemViewModel> cards, string? header = null)
    {
        SealTrailingCard(cards);
        var divider = new LogCardItemViewModel { IsDivider = true, Header = header, Sealed = true };
        cards.Add(divider);
    }

    /// <summary>
    /// 判断末尾卡片是否还没有任何内容（用于「只更新截图、不写日志」的场景避免凭空建卡）。
    /// </summary>
    /// <param name="cards">目标卡片集合。</param>
    public static bool HasTrailingWritableCard(ObservableCollection<LogCardItemViewModel> cards)
        => cards.Count > 0 && !cards[^1].Sealed && !cards[^1].IsDivider;

    /// <summary>
    /// 抓取当前截图并挂到最后一张卡片上。
    /// </summary>
    /// <param name="cards">目标卡片集合。</param>
    /// <param name="forceScreencap">是否强制重新截图（false 时优先复用缓存帧）。</param>
    /// <param name="setToolTipOnLastLogItem">是否把截图同时挂到卡内最后一条日志的 ToolTip 上。</param>
    public static async Task AttachThumbnailToTrailingCardAsync(ObservableCollection<LogCardItemViewModel> cards, bool forceScreencap, bool setToolTipOnLastLogItem = false)
    {
        if (cards.Count == 0)
        {
            _logger.Warning("Cannot attach thumbnail: no log card available.");
            return;
        }

        await AttachThumbnailToCardAsync(cards, cards[^1], forceScreencap, setToolTipOnLastLogItem);
    }

    private static async Task AttachThumbnailToCardAsync(ObservableCollection<LogCardItemViewModel> cards, LogCardItemViewModel card, bool forceScreencap, bool setToolTipOnLastLogItem)
    {
        try
        {
            var thumbnail = await GetOrCaptureLogThumbnailAsync(forceScreencap).ConfigureAwait(false);
            if (thumbnail is null)
            {
                return;
            }

            await Execute.OnUIThreadAsync(() => {
                // 检查卡片是否还在集合中，避免给已清空的卡片赋值
                if (!cards.Contains(card))
                {
                    return;
                }

                card.Thumbnail = thumbnail;
                TrimOldThumbnails(cards);

                // 若需要将当前 Card 图片作为 ToolTip，在缩略图挂载完成后设置最后一条日志的 ToolTip
                if (setToolTipOnLastLogItem && card.Items.Count > 0)
                {
                    card.Items[^1].ToolTip = thumbnail.CreateTooltip();
                }
            });
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to attach thumbnail to log card.");
        }
    }

    private static async Task<BitmapSource?> GetOrCaptureLogThumbnailAsync(bool forceScreencap = false)
    {
        if (!await _logThumbnailSemaphore.WaitAsync(100))
        {
            return null;
        }

        try
        {
            var frameData = await Instances.AsstProxy.AsstGetImageBgrDataAsync(forceScreencap: forceScreencap).ConfigureAwait(false);
            if (frameData is null || frameData.Length == 0)
            {
                return null;
            }

            try
            {
                // 只保留小图，避免日志列表长期运行时占用过多内存。
                return AsstProxy.CreateBgrBitmapSourceScaled(frameData, LogThumbnailWidth, LogThumbnailHeight);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(frameData);
            }
        }
        finally
        {
            _logThumbnailSemaphore.Release();
        }
    }

    private static void TrimOldThumbnails(ObservableCollection<LogCardItemViewModel> cards)
    {
        var cardsWithThumbnails = cards.Where(card => card.Thumbnail != null).ToList();
        int max = MaxLogItemsWithThumbnails;

        foreach (var card in cardsWithThumbnails.Take(Math.Max(0, cardsWithThumbnails.Count - max)))
        {
            card.Thumbnail = null;
        }
    }
}
