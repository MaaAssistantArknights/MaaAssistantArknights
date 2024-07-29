using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Windows;
using DirectN;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.Models;

public class AfterActionSetting : PropertyChangedBase
{
    private bool _once;

    public bool Once
    {
        get => _once;
        set
        {
            SetAndNotify(ref _once, value);
            RefreshDescription();
        }
    }

    private bool _exitArknights = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExitArknights, Boolean.FalseString));
    private bool _backToAndroidHome = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BackToAndroidHome, Boolean.FalseString));
    private bool _exitEmulator = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExitEmulator, Boolean.FalseString));
    private bool _exitSelf = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExitSelf, Boolean.FalseString));
    private bool _ifNoOtherMaa = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IfNoOtherMaa, Boolean.FalseString));
    private bool _hibernate = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Hibernate, Boolean.FalseString));
    private bool _shutdown = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Shutdown, Boolean.FalseString));

    public bool ExitArknights
    {
        get => _exitArknights;
        set
        {
            SetAndNotify(ref _exitArknights, value);
            RefreshDescription();
        }
    }

    public bool BackToAndroidHome
    {
        get => _backToAndroidHome;
        set
        {
            SetAndNotify(ref _backToAndroidHome, value);
            RefreshDescription();
        }
    }

    public bool ExitEmulator
    {
        get => _exitEmulator;
        set
        {
            SetAndNotify(ref _exitEmulator, value);
            RefreshDescription();
        }
    }

    public bool ExitSelf
    {
        get => _exitSelf;
        set
        {
            SetAndNotify(ref _exitSelf, value);
            RefreshDescription();
        }
    }

    public bool IfNoOtherMaa
    {
        get => _ifNoOtherMaa;
        set
        {
            SetAndNotify(ref _ifNoOtherMaa, value);
            RefreshDescription();
        }
    }


    public bool Hibernate
    {
        get => _hibernate;
        set
        {
            SetAndNotify(ref _hibernate, value);
            RefreshDescription();
        }
    }

    public bool Shutdown
    {
        get => _shutdown;
        set
        {
            SetAndNotify(ref _shutdown, value);
            RefreshDescription();
        }
    }

    public static AfterActionSetting Current { get; } = new();

    public void Save()
    {
        if (Once)
        {
            return;
        }

        ConfigurationHelper.SetValue(ConfigurationKeys.ExitArknights, ExitArknights.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.BackToAndroidHome, BackToAndroidHome.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.ExitEmulator, ExitEmulator.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.ExitSelf, ExitSelf.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.IfNoOtherMaa, IfNoOtherMaa.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.Once, Once.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.Hibernate, Hibernate.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.Shutdown, Shutdown.ToString());
    }

    private string _actionDescription = string.Empty;

    public string ActionDescription
    {
        get => _actionDescription;
        private set => SetAndNotify(ref _actionDescription, value);
    }

    private void RefreshDescription()
    {
        List<string> actions = [];
        if (ExitArknights)
        {
            actions.Add(LocalizationHelper.GetString("ExitArknights"));
        }

        if (BackToAndroidHome)
        {
            actions.Add(LocalizationHelper.GetString("BackToAndroidHome"));
        }

        if (ExitEmulator)
        {
            actions.Add(LocalizationHelper.GetString("ExitEmulator"));
        }

        if (ExitSelf)
        {
            actions.Add(LocalizationHelper.GetString("ExitSelf"));
        }

        if (IfNoOtherMaa)
        {
            actions.Add(LocalizationHelper.GetString("IfNoOtherMaa"));
        }

        if (Hibernate)
        {
            actions.Add(LocalizationHelper.GetString("Hibernate"));
        }

        if (Shutdown)
        {
            actions.Add(LocalizationHelper.GetString("Shutdown"));
        }

        ActionDescription = actions.IsEmpty()
            ? LocalizationHelper.GetString("DoNothing")
            : string.Join(" -> ", actions);
    }


    private AfterActionSetting()
    {
        RefreshDescription();
    }
}
