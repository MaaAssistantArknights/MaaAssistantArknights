namespace MAAUnified.Platform;

public static class PlatformErrorCodes
{
    public const string TrayUnsupported = "TrayUnsupported";
    public const string TrayFallback = "TrayFallback";
    public const string TrayInitFailed = "TrayInitFailed";
    public const string TrayMenuDispatchFailed = "TrayMenuDispatchFailed";
    public const string TrayNotInitialized = "TrayNotInitialized";

    public const string NotificationUnsupported = "NotificationUnsupported";
    public const string NotificationFallback = "NotificationFallback";
    public const string NotificationSendFailed = "NotificationSendFailed";

    public const string HotkeyUnsupported = "HotkeyUnsupported";
    public const string HotkeyFallback = "HotkeyFallback";
    public const string HotkeyNameMissing = "HotkeyNameMissing";
    public const string HotkeyInvalidGesture = "HotkeyInvalidGesture";
    public const string HotkeyConflict = "HotkeyConflict";
    public const string HotkeyNotFound = "HotkeyNotFound";
    public const string HotkeyPermissionDenied = "HotkeyPermissionDenied";
    public const string HotkeyHookStartFailed = "HotkeyHookStartFailed";
    public const string HotkeyTriggerDispatchFailed = "HotkeyTriggerDispatchFailed";

    public const string AutostartUnsupported = "AutostartUnsupported";
    public const string AutostartQueryFailed = "AutostartQueryFailed";
    public const string AutostartSetFailed = "AutostartSetFailed";
    public const string AutostartVerificationFailed = "AutostartVerificationFailed";
    public const string AutostartExecutableMissing = "AutostartExecutableMissing";

    public const string OverlayUnsupported = "OverlayUnsupported";
    public const string OverlayPreviewMode = "OverlayPreviewMode";
    public const string OverlayTargetInvalid = "OverlayTargetInvalid";
    public const string OverlayTargetGone = "OverlayTargetGone";
    public const string OverlayQueryFailed = "OverlayQueryFailed";
    public const string OverlayHostNotBound = "OverlayHostNotBound";
    public const string OverlayAttachFailed = "OverlayAttachFailed";

    public const string PostActionUnsupported = "PostActionUnsupported";
    public const string PostActionExecutionFailed = "PostActionExecutionFailed";
    public const string PostActionPowerActionsDisabled = "PostActionPowerActionsDisabled";
}
