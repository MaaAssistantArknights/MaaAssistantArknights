using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Threading;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.Services;

public sealed class AvaloniaDesktopAppLifecycleService : IAppLifecycleService
{
    private readonly IClassicDesktopStyleApplicationLifetime _desktopLifetime;
    private readonly ProcessAppLifecycleService _restartService = new();

    public AvaloniaDesktopAppLifecycleService(IClassicDesktopStyleApplicationLifetime desktopLifetime)
    {
        _desktopLifetime = desktopLifetime;
    }

    public bool SupportsExit => true;

    public Task<UiOperationResult> RestartAsync(CancellationToken cancellationToken = default)
        => _restartService.RestartAsync(cancellationToken);

    public async Task<UiOperationResult> ExitAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        try
        {
            if (Dispatcher.UIThread.CheckAccess())
            {
                _desktopLifetime.Shutdown();
            }
            else
            {
                await Dispatcher.UIThread.InvokeAsync(() => _desktopLifetime.Shutdown(), DispatcherPriority.Send, cancellationToken);
            }

            return UiOperationResult.Ok("Application shutdown requested.");
        }
        catch (Exception ex)
        {
            return UiOperationResult.Fail(UiErrorCode.AppExitFailed, $"Failed to exit application: {ex.Message}");
        }
    }
}
