using System;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Input;
using HandyControl.Tools.Command;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using Newtonsoft.Json;
using Stylet;

public class UpdateInfo
{
    public string CurrentVersion { get; set; } = string.Empty;

    public string TargetVersion { get; set; } = string.Empty;

    public bool IsUpdated { get; set; } = false;

    public bool IsUpdating { get; set; } = false;
}

public static class FakeUpdateHelper
{
    private const string FileName = "FakeUpdate";

    private static UpdateInfo _updateInfo;

    public static string CurrentVersion => _updateInfo.IsUpdated ? _updateInfo.TargetVersion : _updateInfo.CurrentVersion;

    public static bool IsUpdating => _updateInfo.IsUpdating;

    static FakeUpdateHelper()
    {
        _updateInfo = JsonDataHelper.Get<UpdateInfo>(FileName);
    }

    public static void Updating()
    {
        _updateInfo.IsUpdating = true;
        JsonDataHelper.Set(FileName, _updateInfo);
        Bootstrapper.ShutdownAndRestartWithoutArgs();
    }

    public static void Updated()
    {
        _updateInfo.IsUpdated = true;
        _updateInfo.IsUpdating = false;
        JsonDataHelper.Set(FileName, _updateInfo);
        Bootstrapper.ShutdownAndRestartWithoutArgs();
    }
}
