// <copyright file="DemoShotService.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using GlobalGui = MaaWpfGui.Configuration.Global.Gui;

namespace MaaWpfGui.Main.DemoShot;

/// <summary>
/// README 截图演示模式主流程：加载演示数据填充界面，遍历语言与主题截取四个页面并落盘，完成后自动退出。
/// 由 <see cref="Bootstrapper"/> 在主窗口显示后启动；联网、模拟器连接与自动任务的短路在各入口
/// 以 <see cref="Bootstrapper.IsDemoMode"/> 判断实现，本类只负责数据注入、遍历与截图。
/// </summary>
public static class DemoShotService
{
    private static readonly ILogger _logger = Log.ForContext(typeof(DemoShotService));

    /// <summary>截图文档约定的窗口客户区尺寸（RootView XAML 固定为 800x600）。</summary>
    private const int WindowWidth = 800;

    private const int WindowHeight = 600;

    /// <summary>遍历的界面语言，与 LocalizationHelper.SupportedLanguages 及文档目录名保持一致。</summary>
    private static readonly string[] Languages = ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr"];

    /// <summary>
    /// 运行演示截图流程。全程保持在 UI 线程（数据文件读取除外），异常时记日志并以非零码退出。
    /// </summary>
    /// <param name="demoDataPath">演示数据 JSON 路径。</param>
    /// <param name="shotsOutputDir">截图输出根目录。</param>
    /// <returns>表示流程的任务。</returns>
    public static async Task RunAsync(string demoDataPath, string shotsOutputDir)
    {
        try
        {
            _logger.Information("Demo shot service starting, data: {DemoDataPath}, output: {ShotsOutputDir}", demoDataPath, shotsOutputDir);

            await WaitForRootViewReadyAsync();
            await WaitForCoreInitAsync();

            var data = await Task.Run(() => DemoShotData.LoadFromFile(demoDataPath));
            if (data == null)
            {
                throw new InvalidDataException("Failed to load demo data, see log for details");
            }

            await ResolveWindowTitleVersionAsync(data.WindowTitle);

            ApplyStaticSettings(data);
            InjectStaticData(data);

            // 缩略图路径相对演示数据文件所在目录解析
            string dataDir = Path.GetDirectoryName(Path.GetFullPath(demoDataPath)) ?? string.Empty;

            string[] themeSuffixes = ["light", "dark"];
            GlobalGui.DarkModeType[] themes = [GlobalGui.DarkModeType.Light, GlobalGui.DarkModeType.Dark];
            int total = Languages.Length * themes.Length;
            int current = 0;
            foreach (string lang in Languages)
            {
                for (int themeIndex = 0; themeIndex < themes.Length; themeIndex++)
                {
                    current++;
                    _logger.Information("Capturing lang={Lang} theme={Theme} ({Current}/{Total})", lang, themeSuffixes[themeIndex], current, total);
                    await ApplyLanguageAndThemeAsync(data, lang, themes[themeIndex]);
                    ReinjectLanguageData(data, lang, dataDir);

                    string outDir = Path.Combine(shotsOutputDir, lang, "readme");
                    Directory.CreateDirectory(outDir);
                    await CapturePagesAsync(outDir, themeSuffixes[themeIndex]);
                }
            }

            _logger.Information("Demo shot service finished, {Total} screenshots written, shutting down", total * 4);
            Application.Current.Shutdown();
        }
        catch (Exception e)
        {
            _logger.Fatal(e, "Demo shot service failed");
            Log.CloseAndFlush();
            Environment.Exit(1);
        }
    }

    /// <summary>
    /// 等待 RootView 加载完成（Loaded 触发 RootViewModel.OnViewLoaded 填充导航 Items 后才可注入与切页）。
    /// </summary>
    private static async Task WaitForRootViewReadyAsync()
    {
        var start = DateTime.UtcNow;
        while ((DateTime.UtcNow - start).TotalMilliseconds < 60000)
        {
            var window = Application.Current.MainWindow;
            if (window is { IsLoaded: true, IsVisible: true } &&
                Instances.SettingsViewModel.Parent is RootViewModel { Items.Count: > 0 })
            {
                return;
            }

            await Task.Delay(200);
        }

        throw new TimeoutException("Root view was not ready within 60s");
    }

