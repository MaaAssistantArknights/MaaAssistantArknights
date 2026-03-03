namespace MAAUnified.Application.Models;

public static class UiErrorCode
{
    public const string ProfileMissing = "ProfileMissing";
    public const string TaskNotFound = "TaskNotFound";
    public const string TaskParamsMissing = "TaskParamsMissing";
    public const string TaskParamFlushFailed = "TaskParamFlushFailed";
    public const string TaskValidationFailed = "TaskValidationFailed";
    public const string TaskRuntimeCallbackError = "TaskRuntimeCallbackError";

    public const string PostActionLoadFailed = "PostActionLoadFailed";
    public const string PostActionSaveFailed = "PostActionSaveFailed";
    public const string PostActionLegacyParseFailed = "PostActionLegacyParseFailed";
    public const string PostActionSelectionInvalid = "PostActionSelectionInvalid";
    public const string PostActionExecutionFailed = "PostActionExecutionFailed";
    public const string PostActionUnsupported = "PostActionUnsupported";

    public const string InfrastPlanParseFailed = "InfrastPlanParseFailed";
    public const string InfrastPlanOutOfRange = "InfrastPlanOutOfRange";
}
