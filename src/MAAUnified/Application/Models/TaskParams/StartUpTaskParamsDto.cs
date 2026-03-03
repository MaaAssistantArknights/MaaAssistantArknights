namespace MAAUnified.Application.Models.TaskParams;

public sealed class StartUpTaskParamsDto
{
    public string AccountName { get; set; } = string.Empty;

    public string ClientType { get; set; } = "Official";

    public bool StartGameEnabled { get; set; } = true;

    public string ConnectConfig { get; set; } = "General";

    public string ConnectAddress { get; set; } = "127.0.0.1:5555";

    public string AdbPath { get; set; } = string.Empty;

    public string TouchMode { get; set; } = "minitouch";

    public bool AutoDetectConnection { get; set; } = true;

    public string AttachWindowScreencapMethod { get; set; } = "2";

    public string AttachWindowMouseMethod { get; set; } = "64";

    public string AttachWindowKeyboardMethod { get; set; } = "64";
}
