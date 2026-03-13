namespace MAAUnified.Application.Models;

public enum DialogType
{
    Announcement = 0,
    VersionUpdate = 1,
    ProcessPicker = 2,
    EmulatorPath = 3,
    Error = 4,
    AchievementList = 5,
    Text = 6,
}

public enum DialogReturnSemantic
{
    Confirm = 0,
    Cancel = 1,
    Close = 2,
}

public sealed record DialogTraceToken(
    string TraceId,
    DialogType DialogType,
    string SourceScope,
    DateTimeOffset OpenedAtUtc);

public sealed record DialogCompletion<TPayload>(
    DialogReturnSemantic Return,
    TPayload? Payload,
    string Summary);

public sealed record DialogErrorRaisedEvent(
    string Context,
    UiOperationResult Result,
    DateTimeOffset TimestampUtc);

public sealed record AnnouncementDialogRequest(
    string Title,
    string AnnouncementInfo,
    bool DoNotRemindThisAnnouncementAgain,
    bool DoNotShowAnnouncement,
    string ConfirmText = "Confirm",
    string CancelText = "Cancel");

public sealed record AnnouncementDialogPayload(
    string AnnouncementInfo,
    bool DoNotRemindThisAnnouncementAgain,
    bool DoNotShowAnnouncement);

public sealed record VersionUpdateDialogRequest(
    string Title,
    string CurrentVersion,
    string TargetVersion,
    string Summary,
    string Body,
    string ConfirmText = "Confirm",
    string CancelText = "Later");

public sealed record VersionUpdateDialogPayload(
    string Action,
    string CurrentVersion,
    string TargetVersion,
    string Summary);

public sealed record ProcessPickerItem(
    string Id,
    string DisplayName,
    bool IsPrimary);

public sealed record ProcessPickerDialogRequest(
    string Title,
    IReadOnlyList<ProcessPickerItem> Items,
    string? SelectedId,
    string ConfirmText = "Select",
    string CancelText = "Cancel");

public sealed record ProcessPickerDialogPayload(
    string SelectedId,
    string SelectedDisplayName);

public sealed record EmulatorPathDialogRequest(
    string Title,
    IReadOnlyList<string> CandidatePaths,
    string? SelectedPath,
    string ConfirmText = "Confirm",
    string CancelText = "Cancel");

public sealed record EmulatorPathDialogPayload(string SelectedPath);

public sealed record ErrorDialogRequest(
    string Title,
    string Context,
    UiOperationResult Result,
    string? Suggestion = null,
    string ConfirmText = "Close",
    string CancelText = "Ignore",
    string Language = "en-us");

public sealed record ErrorDialogPayload(
    string FormattedErrorText,
    bool Copied,
    bool IssueReportOpened);

public sealed record AchievementListItem(
    string Id,
    string Title,
    string Description,
    string Status);

public sealed record AchievementListDialogRequest(
    string Title,
    IReadOnlyList<AchievementListItem> Items,
    string? InitialFilter,
    string ConfirmText = "Confirm",
    string CancelText = "Cancel");

public sealed record AchievementListDialogPayload(
    string FilterText,
    IReadOnlyList<string> SelectedIds);

public sealed record TextDialogRequest(
    string Title,
    string Prompt,
    string DefaultText,
    bool MultiLine = false,
    string ConfirmText = "Confirm",
    string CancelText = "Cancel");

public sealed record TextDialogPayload(string Text);