    /// <summary>
    /// 等待 Core 初始化完成（AsstProxy.Init 异步执行，Inited 置位前 Link Start 呈未初始化置灰）；
    /// 超时记警告继续截图（按钮保持置灰好过流程中断）。
    /// </summary>
    private static async Task WaitForCoreInitAsync()
    {
        var start = DateTime.UtcNow;
        while ((DateTime.UtcNow - start).TotalMilliseconds < 60000)
        {
            if (States.RunningState.Instance.GetInit())
            {
                return;
            }

            await Task.Delay(200);
        }

        _logger.Warning("Core init was not ready within 60s, screenshots will show uninitialized start button");
    }

    /// <summary>
    /// 一次性固定窗口与标题形态：窗口位置归一、标题只保留客户端类型段、关闭页面过渡动画。
    /// </summary>
    private static void ApplyStaticSettings(DemoShotData data)
    {
        // 成就通知悬浮在窗口内右上角，会出现在截图里，演示期间一律禁用；
        // 必须先于 WindowTitleSelectShowList 等可能解锁成就的设置，否则拦截晚于首次弹窗
        SettingsViewModel.AchievementSettings.AchievementPopupDisabled = true;

        if (Application.Current.MainWindow is { } window)
        {
            window.WindowStartupLocation = WindowStartupLocation.Manual;
            window.Left = 60;
            window.Top = 60;
            window.WindowState = WindowState.Normal;

            // 配置可能恢复出更大的历史窗口尺寸（WindowPlacement），而截图像素尺寸固定 800x600，
            // 窗口超出部分截不到、圆角遮罩也会错位，故尺寸一并归位
            window.Width = WindowWidth;
            window.Height = WindowHeight;
        }

        ConfigFactory.CurrentConfig.Gui.WindowTitlePrefix = string.Empty;
        SettingsViewModel.GuiSettings.WindowTitleSelectShowList =
            [new KeyValuePair<string, string>("4", LocalizationHelper.GetString("ClientType"))];

        // 标题版本段覆盖：ResolveWindowTitleVersionAsync 已把缺省/latest 解析为具体版本（失败保持 null 走原行为）
        SettingsViewModel.DemoWindowTitleVersionOverride = data.WindowTitle?.Version;
        SettingsViewModel.DemoWindowTitleResourceVersionOverride = data.WindowTitle?.ResourceVersion;

        // 关闭过渡动画：切页/切语言立即稳定，避免截图拍到转场中间态
        ConfigFactory.Root.Gui.TransitionSpeed = GlobalGui.TransitionSpeedType.None;
        SettingsViewModel.GuiSettings.ApplyTransitionSpeed();
    }

    /// <summary>
    /// 一次性注入与语言无关的数据：两个识别页的同步时间（供逐语言组重注入使用）、任务顺序与勾选、自动战斗页作业条目。
    /// </summary>
    private static void InjectStaticData(DemoShotData data)
    {
        // 两个识别页的 ｢上次同步时间｣ 展示位统一取演示进程启动当天的本地 03:25（GUI 按 UTC 解析后转本地显示，
        // 故先转 UTC 再格式化为无时区标记字符串）；内嵌 operBox 数据时同样覆写，两页观感一致
        string syncTime = DateTime.Now.Date.AddHours(3).AddMinutes(25).ToUniversalTime().ToString("yyyy-MM-dd'T'HH:mm:ss");
        data.Depot?["syncTime"] = syncTime;
        if (data.OperBox != null)
        {
            data.OperBox["syncTime"] = syncTime;
        }

        // 任务列表：按数据序工厂新建固定任务实例列表并写好勾选，交给 TaskQueueViewModel 同步完成
        // 配置集合替换与条目列表重建；不经拖拽排序的 Dispatcher 异步回写链，返回时顺序、勾选与名称
        // 即已定版，与首个语言组的截图时序无竞争
        Instances.TaskQueueViewModel.ApplyDemoTaskSequence(BuildOrderedDemoTasks(data.TaskQueue.Tasks));

        // 条目状态展示（已完成/进行中）与日志区的演示进度对齐，须在条目列表重建后注入
        InjectTaskStatuses(data.TaskQueue.Tasks);

        // 自动战斗页：作业列表逐项条目 + 顶部输入框展示首个神秘代码；
        // DisplayFilename 赋 maa:// 作业站代码走 IsCopilotCode 分支透传 Filename，
        // 该链上的 UpdateFileDoc 已在演示模式短路，不触网
        if (data.Copilot.CopilotIds.Count > 0)
        {
            Instances.CopilotViewModel.UseCopilotList = true;
            foreach (int copilotId in data.Copilot.CopilotIds)
            {
                Instances.CopilotViewModel.CopilotItemViewModels.Add(new CopilotItemViewModel($"maa://{copilotId}", string.Empty, false, copilotId));
            }

            Instances.CopilotViewModel.CopilotTabIndex = data.Copilot.TabIndex;
            Instances.CopilotViewModel.DisplayFilename = $"maa://{data.Copilot.CopilotIds[0]}";
        }
    }

