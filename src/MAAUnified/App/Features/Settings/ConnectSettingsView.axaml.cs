using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Runtime.InteropServices;
using System.Text;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform.Storage;
using MAAUnified.Application.Models;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class ConnectSettingsView : UserControl
{
    private Window? _screenshotPreviewWindow;
    private Image? _screenshotPreviewImage;
    private Bitmap? _screenshotPreviewBitmap;

    public ConnectSettingsView()
    {
        InitializeComponent();
    }

    private ConnectionGameSharedStateViewModel? VM => DataContext as ConnectionGameSharedStateViewModel;

    private async void OnSelectAdbPathClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.StorageProvider is not { CanOpen: true } storageProvider)
        {
            return;
        }

        var files = await storageProvider.OpenFilePickerAsync(
            new FilePickerOpenOptions
            {
                AllowMultiple = false,
                Title = "选择 ADB 路径",
            });
        var selected = files.FirstOrDefault();
        if (selected is null)
        {
            return;
        }

        var path = selected.TryGetLocalPath();
        if (!string.IsNullOrWhiteSpace(path))
        {
            vm.AdbPath = path;
            _ = vm.ResolveEffectiveAdbPath(updateStateWhenResolved: true);
        }
    }

    private async void OnScreenshotTestClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        try
        {
            vm.TestLinkInfo = "正在连接模拟器...";
            App.Runtime.LogService.Debug("Screenshot test started: trying connection with current settings.");
            var connectResult = await ConnectWithCurrentSettingsAsync(vm);
            if (!connectResult.Success)
            {
                App.Runtime.LogService.Debug(
                    $"Screenshot test connect failed: code={connectResult.Error?.Code}, message={connectResult.Message}");
                vm.TestLinkInfo = BuildConnectFailureMessage(vm, connectResult);
                return;
            }

            App.Runtime.LogService.Debug("Screenshot test connected, starting 3x GetImage probes.");

            var elapsedSamples = new List<long>(3);
            byte[]? latestImage = null;
            for (var i = 0; i < 3; i++)
            {
                var watch = Stopwatch.StartNew();
                var imageResult = await App.Runtime.CoreBridge.GetImageAsync();
                watch.Stop();
                if (!imageResult.Success || imageResult.Value is null || imageResult.Value.Length == 0)
                {
                    var errorMessage = imageResult.Error?.Message ?? "GetImage failed.";
                    App.Runtime.LogService.Debug(
                        $"Screenshot test GetImage failed on sample {i + 1}: {errorMessage}");
                    vm.TestLinkInfo = BuildBilingualMessage(
                        $"截图测试失败：{errorMessage}",
                        $"Screenshot test failed: {errorMessage}");
                    return;
                }

                latestImage = imageResult.Value;
                App.Runtime.LogService.Debug(
                    $"Screenshot test sample {i + 1} succeeded: {watch.ElapsedMilliseconds} ms");
                elapsedSamples.Add(watch.ElapsedMilliseconds);
            }

            var min = elapsedSamples.Min();
            var max = elapsedSamples.Max();
            var avg = (long)Math.Round(elapsedSamples.Average(), MidpointRounding.AwayFromZero);
            vm.UpdateScreencapCost(min, avg, max, DateTimeOffset.Now);
            vm.TestLinkInfo = vm.ScreencapCost;

            if (latestImage is { Length: > 0 })
            {
                ShowOrUpdateScreenshotPreview(latestImage);
            }
        }
        catch (Exception ex)
        {
            vm.TestLinkInfo = BuildBilingualMessage(
                $"截图测试异常：{ex.Message}",
                $"Screenshot test exception: {ex.Message}");
        }
    }

    private async void OnReplaceAdbClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        try
        {
            var package = ResolveAdbPackageInfo();
            if (package is null)
            {
                vm.TestLinkInfo = BuildBilingualMessage(
                    "当前系统不支持自动替换 ADB。",
                    "Automatic ADB replace is not supported on this platform.");
                return;
            }

            vm.TestLinkInfo = "正在下载 ADB...";

            var baseDirectory = AppContext.BaseDirectory;
            var cacheDirectory = Path.Combine(baseDirectory, "cache", "adb");
            Directory.CreateDirectory(cacheDirectory);

            var packagePath = Path.Combine(cacheDirectory, package.Value.FileName);
            await DownloadFileAsync(package.Value.Url, packagePath);

            var extractDirectory = Path.Combine(cacheDirectory, "platform-tools");
            if (Directory.Exists(extractDirectory))
            {
                Directory.Delete(extractDirectory, recursive: true);
            }

            ZipFile.ExtractToDirectory(packagePath, extractDirectory);

            var adbPath = package.Value.ResolveExtractedAdbPath(extractDirectory);
            if (!File.Exists(adbPath))
            {
                vm.TestLinkInfo = BuildBilingualMessage(
                    "ADB 替换失败：未找到解压后的 adb 可执行文件。",
                    "ADB replace failed: extracted adb executable was not found.");
                return;
            }

            vm.AdbPath = adbPath;
            vm.AdbReplaced = true;
            vm.TestLinkInfo = BuildBilingualMessage(
                $"已替换 ADB：{adbPath}",
                $"ADB replaced: {adbPath}");
        }
        catch (Exception ex)
        {
            vm.TestLinkInfo = BuildBilingualMessage(
                $"替换 ADB 失败：{ex.Message}",
                $"Replace ADB failed: {ex.Message}");
        }
    }

    private static async Task<UiOperationResult> ConnectWithCurrentSettingsAsync(ConnectionGameSharedStateViewModel vm)
    {
        var effectiveAdbPath = vm.ResolveEffectiveAdbPath(updateStateWhenResolved: true);
        var adbPath = string.IsNullOrWhiteSpace(effectiveAdbPath) ? null : effectiveAdbPath;
        var candidates = vm.BuildConnectAddressCandidates(includeConfiguredAddress: true);
        App.Runtime.LogService.Debug(
            $"Settings connect candidates prepared: count={candidates.Count}, config={vm.ConnectConfig}, adb={adbPath ?? "<null>"}");
        UiOperationResult? lastFailure = null;

        foreach (var candidate in candidates)
        {
            App.Runtime.LogService.Debug($"Settings trying connect candidate: {candidate}");
            var result = await App.Runtime.ShellFeatureService.ConnectAsync(candidate, vm.ConnectConfig, adbPath);
            if (result.Success)
            {
                App.Runtime.LogService.Debug($"Settings connect succeeded: {candidate}");
                vm.ConnectAddress = candidate;
                return result;
            }

            App.Runtime.LogService.Debug(
                $"Settings connect failed: {candidate}, code={result.Error?.Code}, message={result.Message}");
            lastFailure = result;
        }

        return lastFailure ?? UiOperationResult.Fail(UiErrorCode.UiOperationFailed, "Connection failed.");
    }

    private static async Task DownloadFileAsync(string url, string targetPath)
    {
        using var httpClient = new HttpClient
        {
            Timeout = TimeSpan.FromMinutes(3),
        };
        using var response = await httpClient.GetAsync(url, HttpCompletionOption.ResponseHeadersRead);
        response.EnsureSuccessStatusCode();
        await using var stream = await response.Content.ReadAsStreamAsync();
        await using var file = File.Create(targetPath);
        await stream.CopyToAsync(file);
    }

    private static AdbPackageInfo? ResolveAdbPackageInfo()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return new AdbPackageInfo(
                Url: "https://dl.google.com/android/repository/platform-tools-latest-windows.zip",
                FileName: "platform-tools-latest-windows.zip",
                AdbRelativePath: Path.Combine("platform-tools", "adb.exe"));
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            return new AdbPackageInfo(
                Url: "https://dl.google.com/android/repository/platform-tools-latest-linux.zip",
                FileName: "platform-tools-latest-linux.zip",
                AdbRelativePath: Path.Combine("platform-tools", "adb"));
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
        {
            return new AdbPackageInfo(
                Url: "https://dl.google.com/android/repository/platform-tools-latest-darwin.zip",
                FileName: "platform-tools-latest-darwin.zip",
                AdbRelativePath: Path.Combine("platform-tools", "adb"));
        }

        return null;
    }

    private static string BuildBilingualMessage(string zh, string en)
    {
        return $"{zh}{Environment.NewLine}{en}";
    }

    private static string BuildConnectFailureMessage(
        ConnectionGameSharedStateViewModel vm,
        UiOperationResult connectResult)
    {
        var builder = new StringBuilder();
        builder.AppendLine(BuildBilingualMessage(
            $"连接失败：{connectResult.Message}",
            $"Connect failed: {connectResult.Message}"));

        if (!string.IsNullOrWhiteSpace(connectResult.Error?.Details))
        {
            builder.AppendLine(BuildBilingualMessage(
                $"错误详情：{connectResult.Error.Details}",
                $"Error details: {connectResult.Error.Details}"));
        }

        var settingsHint = vm.BuildConnectionSettingsHintMessage();
        if (!string.IsNullOrWhiteSpace(settingsHint))
        {
            builder.AppendLine(settingsHint);
        }

        return builder.ToString().Trim();
    }

    private void ShowOrUpdateScreenshotPreview(byte[] imageBytes)
    {
        Bitmap bitmap;
        using (var stream = new MemoryStream(imageBytes, writable: false))
        {
            bitmap = new Bitmap(stream);
        }

        EnsureScreenshotPreviewWindow();

        var previous = _screenshotPreviewBitmap;
        _screenshotPreviewBitmap = bitmap;
        if (_screenshotPreviewImage is not null)
        {
            _screenshotPreviewImage.Source = bitmap;
        }

        previous?.Dispose();

        if (_screenshotPreviewWindow is null)
        {
            return;
        }

        if (_screenshotPreviewWindow.IsVisible)
        {
            _screenshotPreviewWindow.Activate();
            return;
        }

        if (TopLevel.GetTopLevel(this) is Window owner)
        {
            _screenshotPreviewWindow.Show(owner);
            return;
        }

        _screenshotPreviewWindow.Show();
    }

    private void EnsureScreenshotPreviewWindow()
    {
        if (_screenshotPreviewWindow is not null && _screenshotPreviewImage is not null)
        {
            return;
        }

        var image = new Image
        {
            Stretch = Stretch.Uniform,
        };

        var host = new Border
        {
            Background = Brushes.Black,
            Padding = new Thickness(4),
            Child = image,
        };

        var window = new Window
        {
            Title = "截图预览",
            Width = 800,
            Height = 480,
            MinWidth = 480,
            MinHeight = 320,
            WindowStartupLocation = WindowStartupLocation.CenterOwner,
            Content = host,
        };

        window.Closed += (_, _) =>
        {
            _screenshotPreviewBitmap?.Dispose();
            _screenshotPreviewBitmap = null;
            _screenshotPreviewImage = null;
            _screenshotPreviewWindow = null;
        };

        _screenshotPreviewImage = image;
        _screenshotPreviewWindow = window;
    }

    private readonly record struct AdbPackageInfo(string Url, string FileName, string AdbRelativePath)
    {
        public string ResolveExtractedAdbPath(string extractRoot)
        {
            return Path.Combine(extractRoot, AdbRelativePath);
        }
    }
}
