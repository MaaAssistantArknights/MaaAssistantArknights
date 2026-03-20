using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.Services;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.Tests;

public sealed class ShellCloseConfirmationServiceTests
{
    [Fact]
    public async Task ConfirmCloseAsync_NoRunningWork_ShouldBypassDialog()
    {
        var dialogService = new RecordingDialogService(DialogReturnSemantic.Cancel);
        var service = new ShellCloseConfirmationService(dialogService);
        var texts = new RootLocalizationTextMap
        {
            Language = "zh-cn",
        };

        var confirmed = await service.ConfirmCloseAsync(
            texts,
            "zh-cn",
            isTaskRunning: false,
            isVersionUpdateRunning: false,
            "App.Shell.Window.Close.Confirm");

        Assert.True(confirmed);
        Assert.Equal(0, dialogService.WarningConfirmCallCount);
    }

    [Fact]
    public async Task ConfirmCloseAsync_TaskRunning_ShouldUseTaskPrompt()
    {
        var dialogService = new RecordingDialogService(DialogReturnSemantic.Confirm);
        var service = new ShellCloseConfirmationService(dialogService);
        var texts = new RootLocalizationTextMap
        {
            Language = "zh-cn",
        };

        var confirmed = await service.ConfirmCloseAsync(
            texts,
            "zh-cn",
            isTaskRunning: true,
            isVersionUpdateRunning: false,
            "App.Shell.Window.Close.Confirm");

        Assert.True(confirmed);
        Assert.Equal(1, dialogService.WarningConfirmCallCount);
        Assert.Equal("MAA 正在运行任务", dialogService.LastWarningConfirmRequest?.Title);
        Assert.Equal("确定要退出吗？", dialogService.LastWarningConfirmRequest?.Message);
        Assert.Equal("退出", dialogService.LastWarningConfirmRequest?.ConfirmText);
        Assert.Equal("取消", dialogService.LastWarningConfirmRequest?.CancelText);
    }

    [Fact]
    public async Task ConfirmCloseAsync_UpdateRunning_ShouldUseUpdatePromptAndRespectCancel()
    {
        var dialogService = new RecordingDialogService(DialogReturnSemantic.Cancel);
        var service = new ShellCloseConfirmationService(dialogService);
        var texts = new RootLocalizationTextMap
        {
            Language = "en-us",
        };

        var confirmed = await service.ConfirmCloseAsync(
            texts,
            "en-us",
            isTaskRunning: false,
            isVersionUpdateRunning: true,
            "App.Shell.Tray.Restart.Confirm");

        Assert.False(confirmed);
        Assert.Equal(1, dialogService.WarningConfirmCallCount);
        Assert.Equal("MAA is updating", dialogService.LastWarningConfirmRequest?.Title);
        Assert.Equal(
            "An update is in progress. Exiting MAA now may cause resource damage.\nAre you sure you want to exit?",
            dialogService.LastWarningConfirmRequest?.Message);
        Assert.Equal("Exit", dialogService.LastWarningConfirmRequest?.ConfirmText);
        Assert.Equal("Cancel", dialogService.LastWarningConfirmRequest?.CancelText);
    }

    private sealed class RecordingDialogService : IAppDialogService
    {
        private readonly DialogReturnSemantic _warningConfirmReturn;

        public RecordingDialogService(DialogReturnSemantic warningConfirmReturn)
        {
            _warningConfirmReturn = warningConfirmReturn;
        }

        public int WarningConfirmCallCount { get; private set; }

        public WarningConfirmDialogRequest? LastWarningConfirmRequest { get; private set; }

        public Task<DialogCompletion<AnnouncementDialogPayload>> ShowAnnouncementAsync(
            AnnouncementDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<AnnouncementDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<VersionUpdateDialogPayload>> ShowVersionUpdateAsync(
            VersionUpdateDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<VersionUpdateDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<ProcessPickerDialogPayload>> ShowProcessPickerAsync(
            ProcessPickerDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<ProcessPickerDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<EmulatorPathDialogPayload>> ShowEmulatorPathAsync(
            EmulatorPathDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<EmulatorPathDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<ErrorDialogPayload>> ShowErrorAsync(
            ErrorDialogRequest request,
            string sourceScope,
            Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = null,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<ErrorDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<AchievementListDialogPayload>> ShowAchievementListAsync(
            AchievementListDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<AchievementListDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<TextDialogPayload>> ShowTextAsync(
            TextDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<TextDialogPayload>(DialogReturnSemantic.Close, null, "recording"));

        public Task<DialogCompletion<WarningConfirmDialogPayload>> ShowWarningConfirmAsync(
            WarningConfirmDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
        {
            WarningConfirmCallCount++;
            LastWarningConfirmRequest = request;
            return Task.FromResult(new DialogCompletion<WarningConfirmDialogPayload>(
                _warningConfirmReturn,
                _warningConfirmReturn == DialogReturnSemantic.Confirm ? new WarningConfirmDialogPayload(true) : null,
                "recording"));
        }
    }
}
