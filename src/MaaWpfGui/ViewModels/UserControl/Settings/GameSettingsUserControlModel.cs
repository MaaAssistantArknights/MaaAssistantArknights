#nullable enable
using System.Collections.Generic;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class GameSettingsUserControlModel : PropertyChangedBase
{
    private static VersionUpdateSettingsUserControlModel VersionUpdateSettings => SettingsViewModel.VersionUpdateDataContext;

    /// <summary>
    /// Gets the list of the client types.
    /// </summary>
    public List<CombinedData> ClientTypeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty },
            new() { Display = LocalizationHelper.GetString("Official"), Value = "Official" },
            new() { Display = LocalizationHelper.GetString("Bilibili"), Value = "Bilibili" },
            new() { Display = LocalizationHelper.GetString("YoStarEN"), Value = "YoStarEN" },
            new() { Display = LocalizationHelper.GetString("YoStarJP"), Value = "YoStarJP" },
            new() { Display = LocalizationHelper.GetString("YoStarKR"), Value = "YoStarKR" },
            new() { Display = LocalizationHelper.GetString("Txwy"), Value = "txwy" },
        ];

    private string _clientType = ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty);

    /// <summary>
    /// Gets or sets the client type.
    /// </summary>
    public string ClientType
    {
        get => _clientType;
        set
        {
            SetAndNotify(ref _clientType, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ClientType, value);
            VersionUpdateSettings.ResourceInfo = VersionUpdateSettingsUserControlModel.GetResourceVersionByClientType(_clientType);
            VersionUpdateSettings.ResourceVersion = VersionUpdateSettings.ResourceInfo.VersionName;
            VersionUpdateSettings.ResourceDateTime = VersionUpdateSettings.ResourceInfo.DateTime;
            Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
            Instances.TaskQueueViewModel.UpdateStageList();
            Instances.TaskQueueViewModel.UpdateDatePrompt();
            Instances.AsstProxy.LoadResource();
            SettingsViewModel.AskRestartToApplySettings(_clientType is "YoStarEN");
        }
    }
}
