using System.Runtime.CompilerServices;
using System.IO.Compression;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsModuleCM1FeatureTests
{
    [Fact]
    public async Task VersionUpdate_SaveChannelAndProxy_UseSeparatedPipelines()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.UpdateProxy] = JsonValue.Create("http://127.0.0.1:5000");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.ProxyType] = JsonValue.Create("http");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.VersionUpdateVersionType = "Beta";
        vm.VersionUpdateResourceSource = "Mirror";
        vm.VersionUpdateStartupCheck = false;
        vm.VersionUpdateScheduledCheck = true;
        vm.VersionUpdateProxy = "https://127.0.0.1:7890";
        vm.VersionUpdateProxyType = "https";
        vm.VersionUpdateResourceApi = "https://example.com/api";

        await vm.SaveVersionUpdateChannelAsync();

        Assert.Equal("Beta", ReadGlobalString(fixture.Config, ConfigurationKeys.VersionType));
        Assert.Equal("Mirror", ReadGlobalString(fixture.Config, ConfigurationKeys.UpdateSource));
        Assert.Equal("False", ReadGlobalString(fixture.Config, ConfigurationKeys.StartupUpdateCheck));
        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.UpdateAutoCheck));
        Assert.Equal("http://127.0.0.1:5000", ReadGlobalString(fixture.Config, ConfigurationKeys.UpdateProxy));
        Assert.Contains("通道", vm.VersionUpdateStatusMessage, StringComparison.Ordinal);

        await vm.SaveVersionUpdateProxyAsync();

        Assert.Equal("https://127.0.0.1:7890", ReadGlobalString(fixture.Config, ConfigurationKeys.UpdateProxy));
        Assert.Equal("https", ReadGlobalString(fixture.Config, ConfigurationKeys.ProxyType));
        Assert.Equal("https://example.com/api", ReadGlobalString(fixture.Config, ConfigurationKeys.ResourceApi));
        Assert.Contains("代理", vm.VersionUpdateStatusMessage, StringComparison.Ordinal);
        Assert.False(vm.HasVersionUpdateErrorMessage);
    }

    [Fact]
    public async Task VersionUpdate_CheckForUpdates_UpdatesStatusMessage()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.VersionUpdateVersionType = "Nightly";
        vm.VersionUpdateResourceSource = "Mirror";

        await vm.CheckVersionUpdateAsync();

        Assert.Contains("Checked updates on channel `Nightly` via `Mirror`.", vm.VersionUpdateStatusMessage, StringComparison.Ordinal);
        Assert.False(vm.HasVersionUpdateErrorMessage);
    }

    [Fact]
    public async Task ConfigurationManager_AddMoveSwitchDelete_SyncsWithUnifiedConfiguration()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.Single(vm.ConfigurationProfiles);
        Assert.Equal("Default", vm.ConfigurationProfiles[0]);

        vm.ConfigurationManagerNewProfileName = "Alpha";
        await vm.AddConfigurationProfileAsync();

        Assert.Contains("Alpha", vm.ConfigurationProfiles, StringComparer.OrdinalIgnoreCase);
        Assert.True(fixture.Config.CurrentConfig.Profiles.ContainsKey("Alpha"));

        vm.ConfigurationManagerSelectedProfile = "Alpha";
        await vm.SwitchConfigurationProfileAsync();
        Assert.Equal("Alpha", fixture.Config.CurrentConfig.CurrentProfile);

        await vm.MoveConfigurationProfileUpAsync();
        Assert.Equal("Alpha", fixture.Config.CurrentConfig.Profiles.Keys.First());

        vm.ConfigurationManagerSelectedProfile = "Default";
        await vm.DeleteConfigurationProfileAsync();

        Assert.False(fixture.Config.CurrentConfig.Profiles.ContainsKey("Default"));
        Assert.Single(vm.ConfigurationProfiles);
        Assert.Equal("Alpha", vm.ConfigurationProfiles[0]);
    }

    [Fact]
    public async Task ConfigurationProfileFeatureService_AddProfile_SaveFailure_RollsBack()
    {
        var config = CreateConfigServiceWithStore(new ThrowOnSaveStore());
        var service = new ConfigurationProfileFeatureService(config);

        var result = await service.AddProfileAsync("Alpha");

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ConfigurationProfileSaveFailed, result.Error?.Code);
        Assert.Single(config.CurrentConfig.Profiles);
        Assert.True(config.CurrentConfig.Profiles.ContainsKey("Default"));
        Assert.False(config.CurrentConfig.Profiles.ContainsKey("Alpha"));
    }

    [Fact]
    public async Task VersionUpdateFeatureService_SaveProxy_SaveFailure_RollsBack()
    {
        var config = CreateConfigServiceWithStore(new ThrowOnSaveStore());
        config.CurrentConfig.GlobalValues[ConfigurationKeys.UpdateProxy] = JsonValue.Create("http://127.0.0.1:7000");
        var service = new VersionUpdateFeatureService(config);

        var policy = VersionUpdatePolicy.Default with
        {
            Proxy = "https://127.0.0.1:7890",
            ProxyType = "https",
            ResourceApi = "https://example.com/resource",
        };

        var result = await service.SaveProxyAsync(policy);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.VersionUpdateSaveFailed, result.Error?.Code);
        Assert.Equal("http://127.0.0.1:7000", ReadGlobalString(config, ConfigurationKeys.UpdateProxy));
    }

    [Fact]
    public async Task IssueReport_BundleContainsRequiredArtifacts_AndIssueActionsAreRunnable()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var openedTargets = new List<string>();
        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            openExternalTargetAsync: (target, _) =>
            {
                openedTargets.Add(target);
                return Task.FromResult(UiOperationResult.Ok($"opened:{target}"));
            });
        await vm.InitializeAsync();

        var debugDirectory = Path.GetDirectoryName(fixture.Runtime.DiagnosticsService.EventLogPath)!;
        Directory.CreateDirectory(debugDirectory);
        await File.WriteAllTextAsync(Path.Combine(debugDirectory, "config-import-report.json"), "{}");
        await fixture.Runtime.DiagnosticsService.RecordErrorAsync("test", "synthetic ui error");
        await fixture.Runtime.DiagnosticsService.RecordEventAsync("test", "synthetic ui event");
        await File.WriteAllTextAsync(fixture.Runtime.DiagnosticsService.PlatformEventLogPath, "{\"event\":\"synthetic\"}");

        await vm.BuildIssueReportAsync();
        Assert.False(string.IsNullOrWhiteSpace(vm.IssueReportPath));
        Assert.True(File.Exists(vm.IssueReportPath));

        using (var archive = ZipFile.OpenRead(vm.IssueReportPath))
        {
            Assert.Contains(archive.Entries, e => e.FullName == "config/avalonia.json");
            Assert.Contains(archive.Entries, e => e.FullName == "debug/config-import-report.json");
            Assert.Contains(archive.Entries, e => e.FullName == "debug/avalonia-ui-errors.log");
            Assert.Contains(archive.Entries, e => e.FullName == "debug/avalonia-ui-events.log");
            Assert.Contains(archive.Entries, e => e.FullName == "debug/avalonia-platform-events.log");
        }

        await vm.OpenIssueReportHelpAsync();
        await vm.OpenIssueReportEntryAsync();
        await vm.OpenIssueReportDebugDirectoryAsync();
        Assert.Equal(3, openedTargets.Count);
        Assert.Contains("https://maa.plus/docs/", openedTargets, StringComparer.Ordinal);
        Assert.Contains("issues/new/choose", openedTargets[1], StringComparison.Ordinal);
        Assert.Equal(debugDirectory, openedTargets[2]);
    }

    [Fact]
    public async Task IssueReport_ClearImageCache_RemovesFiles()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            openExternalTargetAsync: (target, _) => Task.FromResult(UiOperationResult.Ok(target)));
        await vm.InitializeAsync();

        var baseDirectory = Directory.GetParent(Path.GetDirectoryName(fixture.Runtime.DiagnosticsService.EventLogPath)!)!.FullName;
        var cacheDirectory = Path.Combine(baseDirectory, "cache", "images");
        Directory.CreateDirectory(Path.Combine(cacheDirectory, "nested"));
        await File.WriteAllTextAsync(Path.Combine(cacheDirectory, "a.tmp"), "1");
        await File.WriteAllTextAsync(Path.Combine(cacheDirectory, "nested", "b.tmp"), "2");

        await vm.ClearIssueReportImageCacheAsync();

        Assert.True(Directory.Exists(cacheDirectory));
        Assert.Empty(Directory.EnumerateFiles(cacheDirectory, "*", SearchOption.AllDirectories));
        Assert.False(vm.HasIssueReportErrorMessage);
    }

    [Fact]
    public async Task AboutAndAchievement_ActionsRouteToFeatureServicesAndLauncher()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var openedTargets = new List<string>();
        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            openExternalTargetAsync: (target, _) =>
            {
                openedTargets.Add(target);
                return Task.FromResult(UiOperationResult.Ok($"opened:{target}"));
            });
        await vm.InitializeAsync();

        Assert.False(string.IsNullOrWhiteSpace(vm.AboutVersionInfo));

        await vm.RefreshAchievementPolicyAsync();
        Assert.Contains("当前策略：", vm.AchievementPolicySummary, StringComparison.Ordinal);
        Assert.False(vm.HasAchievementErrorMessage);

        await vm.OpenAchievementGuideAsync();
        await vm.OpenAboutOfficialWebsiteAsync();
        await vm.OpenAboutCommunityAsync();
        await vm.OpenAboutDownloadAsync();
        await vm.CheckAboutAnnouncementAsync();

        Assert.True(openedTargets.Count >= 4);
        Assert.Contains(openedTargets, target => target.Contains("maa.plus", StringComparison.OrdinalIgnoreCase));
        Assert.False(string.IsNullOrWhiteSpace(vm.AboutStatusMessage));
        Assert.False(vm.HasAboutErrorMessage);
    }

    private static UnifiedConfigurationService CreateConfigServiceWithStore(IUnifiedConfigStore store)
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        return new UnifiedConfigurationService(
            store,
            new GuiNewJsonConfigImporter(),
            new GuiJsonConfigImporter(),
            new UiLogService(),
            root);
    }

    private static string ReadGlobalString(UnifiedConfigurationService config, string key)
    {
        if (!config.CurrentConfig.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return string.Empty;
        }

        if (node is JsonValue value && value.TryGetValue(out string? text) && text is not null)
        {
            return text;
        }

        return node.ToString();
    }

    private sealed class ThrowOnSaveStore : IUnifiedConfigStore
    {
        public string ConfigPath => "throw-on-save";

        public bool Exists() => false;

        public Task<UnifiedConfig?> LoadAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult<UnifiedConfig?>(null);
        }

        public Task SaveAsync(UnifiedConfig config, CancellationToken cancellationToken = default)
        {
            throw new IOException("Simulated save failure.");
        }

        public Task BackupAsync(string suffix, CancellationToken cancellationToken = default)
        {
            return Task.CompletedTask;
        }
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(
            string root,
            MAAUnifiedRuntime runtime,
            UnifiedConfigurationService config,
            bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            Config = config;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public UnifiedConfigurationService Config { get; }

        public static async Task<RuntimeFixture> CreateAsync(string? root = null, bool cleanupRoot = true)
        {
            root ??= Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Path.Combine(root, "config"));

            var log = new UiLogService();
            var diagnostics = new UiDiagnosticsService(root, log);
            var config = new UnifiedConfigurationService(
                new AvaloniaJsonConfigStore(root),
                new GuiNewJsonConfigImporter(),
                new GuiJsonConfigImporter(),
                log,
                root);
            await config.LoadOrBootstrapAsync();

            var bridge = new FakeBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var platform = new PlatformServiceBundle
            {
                TrayService = new NoOpTrayService(),
                NotificationService = new NoOpNotificationService(),
                HotkeyService = new NoOpGlobalHotkeyService(),
                AutostartService = new NoOpAutostartService(),
                FileDialogService = new NoOpFileDialogService(),
                OverlayService = new NoOpOverlayCapabilityService(),
                PostActionExecutorService = new NoOpPostActionExecutorService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var connect = new ConnectFeatureService(session, config);
            var shell = new ShellFeatureService(connect);
            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log),
                SessionService = session,
                Platform = platform,
                LogService = log,
                DiagnosticsService = diagnostics,
                ConnectFeatureService = connect,
                ShellFeatureService = shell,
                TaskQueueFeatureService = new TaskQueueFeatureService(session, config),
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                ConfigurationProfileFeatureService = new ConfigurationProfileFeatureService(config),
                VersionUpdateFeatureService = new VersionUpdateFeatureService(config),
                AchievementFeatureService = new AchievementFeatureService(config),
                AnnouncementFeatureService = new AnnouncementFeatureService(config),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, runtime, config, cleanupRoot);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
            if (!_cleanupRoot)
            {
                return;
            }

            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    private sealed class FakeBridge : IMaaCoreBridge
    {
        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));
        }

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<int>.Ok(1));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));
        }

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));
        }

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));
        }

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync()
        {
            return ValueTask.CompletedTask;
        }
    }
}