    /// <summary>
    /// 可添加任务类型 key 到任务实例工厂的映射，键与任务列表 ｢添加｣ 菜单的数据源
    /// <c>TaskQueueViewModel.TaskTypeList</c> 的成员（<c>TaskQueueView.xaml</c> 的菜单项）一一对应，
    /// 即任务列表可出现的全部条目；<c>CustomTask</c> 为自定义空壳不在演示之列。
    /// </summary>
    private static readonly Dictionary<string, Func<BaseTask>> DemoTaskFactories =
        new(StringComparer.OrdinalIgnoreCase)
        {
            ["StartUp"] = () => new StartUpTask(),
            ["Fight"] = () => new FightTask(),
            ["OperProgress"] = () => new OperProgressTask(),
            ["Infrast"] = () => new InfrastTask(),
            ["Recruit"] = () => new RecruitTask(),
            ["Mall"] = () => new MallTask(),
            ["Award"] = () => new AwardTask(),
            ["UserDataUpdate"] = () => new UserDataUpdateTask(),
            ["DepotMaintain"] = () => new DepotMaintainTask(),
            ["SwitchTheme"] = () => new SwitchThemeTask(),
            ["Roguelike"] = () => new RoguelikeTask(),
            ["Reclamation"] = () => new ReclamationTask(),
        };

    /// <summary>
    /// 按演示数据序列以 <see cref="DemoTaskFactories"/> 全新建固定任务实例列表，不读取配置：
    /// 任务名一律为 TaskType 标准本地化名（<see cref="BaseTask.Name"/> 留空），不受运行目录
    /// 遗留的自定义任务名与任务集差异影响。
    /// </summary>
    /// <param name="tasks">演示数据的有序任务条目。</param>
    /// <returns>可直接交给 <see cref="TaskQueueViewModel.ApplyDemoTaskSequence"/> 的有序任务列表。</returns>
    private static List<BaseTask> BuildOrderedDemoTasks(IReadOnlyList<DemoTaskEntry> tasks)
    {
        var ordered = new List<BaseTask>(tasks.Count);
        foreach (var entry in tasks)
        {
            if (!DemoTaskFactories.TryGetValue(entry.Type, out var factory))
            {
                _logger.Warning("Demo task type {Type} not found in factories, skipped", entry.Type);
                continue;
            }

            var task = factory();
            task.IsEnable = entry.Enabled;
            ordered.Add(task);
        }

        return ordered;
    }

    /// <summary>
    /// 解析窗口标题版本段：版本字段缺省或为 latest 时联网取 GitHub 最新 Release 的 tag_name。
    /// 这是演示模式唯一的联网点；请求失败或超时静默回退 null，即走原行为显示本机版本（DEBUG_VERSION 亦然），不弹窗不阻塞截图。
    /// </summary>
    /// <param name="windowTitle">窗口标题覆盖配置；整体缺省时不联网，走原行为。</param>
    /// <returns>表示解析过程的任务。</returns>
    private static async Task ResolveWindowTitleVersionAsync(DemoWindowTitleData? windowTitle)
    {
        if (windowTitle == null ||
            (!string.IsNullOrEmpty(windowTitle.Version) && !string.Equals(windowTitle.Version, "latest", StringComparison.OrdinalIgnoreCase)))
        {
            return;
        }

        windowTitle.Version = await FetchLatestReleaseTagAsync();
    }

    /// <summary>
    /// 查询 GitHub 最新 Release 的 tag_name，10 秒超时；任何失败都记 Warning 并返回 null。
    /// </summary>
    /// <returns>tag_name；失败时为 null。</returns>
    private static async Task<string?> FetchLatestReleaseTagAsync()
    {
        try
        {
            using var timeoutCts = new CancellationTokenSource(TimeSpan.FromSeconds(10));
            using var response = await Instances.HttpService.GetAsync(
                new Uri("https://api.github.com/repos/MaaAssistantArknights/MaaAssistantArknights/releases/latest"),
                token: timeoutCts.Token);
            if (!response.IsSuccessStatusCode)
            {
                _logger.Warning("GitHub latest release query returned HTTP {StatusCode}, falling back to local version", (int)response.StatusCode);
                return null;
            }

            string? tag = JObject.Parse(await response.Content.ReadAsStringAsync(timeoutCts.Token))["tag_name"]?.ToString();
            if (string.IsNullOrEmpty(tag))
            {
                _logger.Warning("GitHub latest release response has no tag_name, falling back to local version");
            }

            return tag;
        }
        catch (Exception e)
        {
            _logger.Warning(e, "GitHub latest release query failed, falling back to local version");
            return null;
        }
    }

