using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;

namespace MAAUnified.Tests;

public sealed class DialogModuleP1FeatureTests
{
    [Fact]
    public void DialogContracts_ShouldCoverAllSevenDialogTypes()
    {
        var values = Enum.GetValues<DialogType>();
        var expected = new[]
        {
            DialogType.Announcement,
            DialogType.VersionUpdate,
            DialogType.ProcessPicker,
            DialogType.EmulatorPath,
            DialogType.Error,
            DialogType.AchievementList,
            DialogType.Text,
        };

        Assert.Equal(expected.Length, values.Length);
        Assert.Equal(expected, values);
    }

    [Fact]
    public void DialogReturnSemantic_ShouldContainConfirmCancelClose()
    {
        var values = Enum.GetValues<DialogReturnSemantic>();
        var expected = new[]
        {
            DialogReturnSemantic.Confirm,
            DialogReturnSemantic.Cancel,
            DialogReturnSemantic.Close,
        };

        Assert.Equal(expected.Length, values.Length);
        Assert.Equal(expected, values);
    }

    [Fact]
    public void ErrorDialogRequest_ShouldCarryUiOperationResult()
    {
        var result = UiOperationResult.Fail(
            UiErrorCode.PlatformOperationFailed,
            "Synthetic failure for dialog payload verification.",
            "details");
        var request = new ErrorDialogRequest(
            Title: "Error",
            Context: "Dialog.Test",
            Result: result,
            Suggestion: "Try again.");

        Assert.Equal("Dialog.Test", request.Context);
        Assert.Equal(UiErrorCode.PlatformOperationFailed, request.Result.Error?.Code);
        Assert.Equal("details", request.Result.Error?.Details);
        Assert.Equal("Try again.", request.Suggestion);
        Assert.Equal("en-us", request.Language);
    }

    [Fact]
    public void DialogTextCatalog_ShouldFallbackNonChineseDialogsToEnglish()
    {
        Assert.Equal("Warning", DialogTextCatalog.WarningDialogTitle("ja-jp"));
        Assert.Equal("Close", DialogTextCatalog.ErrorDialogCloseButton("ko-kr"));
    }

    [Fact]
    public void DialogTextCatalog_ShouldProvideEditableConfigHints_ForProfileNameErrors()
    {
        var result = UiOperationResult.Fail(
            UiErrorCode.ConfigurationProfileInvalidName,
            "Profile name cannot be empty.");

        var localized = DialogTextCatalog.LocalizeErrorResult("zh-cn", result);

        Assert.Equal("配置名称不能为空。", localized.Message);
        Assert.Equal("请输入配置名称后再试。", DialogTextCatalog.BuildErrorSuggestion("zh-cn", result));
        Assert.Contains("原始消息", localized.Error?.Details ?? string.Empty, StringComparison.Ordinal);
    }

