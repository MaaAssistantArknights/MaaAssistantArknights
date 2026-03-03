using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels.Copilot;

public sealed class CopilotItemViewModel : ObservableObject
{
    private string _name;
    private string _type;
    private string _status = "Ready";

    public CopilotItemViewModel(string name, string type)
    {
        _name = name;
        _type = type;
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
}