    /// <summary>
    /// 从 battle_data 干员全集生成全干员满练度识别数据（own=true、满潜；练度按星级上限：
    /// 6 星精二 90、5 星精二 80、4 星精二 70、3 星精一 60、2/1 星不精英化 45/30）。
    /// 排除 <paramref name="lang"/> 对应客户端未实装的干员（IsCharacterAvailableInClient），
    /// 否则外服语言界面会出现中文名占位。
    /// 名称仅兜底，展示名由 GUI 按 id 查表本地化，生成数据语言无关。
    /// </summary>
    /// <param name="data">演示数据。</param>
    /// <param name="lang">当前界面语言（同时决定可用干员集合）。</param>
    /// <returns>可直接交给 <see cref="ToolboxViewModel.OperBoxParse"/> 的识别结果。</returns>
    private static JObject BuildFullOperBoxDetails(DemoShotData data, string lang)
    {
        var ownOpers = new JArray();
        foreach (var (id, character) in DataHelper.Operators)
        {
            if (!DataHelper.IsCharacterAvailableInClient(character, lang))
            {
                continue;
            }

            int level = character.Rarity switch {
                6 => 90,
                5 => 80,
                4 => 70,
                3 => 60,
                2 => 45,
                _ => 30,
            };
            int elite = character.Rarity switch {
                >= 4 => 2,
                3 => 1,
                _ => 0,
            };

            ownOpers.Add(new JObject
            {
                ["id"] = id,
                ["name"] = character.Name ?? string.Empty,
                ["own"] = true,
                ["elite"] = elite,
                ["level"] = level,
                ["potential"] = 6,
                ["rarity"] = character.Rarity,
            });
        }

        // 同步时间展示位与仓库识别保持同一演示时刻，两个页面观感一致；仓库数据缺省时退回当前时间
        string syncTime = data.Depot?["syncTime"]?.ToString() ?? DateTimeOffset.Now.ToString("yyyy-MM-dd'T'HH:mm:ss");
        _logger.Information("Generated demo operBox with {Count} operators from battle_data", ownOpers.Count);
        return new JObject
        {
            ["done"] = true,
            ["own_opers"] = ownOpers,
            ["syncTime"] = syncTime,
        };
    }

    /// <summary>
    /// 语言切换后重注入随语言变化的数据：仓库与干员识别数据、关卡候选、两个页面的日志与日志卡片缩略图。
    /// </summary>
    private static void ReinjectLanguageData(DemoShotData data, string lang, string dataDir)
    {
        // 关卡活动数据按当组客户端类型重新本地解析（构造时解析用的是启动配置的客户端类型），
        // 随后重算「今日关卡小提示」
        Instances.StageManager.UpdateStageLocal();
        Instances.TaskQueueViewModel.UpdateDatePrompt();

        // 仓库识别数据随语言组重注入：演示模式 data/ 缓存写入由 JsonDataHelper 层统一拦截，
        // 语言切换的重载回调无落盘数据可读，材料名的按语言重建只能由重注入完成
        Instances.ToolboxViewModel.ResetDepotRecognitionState();
        if (data.Depot != null)
        {
            Instances.ToolboxViewModel.DepotParse(data.Depot, updateSyncTime: false);
        }

        // 干员识别数据按当组客户端类型生成（排除当前服未实装的干员，避免外服界面出现中文占位名），
        // 因此必须随语言组重注入而非一次性生成
        Instances.ToolboxViewModel.ResetOperBoxRecognitionState();
        var operBox = data.OperBox ?? BuildFullOperBoxDetails(data, lang);
        Instances.ToolboxViewModel.OperBoxParse(operBox, updateSyncTime: false);

        // 候选关卡全语言共享（关卡代号语言无关），每个语言组重注入一次
        if (data.TaskQueue.Stages.Count > 0)
        {
            FightSettingsUserControlModel.Instance.InjectDemoStages(data.TaskQueue.Stages);
        }

        InjectTaskQueueLogs(data, lang, dataDir);
        InjectCopilotLogs(data.Copilot, lang);
    }

