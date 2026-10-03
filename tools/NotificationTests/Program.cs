// <copyright file="Program.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using MaaWpfGui.Configuration;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.Views.UserControl.Settings;

namespace NotificationTests;

// Run: dotnet run --project tools/NotificationTests -r win-x64
internal static class Program
{
    private static readonly DateTimeOffset Now = DateTimeOffset.Parse("2026-10-03T12:00:00+08:00");
    private static int _passed;

    [STAThread]
    private static int Main(string[] args)
    {
        try
        {
            Test("channel defaults and original event tags", Defaults);
            Test("empty, invalid, changed and timed-out regex rules", Filters);
            Test("context limits, source isolation and clearing", History);
            Test("overlay eviction cannot delete external context", Overlay);
            Test("typed UI updates and customization survives toggling", Settings);
            Test("migration of all profiles and round-trip persistence", Migration);
            if (args.Length == 2 && args[0] == "--render")
            {
                Render(args[1]);
            }

            Console.WriteLine($"PASS: {_passed} notification regression groups");
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex);
            return 1;
        }
    }

    private static void Test(string name, Action test)
    {
        test();
        ++_passed;
        Console.WriteLine("PASS: " + name);
    }

    private static void Check(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }

    private static NotificationEvent Event(string text, NotificationTag? tag = null,
        int ageMinutes = 0, NotificationSource source = NotificationSource.TaskQueue) =>
        new(Now.AddMinutes(-ageMinutes), source, text, UiLogColor.Info,
            tag.HasValue ? new(tag.Value, "Localized title", text) : null);

    private static void Defaults()
    {
        foreach (var channel in Enum.GetValues<NotificationChannel>())
        {
            var filter = new NotificationFilter();
            var policy = NotificationSettings.Channel.CreateDefault(channel);
            foreach (var tag in Enum.GetValues<NotificationTag>())
            {
                var expected = channel switch {
                    NotificationChannel.SystemNotification => tag != NotificationTag.Stalled,
                    NotificationChannel.External => tag != NotificationTag.Test,
                    _ => true,
                };
                Check(filter.ShouldSend(policy, Event("localized content", tag)) == expected,
                    $"{channel} unexpectedly selected {tag}");
            }

            Check(filter.ShouldSend(policy, Event("ordinary log")) ==
                  (channel is NotificationChannel.Overlay or NotificationChannel.TaskQueueLog),
                $"Unexpected untagged routing: {channel}");
            policy.Enable = false;
            Check(!filter.ShouldSend(policy, Event("completed", NotificationTag.TaskComplete)), "Disabled channel sent an event");
        }
    }

    private static void Filters()
    {
        var filter = new NotificationFilter();
        var policy = new NotificationSettings.Channel { FilterMode = NotificationFilterMode.Whitelist };
        Check(!filter.ShouldSend(policy, Event("anything")), "Empty whitelist must reject events");
        policy.FilterMode = NotificationFilterMode.Blacklist;
        Check(filter.ShouldSend(policy, Event("anything")), "Empty blacklist must allow events");
        policy.FilterList = "<TaskError>";
        Check(!filter.ShouldSend(policy, Event("error", NotificationTag.TaskError)), "Blacklist ignored the event's real tag");
        Check(filter.ShouldSend(policy, Event("complete", NotificationTag.TaskComplete)), "Wrong tag assigned to event");
        policy.FilterMode = NotificationFilterMode.Whitelist;
        foreach (var tag in Enum.GetValues<NotificationTag>())
        {
            policy.FilterList = $"^<{tag}> ";
            Check(NotificationFilter.IsValid(policy.FilterList), "Tag required regex escaping");
            foreach (var candidate in Enum.GetValues<NotificationTag>())
            {
                Check(filter.ShouldSend(policy, Event("localized body", candidate)) == (tag == candidate),
                    "Literal tag selected the wrong notification type");
            }
        }

        policy.FilterList = "first\nsecond";
        Check(filter.ShouldSend(policy, Event("second")), "Multiline OR rule failed");
        policy.FilterList = "changed";
        Check(!filter.ShouldSend(policy, Event("second")), "Cached rule was not invalidated");
        policy.FilterList = "[";
        Check(!NotificationFilter.IsValid(policy.FilterList), "Invalid expression accepted");
        Check(!filter.ShouldSend(policy, Event("anything")), "Invalid whitelist sent an event");
        policy.FilterMode = NotificationFilterMode.Blacklist;
        Check(!filter.ShouldSend(policy, Event("anything")), "Invalid blacklist sent an event");
        policy.FilterList = "(a+)+$";
        var stopwatch = Stopwatch.StartNew();
        Check(!filter.ShouldSend(policy, Event(new string('a', 20000) + "!")), "Timed-out blacklist sent an event");
        Check(stopwatch.Elapsed < TimeSpan.FromSeconds(1), "Regex processing exceeded its timeout budget");
    }

    private static void History()
    {
        var history = new NotificationHistory();
        history.Add(Event("expired", ageMinutes: 120));
        history.Add(Event("older context", ageMinutes: 5));
        history.Add(Event("other source", source: NotificationSource.Copilot));
        history.Add(Event("latest context", ageMinutes: 1));
        var current = Event("original display", NotificationTag.TaskComplete) with {
            Message = new(NotificationTag.TaskComplete, "Completion title", "preset and recovery report")
        };
        history.Add(current);
        var policy = new NotificationSettings.Channel { MaxEntries = 1, TimeMinutes = 60 };
        var bundled = history.Bundle(current, policy);
        Check(bundled.Contains("latest context") && bundled.EndsWith("preset and recovery report"), "Latest context or payload missing");
        Check(!bundled.Contains("older context") && !bundled.Contains("other source"), "Count or source isolation failed");
        Check(!bundled.Contains("original display"), "Trigger body was duplicated");
        policy.MaxEntries = 100;
        Check(!history.Bundle(current, policy).Contains("expired"), "Expired context included");
        policy.TimeMinutes = 0;
        Check(history.Bundle(current, policy).Contains("expired"), "Zero expiry must retain available context");
        policy.MaxEntries = 0;
        Check(history.Bundle(current, policy) == "preset and recovery report", "Zero entries must send only payload");
        history.Clear(NotificationSource.TaskQueue);
        policy.MaxEntries = 100;
        Check(!history.Bundle(current, policy).Contains("latest context"), "Clear retained source history");
        Check(history.Bundle(Event("copilot result", source: NotificationSource.Copilot), policy).Contains("other source"),
            "Clear removed another source's history");

        for (var index = 0; index < 10001; ++index)
        {
            history.Add(Event($"bounded-{index}"));
        }
        policy.MaxEntries = 10000;
        Check(!history.Bundle(current, policy).Contains("bounded-0" + Environment.NewLine), "History exceeded capacity");
    }

    private static void Overlay()
    {
        var history = new NotificationHistory();
        var buffer = new NotificationLogBuffer();
        var old = Event("context retained", ageMinutes: 2);
        var latest = Event("latest", NotificationTag.TaskError);
        history.Add(old);
        history.Add(latest);
        buffer.Add(old.Timestamp, new LogItemViewModel(old.Content, dateFormat: "HH:mm:ss"));
        buffer.Add(latest.Timestamp, new LogItemViewModel(latest.Content, dateFormat: "HH:mm:ss"));
        var policy = new NotificationSettings.Channel { MaxEntries = 1, TimeMinutes = 60 };
        buffer.Trim(policy, Now);
        Check(buffer.Items.Count == 1 && buffer.Items[0].Content == "latest", "Overlay count limit failed");
        Check(history.Bundle(latest, new()).Contains("context retained"), "Overlay eviction deleted external context");
        buffer.Trim(policy, Now.AddMinutes(61));
        Check(buffer.Items.Count == 0, "Overlay expiration requires a new log");
        buffer.Clear();
        buffer.Add(Now, new LogItemViewModel("after clear", dateFormat: "HH:mm:ss"));
        Check(buffer.Items.Count == 1, "Clear desynchronized timestamps");
    }

    private static void Settings()
    {
        var config = NotificationSettings.Channel.CreateDefault(NotificationChannel.External);
        var settings = new NotificationSettingsItem(config, NotificationChannel.External);
        var changes = new HashSet<string?>();
        settings.PropertyChanged += (_, args) => changes.Add(args.PropertyName);
        settings.UseIndependent = true;
        settings.EnableBlacklist = true;
        Check(!settings.EnableWhitelist && settings.ShowFilterList, "Filter modes were not mutually exclusive");
        Check(changes.Contains(nameof(settings.EnableWhitelist)) && changes.Contains(nameof(settings.ShowFilterList)),
            "Derived UI bindings were not notified");
        settings.FilterList = "custom expression";
        settings.UseIndependent = false;
        Check(settings.Effective.FilterList != "custom expression", "Customization mutated defaults");
        settings.UseIndependent = true;
        Check(settings.FilterList == "custom expression", "Re-enabling personalization lost saved customization");
    }

    private static void Migration()
    {
        var options = (JsonSerializerOptions)typeof(ConfigFactory)
            .GetField("_options", BindingFlags.NonPublic | BindingFlags.Static)!.GetValue(null)!;
        const string oldJson = """
            {"Current":"one","Configurations":{
              "one":{"Gui":{
                "RuntimeSettings":{"EnableStallTimeout":false,"StallTimeoutMinutes":12,"StallTimeoutReminderIntervalMinutes":7},
                "ExternalNotification":{"SendWhenComplete":false,"SendWhenError":true,"SendWhenStalled":true,"ShowWhenCompleteWithDetails":false}}},
              "two":{"Gui":{
                "RuntimeSettings":{"StallTimeoutMinutes":0},
                "ExternalNotification":{"SendWhenComplete":false,"SendWhenError":false,"SendWhenStalled":false}}},
              "new":{"Gui":{"Notification":{"StallTimeoutMinutes":19,"External":{
                "UseIndependent":true,"FilterMode":1,"FilterList":"custom","MaxEntries":6}},
                "ExternalNotification":{"SendWhenError":false}}},
              "tag-rules":{"Gui":{"Notification":{
                "Overlay":{"UseIndependent":true,"FilterMode":2,
                  "FilterList":"^\\[TaskError\\]|\\[TaskComplete\\]|\\[Stalled\\]|\\[Test\\]\nordinary.*|\\[Other\\]"},
                "External":{"FilterList":"[TaskError]"},
                "TaskQueueLog":{"UseIndependent":true,"FilterMode":1,"FilterList":"\\[Stalled\\]"}}}}
            }}
            """;
        var root = JsonSerializer.Deserialize<Root>(oldJson, options)!;
        var first = root.Configurations["one"].Gui.Notification;
        Check(!first.EnableStallTimeout && first.StallTimeoutMinutes == 12 && first.ReminderIntervalMinutes == 7,
            "Stall migration lost values");
        Check(first.External.UseIndependent && first.External.MaxEntries == 0, "Details preference lost");
        var filter = new NotificationFilter();
        Check(!filter.ShouldSend(first.External, Event("complete", NotificationTag.TaskComplete)), "Disabled completion re-enabled");
        Check(filter.ShouldSend(first.External, Event("error", NotificationTag.TaskError)), "Error choice lost");
        Check(filter.ShouldSend(first.External, Event("stalled", NotificationTag.Stalled)), "Stall choice lost");
        Check(!root.Configurations["two"].Gui.Notification.External.Enable, "All-off profile re-enabled notifications");
        Check(root.Configurations["two"].Gui.Notification.StallTimeoutMinutes == 0, "Legacy disabled timeout lost");
        Check(root.Configurations["new"].Gui.Notification.External.FilterList == "custom", "Migration overwrote new settings");
        var tagRules = root.Configurations["tag-rules"].Gui.Notification;
        Check(tagRules.Overlay.FilterList == "^<TaskError>|<TaskComplete>|<Stalled>|<Test>\nordinary.*|\\[Other\\]",
            "Tag migration changed unrelated regex rules");
        Check(tagRules.External.FilterList == "[TaskError]", "Tag migration changed a regex character class");
        Check(tagRules.TaskQueueLog.FilterList == "<Stalled>"
              && !filter.ShouldSend(tagRules.TaskQueueLog, Event("stalled", NotificationTag.Stalled)),
            "Inactive profile's blacklist did not migrate");
        foreach (var tag in Enum.GetValues<NotificationTag>())
        {
            Check(filter.ShouldSend(tagRules.Overlay, Event("localized body", tag)), "Migrated tag no longer matches");
        }

        var serialized = JsonSerializer.Serialize(root, options);
        Check(!serialized.Contains("SendWhenError") && !serialized.Contains("StallTimeoutReminderIntervalMinutes"),
            "Retired properties were written back");
        var restored = JsonSerializer.Deserialize<Root>(serialized, options)!;
        Check(restored.Configurations["one"].Gui.Notification.External.FilterList == first.External.FilterList,
            "Notification settings did not survive round trip");
        Check(restored.Configurations["tag-rules"].Gui.Notification.Overlay.FilterList == tagRules.Overlay.FilterList,
            "Tag migration was not idempotent");
        var partial = JsonSerializer.Deserialize<NotificationSettings>("""
            {"Overlay":null,"External":{"FilterList":null,"MaxEntries":-1,"TimeMinutes":114514}}
            """)!;
        Check(partial.Overlay is not null && partial.External.FilterList == string.Empty
              && partial.External.MaxEntries == 0 && partial.External.TimeMinutes == 10080,
            "Partial notification configuration was not normalized");
    }

    private static void Render(string outputDirectory)
    {
        Directory.CreateDirectory(outputDirectory);
        var application = new Application();
        application.ShutdownMode = ShutdownMode.OnExplicitShutdown;
        foreach (var resource in new[] { "Res/Theme.xaml", "Res/Themes/Light.xaml", "Res/Style.xaml", "Res/Localizations/zh-cn.xaml" })
        {
            application.Resources.MergedDictionaries.Add(new ResourceDictionary {
                Source = new Uri($"pack://application:,,,/MAA;component/{resource}")
            });
        }

        var overlay = NotificationSettings.Channel.CreateDefault(NotificationChannel.Overlay);
        var external = NotificationSettings.Channel.CreateDefault(NotificationChannel.External);
        foreach (var independent in new[] { false, true })
        {
            overlay.UseIndependent = external.UseIndependent = independent;
            var context = new PreviewSettings(
                new NotificationSettingsItem(overlay, NotificationChannel.Overlay),
                new NotificationSettingsItem(external, NotificationChannel.External));
            var view = new NotificationSettingsUserControl { DataContext = context };
            var name = independent ? "notification-personalized.png" : "notification-default.png";
            var bitmap = Capture(view, Path.Combine(outputDirectory, name));
            var referencePath = Path.Combine(outputDirectory, "reference.xaml");
            if (File.Exists(referencePath))
            {
                var reference = (FrameworkElement)XamlReader.Parse(File.ReadAllText(referencePath));
                reference.DataContext = context;
                var referenceBitmap = Capture(reference, Path.Combine(outputDirectory, "reference-" + name));
                Check(bitmap.PixelWidth == referenceBitmap.PixelWidth && bitmap.PixelHeight == referenceBitmap.PixelHeight,
                    "Refactored notification settings changed reference layout dimensions");
                var size = bitmap.PixelWidth * bitmap.PixelHeight * 4;
                var actualPixels = new byte[size];
                var referencePixels = new byte[size];
                bitmap.CopyPixels(actualPixels, bitmap.PixelWidth * 4, 0);
                referenceBitmap.CopyPixels(referencePixels, bitmap.PixelWidth * 4, 0);
                var differences = actualPixels.Zip(referencePixels).Count(pair => pair.First != pair.Second);
                Check(differences == 0, $"Notification settings differ from reference: {differences} color bytes");
                Console.WriteLine("PASS: reference PR rendering matches " + name);
            }
        }
    }

    private static RenderTargetBitmap Capture(FrameworkElement view, string path)
    {
        var border = new Border { Width = 574, Padding = new Thickness(20), Background = Brushes.White, Child = view };
        // Host off screen so controls receive their real Loaded lifecycle.
        var window = new Window {
            Left = -32000, Top = -32000, ShowActivated = false, ShowInTaskbar = false,
            Opacity = 0, SizeToContent = SizeToContent.WidthAndHeight, Content = border,
        };
        window.Show();
        Dispatcher.CurrentDispatcher.Invoke(() => { }, DispatcherPriority.ApplicationIdle);
        var frame = new DispatcherFrame();
        var settleTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(600) };
        settleTimer.Tick += (_, _) => {
            settleTimer.Stop();
            frame.Continue = false;
        };
        settleTimer.Start();
        Dispatcher.PushFrame(frame);
        border.Measure(new Size(574, double.PositiveInfinity));
        border.Arrange(new Rect(new Point(), border.DesiredSize));
        border.UpdateLayout();
        var bitmap = new RenderTargetBitmap(574, (int)Math.Ceiling(border.ActualHeight), 96, 96, PixelFormats.Pbgra32);
        bitmap.Render(border);
        var encoder = new PngBitmapEncoder();
        encoder.Frames.Add(BitmapFrame.Create(bitmap));
        using var stream = File.Create(path);
        encoder.Save(stream);
        window.Close();
        Console.WriteLine("Rendered: " + path);
        return bitmap;
    }

    private sealed record PreviewSettings(NotificationSettingsItem Overlay, NotificationSettingsItem External)
    {
        public bool StallTimeoutEnabled { get; set; } = true;

        public int StallTimeoutMinutes { get; set; } = 30;

        public int ReminderIntervalMinutes { get; set; } = 30;

        public bool UseNotify { get; set; } = true;
    }
}
