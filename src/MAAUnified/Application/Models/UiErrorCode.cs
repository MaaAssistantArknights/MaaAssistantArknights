namespace MAAUnified.Application.Models;

public static class UiErrorCode
{
    public const string ProfileMissing = "ProfileMissing";
    public const string TaskNotFound = "TaskNotFound";
    public const string TaskParamsMissing = "TaskParamsMissing";
    public const string TaskParamFlushFailed = "TaskParamFlushFailed";
    public const string TaskValidationFailed = "TaskValidationFailed";
    public const string TaskFieldTypeInvalid = "TaskFieldTypeInvalid";
    public const string DelimitedInputParseFailed = "DelimitedInputParseFailed";
    public const string TaskRuntimeCallbackError = "TaskRuntimeCallbackError";

    public const string PostActionLoadFailed = "PostActionLoadFailed";
    public const string PostActionSaveFailed = "PostActionSaveFailed";
    public const string PostActionLegacyParseFailed = "PostActionLegacyParseFailed";
    public const string PostActionSelectionInvalid = "PostActionSelectionInvalid";
    public const string PostActionExecutionFailed = "PostActionExecutionFailed";
    public const string PostActionUnsupported = "PostActionUnsupported";

    public const string InfrastPlanParseFailed = "InfrastPlanParseFailed";
    public const string InfrastPlanOutOfRange = "InfrastPlanOutOfRange";

    public const string RemoteControlInvalidParameters = "RemoteControlInvalidParameters";
    public const string RemoteControlNetworkFailure = "RemoteControlNetworkFailure";
    public const string RemoteControlUnsupported = "RemoteControlUnsupported";

    public const string NotificationProviderInvalidParameters = "NotificationProviderInvalidParameters";
    public const string NotificationProviderNetworkFailure = "NotificationProviderNetworkFailure";
    public const string NotificationProviderUnsupported = "NotificationProviderUnsupported";

    public const string MallCreditFightDowngraded = "MallCreditFightDowngraded";
    public const string VersionUpdateInvalidParameters = "VersionUpdateInvalidParameters";
    public const string AnnouncementStateInvalid = "AnnouncementStateInvalid";
    public const string WebApiPortConflict = "WebApiPortConflict";
    public const string StageManagerInvalidStageCode = "StageManagerInvalidStageCode";
}
