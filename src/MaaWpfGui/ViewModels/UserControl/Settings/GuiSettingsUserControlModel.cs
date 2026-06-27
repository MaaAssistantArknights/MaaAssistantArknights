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
using DarkModeType = MaaWpfGui.Configuration.Global.GUI.DarkModeType;

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
    public LocalizedObservableList<string> InverseClearModeList { get; } = new(
        ("Clear", "Clear"),
        ("Inverse", "Inverse"),
        ("ClearInverse", "Switchable"));

    private bool _useTray = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UseTray, true);

    /// <summary>
    /// Gets or sets a value indicating whether to use tray icon.
    /// </summary>
    public bool UseTray
    {
        get => _useTray;
        set {
            if (!value)
            {
                MinimizeToTray = false;
            }

            SetAndNotify(ref _useTray, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.UseTray, value.ToString());
            Instances.MainWindowManager.SetUseTrayIcon(value);
        }
    }

    private bool _minimizeToTray = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MinimizeToTray, false);

    /// <summary>
    /// Gets or sets a value indicating whether to minimize to tray.
    /// </summary>
    public bool MinimizeToTray
    {
        get => _minimizeToTray;
        set {
            SetAndNotify(ref _minimizeToTray, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.MinimizeToTray, value.ToString());
            Instances.MainWindowManager.SetMinimizeToTray(value);
            if (_minimizeToTray)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.DisappearTrick);
            }
        }
    }

    private bool _windowTitleScrollable = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowTitleScrollable, false);

    /// <summary>
    /// Gets or sets a value indicating whether to make window title scrollable.
    /// </summary>
    public bool WindowTitleScrollable
    {
        get => _windowTitleScrollable;
        set {
            SetAndNotify(ref _windowTitleScrollable, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.WindowTitleScrollable, value.ToString());
            var rvm = (RootViewModel)Instances.SettingsViewModel.Parent;
            rvm.WindowTitleScrollable = value;
        }
    }

    private bool _hideCloseButton = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.HideCloseButton, false);

    /// <summary>
    /// Gets or sets a value indicating whether to hide close button.
    /// </summary>
    public bool HideCloseButton
    {
        get => _hideCloseButton;
        set {
            SetAndNotify(ref _hideCloseButton, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.HideCloseButton, value.ToString());
            var rvm = (RootViewModel)Instances.SettingsViewModel.Parent;
            rvm.ShowCloseButton = !value;
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use notification.
    /// </summary>
    public bool UseNotify
    {
        get => ConfigFactory.Root.GUI.UseNotify;
        set {
            ConfigFactory.Root.GUI.UseNotify = value;
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
        get => ConfigFactory.Root.GUI.MainTasksInvertNullFunction;
        set {
            ConfigFactory.Root.GUI.MainTasksInvertNullFunction = value;
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

    private string _logItemDateFormatString = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.LogItemDateFormat, "HH:mm:ss");

    public string LogItemDateFormatString
    {
        get => _logItemDateFormatString;
        set {
            SetAndNotify(ref _logItemDateFormatString, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.LogItemDateFormat, value);
        }
    }

    /// <summary>
    /// Gets or sets the dark mode.
    /// </summary>
    public DarkModeType DarkMode
    {
        get => ConfigFactory.Root.GUI.DarkMode;
        set {
            ConfigFactory.Root.GUI.DarkMode = value;
            NotifyOfPropertyChange();
            SwitchDarkMode();

            /*
            AskToRestartToApplySettings();
            */
        }
    }

    public void SwitchDarkMode()
    {
        DarkModeType darkModeType = ConfigFactory.Root.GUI.DarkMode;
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

    private enum InverseClearType
    {
        Clear,
        Inverse,
        ClearInverse,
    }

    private InverseClearType _inverseClearMode =
        Enum.TryParse(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.InverseClearMode, InverseClearType.Clear.ToString()), out InverseClearType temp)
            ? temp
            : InverseClearType.Clear;

    /// <summary>
    /// Gets or sets the inverse clear mode.
    /// </summary>
    public string InverseClearMode
    {
        get => _inverseClearMode.ToString();
        set {
            if (!Enum.TryParse(value, out InverseClearType tempEnumValue))
            {
                return;
            }

            SetAndNotify(ref _inverseClearMode, tempEnumValue);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.InverseClearMode, value);
            switch (tempEnumValue)
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
    }

    private bool _useCardLog = ConfigurationHelper.GetValue(ConfigurationKeys.UseCardLog, true);

    /// <summary>
    /// Gets or sets a value indicating whether to use card log format.
    /// </summary>
    public bool UseCardLog
    {
        get => _useCardLog;
        set {
            if (!SetAndNotify(ref _useCardLog, value))
            {
                return;
            }
            ConfigurationHelper.SetValue(ConfigurationKeys.UseCardLog, value.ToString());
        }
    }

    private int _maxNumberOfLogThumbnails = ConfigurationHelper.GetValue(ConfigurationKeys.MaxNumberOfLogThumbnails, 100);

    public int MaxNumberOfLogThumbnails
    {
        get => _maxNumberOfLogThumbnails;
        set {
            SetAndNotify(ref _maxNumberOfLogThumbnails, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.MaxNumberOfLogThumbnails, value.ToString());
        }
    }

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
        var config = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowTitleSelectShowList, "2 3 4");

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

    private static object[] _windowTitleSelectShowList = [.. ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowTitleSelectShowList, "2 3 4")
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
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.WindowTitleSelectShowList, config);
            if (config != "2 3 4")
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.TitleTweaker);
            }
        }
    }

    private string _language = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);

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

            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.Localization, value);

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

        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.Localization, value);
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
    private string _operNameLanguage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.OperNameLanguage, "OperNameLanguageMAA");

    public string OperNameLanguage
    {
        get {
            if (!_operNameLanguage.Contains('.'))
            {
                return _operNameLanguage;
            }

            if (_operNameLanguage.Split('.')[0] != "OperNameLanguageForce" || !LocalizationHelper.SupportedLanguages.ContainsKey(_operNameLanguage.Split('.')[1]))
            {
                return _operNameLanguage;
            }

            // 去重：getter 每次绑定时都可能被读取，避免重复添加"强制指定语言"选项
            if (!OperNameLanguageModeList.Items.Any(i => i.Value == "OperNameLanguageForce"))
            {
                OperNameLanguageModeList.Add("OperNameLanguageForce", "OperNameLanguageForce");
            }

            return "OperNameLanguageForce";
        }

        set {
            if (value == _operNameLanguage.Split('.')[0])
            {
                return;
            }

            switch (value)
            {
                case "OperNameLanguageClient":
                    ConfigurationHelper.SetGlobalValue(ConfigurationKeys.OperNameLanguage, value);
                    break;

                case "OperNameLanguageMAA":
                default:
                    ConfigurationHelper.SetGlobalValue(ConfigurationKeys.OperNameLanguage, "OperNameLanguageMAA");
                    break;
            }

            SetAndNotify(ref _operNameLanguage, value);

            // 切换到非 Force 选项后，移除运行时动态插入的 Force 项，避免下拉框残留
            OperNameLanguageModeList.Remove("OperNameLanguageForce");

            OperNameLanguageChanged?.Invoke();
        }
    }

    /// <summary>
    /// 干员名语言变更事件，订阅者应刷新干员相关数据。
    /// </summary>
    public event Action? OperNameLanguageChanged;

    public string OperNameLocalization
    {
        get {
            if (_operNameLanguage == "OperNameLanguageClient")
            {
                return DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType];
            }

            if (!_operNameLanguage.Contains('.'))
            {
                return _language;
            }

            if (_operNameLanguage.Split('.')[0] == "OperNameLanguageForce" && LocalizationHelper.SupportedLanguages.ContainsKey(_operNameLanguage.Split('.')[1]))
            {
                return _operNameLanguage.Split('.')[1];
            }

            return _language;
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to ignore bad modules and use software rendering.
    /// </summary>
    public bool IgnoreBadModulesAndUseSoftwareRendering
    {
        get => ConfigFactory.Root.GUI.IgnoreBadModulesAndUseSoftwareRendering;
        set {
            ConfigFactory.Root.GUI.IgnoreBadModulesAndUseSoftwareRendering = value;
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
