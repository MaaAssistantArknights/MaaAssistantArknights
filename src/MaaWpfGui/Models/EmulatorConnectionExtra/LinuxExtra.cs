// <copyright file="LinuxExtra.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums.Core;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class LinuxExtra : ExtraConfig
{
    private static readonly LocalizedObservableList<AsstLinuxScreencapMethod> _screencapMethodList =
        new(
            (AsstLinuxScreencapMethod.Wlr, "LinuxScreencapWlr"),
            (AsstLinuxScreencapMethod.PipeWire, "LinuxScreencapPipeWire"));

    private static readonly LocalizedObservableList<AsstLinuxInputMethod> _inputMethodList =
        new(
            (AsstLinuxInputMethod.Wlr, "LinuxInputWlr"),
            (AsstLinuxInputMethod.UInput, "LinuxInputUInput"),
            (AsstLinuxInputMethod.Libei, "LinuxInputLibei"));

    static LinuxExtra()
    {
        LocalizationHelper.LanguageChanged += RefreshListsLocalization;
    }

    public LocalizedObservableList<AsstLinuxScreencapMethod> ScreencapMethodList => _screencapMethodList;

    public LocalizedObservableList<AsstLinuxInputMethod> InputMethodList => _inputMethodList;

    public AsstLinuxScreencapMethod ScreencapMethod
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.ScreencapMethod = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.ScreencapMethod;

    public AsstLinuxInputMethod InputMethod
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.InputMethod = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.InputMethod;

    public string WlrSocketPath
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.WlrSocketPath = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.WlrSocketPath;

    public uint PipeWireNodeId
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.PipeWireNodeId = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.PipeWireNodeId;

    public int UInputScreenWidth
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.UInputScreenWidth = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.UInputScreenWidth;

    public int UInputScreenHeight
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.UInputScreenHeight = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.UInputScreenHeight;

    public string UInputPath
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.UInputPath = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.UInputPath;

    public string EisSocketPath
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.EisSocketPath = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.EisSocketPath;

    public bool TargetIsPC
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.TargetIsPC = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LinuxExtra.TargetIsPC;

    public string Config
    {
        get {
            var config = new JObject {
                ["library_name"] = "MaaLinuxControlUnit",
                ["screencap_method"] = (ulong)ScreencapMethod,
                ["input_method"] = (ulong)InputMethod,
                ["use_win32_vk_code"] = false,
            };

            if (ScreencapMethod == AsstLinuxScreencapMethod.Wlr || InputMethod == AsstLinuxInputMethod.Wlr)
            {
                config["wlr_socket_path"] = WlrSocketPath;
            }

            if (ScreencapMethod == AsstLinuxScreencapMethod.PipeWire)
            {
                config["pw_node_id"] = PipeWireNodeId;
            }

            if (InputMethod == AsstLinuxInputMethod.UInput)
            {
                config["uinput_path"] = UInputPath;
                config["uinput_screen_width"] = UInputScreenWidth;
                config["uinput_screen_height"] = UInputScreenHeight;
            }
            else if (InputMethod == AsstLinuxInputMethod.Libei)
            {
                config["eis_socket_path"] = EisSocketPath;
            }

            return JsonConvert.SerializeObject(config);
        }
    }

    private static void RefreshListsLocalization()
    {
        _screencapMethodList.RefreshLocalization();
        _inputMethodList.RefreshLocalization();
    }
}
