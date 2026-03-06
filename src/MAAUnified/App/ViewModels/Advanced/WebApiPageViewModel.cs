using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Advanced;

public sealed class WebApiPageViewModel : PageViewModelBase
{
    private bool _enabled;
    private string _host = "127.0.0.1";
    private int _port = 51888;
    private string _accessToken = string.Empty;
    private bool _isRunning;

    public WebApiPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
    }

    public bool Enabled
    {
        get => _enabled;
        set => SetProperty(ref _enabled, value);
    }

    public string Host
    {
        get => _host;
        set => SetProperty(ref _host, value?.Trim() ?? string.Empty);
    }

    public int Port
    {
        get => _port;
        set => SetProperty(ref _port, Math.Clamp(value, 1, 65535));
    }

    public string AccessToken
    {
        get => _accessToken;
        set => SetProperty(ref _accessToken, value ?? string.Empty);
    }

    public bool IsRunning
    {
        get => _isRunning;
        private set => SetProperty(ref _isRunning, value);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        var config = await ApplyResultAsync(
            await Runtime.WebApiFeatureService.LoadConfigAsync(cancellationToken),
            "Advanced.WebApi.Load",
            cancellationToken);
        if (config is not null)
        {
            Enabled = config.Enabled;
            Host = config.Host;
            Port = config.Port;
            AccessToken = config.AccessToken;
        }

        await RefreshRunningStatusAsync(cancellationToken);
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        var config = new WebApiConfig(Enabled, Host, Port, AccessToken);
        await ApplyResultAsync(
            await Runtime.WebApiFeatureService.SaveConfigAsync(config, cancellationToken),
            "Advanced.WebApi.Save",
            cancellationToken);
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(
            await Runtime.WebApiFeatureService.StartAsync(cancellationToken),
            "Advanced.WebApi.Start",
            cancellationToken))
        {
            return;
        }

        await RefreshRunningStatusAsync(cancellationToken);
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(
            await Runtime.WebApiFeatureService.StopAsync(cancellationToken),
            "Advanced.WebApi.Stop",
            cancellationToken))
        {
            return;
        }

        await RefreshRunningStatusAsync(cancellationToken);
    }

    public async Task RefreshRunningStatusAsync(CancellationToken cancellationToken = default)
    {
        var statusResult = await Runtime.WebApiFeatureService.GetRunningStatusAsync(cancellationToken);
        var status = await ApplyResultAsync(
            statusResult,
            "Advanced.WebApi.Status",
            cancellationToken);
        if (!statusResult.Success)
        {
            return;
        }

        IsRunning = status;
    }
}
