using System.Collections.ObjectModel;
using System.ComponentModel;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.Advanced;

public sealed class OverlayAdvancedPageViewModel : PageViewModelBase
{
    private OverlayTarget? _selectedTarget;
    private bool _visible;
    private string _capabilitySummary = string.Empty;
    private readonly OverlaySharedState _overlaySharedState;

    public OverlayAdvancedPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Targets = new ObservableCollection<OverlayTarget>();
        _overlaySharedState = OverlaySharedStateRegistry.Get(runtime);
        _visible = _overlaySharedState.Visible;
        _overlaySharedState.PropertyChanged += OnOverlaySharedStateChanged;
    }

    public ObservableCollection<OverlayTarget> Targets { get; }

    public OverlayTarget? SelectedTarget
    {
        get => _selectedTarget;
        set
        {
            if (!SetProperty(ref _selectedTarget, value))
            {
                return;
            }

            _overlaySharedState.SelectedTargetId = value?.Id ?? "preview";
        }
    }

    public bool Visible
    {
        get => _visible;
        set
        {
            if (!SetProperty(ref _visible, value))
            {
                return;
            }

            _overlaySharedState.Visible = value;
        }
    }

    public string CapabilitySummary
    {
        get => _capabilitySummary;
        private set => SetProperty(ref _capabilitySummary, value);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await ReloadTargetsAsync(cancellationToken);
    }

    public async Task ReloadTargetsAsync(CancellationToken cancellationToken = default)
    {
        var targets = await ApplyResultAsync(
            await Runtime.OverlayFeatureService.GetOverlayTargetsAsync(cancellationToken),
            "Advanced.Overlay.QueryTargets",
            cancellationToken);
        if (targets is null)
        {
            return;
        }

        Targets.Clear();
        foreach (var target in targets)
        {
            Targets.Add(target);
        }

        SelectedTarget = OverlayTargetPersistence.ResolveSelection(
            Targets,
            Runtime.ConfigurationService.CurrentConfig.GlobalValues,
            _overlaySharedState.SelectedTargetId)
            ?? Targets.FirstOrDefault(t => t.IsPrimary)
            ?? Targets.FirstOrDefault();

        var snapshot = await ApplyResultAsync(
            await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken),
            "Advanced.Overlay.QueryCapability",
            cancellationToken);
        if (snapshot is not null)
        {
            var capability = snapshot.Overlay;
            CapabilitySummary = $"provider={capability.Provider}; supported={capability.Supported}; fallback={capability.FallbackMode ?? "none"}";
        }
    }

    public async Task ToggleOverlayAsync(CancellationToken cancellationToken = default)
    {
        if (!await SelectAndPersistTargetAsync(SelectedTarget?.Id ?? "preview", cancellationToken))
        {
            return;
        }

        var requestedVisible = !Visible;
        var toggleResult = await Runtime.OverlayFeatureService.ToggleOverlayVisibilityAsync(requestedVisible, cancellationToken);
        if (!await ApplyResultAsync(
            toggleResult,
            "Advanced.Overlay.ToggleVisible",
            cancellationToken))
        {
            return;
        }

        Visible = requestedVisible;
    }

    private async Task<bool> SelectAndPersistTargetAsync(string targetId, CancellationToken cancellationToken)
    {
        var selectResult = await ApplyResultAsync(
            await Runtime.OverlayFeatureService.SelectOverlayTargetAsync(targetId, cancellationToken),
            "Advanced.Overlay.SelectTarget",
            cancellationToken);
        if (!selectResult)
        {
            return false;
        }

        SelectedTarget = Targets.FirstOrDefault(target => string.Equals(target.Id, targetId, StringComparison.Ordinal))
                         ?? SelectedTarget
                         ?? new OverlayTarget(targetId, targetId, false);

        await PersistSelectedTargetBestEffortAsync(SelectedTarget, cancellationToken);
        return true;
    }

    private async Task PersistSelectedTargetBestEffortAsync(OverlayTarget? selectedTarget, CancellationToken cancellationToken)
    {
        if (selectedTarget is null)
        {
            return;
        }

        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(
            Compat.Constants.ConfigurationKeys.OverlayTarget,
            OverlayTargetPersistence.Serialize(selectedTarget),
            cancellationToken);
        if (saveResult.Success)
        {
            await RecordEventAsync("Advanced.Overlay.SaveTarget", saveResult.Message, cancellationToken);
            return;
        }

        LastErrorMessage = saveResult.Message;
        await RecordFailedResultAsync("Advanced.Overlay.SaveTarget", saveResult, cancellationToken);
    }

    private void OnOverlaySharedStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (string.Equals(e.PropertyName, nameof(OverlaySharedState.Visible), StringComparison.Ordinal))
        {
            if (_visible == _overlaySharedState.Visible)
            {
                return;
            }

            _visible = _overlaySharedState.Visible;
            OnPropertyChanged(nameof(Visible));
            return;
        }

        if (!string.Equals(e.PropertyName, nameof(OverlaySharedState.SelectedTargetId), StringComparison.Ordinal)
            || Targets.Count == 0)
        {
            return;
        }

        var selected = Targets.FirstOrDefault(target =>
            string.Equals(target.Id, _overlaySharedState.SelectedTargetId, StringComparison.Ordinal));
        if (selected is null || Equals(_selectedTarget, selected))
        {
            return;
        }

        _selectedTarget = selected;
        OnPropertyChanged(nameof(SelectedTarget));
    }
}