    [Fact]
    public async Task DialogFeatureService_BeginActionComplete_ShouldWriteTraceEvents()
    {
        await using var fixture = DialogFeatureFixture.Create();

        var token = await fixture.Service.BeginDialogAsync(
            DialogType.Text,
            "Dialog.P1.Trace",
            "Trace Title");
        await fixture.Service.RecordDialogActionAsync(token, "return", DialogReturnSemantic.Confirm.ToString());
        await fixture.Service.CompleteDialogAsync(token, DialogReturnSemantic.Confirm, "done");

        var eventLog = await File.ReadAllTextAsync(fixture.Diagnostics.EventLogPath);
        Assert.Contains("[EVENT] [Dialog.Open]", eventLog, StringComparison.Ordinal);
        Assert.Contains($"trace={token.TraceId}; dialog={DialogType.Text}; source=Dialog.P1.Trace; title=Trace Title", eventLog, StringComparison.Ordinal);
        Assert.Contains("[EVENT] [Dialog.Action]", eventLog, StringComparison.Ordinal);
        Assert.Contains("action=return; detail=Confirm", eventLog, StringComparison.Ordinal);
        Assert.Contains("[EVENT] [Dialog.Close]", eventLog, StringComparison.Ordinal);
        Assert.Contains("return=Confirm; summary=done", eventLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task DialogFeatureService_ReportError_ShouldRaiseErrorEvent_AndWriteFailedResult()
    {
        await using var fixture = DialogFeatureFixture.Create();
        DialogErrorRaisedEvent? raised = null;
        fixture.Service.ErrorRaised += (_, e) => raised = e;

        var result = UiOperationResult.Fail(
            UiErrorCode.TaskLoadFailed,
            "Synthetic dialog failure.");
        await fixture.Service.ReportErrorAsync("Dialog.P1.ErrorReport", result);

        Assert.NotNull(raised);
        Assert.Equal("Dialog.P1.ErrorReport", raised!.Context);
        Assert.Equal(UiErrorCode.TaskLoadFailed, raised.Result.Error?.Code);
        Assert.Equal("Synthetic dialog failure.", raised.Result.Message);

        var errorLog = await File.ReadAllTextAsync(fixture.Diagnostics.ErrorLogPath);
        Assert.Contains("[FAILED] [Dialog.P1.ErrorReport]", errorLog, StringComparison.Ordinal);
        Assert.Contains($"code={UiErrorCode.TaskLoadFailed}", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task NoOpAppDialogService_AllDialogs_ShouldReturnCloseSemantic()
    {
        var service = NoOpAppDialogService.Instance;

        var announcement = await service.ShowAnnouncementAsync(
            new AnnouncementDialogRequest("Announcement", "Info", false, false),
            "Dialog.P1.NoOp.Announcement");
        var versionUpdate = await service.ShowVersionUpdateAsync(
            new VersionUpdateDialogRequest("VersionUpdate", "1.0.0", "1.0.1", "Summary", "Body"),
            "Dialog.P1.NoOp.VersionUpdate");
        var processPicker = await service.ShowProcessPickerAsync(
            new ProcessPickerDialogRequest(
                "ProcessPicker",
                [new ProcessPickerItem("process-1", "Process 1", IsPrimary: true)],
                "process-1"),
            "Dialog.P1.NoOp.ProcessPicker");
        var emulatorPath = await service.ShowEmulatorPathAsync(
            new EmulatorPathDialogRequest("EmulatorPath", ["/tmp/emulator"], "/tmp/emulator"),
            "Dialog.P1.NoOp.EmulatorPath");
        var error = await service.ShowErrorAsync(
            new ErrorDialogRequest(
                "Error",
                "Dialog.P1.NoOp.Error",
                UiOperationResult.Fail(UiErrorCode.UiError, "no-op-error")),
            "Dialog.P1.NoOp.Error");
        var achievementList = await service.ShowAchievementListAsync(
            new AchievementListDialogRequest(
                "Achievement",
                [new AchievementListItem("a1", "Title", "Description", "Status")],
                InitialFilter: string.Empty),
            "Dialog.P1.NoOp.Achievement");
        var text = await service.ShowTextAsync(
            new TextDialogRequest("Text", "Prompt", "Default"),
            "Dialog.P1.NoOp.Text");

        AssertAllCloseSemantics(
            announcement.Return,
            announcement.Payload,
            announcement.Summary);
        AssertAllCloseSemantics(
            versionUpdate.Return,
            versionUpdate.Payload,
            versionUpdate.Summary);
        AssertAllCloseSemantics(
            processPicker.Return,
            processPicker.Payload,
            processPicker.Summary);
        AssertAllCloseSemantics(
            emulatorPath.Return,
            emulatorPath.Payload,
            emulatorPath.Summary);
        AssertAllCloseSemantics(
            error.Return,
            error.Payload,
            error.Summary);
        AssertAllCloseSemantics(
            achievementList.Return,
            achievementList.Payload,
            achievementList.Summary);
        AssertAllCloseSemantics(
            text.Return,
            text.Payload,
            text.Summary);
    }

    private static void AssertAllCloseSemantics<TPayload>(
        DialogReturnSemantic semantic,
        TPayload? payload,
        string summary)
    {
        Assert.Equal(DialogReturnSemantic.Close, semantic);
        Assert.Null(payload);
        Assert.Equal("dialog-service-unavailable", summary);
    }

    private sealed class DialogFeatureFixture : IAsyncDisposable
    {
        private DialogFeatureFixture(
            string root,
            UiDiagnosticsService diagnostics,
            DialogFeatureService service)
        {
            Root = root;
            Diagnostics = diagnostics;
            Service = service;
        }

        public string Root { get; }

        public UiDiagnosticsService Diagnostics { get; }

        public DialogFeatureService Service { get; }

        public static DialogFeatureFixture Create()
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);
            var log = new UiLogService();
            var diagnostics = new UiDiagnosticsService(root, log);
            var service = new DialogFeatureService(diagnostics);
            return new DialogFeatureFixture(root, diagnostics, service);
        }

        public ValueTask DisposeAsync()
        {
            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore temp cleanup failures
            }

            return ValueTask.CompletedTask;
        }
    }
}
