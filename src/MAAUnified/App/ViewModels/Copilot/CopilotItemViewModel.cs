using System;
using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels.Copilot;

public sealed class CopilotItemViewModel : ObservableObject
{
    private string _name;
    private string _type;
    private string _status = "Ready";
    private string _sourcePath = string.Empty;
    private string _inlinePayload = string.Empty;
    private bool _isChecked = true;
    private bool _isRaid;
    private int _copilotId;
    private int? _tabIndex;

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
        set
        {
            if (SetProperty(ref _name, value))
            {
                OnPropertyChanged(nameof(DisplayName));
            }
        }
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

    public bool IsChecked
    {
        get => _isChecked;
        set => SetProperty(ref _isChecked, value);
    }

    public bool IsRaid
    {
        get => _isRaid;
        set
        {
            if (SetProperty(ref _isRaid, value))
            {
                OnPropertyChanged(nameof(DisplayName));
            }
        }
    }

    public int CopilotId
    {
        get => _copilotId;
        set => SetProperty(ref _copilotId, Math.Max(0, value));
    }

    public int? TabIndex
    {
        get => _tabIndex;
        set => SetProperty(ref _tabIndex, value);
    }

    public string ExecutionPathHint
        => !string.IsNullOrWhiteSpace(SourcePath)
            ? SourcePath
            : !string.IsNullOrWhiteSpace(InlinePayload)
                ? "inline-json"
                : string.Empty;

    public string DisplayName => IsRaid ? $"{Name} (突袭)" : Name;
}
