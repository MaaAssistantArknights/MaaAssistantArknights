using MAAUnified.Application.Models;
using MAAUnified.Application.Services.Localization;

namespace MAAUnified.App.ViewModels.Infrastructure;

public static class DialogTextCatalog
{
    public static bool UseChinese(string? language)
    {
        var normalized = UiLanguageCatalog.Normalize(language);
        return string.Equals(normalized, "zh-cn", StringComparison.OrdinalIgnoreCase)
               || string.Equals(normalized, "zh-tw", StringComparison.OrdinalIgnoreCase);
    }

    public static string Select(string? language, string zh, string en)
    {
        return UseChinese(language) ? zh : en;
    }

    public static string ErrorDialogTitle(string? language)
    {
        return Select(language, "错误提示", "Error");
    }

    public static string ErrorDialogSectionTitle(string? language)
    {
        return Select(language, "错误详情", "Error");
    }

    public static string ErrorDialogCopyButton(string? language)
    {
        return Select(language, "复制", "Copy");
    }

    public static string ErrorDialogIssueReportButton(string? language)
    {
        return Select(language, "问题反馈", "IssueReport");
    }

    public static string ErrorDialogCloseButton(string? language)
    {
        return Select(language, "关闭", "Close");
    }

    public static string ErrorDialogIgnoreButton(string? language)
    {
        return Select(language, "忽略", "Ignore");
    }

    public static string ErrorDialogTimestampLabel(string? language)
    {
        return Select(language, "时间", "TimestampUtc");
    }

    public static string ErrorDialogContextLabel(string? language)
    {
        return Select(language, "上下文", "Context");
    }

    public static string ErrorDialogCodeLabel(string? language)
    {
        return Select(language, "错误码", "Code");
    }

    public static string ErrorDialogMessageLabel(string? language)
    {
        return Select(language, "消息", "Message");
    }

    public static string ErrorDialogDetailsLabel(string? language)
    {
        return Select(language, "详情", "Details");
    }

    public static string ErrorDialogSuggestionLabel(string? language)
    {
        return Select(language, "建议", "Suggestion");
    }

    public static string WarningDialogTitle(string? language)
    {
        return Select(language, "警告", "Warning");
    }

    public static string WarningDialogPrompt(string? language)
    {
        return Select(language, "确认执行此操作？", "Do you want to continue?");
    }

    public static string WarningDialogConfirmButton(string? language)
    {
        return Select(language, "确认", "Confirm");
    }

    public static string WarningDialogCancelButton(string? language)
    {
        return Select(language, "取消", "Cancel");
    }

    public static UiOperationResult LocalizeErrorResult(string? language, UiOperationResult result)
    {
        if (result.Success || result.Error is null)
        {
            return result;
        }

        var localizedMessage = BuildErrorMessage(language, result);
        var details = BuildLocalizedDetails(language, result, localizedMessage);
        return result with
        {
            Message = localizedMessage,
            Error = result.Error with
            {
                Message = localizedMessage,
                Details = details,
            },
        };
    }

    public static string BuildErrorSuggestion(string? language, UiOperationResult result)
    {
        var rawMessage = result.Error?.Message ?? result.Message;
        return result.Error?.Code switch
        {
            UiErrorCode.ConfigurationProfileInvalidName
                when rawMessage.Contains("cannot be empty", StringComparison.OrdinalIgnoreCase)
                => Select(language, "请输入配置名称后再试。", "Enter a profile name and try again."),

            UiErrorCode.ConfigurationProfileInvalidName
                when rawMessage.Contains("control characters", StringComparison.OrdinalIgnoreCase)
                => Select(language, "请移除换行等不可见控制字符后重试。", "Remove control characters such as line breaks and try again."),

            UiErrorCode.ConfigurationProfileAlreadyExists
                => Select(language, "请换一个未使用的配置名称。", "Choose a different unused profile name."),

            UiErrorCode.ConfigurationProfileNotFound or UiErrorCode.ProfileMissing
                => Select(language, "请刷新配置列表，确认目标配置存在后重试。", "Refresh the profile list and make sure the target profile still exists."),

            UiErrorCode.TaskNameMissing
                => Select(language, "请输入任务名称后再试。", "Enter a task name and try again."),

            UiErrorCode.EmulatorPathMissing
                => Select(language, "请先填写模拟器路径。", "Set the emulator path before trying again."),

            UiErrorCode.EmulatorPathNotFound
                => Select(language, "请检查模拟器路径是否存在。", "Check whether the emulator path exists."),

            UiErrorCode.PlatformOperationFailed
                => Select(
                    language,
                    "请检查当前平台能力状态后重试；如仍失败，可复制错误详情并通过 IssueReport 上报。",
                    "Check the platform capability state and try again. If it still fails, copy the error details and submit an IssueReport."),

            _ => Select(
                language,
                "可以先检查当前输入或配置，修正后重试；如仍失败，可复制错误详情并通过 IssueReport 上报。",
                "Check the current input or configuration, fix it, and try again. If it still fails, copy the error details and submit an IssueReport."),
        };
    }

    private static string BuildErrorMessage(string? language, UiOperationResult result)
    {
        var rawMessage = result.Error?.Message ?? result.Message;
        return result.Error?.Code switch
        {
            UiErrorCode.ConfigurationProfileInvalidName
                when rawMessage.Contains("cannot be empty", StringComparison.OrdinalIgnoreCase)
                => Select(language, "配置名称不能为空。", "Profile name cannot be empty."),

            UiErrorCode.ConfigurationProfileInvalidName
                when rawMessage.Contains("control characters", StringComparison.OrdinalIgnoreCase)
                => Select(language, "配置名称不能包含控制字符。", "Profile name cannot contain control characters."),

            UiErrorCode.ConfigurationProfileInvalidName
                => Select(language, "配置名称无效。", "Profile name is invalid."),

            UiErrorCode.ConfigurationProfileAlreadyExists
                => Select(language, "配置名称已存在。", "Profile name already exists."),

            UiErrorCode.ConfigurationProfileNotFound
                => Select(language, "配置不存在。", "Profile does not exist."),

            UiErrorCode.ConfigurationProfileSaveFailed
                => Select(language, "配置保存失败。", "Failed to save configuration profile."),

            UiErrorCode.ProfileMissing
                => Select(language, "当前配置不存在。", "Current profile is missing."),

            UiErrorCode.TaskNameMissing
                => Select(language, "任务名称不能为空。", "Task name cannot be empty."),

            UiErrorCode.EmulatorPathMissing
                => Select(language, "模拟器路径为空。", "Emulator path is missing."),

            UiErrorCode.EmulatorPathNotFound
                => Select(language, "找不到模拟器路径。", "Emulator path was not found."),

            _ => PlatformCapabilityTextMap.FormatErrorCode(
                UseChinese(language) ? "zh-cn" : "en-us",
                result.Error?.Code,
                result.Message),
        };
    }

    private static string BuildLocalizedDetails(string? language, UiOperationResult result, string localizedMessage)
    {
        var details = result.Error?.Details ?? string.Empty;
        if (string.Equals(localizedMessage, result.Message, StringComparison.Ordinal)
            || string.IsNullOrWhiteSpace(result.Message))
        {
            return details;
        }

        var originalMessage = $"{Select(language, "原始消息", "Original message")}: {result.Message}";
        if (string.IsNullOrWhiteSpace(details))
        {
            return originalMessage;
        }

        if (details.Contains(result.Message, StringComparison.Ordinal))
        {
            return details;
        }

        return $"{details}{Environment.NewLine}{originalMessage}";
    }
}
