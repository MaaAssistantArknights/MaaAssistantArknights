// <copyright file="GuiSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using HandyControl.Controls;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Stylet;
using static MaaWpfGui.Configuration.Global.Gui;
using DarkModeType = MaaWpfGui.Configuration.Global.Gui.DarkModeType;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class GuiSettingsUserControlModel : PropertyChangedBase
{
    static GuiSettingsUserControlModel()
    {
        Instance = new();
    }

    public GuiSettingsUserControlModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        LocalizationHelper.LanguageChanged += RefreshLocalization;
        ApplyTransitionSpeed();
    }

    public static GuiSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets or sets the language list.
    /// </summary>
    public List<CombinedData> LanguageList { get; set; } = [];

    /// <summary>
    /// Gets or sets the list of operator name language settings
    /// </summary>
    public LocalizedObservableList<string> OperNameLanguageModeList { get; } = new(
        ("OperNameLanguageMAA", "OperNameLanguageMAA"),
        ("OperNameLanguageClient", "OperNameLanguageClient"));

    /// <summary>
    /// Gets the list of dark mode.
    /// </summary>
    public LocalizedObservableList<DarkModeType> DarkModeList { get; } = new(
        (DarkModeType.Light, "Light"),
        (DarkModeType.Dark, "Dark"),
        (DarkModeType.SyncWithOs, "SyncWithOs"));

    /// <summary>
    /// Gets the list of inverse clear modes.
    /// </summary>
    public LocalizedObservableList<InverseClearType> InverseClearModeList { get; } = new(
        (InverseClearType.Clear, "Clear"),
        (InverseClearType.Inverse, "Inverse"),
        (InverseClearType.ClearInverse, "Switchable"));

    /// <summary>
    /// Gets the list of transition animation speeds.
    /// </summary>
    public LocalizedObservableList<TransitionSpeedType> TransitionSpeedList { get; } = new(
        (TransitionSpeedType.Normal, "TransitionSpeedNormal"),
        (TransitionSpeedType.Fast, "TransitionSpeedFast"),
        (TransitionSpeedType.None, "TransitionSpeedNone"));

    /// <summary>
    /// Gets or sets a value indicating whether to use tray icon.
    /// </summary>
    public bool UseTray
    {
        get; set {
            if (!value)
            {
                MinimizeToTray = false;
            }

            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.UseTray = value;
            Instances.MainWindowManager.SetUseTrayIcon(value);
        }
    } = ConfigFactory.Root.Gui.UseTray;

    /// <summary>
    /// Gets or sets a value indicating whether to minimize to tray.
    /// </summary>
    public bool MinimizeToTray
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.MinimizeToTray = value;
            Instances.MainWindowManager.SetMinimizeToTray(value);
            if (value)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.DisappearTrick);
            }
        }
    } = ConfigFactory.Root.Gui.MinimizeToTray;

    /// <summary>
    /// Gets or sets a value indicating whether to make window title scrollable.
    /// </summary>
    public bool WindowTitleScrollable
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.WindowTitleScrollable = value;
            var rvm = (RootViewModel)Instances.SettingsViewModel.Parent;
            rvm.WindowTitleScrollable = value;
        }
    } = ConfigFactory.Root.Gui.WindowTitleScrollable;

    /// <summary>
    /// Gets or sets a value indicating whether to hide close button.
    /// </summary>
    public bool HideCloseButton
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.HideCloseButton = value;
            var rvm = (RootViewModel)Instances.SettingsViewModel.Parent;
            rvm.ShowCloseButton = !value;
        }
    } = ConfigFactory.Root.Gui.HideCloseButton;

    /// <summary>
    /// Gets or sets a value indicating whether to use notification.
    /// </summary>
    public bool UseNotify
    {
        get => ConfigFactory.Root.Gui.UseNotify;
        set {
            ConfigFactory.Root.Gui.UseNotify = value;
            NotifyOfPropertyChange();
            if (value)
            {
                ToastNotification.ShowDirect("Test test");
                var (isAvailable, detail) = ToastNotification.ToastNotificationCheck();
                if (!isAvailable)
                {
                    Growl.Error(LocalizationHelper.GetStringFormat("ToastNotificationUnavailable", detail));
                }
            }
        }
    }

    public bool MainTasksInvertNullFunction
    {
        get => ConfigFactory.Root.Gui.MainTasksInvertNullFunction;
        set {
            ConfigFactory.Root.Gui.MainTasksInvertNullFunction = value;
            NotifyOfPropertyChange();
        }
    }

    public List<string> LogItemDateFormatStringList { get; } =
    [
        "HH:mm:ss",
        "MM-dd  HH:mm:ss",
        "MM/dd  HH:mm:ss",
        "MM.dd  HH:mm:ss",
        "dd-MM  HH:mm:ss",
        "dd/MM  HH:mm:ss",
        "dd.MM  HH:mm:ss",
    ];

    public string LogItemDateFormatString
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.LogItemDateFormat = value;
        }
    } = ConfigFactory.Root.Gui.LogItemDateFormat;

    /// <summary>
    /// Gets or sets the dark mode.
    /// </summary>
    public DarkModeType DarkMode
    {
        get => ConfigFactory.Root.Gui.DarkMode;
        set {
            ConfigFactory.Root.Gui.DarkMode = value;
            NotifyOfPropertyChange();
            SwitchDarkMode();

            /*
            AskToRestartToApplySettings();
            */
        }
    }

    public TransitionSpeedType TransitionSpeed
    {
        get => ConfigFactory.Root.Gui.TransitionSpeed;
        set {
            ConfigFactory.Root.Gui.TransitionSpeed = value;
            NotifyOfPropertyChange();
            ApplyTransitionSpeed();
        }
    }

    /// <summary>
    /// 将过渡速度档位同步到过渡控件的全局时长，启动与切换档位时调用，改档立即生效。
    /// </summary>
    public void ApplyTransitionSpeed()
    {
        var normalMs = Styles.Controls.TransitioningContentControl.NormalDurationMilliseconds;
        Styles.Controls.TransitioningContentControl.TransitionDuration = ConfigFactory.Root.Gui.TransitionSpeed switch {
            TransitionSpeedType.Fast => TimeSpan.FromMilliseconds(normalMs / 2),
            TransitionSpeedType.None => TimeSpan.Zero,
            _ => TimeSpan.FromMilliseconds(normalMs),
        };
    }

    public void SwitchDarkMode()
    {
        DarkModeType darkModeType = ConfigFactory.Root.Gui.DarkMode;
        switch (darkModeType)
        {
            case DarkModeType.Light:
                ThemeHelper.SwitchToLightMode();
                break;

            case DarkModeType.Dark:
                ThemeHelper.SwitchToDarkMode();
                break;

            case DarkModeType.SyncWithOs:
                ThemeHelper.SwitchToSyncWithOsMode();
                break;

            default:
                throw new ArgumentOutOfRangeException();
        }
    }

    /// <summary>
    /// Gets or sets the inverse clear mode.
    /// </summary>
    public InverseClearType InverseClearMode
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.InverseClearMode = value;
            switch (value)
            {
                case InverseClearType.Clear:
                    Instances.TaskQueueViewModel.InverseMode = false;
                    Instances.TaskQueueViewModel.ShowInverse = false;
                    Instances.TaskQueueViewModel.SelectedAllWidth = 90;
                    break;

                case InverseClearType.Inverse:
                    Instances.TaskQueueViewModel.InverseMode = true;
                    Instances.TaskQueueViewModel.ShowInverse = false;
                    Instances.TaskQueueViewModel.SelectedAllWidth = 90;
                    break;

                case InverseClearType.ClearInverse:
                    Instances.TaskQueueViewModel.ShowInverse = true;
                    Instances.TaskQueueViewModel.SelectedAllWidth = TaskQueueViewModel.SelectedAllWidthWhenBoth;
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }
    } = ConfigFactory.Root.Gui.InverseClearMode;

    /// <summary>
    /// Gets or sets a value indicating whether to use card log format.
    /// </summary>
    public bool UseCardLog
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            ConfigFactory.Root.Gui.UseCardLog = value;
        }
    } = ConfigFactory.Root.Gui.UseCardLog;

    public int MaxNumberOfLogThumbnails
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.MaxNumberOfLogThumbnails = value;
        }
    } = ConfigFactory.Root.Gui.MaxNumberOfLogThumbnails;

    private static ObservableCollection<KeyValuePair<string, string>> _windowTitleAllShowDict = new(new Dictionary<string, string>
    {
        { "1", LocalizationHelper.GetString("ConfigurationName") },
        { "2", LocalizationHelper.GetString("ConnectionPreset") },
        { "3", LocalizationHelper.GetString("ConnectionAddress") },
        { "4", LocalizationHelper.GetString("ClientType") },
    });

    private void RefreshWindowTitleAllShowDict()
    {
        // 重建 SelectedItems 数组，保持当前选中的 key
        var config = ConfigFactory.Root.Gui.WindowTitleSelectShowList;

        WindowTitleAllShowDict[0] = new("1", LocalizationHelper.GetString("ConfigurationName"));
        WindowTitleAllShowDict[1] = new("2", LocalizationHelper.GetString("ConnectionPreset"));
        WindowTitleAllShowDict[2] = new("3", LocalizationHelper.GetString("ConnectionAddress"));
        WindowTitleAllShowDict[3] = new("4", LocalizationHelper.GetString("ClientType"));
        NotifyOfPropertyChange(nameof(WindowTitleAllShowDict));
        WindowTitleSelectShowList = [.. config
            .Split(' ')
            .Where(s => _windowTitleAllShowDict.Any(kv => kv.Key == s))
            .Select(s => (object)new KeyValuePair<string, string>(s, _windowTitleAllShowDict.First(kv => kv.Key == s).Value))];
    }

    public ObservableCollection<KeyValuePair<string, string>> WindowTitleAllShowDict { get => _windowTitleAllShowDict; }

    private static object[] _windowTitleSelectShowList = [.. ConfigFactory.Root.Gui.WindowTitleSelectShowList
        .Split(' ')
        .Where(s => _windowTitleAllShowDict.Any(kv => kv.Key == s))
        .Select(s => (object)new KeyValuePair<string, string>(s, _windowTitleAllShowDict.First(kv => kv.Key == s).Value))];

    public object[] WindowTitleSelectShowList
    {
        get => _windowTitleSelectShowList;
        set {
            SetAndNotify(ref _windowTitleSelectShowList, value);
            Instances.SettingsViewModel.UpdateWindowTitle();
            var config = string.Join(' ', _windowTitleSelectShowList.Cast<KeyValuePair<string, string>>().Select(pair => pair.Key).ToList());
            ConfigFactory.Root.Gui.WindowTitleSelectShowList = config;
            if (config != "2 3 4")
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.TitleTweaker);
            }
        }
    }

    private string _language = ConfigFactory.Root.Gui.Localization;

    /// <summary>
    /// Gets or sets the language.
    /// </summary>
    public string Language
    {
        get => _language;
        set {
            if (value == _language)
            {
                return;
            }

            if (value != SettingsViewModel.PallasLangKey)
            {
                Instances.SettingsViewModel.SoberLanguage = value;
            }

            ConfigFactory.Root.Gui.Localization = value;

            AchievementTrackerHelper.Instance.Unlock(AchievementIds.Linguist);

            var mainWindow = Application.Current.MainWindow;
            if (mainWindow != null)
            {
                mainWindow.Show();
                mainWindow.WindowState = mainWindow.WindowState = WindowState.Normal;
                mainWindow.Activate();
            }

            var result = MessageBoxHelper.Show(
                FormatText("{0}\n{1}", "LanguageChangedTip"),
                FormatText("{0}({1})", "Tip"),
                MessageBoxButton.YesNoCancel,
                MessageBoxImage.Question,
                yes: FormatText("{0}/{1}", "LanguageSwitchNow"),
                no: FormatText("{0}/{1}", "LanguageRestartNow"),
                cancel: FormatText("{0}/{1}", "ManualRestart"));
            if (result == MessageBoxResult.Yes)
            {
                // 时序要求：先静默更新 _language，使 Reload 触发 LanguageChanged 时，
                // 订阅者读到的 Language 属性已是新值；Reload 内部先替换字典再触发事件；
                // 最后 NotifyOfPropertyChange 触发 PropertyDependsOn(Language) 回调。
                _language = value;
                LocalizationHelper.Reload(value);
                NotifyOfPropertyChange(nameof(Language));
            }
            else if (result == MessageBoxResult.No)
            {
                // 重启以完整应用语言更改
                Bootstrapper.ShutdownAndRestartWithoutArgs();
            }
            else
            {
                // 稍后：仅保存配置，下次重启时生效
                SetAndNotify(ref _language, value);
            }

            return;

            string FormatText(string text, string key)
                => string.Format(text, LocalizationHelper.GetString(key, value), LocalizationHelper.GetString(key, _language));
        }
    }

    /// <summary>
    /// 直接热切换语言，不弹确认窗、不触发 pallas 宿醉等 setter 副作用。
    /// 供彩蛋逻辑（GetDrunk / HangoverEnd）在已完成状态转换后调用。
    /// </summary>
    /// <param name="value">目标语言代码</param>
    public void SetLanguageInternal(string value)
    {
        if (value == _language)
        {
            return;
        }

        ConfigFactory.Root.Gui.Localization = value;
        _language = value;
        LocalizationHelper.Reload(value);
        NotifyOfPropertyChange(nameof(Language));
    }

    /// <summary>
    /// Gets the language info.
    /// </summary>
    [PropertyDependsOn(nameof(Language))]
    public string LanguageInfo
    {
        get {
            var language = (string?)Application.Current.Resources["Language"];
            return language == "Language" ? language : language + " / Language";
        }
    }

    /// <summary>
    /// Opername display language, can set force display when it was set as "OperNameLanguageForce.en-us"
    /// </summary>
    public string OperNameLanguage
    {
        get {
            if (!field.Contains('.'))
            {
                return field;
            }

            if (field.Split('.')[0] != "OperNameLanguageForce" || !LocalizationHelper.SupportedLanguages.ContainsKey(field.Split('.')[1]))
            {
                return field;
            }

            // 去重：getter 每次绑定时都可能被读取，避免重复添加"强制指定语言"选项
            if (!OperNameLanguageModeList.Items.Any(i => i.Value == "OperNameLanguageForce"))
            {
                OperNameLanguageModeList.Add("OperNameLanguageForce", "OperNameLanguageForce");
            }

            return "OperNameLanguageForce";
        }

        set {
            if (value == field.Split('.')[0])
            {
                return;
            }

            switch (value)
            {
                case "OperNameLanguageClient":
                    ConfigFactory.Root.Gui.OperNameLanguage = value;
                    break;

                case "OperNameLanguageMAA":
                default:
                    ConfigFactory.Root.Gui.OperNameLanguage = "OperNameLanguageMAA";
                    break;
            }

            SetAndNotify(ref field, value);

            // 切换到非 Force 选项后，移除运行时动态插入的 Force 项，避免下拉框残留
            OperNameLanguageModeList.Remove("OperNameLanguageForce");

            OperNameLanguageChanged?.Invoke();
        }
    } = ConfigFactory.Root.Gui.OperNameLanguage;

    /// <summary>
    /// 干员名语言变更事件，订阅者应刷新干员相关数据。
    /// </summary>
    public event Action? OperNameLanguageChanged;

    public string OperNameLocalization
    {
        get {
            if (OperNameLanguage == "OperNameLanguageClient")
            {
                return DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType];
            }

            if (!OperNameLanguage.Contains('.'))
            {
                return _language;
            }

            if (OperNameLanguage.Split('.')[0] == "OperNameLanguageForce" && LocalizationHelper.SupportedLanguages.ContainsKey(OperNameLanguage.Split('.')[1]))
            {
                return OperNameLanguage.Split('.')[1];
            }

            return _language;
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to ignore bad modules and use software rendering.
    /// </summary>
    public bool IgnoreBadModulesAndUseSoftwareRendering
    {
        get => ConfigFactory.Root.Gui.IgnoreBadModulesAndUseSoftwareRendering;
        set {
            ConfigFactory.Root.Gui.IgnoreBadModulesAndUseSoftwareRendering = value;
            NotifyOfPropertyChange();
            if (value)
            {
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("BadModules.ResetWarning"),
                    LocalizationHelper.GetString("Tip"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
            }
            else
            {
                var mainWindow = Application.Current.MainWindow;
                if (mainWindow != null)
                {
                    mainWindow.Show();
                    mainWindow.WindowState = WindowState.Normal;
                    mainWindow.Activate();
                }

                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("BadModules.ResetSuccess"),
                    LocalizationHelper.GetString("Tip"),
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Information,
                    ok: LocalizationHelper.GetString("Ok"),
                    cancel: LocalizationHelper.GetString("ManualRestart"));
                if (result == MessageBoxResult.OK)
                {
                    Bootstrapper.ShutdownAndRestartWithoutArgs();
                }
            }
        }
    }

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    public void RefreshLocalization()
    {
        OperNameLanguageModeList.RefreshLocalization();

        DarkModeList.RefreshLocalization();
        InverseClearModeList.RefreshLocalization();
        RefreshWindowTitleAllShowDict();
        Instances.SettingsViewModel.UpdateWindowTitle();
    }
}