    /// <summary>
    /// 注入长草页日志并覆写展示时间，随后按各卡片首条日志条目的 thumbnail 索引挂载缩略图。
    /// </summary>
    private static void InjectTaskQueueLogs(DemoShotData data, string lang, string dataDir)
    {
        var vm = Instances.TaskQueueViewModel;
        vm.ClearLog();

        var injected = new List<(string Time, DemoLogEntry Entry)>();

        // 日志卡片与其首条日志条目的映射：AddLog 在 UI 线程同步执行，返回后最后一张卡片
        // 即该条日志落入的卡片，卡片首次出现时对应的条目就是它的第一条日志
        var cardThumbnails = new List<(LogCardItemViewModel Card, int? Thumbnail)>();
        foreach (var entry in data.TaskQueueLogs)
        {
            string text = ResolveText(entry.Text, lang);
            if (string.IsNullOrEmpty(text))
            {
                continue;
            }

            vm.AddLog(text, ResolveColor(entry.Color), entry.Weight, splitMode: ParseSplitMode(entry.Split), notifyActivity: false);
            injected.Add((entry.Time, entry));

            if (vm.LogCardViewModels.Count > 0)
            {
                var card = vm.LogCardViewModels[^1];
                if (cardThumbnails.Count == 0 || !ReferenceEquals(cardThumbnails[^1].Card, card))
                {
                    cardThumbnails.Add((card, entry.Thumbnail));
                }
            }
        }

        // AddLog 经 TryMergeIntoLastCard 把同一 LogItemViewModel 同时挂到扁平集合与日志卡片，
        // 覆写扁平集合的时间即同时生效
        var items = vm.LogItemViewModels;
        for (int i = 0; i < injected.Count && i < items.Count; i++)
        {
            if (!string.IsNullOrEmpty(injected[i].Time))
            {
                items[i].Time = injected[i].Time;
            }
        }

        // 缩略图直接设置卡片属性，不走 AddLog 的 updateCardImage（其经 core 实时抓图，演示模式不可用）；
        // ClearLog 后卡片集合已重建，每次重注入都按当组数据重新挂载
        foreach (var (card, thumbnailIndex) in cardThumbnails)
        {
            if (thumbnailIndex is not int index)
            {
                continue;
            }

            if (index < 0 || index >= data.Thumbnails.Count)
            {
                _logger.Warning("Demo thumbnail index {Index} out of range ({Count} thumbnails), skipping", index, data.Thumbnails.Count);
                continue;
            }

            var image = LoadDemoThumbnail(Path.Combine(dataDir, data.Thumbnails[index]));
            if (image != null)
            {
                card.Thumbnail = image;
            }
        }
    }

    /// <summary>
    /// 从磁盘加载缩略图。用 <see cref="BitmapCacheOption.OnLoad"/> 在 EndInit 时一次性读入内存，
    /// 文件流随即关闭不锁定；加载后 Freeze 使其可跨线程安全共享。失败记 Warning 返回 null。
    /// </summary>
    /// <param name="path">图片文件路径。</param>
    /// <returns>解码后的位图；加载失败时为 null。</returns>
    private static BitmapImage? LoadDemoThumbnail(string path)
    {
        try
        {
            var bitmap = new BitmapImage();
            using (var stream = File.OpenRead(path))
            {
                bitmap.BeginInit();
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.StreamSource = stream;
                bitmap.EndInit();
            }

            bitmap.Freeze();
            return bitmap;
        }
        catch (Exception e)
        {
            _logger.Warning(e, "Failed to load demo thumbnail: {Path}", path);
            return null;
        }
    }

    /// <summary>
    /// 注入自动战斗页日志：头部行无时间戳，日志条目带时间戳并覆写展示时间。
    /// </summary>
    private static void InjectCopilotLogs(DemoCopilotData copilot, string lang)
    {
        var vm = Instances.CopilotViewModel;

        // CopilotViewModel.ClearLog 为 private 且会追加 CopilotTip，这里直接清空公开集合后按演示数据重建
        vm.LogItemViewModels.Clear();

        foreach (var line in copilot.HeaderLines)
        {
            string text = ResolveText(line.Text, lang);
            if (!string.IsNullOrEmpty(text))
            {
                vm.AddLog(text, ResolveColor(line.Color), line.Weight, showTime: false);
            }
        }

        var timed = new List<(LogItemViewModel Item, string Time)>();
        foreach (var log in copilot.Logs)
        {
            string text = ResolveText(log.Text, lang);
            if (string.IsNullOrEmpty(text))
            {
                continue;
            }

            vm.AddLog(text, ResolveColor(log.Color), log.Weight, showTime: true);
            timed.Add((vm.LogItemViewModels[^1], log.Time));
        }

        foreach (var (item, time) in timed)
        {
            if (!string.IsNullOrEmpty(time))
            {
                item.Time = time;
            }
        }
    }

