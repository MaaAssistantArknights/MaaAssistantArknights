using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels.Copilot;

public sealed class CopilotItemViewModel : ObservableObject
{
    private string _name;
    private string _type;
    private string _status = "Ready";
    private string _sourcePath = string.Empty;
    private string _inlinePayload = string.Empty;

    public CopilotItemViewModel(
        string name,
        string type,
        string? sourcePath = null,
        string? inlinePayload = null)
    {
        _name = name;
        _type = type;
        _sourcePath = sourcePath ?? string.Empty;
        _inlinePayload = inlinePayload ?? string.Empty;
    }

    public string Name
    {
        get => _name;
        set => SetProperty(ref _name, value);
    }

    public string Type
    {
        get => _type;
        set => SetProperty(ref _type, value);
    }

    public string Status
    {
        get => _status;
        set => SetProperty(ref _status, value);
    }

    public string SourcePath
    {
        get => _sourcePath;
        set => SetProperty(ref _sourcePath, value ?? string.Empty);
    }

    public string InlinePayload
    {
        get => _inlinePayload;
        set => SetProperty(ref _inlinePayload, value ?? string.Empty);
    }
}
