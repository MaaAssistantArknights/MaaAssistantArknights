using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels;

public sealed class RootPageHostViewModel : ObservableObject
{
    private RootPageLoadState _loadState;
    private object? _loadedContent;
    private string _statusTitle;
    private string _statusMessage;
    private string _errorMessage = string.Empty;

    public RootPageHostViewModel(string statusTitle, string statusMessage)
    {
        _statusTitle = statusTitle;
        _statusMessage = statusMessage;
    }

    public RootPageLoadState LoadState
    {
        get => _loadState;
        private set
        {
            if (SetProperty(ref _loadState, value))
            {
                OnPropertyChanged(nameof(IsLoading));
                OnPropertyChanged(nameof(IsLoaded));
                OnPropertyChanged(nameof(IsFailed));
                OnPropertyChanged(nameof(PageContent));
            }
        }
    }

    public string StatusTitle
    {
        get => _statusTitle;
        private set => SetProperty(ref _statusTitle, value);
    }

    public string StatusMessage
    {
        get => _statusMessage;
        private set => SetProperty(ref _statusMessage, value);
    }

    public string ErrorMessage
    {
        get => _errorMessage;
        private set
        {
            if (SetProperty(ref _errorMessage, value))
            {
                OnPropertyChanged(nameof(HasErrorMessage));
            }
        }
    }

    public bool IsLoading => LoadState is RootPageLoadState.NotStarted or RootPageLoadState.Loading;

    public bool IsLoaded => LoadState == RootPageLoadState.Loaded;

    public bool IsFailed => LoadState == RootPageLoadState.Failed;

    public bool HasErrorMessage => !string.IsNullOrWhiteSpace(ErrorMessage);

    public object PageContent => IsLoaded && _loadedContent is not null ? _loadedContent : this;

    public void MarkPending(string statusTitle, string statusMessage)
    {
        _loadedContent = null;
        StatusTitle = statusTitle;
        StatusMessage = statusMessage;
        ErrorMessage = string.Empty;
        LoadState = RootPageLoadState.NotStarted;
        OnPropertyChanged(nameof(PageContent));
    }

    public void MarkLoading(string statusTitle, string statusMessage)
    {
        _loadedContent = null;
        StatusTitle = statusTitle;
        StatusMessage = statusMessage;
        ErrorMessage = string.Empty;
        LoadState = RootPageLoadState.Loading;
        OnPropertyChanged(nameof(PageContent));
    }

    public void MarkLoaded(object content)
    {
        _loadedContent = content;
        StatusTitle = string.Empty;
        StatusMessage = string.Empty;
        ErrorMessage = string.Empty;
        LoadState = RootPageLoadState.Loaded;
        OnPropertyChanged(nameof(PageContent));
    }

    public void MarkFailed(string statusTitle, string statusMessage, string? errorMessage = null)
    {
        _loadedContent = null;
        StatusTitle = statusTitle;
        StatusMessage = statusMessage;
        ErrorMessage = errorMessage?.Trim() ?? string.Empty;
        LoadState = RootPageLoadState.Failed;
        OnPropertyChanged(nameof(PageContent));
    }
}