    /// <summary>
    /// 应用单组语言与主题：语言热切换、客户端类型（绕开 setter 副作用）、明暗主题与窗口标题，然后等待界面稳定。
    /// </summary>
    private static async Task ApplyLanguageAndThemeAsync(DemoShotData data, string lang, GlobalGui.DarkModeType theme)
    {
        SettingsViewModel.GuiSettings.SetLanguageInternal(lang);

        if (data.ClientType.TryGetValue(lang, out var clientTypeName) &&
            Enum.TryParse(clientTypeName, ignoreCase: true, out ClientType clientType))
        {
            GameSettingsUserControlModel.Instance.SetClientTypeQuietly(clientType);
        }
        else
        {
            _logger.Warning("Client type missing or invalid for language {Lang}, keeping current", lang);
        }

        ConfigFactory.Root.Gui.DarkMode = theme;
        SettingsViewModel.GuiSettings.SwitchDarkMode();
        Instances.SettingsViewModel.UpdateWindowTitle();

        // 语言切换会触发一串回调（本地化刷新、仓库/干员重载、关卡列表重建，部分排在 Dispatcher.Loaded），
        // 主题切换会重建资源字典；轮转多个低优先级帧加短暂延时，等全部落地
        for (int i = 0; i < 4; i++)
        {
            await Application.Current.Dispatcher.InvokeAsync(() => { }, DispatcherPriority.Background);
        }

        await Task.Delay(500);
    }

    /// <summary>
    /// 依次切页截取四个页面：长草、自动战斗、小工具-干员识别（已拥有）、小工具-仓库识别。
    /// </summary>
    private static async Task CapturePagesAsync(string outDir, string themeSuffix)
    {
        var root = Instances.SettingsViewModel.Parent as RootViewModel ?? throw new InvalidOperationException("RootViewModel is not ready");

        // 页 1：一键长草。Idle 置为运行中：与任务条目 ｢公招进行中｣ 的演示进度一致，Link Start 呈
        // 运行态；仅观感模拟，任务入口已被 TryGetTaskBlockReason 统一拦截。每组语言主题循环都会
        // 经过本页，故在此恢复（上一组小工具页已切回空闲）
        States.RunningState.Instance.SetIdle(false);
        root.ActiveItem = Instances.TaskQueueViewModel;
        await WaitUiSettledAsync();
        await CaptureAsync(Path.Combine(outDir, $"1-{themeSuffix}.png"));

        // 页 2：自动战斗
        root.ActiveItem = Instances.CopilotViewModel;
        await WaitUiSettledAsync();
        await CaptureAsync(Path.Combine(outDir, $"2-{themeSuffix}.png"));

        // 页 3：小工具-干员识别，切到「已拥有」页。
        // Idle 切回空闲：小工具页的识别按钮呈可点观感（演示数据是识别完成的结果态），
        // 长草/自动战斗页则保持运行中（与日志的演示进度一致）
        States.RunningState.Instance.SetIdle(true);
        // TabControlSliding 的滑块依赖可见状态下的选择变化事件移动，且视图分离期间的索引变更
        // 会以未布局的位置参与动画导致滑块错位；故挂载稳定后先抖到 0 再落到 1，强制一次完整动画。
        // 内层 OperBox TabControl 同理抖一次：语言组的热切换重建会把索引拉回 0，直接设 1 滑块可能不跟随
        root.ActiveItem = Instances.ToolboxViewModel;
        await WaitUiSettledAsync(300);
        Instances.ToolboxViewModel.ToolboxSelectedIndex = 0;
        await Task.Delay(300);
        Instances.ToolboxViewModel.ToolboxSelectedIndex = 1;
        Instances.ToolboxViewModel.OperBoxSelectedIndex = 0;
        await Task.Delay(300);
        Instances.ToolboxViewModel.OperBoxSelectedIndex = 1;
        await WaitUiSettledAsync(800);
        await CaptureAsync(Path.Combine(outDir, $"3-{themeSuffix}.png"));

        // 页 4：小工具-仓库识别；滑块位移动画走完再截
        Instances.ToolboxViewModel.ToolboxSelectedIndex = 2;
        await WaitUiSettledAsync(800);
        await CaptureAsync(Path.Combine(outDir, $"4-{themeSuffix}.png"));
    }

