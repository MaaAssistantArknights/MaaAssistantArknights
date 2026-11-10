// <copyright file="PerformanceUserControlModel.cs" company="MaaAssistantArknights">
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

#nullable enable
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 性能设置
/// </summary>
public class PerformanceUserControlModel : PropertyChangedBase
{
    static PerformanceUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
        AsstProxy.OpenVinoAvailabilityChanged += Instance.RefreshOpenVinoAvailability;
    }

    public static PerformanceUserControlModel Instance { get; }

    public PerformanceUserControlModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    private List<GpuOptionItem>? _gpuOptions;

    public List<GpuOptionItem> GpuOptions => _gpuOptions ??= [.. GpuOption.GetGpuOptions().Select(o => new GpuOptionItem(o))];

    private GpuOptionItem? _activeGpuOption;

    public GpuOptionItem? ActiveGpuOption
    {
        get => _activeGpuOption ??= GpuOptions.FirstOrDefault(x => x.Value.Equals(GpuOption.GetCurrent()));
        set
        {
            if (!SetAndNotify(ref _activeGpuOption, value) || value is null)
            {
                return;
            }

            GpuOption.SetCurrent(value.Value);
            SettingsViewModel.AskRestartToApplySettings();
        }
    }

    public LocalizedObservableList<BackendFamily> BackendFamilyOptions { get; } = new(
        (BackendFamily.Auto, "InferenceBackendAuto"),
        (BackendFamily.DirectML, "InferenceBackendDirectML"),
        (BackendFamily.WebGPU, "InferenceBackendWebGpu"),
        (BackendFamily.OpenVINO, "InferenceBackend.OpenVino"));

    public LocalizedObservableList<OpenVinoDeviceChoice> OpenVinoDeviceOptions { get; } = new(
        (OpenVinoDeviceChoice.CPU, "InferenceBackend.OpenVinoDevice.Cpu"),
        (OpenVinoDeviceChoice.NPU, "InferenceBackend.OpenVinoDevice.Npu"),
        (OpenVinoDeviceChoice.Hybrid, "InferenceBackend.OpenVinoDevice.Hybrid"));

    [PropertyDependsOn(nameof(ActiveBackend))]
    public BackendFamily ActiveBackendFamily
    {
        get => ActiveBackend switch
        {
            InferenceBackend.DirectML => BackendFamily.DirectML,
            InferenceBackend.WebGPU => BackendFamily.WebGPU,
            InferenceBackend.OpenVINOCpu or InferenceBackend.OpenVINONpu or InferenceBackend.OpenVINOHybrid => BackendFamily.OpenVINO,
            _ => BackendFamily.Auto,
        };
        set
        {
            switch (value)
            {
                case BackendFamily.DirectML:
                    ActiveBackend = InferenceBackend.DirectML;
                    break;
                case BackendFamily.WebGPU:
                    ActiveBackend = InferenceBackend.WebGPU;
                    break;
                case BackendFamily.OpenVINO:
                    // 切换到 OpenVINO 时沿用已保存的设备；从其他后端切来时选择当前可用的设备，
                    // 避免保存一个不可用（置灰）的选项导致启动时被回退为 Auto。
                    ActiveBackend = ActiveBackend switch
                    {
                        InferenceBackend.OpenVINOCpu => InferenceBackend.OpenVINOCpu,
                        InferenceBackend.OpenVINONpu => InferenceBackend.OpenVINONpu,
                        InferenceBackend.OpenVINOHybrid => InferenceBackend.OpenVINOHybrid,
                        _ when !AsstProxy.OpenVinoProbed => InferenceBackend.OpenVINONpu,
                        _ when OpenVinoNpuAvailable => InferenceBackend.OpenVINONpu,
                        _ when OpenVinoCpuAvailable => InferenceBackend.OpenVINOCpu,
                        _ => InferenceBackend.Auto,
                    };
                    break;
                default:
                    ActiveBackend = InferenceBackend.Auto;
                    break;
            }

            NotifyOfPropertyChange();
        }
    }

    [PropertyDependsOn(nameof(ActiveBackend))]
    public OpenVinoDeviceChoice ActiveOpenVinoDevice
    {
        get => ActiveBackend switch
        {
            InferenceBackend.OpenVINOCpu => OpenVinoDeviceChoice.CPU,
            InferenceBackend.OpenVINOHybrid => OpenVinoDeviceChoice.Hybrid,
            _ => OpenVinoDeviceChoice.NPU,
        };
        set
        {
            ActiveBackend = value switch
            {
                OpenVinoDeviceChoice.CPU => InferenceBackend.OpenVINOCpu,
                OpenVinoDeviceChoice.Hybrid => InferenceBackend.OpenVINOHybrid,
                _ => InferenceBackend.OpenVINONpu,
            };
            NotifyOfPropertyChange();
        }
    }

    [PropertyDependsOn(nameof(ActiveBackend))]
    public bool IsOpenVinoDeviceVisible => ActiveBackendFamily == BackendFamily.OpenVINO;

    public InferenceBackend ActiveBackend
    {
        get => ConfigFactory.Root.Gui.Performance.Backend;
        set
        {
            if (ConfigFactory.Root.Gui.Performance.Backend == value)
            {
                return;
            }

            ConfigFactory.Root.Gui.Performance.Backend = value;
            NotifyOfPropertyChange();
            if (!_suppressRestartPrompt)
            {
                SettingsViewModel.AskRestartToApplySettings();
            }
        }
    }

    private bool _suppressRestartPrompt;

    public void SetBackendWithoutRestart(InferenceBackend backend)
    {
        if (ActiveBackend == backend)
        {
            return;
        }

        _suppressRestartPrompt = true;
        try
        {
            ActiveBackend = backend;
        }
        finally
        {
            _suppressRestartPrompt = false;
        }
    }

    [PropertyDependsOn(nameof(ActiveBackend))]
    public bool IsGpuSelectionVisible => !IsOpenVino(ActiveBackend);

    /// <summary>
    /// 判断后端是否属于 OpenVINO 系列。
    /// </summary>
    /// <param name="backend">推理后端。</param>
    /// <returns>属于 OpenVINO CPU/NPU/Hybrid 时返回 true。</returns>
    public static bool IsOpenVino(InferenceBackend backend) =>
        backend is InferenceBackend.OpenVINOCpu or InferenceBackend.OpenVINONpu or InferenceBackend.OpenVINOHybrid;

    [PropertyDependsOn(nameof(ActiveBackend))]
    public bool IsOpenVinoNpuSelected => ActiveBackend == InferenceBackend.OpenVINONpu;

    [PropertyDependsOn(nameof(ActiveBackend))]
    public bool IsOpenVinoHybridSelected => ActiveBackend == InferenceBackend.OpenVINOHybrid;

    public bool OpenVinoCpuAvailable => !AsstProxy.OpenVinoProbed || AsstProxy.OpenVinoCpuAvailable;

    public bool OpenVinoNpuAvailable => !AsstProxy.OpenVinoProbed || AsstProxy.OpenVinoNpuAvailable;

    /// <summary>
    /// 首选 OpenVINO 后端不可用时，返回仍可用的 CPU/NPU 模式；都不可用则回退 Auto。
    /// </summary>
    /// <param name="backend">当前保存的推理后端。</param>
    /// <param name="cpuAvailable">OpenVINO CPU 是否可用。</param>
    /// <param name="npuAvailable">OpenVINO NPU 是否可用。</param>
    /// <returns>可用的回退后端；首选后端可用时原样返回。</returns>
    public static InferenceBackend GetAvailableFallback(InferenceBackend backend, bool cpuAvailable, bool npuAvailable) => backend switch
    {
        InferenceBackend.OpenVINOCpu when !cpuAvailable =>
            npuAvailable ? InferenceBackend.OpenVINONpu : InferenceBackend.Auto,
        InferenceBackend.OpenVINONpu when !npuAvailable =>
            cpuAvailable ? InferenceBackend.OpenVINOCpu : InferenceBackend.Auto,
        InferenceBackend.OpenVINOHybrid when !(cpuAvailable && npuAvailable) =>
            npuAvailable ? InferenceBackend.OpenVINONpu
            : cpuAvailable ? InferenceBackend.OpenVINOCpu
            : InferenceBackend.Auto,
        _ => backend,
    };

    private void RefreshOpenVinoAvailability()
    {
        NotifyOfPropertyChange(nameof(OpenVinoCpuAvailable));
        NotifyOfPropertyChange(nameof(OpenVinoNpuAvailable));

        // 探针完成时修正不可用的选择，避免启动时被静默回退为 Auto。
        if (AsstProxy.OpenVinoProbed && IsOpenVino(ActiveBackend))
        {
            SetBackendWithoutRestart(
                GetAvailableFallback(ActiveBackend, OpenVinoCpuAvailable, OpenVinoNpuAvailable));
        }
    }

    /// <summary>
    /// 刷新本地化选项显示文本（语言切换时调用）。
    /// </summary>
    private void RefreshLocalization()
    {
        RefreshGpuOptions();
        BackendFamilyOptions.RefreshLocalization();
        OpenVinoDeviceOptions.RefreshLocalization();
    }

    /// <summary>
    /// 刷新 GPU 选项的显示文本。
    /// 只更新每个 item 的 Display 字符串，选中项引用保持稳定，避免 ComboBox 选中框刷新问题。
    /// </summary>
    private void RefreshGpuOptions()
    {
        foreach (var item in GpuOptions)
        {
            item.RefreshDisplay();
        }
    }

    public bool AllowDeprecatedGpu
    {
        get => GpuOption.AllowDeprecatedGpu;
        set {
            GpuOption.AllowDeprecatedGpu = value;
            NotifyOfPropertyChange();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to ignore bad modules and use software rendering.
    /// </summary>
    public bool IgnoreBadModulesAndUseSoftwareRendering
    {
        get => ConfigFactory.Root.Gui.Performance.IgnoreBadModulesAndUseSoftwareRendering;
        set {
            ConfigFactory.Root.Gui.Performance.IgnoreBadModulesAndUseSoftwareRendering = value;
            NotifyOfPropertyChange();
            if (value)
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("Error.BadModules.ResetWarning"),
                    LocalizationHelper.GetString("Common.Tip"),
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
                    LocalizationHelper.GetString("Error.BadModules.ResetSuccess"),
                    LocalizationHelper.GetString("Common.Tip"),
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Information,
                    ok: LocalizationHelper.GetString("Common.Confirm"),
                    cancel: LocalizationHelper.GetString("Common.ManualRestart"));
                if (result == MessageBoxResult.OK)
                {
                    Bootstrapper.ShutdownAndRestartWithoutArgs();
                }
            }
        }
    }
}
