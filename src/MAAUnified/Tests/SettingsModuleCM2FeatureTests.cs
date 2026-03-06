using System.IO.Compression;
using System.Runtime.CompilerServices;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsModuleCM2FeatureTests
{
    [Fact]
    public async Task UiDiagnosticsService_BundleAlwaysContainsRequiredEntries()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(root);
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());

        var bundlePath = await diagnostics.BuildIssueReportBundleAsync(root);

        Assert.True(File.Exists(bundlePath));
        using var archive = ZipFile.OpenRead(bundlePath);
        Assert.Contains(archive.Entries, entry => entry.FullName == "config/avalonia.json");
        Assert.Contains(archive.Entries, entry => entry.FullName == "debug/config-import-report.json");
        Assert.Contains(archive.Entries, entry => entry.FullName == "debug/avalonia-ui-errors.log");
        Assert.Contains(archive.Entries, entry => entry.FullName == "debug/avalonia-ui-events.log");
        Assert.Contains(archive.Entries, entry => entry.FullName == "debug/avalonia-platform-events.log");
    }

    [Fact]
    public async Task IssueReport_DebugDirectoryAndImageCacheActions_WorkAndExposeStatus()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var openedTargets = new List<string>();

        Task<UiOperationResult> OpenExternalTargetAsync(string target, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            openedTargets.Add(target);
            return Task.FromResult(UiOperationResult.Ok($"Opened target: {target}"));
        }

        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            null,
            OpenExternalTargetAsync);
        await vm.InitializeAsync();

        var imageCacheRoot = Path.Combine(fixture.Root, "cache", "images");
        var nestedDirectory = Path.Combine(imageCacheRoot, "nested");
        Directory.CreateDirectory(nestedDirectory);
        await File.WriteAllTextAsync(Path.Combine(imageCacheRoot, "image-a.png"), "placeholder");
        await File.WriteAllTextAsync(Path.Combine(nestedDirectory, "image-b.jpg"), "placeholder");

        await vm.OpenIssueReportDebugDirectoryAsync();
        Assert.NotEmpty(openedTargets);
        Assert.True(Directory.Exists(openedTargets[^1]));
        Assert.Contains("已打开目录", vm.IssueReportStatusMessage, StringComparison.Ordinal);
        Assert.False(vm.HasIssueReportErrorMessage);

        await vm.ClearIssueReportImageCacheAsync();
        Assert.True(Directory.Exists(imageCacheRoot));
        Assert.Empty(Directory.EnumerateFileSystemEntries(imageCacheRoot, "*", SearchOption.AllDirectories));
        Assert.Contains("图像缓存已清理", vm.IssueReportStatusMessage, StringComparison.Ordinal);
        Assert.False(vm.HasIssueReportErrorMessage);
    }

    [Fact]
    public async Task AchievementAndAbout_ActionsRouteAndExposeData()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var openedTargets = new List<string>();

        Task<UiOperationResult> OpenExternalTargetAsync(string target, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            openedTargets.Add(target);
            return Task.FromResult(UiOperationResult.Ok($"Opened target: {target}"));
        }

        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            null,
            OpenExternalTargetAsync);
        await vm.InitializeAsync();

        Assert.False(string.IsNullOrWhiteSpace(vm.AboutVersionInfo));

        await vm.RefreshAchievementPolicyAsync();
        Assert.Contains("当前策略", vm.AchievementPolicySummary, StringComparison.Ordinal);
        Assert.False(vm.HasAchievementErrorMessage);

        await vm.OpenAchievementGuideAsync();
        await vm.OpenAboutOfficialWebsiteAsync();
        await vm.OpenAboutCommunityAsync();
        await vm.OpenAboutDownloadAsync();

        Assert.True(openedTargets.Count >= 4);
        Assert.Contains("https://maa.plus/docs/manual/introduction/", openedTargets, StringComparer.Ordinal);
        Assert.Contains("https://maa.plus/", openedTargets, StringComparer.Ordinal);
        Assert.Contains("https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions", openedTargets, StringComparer.Ordinal);
        Assert.Contains("https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases", openedTargets, StringComparer.Ordinal);
        Assert.False(vm.HasAboutErrorMessage);

        await vm.CheckAboutAnnouncementAsync();
        Assert.Contains("公告状态", vm.AboutStatusMessage, StringComparison.Ordinal);
        Assert.False(vm.HasAboutErrorMessage);
    }

    private static string CreateTempRoot()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        return root;
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
            root ??= CreateTempRoot();
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
                ShellFeatureService = new ShellFeatureService(connect),
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