    /// <summary>
    /// 渲染主窗口为 PNG。
    /// </summary>
    private static async Task CaptureAsync(string path)
    {
        var window = Application.Current.MainWindow ?? throw new InvalidOperationException("Main window is not available");

        // 再等一帧高优先级布局，确保绑定与 DynamicResource 刷新完成后再渲染
        await window.Dispatcher.InvokeAsync(() => { }, DispatcherPriority.Loaded);

        var rtb = new RenderTargetBitmap(WindowWidth, WindowHeight, 96, 96, PixelFormats.Pbgra32);
        rtb.Render(window);

        // Pbgra32 为预乘 alpha：只改 alpha 不同比缩放 BGR，PNG 编码反预乘时会把半透明边缘像素提亮
        // （暗色背景下圆角出现浅色光晕）；先转直通 alpha 的 Bgra32 再做钳制与遮罩
        var direct = new FormatConvertedBitmap(rtb, PixelFormats.Bgra32, null, 0);

        // 渲染产生的轻微半透明像素（抗锯齿/描边，alpha 237~254）钳到 255 对齐基准的全不透明主体。
        // 演示界面无深半透明内容（背景不透明、无遮罩），钳制不引入可见变化
        var opaque = new WriteableBitmap(direct);
        int stride = opaque.PixelWidth * 4;
        var pixels = new byte[stride * opaque.PixelHeight];
        opaque.CopyPixels(pixels, stride, 0);
        for (int i = 3; i < pixels.Length; i += 4)
        {
            if (pixels[i] != 255)
            {
                pixels[i] = 255;
            }
        }

        ApplyCornerMask(pixels, stride, opaque.PixelWidth, opaque.PixelHeight);
        opaque.WritePixels(new Int32Rect(0, 0, opaque.PixelWidth, opaque.PixelHeight), pixels, stride, 0);

        var encoder = new PngBitmapEncoder();
        encoder.Frames.Add(BitmapFrame.Create(opaque));
        using var stream = File.Create(path);
        encoder.Save(stream);
        _logger.Information("Captured {Path}", path);
    }

    private const int CornerMaskSize = 14;

