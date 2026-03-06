using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public abstract class TypedTaskModuleViewModelBase<TDto> : ObservableObject
    where TDto : class, new()
{
    private bool _isTaskBound;
    private bool _isDirty;
    private bool _isApplyingDto;
    private int _boundTaskIndex = -1;
    private string _statusMessage = string.Empty;
    private string _lastErrorMessage = string.Empty;

    protected TypedTaskModuleViewModelBase(MAAUnifiedRuntime runtime, LocalizedTextMap texts, string scope)
    {
        Runtime = runtime;
        Texts = texts;
        Scope = scope;
    }

    protected MAAUnifiedRuntime Runtime { get; }

    public LocalizedTextMap Texts { get; }

    protected string Scope { get; }

    protected int BoundTaskIndex => _boundTaskIndex;

    protected bool IsApplyingDto => _isApplyingDto;

    public ObservableCollection<string> ValidationMessages { get; } = [];

    public bool IsTaskBound
    {
        get => _isTaskBound;
        private set => SetProperty(ref _isTaskBound, value);
    }

    public bool IsDirty
    {
        get => _isDirty;
        protected set => SetProperty(ref _isDirty, value);
    }

    public string StatusMessage
    {
        get => _statusMessage;
        protected set => SetProperty(ref _statusMessage, value);
    }

    public string LastErrorMessage
    {
        get => _lastErrorMessage;
        protected set => SetProperty(ref _lastErrorMessage, value);
    }

    public bool HasValidationIssues => ValidationMessages.Count > 0;

    public async Task BindAsync(int index, CancellationToken cancellationToken = default)
    {
        _boundTaskIndex = index;

        var loaded = await LoadDtoAsync(index, cancellationToken);
        if (!loaded.Success || loaded.Value is null)
        {
            IsTaskBound = false;
            _boundTaskIndex = -1;
            LastErrorMessage = loaded.Message;
            await Runtime.DiagnosticsService.RecordFailedResultAsync(
                $"{Scope}.Load",
                UiOperationResult.Fail(loaded.Error?.Code ?? UiErrorCode.TaskLoadFailed, loaded.Message, loaded.Error?.Details),
                cancellationToken);
            return;
        }

        _isApplyingDto = true;
        try
        {
            ApplyDto(loaded.Value);
        }
        finally
        {
            _isApplyingDto = false;
        }

        ValidationMessages.Clear();
        OnPropertyChanged(nameof(HasValidationIssues));
        IsTaskBound = true;
        IsDirty = false;
        LastErrorMessage = string.Empty;
        StatusMessage = loaded.Message;
    }

    public virtual void ClearBinding()
    {
        _boundTaskIndex = -1;
        IsTaskBound = false;
        IsDirty = false;
        ValidationMessages.Clear();
        OnPropertyChanged(nameof(HasValidationIssues));
    }

    public Task<bool> SaveIfDirtyAsync(CancellationToken cancellationToken = default)
    {
        if (!IsTaskBound || !IsDirty)
        {
            return Task.FromResult(true);
        }

        return SaveAsync(cancellationToken);
    }

    public async Task<bool> SaveAsync(CancellationToken cancellationToken = default)
    {
        if (!IsTaskBound || _boundTaskIndex < 0)
        {
            return true;
        }

        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            LastErrorMessage = "Current profile is missing.";
            await Runtime.DiagnosticsService.RecordErrorAsync($"{Scope}.Save", LastErrorMessage, cancellationToken: cancellationToken);
            return false;
        }

        var preValidationIssues = ValidateBeforeSave();
        if (preValidationIssues.Any(i => i.Blocking))
        {
            ApplyValidationIssues(preValidationIssues);
            var preMessage = BuildValidationSummary(preValidationIssues.Where(i => i.Blocking));
            LastErrorMessage = preMessage;
            await Runtime.DiagnosticsService.RecordFailedResultAsync(
                $"{Scope}.Validate",
                UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, preMessage, BuildValidationDetails(preValidationIssues)),
                cancellationToken);
            return false;
        }

        var dto = BuildDto();
        var compiled = CompileDto(dto, profile, Runtime.ConfigurationService.CurrentConfig);
        var allIssues = preValidationIssues.Count == 0
            ? compiled.Issues
            : preValidationIssues.Concat(compiled.Issues).ToList();
        ApplyValidationIssues(allIssues);
        if (allIssues.Any(i => i.Blocking))
        {
            var message = BuildValidationSummary(allIssues.Where(i => i.Blocking));
            LastErrorMessage = message;
            await Runtime.DiagnosticsService.RecordFailedResultAsync(
                $"{Scope}.Validate",
                UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, message, BuildValidationDetails(allIssues)),
                cancellationToken);
            return false;
        }

        var save = await SaveDtoAsync(_boundTaskIndex, dto, cancellationToken);
        if (!save.Success)
        {
            LastErrorMessage = save.Message;
            await Runtime.DiagnosticsService.RecordFailedResultAsync($"{Scope}.Save", save, cancellationToken);
            return false;
        }

        IsDirty = false;
        LastErrorMessage = string.Empty;
        StatusMessage = save.Message;
        return true;
    }

    protected bool SetTrackedProperty<T>(ref T backingField, T value, [CallerMemberName] string? propertyName = null)
    {
        if (!SetProperty(ref backingField, value, propertyName))
        {
            return false;
        }

        if (!_isApplyingDto)
        {
            IsDirty = true;
        }

        return true;
    }

    protected void MarkDirty()
    {
        if (!_isApplyingDto)
        {
            IsDirty = true;
        }
    }

    protected virtual void ApplyValidationIssues(IReadOnlyList<TaskValidationIssue> issues)
    {
        ValidationMessages.Clear();
        foreach (var issue in issues)
        {
            var level = issue.Blocking
                ? Texts.GetOrDefault("Common.ErrorPrefix", "Error")
                : Texts.GetOrDefault("Common.WarningPrefix", "Warning");
            var localizedMessage = Texts.GetOrDefault($"Issue.{issue.Code}", issue.Message);
            ValidationMessages.Add($"{level} [{issue.Field}] {localizedMessage}");
        }

        OnPropertyChanged(nameof(HasValidationIssues));
    }

    protected static string BuildValidationSummary(IEnumerable<TaskValidationIssue> issues)
    {
        return string.Join("; ", issues.Select(i => $"{i.Field}: {i.Message}"));
    }

    protected static string BuildValidationDetails(IEnumerable<TaskValidationIssue> issues)
    {
        return string.Join(" | ", issues.Select(i => $"{i.Code}:{i.Field}:{i.Message}"));
    }

    protected virtual IReadOnlyList<TaskValidationIssue> ValidateBeforeSave()
    {
        return [];
    }

    protected abstract Task<UiOperationResult<TDto>> LoadDtoAsync(int index, CancellationToken cancellationToken);

    protected abstract Task<UiOperationResult> SaveDtoAsync(int index, TDto dto, CancellationToken cancellationToken);

    protected abstract TaskCompileOutput CompileDto(TDto dto, UnifiedProfile profile, UnifiedConfig config);

    protected abstract void ApplyDto(TDto dto);

    protected abstract TDto BuildDto();
}
