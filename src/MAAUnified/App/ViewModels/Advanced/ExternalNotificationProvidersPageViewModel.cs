using System.Collections.ObjectModel;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Advanced;

public sealed class ExternalNotificationProvidersPageViewModel : PageViewModelBase
{
    private string _selectedProvider = string.Empty;
    private string _parametersText = string.Empty;
    private string _title = "MAA Test";
    private string _message = "Advanced external notification test";

    public ExternalNotificationProvidersPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Providers = new ObservableCollection<string>();
    }

    public ObservableCollection<string> Providers { get; }

    public string SelectedProvider
    {
        get => _selectedProvider;
        set => SetProperty(ref _selectedProvider, value ?? string.Empty);
    }

    public string ParametersText
    {
        get => _parametersText;
        set => SetProperty(ref _parametersText, value ?? string.Empty);
    }

    public string Title
    {
        get => _title;
        set => SetProperty(ref _title, value ?? string.Empty);
    }

    public string Message
    {
        get => _message;
        set => SetProperty(ref _message, value ?? string.Empty);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await RefreshProvidersAsync(cancellationToken);
    }

    public async Task RefreshProvidersAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var providers = await Runtime.NotificationProviderFeatureService.GetAvailableProvidersAsync(cancellationToken);
            Providers.Clear();
            foreach (var provider in providers)
            {
                Providers.Add(provider);
            }

            if (Providers.Count == 0)
            {
                SelectedProvider = string.Empty;
                StatusMessage = "No external notification provider available.";
                LastErrorMessage = string.Empty;
                await RecordEventAsync("Advanced.ExternalNotificationProviders.Query", StatusMessage, cancellationToken);
                return;
            }

            if (string.IsNullOrWhiteSpace(SelectedProvider) || !Providers.Contains(SelectedProvider))
            {
                SelectedProvider = Providers[0];
            }

            StatusMessage = $"Loaded {Providers.Count} external notification provider(s).";
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Advanced.ExternalNotificationProviders.Query", StatusMessage, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "Advanced.ExternalNotificationProviders.Query",
                ex,
                UiErrorCode.NotificationProviderFailed,
                "Failed to query external notification providers.",
                cancellationToken);
        }
    }

    public async Task ValidateAsync(CancellationToken cancellationToken = default)
    {
        var result = await Runtime.NotificationProviderFeatureService.ValidateProviderParametersAsync(
            new NotificationProviderRequest(SelectedProvider, ParametersText),
            cancellationToken);
        await ApplyResultAsync(result, "Advanced.ExternalNotificationProviders.Validate", cancellationToken);
    }

    public async Task SendTestAsync(CancellationToken cancellationToken = default)
    {
        var result = await Runtime.NotificationProviderFeatureService.SendTestAsync(
            new NotificationProviderTestRequest(SelectedProvider, ParametersText, Title, Message),
            cancellationToken);
        await ApplyResultAsync(result, "Advanced.ExternalNotificationProviders.SendTest", cancellationToken);
    }
}