    /// <summary>
    /// README 透明圆角的 14×14 alpha 模板（左上角），逐像素取自历史基准图
    /// （tools 产线认可的 README 圆角形态，弧线/羽化为实测值，非理想几何圆）；
    /// 其余三角与左上角互为严格镜像，按翻转取下标即可。
    /// </summary>
    private static readonly byte[,] CornerAlphaMask =
    {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 96, 159, 191 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 32, 159, 255, 255, 255, 255 },
        { 0, 0, 0, 0, 0, 0, 16, 143, 255, 255, 255, 255, 255, 255 },
        { 0, 0, 0, 0, 0, 48, 207, 255, 255, 255, 255, 255, 255, 255 },
        { 0, 0, 0, 0, 48, 239, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 0, 0, 0, 48, 239, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 0, 0, 16, 207, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 0, 0, 143, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 0, 32, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 0, 159, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 16, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 96, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 159, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        { 191, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    };

    /// <summary>
    /// 把四角的 alpha 模板写到像素缓冲（TR=TL 水平翻转、BL=竖直翻转、BR=中心对称）。
    /// </summary>
    private static void ApplyCornerMask(byte[] pixels, int stride, int width, int height)
    {
        var corners = new (int OriginX, int OriginY, bool FlipX, bool FlipY)[]
        {
            (0, 0, false, false),
            (width - CornerMaskSize, 0, true, false),
            (0, height - CornerMaskSize, false, true),
            (width - CornerMaskSize, height - CornerMaskSize, true, true),
        };

        foreach (var (originX, originY, flipX, flipY) in corners)
        {
            for (int y = 0; y < CornerMaskSize; y++)
            {
                int maskY = flipY ? CornerMaskSize - 1 - y : y;
                int rowBase = ((originY + y) * stride) + (originX * 4);
                for (int x = 0; x < CornerMaskSize; x++)
                {
                    int maskX = flipX ? CornerMaskSize - 1 - x : x;
                    pixels[rowBase + (x * 4) + 3] = CornerAlphaMask[maskY, maskX];
                }
            }
        }
    }

    /// <summary>
    /// 等待切页后的界面稳定：数个低优先级帧 + 短延时。
    /// </summary>
    private static async Task WaitUiSettledAsync(int delayMs = 200)
    {
        for (int i = 0; i < 3; i++)
        {
            await Application.Current.Dispatcher.InvokeAsync(() => { }, DispatcherPriority.Background);
        }

        await Task.Delay(delayMs);
    }

    /// <summary>
    /// 取指定语言的文案，缺语言时回退 zh-cn，仍缺时取首项并记警告。
    /// </summary>
    private static string ResolveText(Dictionary<string, string> text, string lang)
    {
        if (text.Count == 0)
        {
            return string.Empty;
        }

        if (text.TryGetValue(lang, out var value))
        {
            return value;
        }

        _logger.Warning("Demo text missing language {Lang}, falling back to zh-cn", lang);
        if (text.TryGetValue("zh-cn", out var fallback))
        {
            return fallback;
        }

        _logger.Warning("Demo text missing zh-cn too, using first entry");
        return text.Values.First();
    }

    /// <summary>
    /// UiLogColor 常量名到常量值的映射。演示数据中的 color 是常量名（如 Trace），
    /// 而 AddLog 接受的是常量值即 brush 资源 key（如 TraceLogBrush），直传常量名会查不到资源回退默认色。
    /// </summary>
    private static readonly Dictionary<string, string> LogColorMap =
        typeof(UiLogColor)
            .GetFields(BindingFlags.Public | BindingFlags.Static)
            .Where(f => f.IsLiteral && f.FieldType == typeof(string))
            .ToDictionary(f => f.Name, f => (string)f.GetValue(null)!, StringComparer.OrdinalIgnoreCase);

    /// <summary>
    /// 将演示数据中的日志颜色（UiLogColor 常量名）解析为 AddLog 可用的 brush 资源 key，未知名回退 Trace。
    /// </summary>
    private static string ResolveColor(string? color)
    {
        if (string.IsNullOrEmpty(color))
        {
            return UiLogColor.Trace;
        }

        if (LogColorMap.TryGetValue(color, out string? brushKey))
        {
            return brushKey;
        }

        _logger.Warning("Unknown demo log color {Color}, falling back to Trace", color);
        return UiLogColor.Trace;
    }

    /// <summary>
    /// 按演示数据的 <c>status</c> 字段注入任务条目状态展示（如公招前的任务已完成、公招进行中，
    /// 与日志区的演示进度对齐）。任务类型在固定列表中唯一，按类型匹配条目；
    /// 未知取值记警告按 idle 处理。
    /// </summary>
    /// <param name="entries">演示数据的任务条目。</param>
    private static void InjectTaskStatuses(IReadOnlyList<DemoTaskEntry> entries)
    {
        var statusByType = new Dictionary<string, Constants.Enums.TaskItemStatus>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in entries)
        {
            var status = entry.Status.ToLowerInvariant() switch
            {
                "inprogress" => Constants.Enums.TaskItemStatus.InProgress,
                "completed" => Constants.Enums.TaskItemStatus.Completed,
                "idle" => Constants.Enums.TaskItemStatus.Idle,
                _ => Constants.Enums.TaskItemStatus.Idle,
            };

            if (status == Constants.Enums.TaskItemStatus.Idle && !string.Equals(entry.Status, "idle", StringComparison.OrdinalIgnoreCase))
            {
                _logger.Warning("Unknown demo task status {Status} for {Type}, using idle", entry.Status, entry.Type);
            }

            statusByType[entry.Type] = status;
        }

        // 演示模式无真实任务回调，注入后不会被覆盖；须在 ApplyDemoTaskSequence 重建条目列表后调用
        foreach (var item in Instances.TaskQueueViewModel.TaskItemViewModels)
        {
            var task = ConfigFactory.CurrentConfig.TaskQueue[item.Index];
            if (statusByType.TryGetValue(task.TaskType.ToString(), out var status))
            {
                item.StatusDisplay = status;
            }
        }
    }

    /// <summary>
    /// 解析演示数据中的卡片拆分方式（None/Before/After/Both，忽略大小写）。
    /// </summary>
    private static TaskQueueViewModel.LogCardSplitMode ParseSplitMode(string? split) => split?.ToLowerInvariant() switch
    {
        "before" => TaskQueueViewModel.LogCardSplitMode.Before,
        "after" => TaskQueueViewModel.LogCardSplitMode.After,
        "both" => TaskQueueViewModel.LogCardSplitMode.Both,
        _ => TaskQueueViewModel.LogCardSplitMode.None,
    };
}
