namespace MAAUnified.Application.Models.TaskParams;

public sealed record TaskValidationIssue(
    string Code,
    string Field,
    string Message,
    bool Blocking = true);
