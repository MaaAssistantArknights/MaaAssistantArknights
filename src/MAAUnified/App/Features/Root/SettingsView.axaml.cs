using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Threading;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Root;

public partial class SettingsView : UserControl
{
    private readonly Dictionary<string, Control> _sectionAnchors = new(StringComparer.OrdinalIgnoreCase);
    private ScrollViewer? _sectionScrollViewer;
    private StackPanel? _sectionContentPanel;
    private bool _suppressSectionSelectionChanged;
    private bool _suppressSectionScrollChanged;

    public SettingsView()
    {
        InitializeComponent();
        AttachedToVisualTree += (_, _) =>
        {
            _sectionScrollViewer = this.FindControl<ScrollViewer>("SectionScrollViewer");
            _sectionContentPanel = this.FindControl<StackPanel>("SectionContentPanel");
            RebuildSectionAnchors();
            Dispatcher.UIThread.Post(ScrollToSelectedSection, DispatcherPriority.Loaded);
        };
        DataContextChanged += (_, _) =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                RebuildSectionAnchors();
                ScrollToSelectedSection();
            }, DispatcherPriority.Loaded);
        };
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private void OnSectionSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (_suppressSectionSelectionChanged)
        {
            return;
        }

        Dispatcher.UIThread.Post(ScrollToSelectedSection, DispatcherPriority.Background);
    }

    private void OnSectionScrollChanged(object? sender, ScrollChangedEventArgs e)
    {
        if (_suppressSectionScrollChanged)
        {
            return;
        }

        UpdateSelectedSectionFromScroll();
    }

    private void RebuildSectionAnchors()
    {
        _sectionAnchors.Clear();
        RegisterSectionAnchor("ConfigurationManager", "SectionConfigurationManager");
        RegisterSectionAnchor("Timer", "SectionTimer");
        RegisterSectionAnchor("Performance", "SectionPerformance");
        RegisterSectionAnchor("Game", "SectionGame");
        RegisterSectionAnchor("Connect", "SectionConnect");
        RegisterSectionAnchor("Start", "SectionStart");
        RegisterSectionAnchor("RemoteControl", "SectionRemoteControl");
        RegisterSectionAnchor("GUI", "SectionGui");
        RegisterSectionAnchor("Background", "SectionBackground");
        RegisterSectionAnchor("ExternalNotification", "SectionExternalNotification");
        RegisterSectionAnchor("HotKey", "SectionHotKey");
        RegisterSectionAnchor("Achievement", "SectionAchievement");
        RegisterSectionAnchor("VersionUpdate", "SectionVersionUpdate");
        RegisterSectionAnchor("IssueReport", "SectionIssueReport");
        RegisterSectionAnchor("About", "SectionAbout");
    }

    private void RegisterSectionAnchor(string key, string controlName)
    {
        if (this.FindControl<Control>(controlName) is { } anchor)
        {
            _sectionAnchors[key] = anchor;
        }
    }

    private void ScrollToSelectedSection()
    {
        var vm = VM;
        var scrollViewer = _sectionScrollViewer;
        var contentPanel = _sectionContentPanel;
        if (vm?.SelectedSection is null || scrollViewer is null || contentPanel is null)
        {
            return;
        }

        if (!_sectionAnchors.TryGetValue(vm.SelectedSection.Key, out var anchor))
        {
            return;
        }

        var point = anchor.TranslatePoint(default, contentPanel);
        if (!point.HasValue)
        {
            return;
        }

        SuppressSectionScrollChangedOnce();
        scrollViewer.Offset = new Vector(
            scrollViewer.Offset.X,
            Math.Max(point.Value.Y, 0d));
    }

    private void UpdateSelectedSectionFromScroll()
    {
        var vm = VM;
        var scrollViewer = _sectionScrollViewer;
        var contentPanel = _sectionContentPanel;
        if (vm is null || scrollViewer is null || contentPanel is null || _sectionAnchors.Count == 0)
        {
            return;
        }

        var threshold = scrollViewer.Offset.Y + Math.Max(24d, scrollViewer.Viewport.Height * 0.25d);
        SettingsSectionViewModel? candidate = null;
        var candidateTop = double.MinValue;

        foreach (var section in vm.Sections)
        {
            if (!_sectionAnchors.TryGetValue(section.Key, out var anchor))
            {
                continue;
            }

            var point = anchor.TranslatePoint(default, contentPanel);
            if (!point.HasValue)
            {
                continue;
            }

            var top = point.Value.Y;
            if (top <= threshold && top >= candidateTop)
            {
                candidate = section;
                candidateTop = top;
            }
        }

        candidate ??= vm.Sections.Count > 0 ? vm.Sections[0] : null;
        if (candidate is null || ReferenceEquals(candidate, vm.SelectedSection))
        {
            return;
        }

        SuppressSectionSelectionChangedOnce();
        vm.SelectedSection = candidate;
    }

    private void SuppressSectionSelectionChangedOnce()
    {
        _suppressSectionSelectionChanged = true;
        Dispatcher.UIThread.Post(
            () => _suppressSectionSelectionChanged = false,
            DispatcherPriority.Background);
    }

    private void SuppressSectionScrollChangedOnce()
    {
        _suppressSectionScrollChanged = true;
        Dispatcher.UIThread.Post(
            () => _suppressSectionScrollChanged = false,
            DispatcherPriority.Background);
    }
}
