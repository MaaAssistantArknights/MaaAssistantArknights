using MAAUnified.Application.Models;
using MAAUnified.Platform;

namespace MAAUnified.Application.Services;

public sealed record PostActionPromptRequest(
    PostActionType Action,
    TimeSpan Countdown,
    string? Language = null);

public interface IPostActionPromptService
{
    Task<UiOperationResult> ConfirmPowerActionAsync(
        PostActionPromptRequest request,
        CancellationToken cancellationToken = default);
}

public sealed class NoOpPostActionPromptService : IPostActionPromptService
{
    public Task<UiOperationResult> ConfirmPowerActionAsync(
        PostActionPromptRequest request,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(UiOperationResult.Cancelled("Post action confirmation service is unavailable."));
    }
}
