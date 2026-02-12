// <copyright file="AsstProxy.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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

using System;
using System.Buffers;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using HandyControl.Data;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.Services.Web;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using ObservableCollections;
using Serilog;
using Stylet;
using Windows.Win32;
using static MaaWpfGui.Helper.Instances.Data;
using AsstHandle = nint;
using AsstInstanceOptionKey = System.Int32;

using AsstTaskId = System.Int32;

using FightTask = MaaWpfGui.ViewModels.UserControl.TaskQueue.FightSettingsUserControlModel;
using ToastNotification = MaaWpfGui.Helper.ToastNotification;

namespace MaaWpfGui.Main;

/// <summary>
/// MaaCore 代理类。
/// </summary>
public class AsstProxy
{
    private readonly RunningState _runningState;
    private static readonly ILogger _logger = Log.ForContext<AsstProxy>();

    public DateTimeOffset StartTaskTime { get; set; }

    private static unsafe byte[] EncodeNullTerminatedUtf8(string s)
    {
        var enc = Encoding.UTF8.GetEncoder();
        fixed (char* c = s)
        {
            var len = enc.GetByteCount(c, s.Length, true);
            var buf = new byte[len + 1];
            fixed (byte* ptr = buf)
            {
                enc.Convert(c, s.Length, ptr, len, true, out _, out _, out _);
            }

            return buf;
        }
    }

    private static unsafe bool AsstSetUserDir(string dirname)
    {
        fixed (byte* ptr = EncodeNullTerminatedUtf8(dirname))
        {
            _logger.Information("AsstSetUserDir dirame: {Dirname}", dirname);
            var ret = MaaService.AsstSetUserDir(ptr);
            _logger.Information("AsstSetUserDir ret: {Ret}", ret);
            return ret;
        }
    }

    private static unsafe bool AsstLoadResource(string dirname)
    {
        fixed (byte* ptr = EncodeNullTerminatedUtf8(dirname))
        {
            _logger.Information("AsstLoadResource dirname: {Dirname}", dirname);
            var ret = MaaService.AsstLoadResource(ptr);
            _logger.Information("AsstLoadResource ret: {Ret}", ret);
            return ret;
        }
    }

    private static unsafe bool AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, string value)
    {
        fixed (byte* ptr1 = EncodeNullTerminatedUtf8(value))
        {
            return MaaService.AsstSetInstanceOption(handle, key, ptr1);
        }
    }

    private static unsafe bool AsstConnect(AsstHandle handle, string adbPath, string address, string config)
    {
        _logger.Information("handle: {Handle}, adbPath: {AdbPath}, address: {Address}, config: {Config}", (long)handle, adbPath, address, config);

        fixed (byte* ptr1 = EncodeNullTerminatedUtf8(adbPath),
            ptr2 = EncodeNullTerminatedUtf8(address),
            ptr3 = EncodeNullTerminatedUtf8(config))
        {
            bool ret = MaaService.AsstConnect(handle, ptr1, ptr2, ptr3);
            _logger.Information("handle: {Handle}, adbPath: {AdbPath}, address: {Address}, config: {Config}, return: {Ret}", (long)handle, adbPath, address, config, ret);
            return ret;
        }
    }

    private static unsafe void AsstSetConnectionExtras(string name, string extras)
    {
        _logger.Information("name: {Name}, extras: {Extras}", name, extras);

        fixed (byte* ptr1 = EncodeNullTerminatedUtf8(name),
            ptr2 = EncodeNullTerminatedUtf8(extras))
        {
            MaaService.AsstSetConnectionExtras(ptr1, ptr2);
        }
    }

    private static bool AsstAttachWindow(AsstHandle handle, IntPtr hwnd, ulong screencapMethod, ulong mouseMethod, ulong keyboardMethod)
    {
        _logger.Information("handle: {Handle}, hwnd: {Hwnd}, screencapMethod: {ScreencapMethod}, mouseMethod: {MouseMethod}, keyboardMethod: {KeyboardMethod}",
            (long)handle, hwnd, screencapMethod, mouseMethod, keyboardMethod);

        bool ret = MaaService.AsstAttachWindow(handle, hwnd, screencapMethod, mouseMethod, keyboardMethod);
        _logger.Information("handle: {Handle}, hwnd: {Hwnd}, return: {Ret}", (long)handle, hwnd, ret);
        return ret;
    }

    private static void AsstSetConnectionExtrasMuMu12(string extras)
    {
        AsstSetConnectionExtras("MuMuEmulator12", extras);
    }

    private static void AsstSetConnectionExtrasLdPlayer(string extras)
    {
        AsstSetConnectionExtras("LDPlayer", extras);
    }

    private static unsafe AsstTaskId AsstAppendTask(AsstHandle handle, string type, string taskParams)
    {
        fixed (byte* ptr1 = EncodeNullTerminatedUtf8(type),
            ptr2 = EncodeNullTerminatedUtf8(taskParams))
        {
            return MaaService.AsstAppendTask(handle, ptr1, ptr2);
        }
    }

    private static unsafe bool AsstSetTaskParams(AsstHandle handle, AsstTaskId id, string taskParams)
    {
        fixed (byte* ptr1 = EncodeNullTerminatedUtf8(taskParams))
        {
            return MaaService.AsstSetTaskParams(handle, id, ptr1);
        }
    }

    // 现在拆分了 core 和 UI 的日志，所以这个函数暂时没用到
    /*
    [DllImport("MaaCore.dll")]
    private static extern unsafe void AsstLog(byte* level, byte* message);

    /// <summary>
    /// 记录日志。
    /// </summary>
    /// <param name="message">日志内容。</param>
    public static unsafe void AsstLog(string message)
    {
        var level = new ReadOnlySpan<byte>(new byte[] { (byte)'G', (byte)'U', (byte)'I', 0 });
        fixed (byte* ptr1 = level, ptr2 = EncodeNullTerminatedUtf8(message))
        {
            AsstLog(ptr1, ptr2);
        }
    }
    */

    public static unsafe BitmapImage? AsstGetImage(AsstHandle handle)
    {
        var buffer = ArrayPool<byte>.Shared.Rent(1280 * 720 * 3);
        try
        {
            ulong readSize;
            fixed (byte* ptr = buffer)
            {
                readSize = MaaService.AsstGetImage(handle, ptr, (ulong)buffer.Length);
            }

            if (readSize == MaaService.AsstGetNullSize())
            {
                return null;
            }

            // buff is a png data
            var image = new BitmapImage();
            image.BeginInit();
            using var stream = new MemoryStream(buffer, 0, (int)readSize, false);
            image.StreamSource = stream;
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.CreateOptions = BitmapCreateOptions.IgnoreColorProfile;
            image.EndInit();
            image.Freeze();
            return image;
        }
        finally
        {
            ArrayPool<byte>.Shared.Return(buffer);
        }
    }

    public BitmapImage? AsstGetImage()
    {
        return AsstGetImage(_handle);
    }

    public BitmapImage? AsstGetImage(bool forceScreencap)
    {
        // UI 端有两类取图场景：
        // - 首页预览/缩略图：直接取 core 的缓存帧即可（避免频繁主动截图）
        // - 监控/诊断：需要强制触发一次截图以拿到“此刻”的帧
        if (forceScreencap)
        {
            MaaService.AsstAsyncScreencap(_handle, true);
        }

        return AsstGetImage(_handle);
    }

    public BitmapImage? AsstGetFreshImage()
    {
        return AsstGetImage(forceScreencap: true);
    }

    public static async Task<BitmapImage?> AsstGetImageAsync(AsstHandle handle)
    {
        return await Task.Run(() => AsstGetImage(handle));
    }

    public async Task<BitmapImage?> AsstGetImageAsync()
    {
        return await AsstGetImageAsync(_handle);
    }

    public async Task<BitmapImage?> AsstGetImageAsync(bool forceScreencap)
    {
        if (forceScreencap)
        {
            MaaService.AsstAsyncScreencap(_handle, true);
        }

        return await AsstGetImageAsync(_handle);
    }

    public async Task<BitmapImage?> AsstGetFreshImageAsync()
    {
        return await AsstGetImageAsync(forceScreencap: true);
    }

    // 需要外部调用 ArrayPool<byte>.Shared.Return(buffer)
    public static unsafe byte[]? AsstGetImageBgrData(AsstHandle handle)
    {
        const int Width = 1280, Height = 720, Channels = 3;
        const int TotalSize = Width * Height * Channels;

        var buffer = ArrayPool<byte>.Shared.Rent(TotalSize);

        ulong readSize;
        fixed (byte* ptr = buffer)
        {
            readSize = MaaService.AsstGetImageBgr(handle, ptr, TotalSize);
        }

        if (readSize == MaaService.AsstGetNullSize())
        {
            ArrayPool<byte>.Shared.Return(buffer);
            return null;
        }

        // **不拷贝，直接返回池内存**
        // 外层代码用完必须调用 ArrayPool<byte>.Shared.Return(buffer)
        return buffer;
    }

    // 需要外部调用 ArrayPool<byte>.Shared.Return(buffer)
    public byte[]? AsstGetImageBgrData()
    {
        return AsstGetImageBgrData(_handle);
    }

    public byte[]? AsstGetImageBgrData(bool forceScreencap)
    {
        if (forceScreencap)
        {
            MaaService.AsstAsyncScreencap(_handle, true);
        }

        return AsstGetImageBgrData(_handle);
    }

    // 需要外部调用 ArrayPool<byte>.Shared.Return(buffer)
    public byte[]? AsstGetFreshImageBgrData()
    {
        return AsstGetImageBgrData(forceScreencap: true);
    }

    // 需要外部调用 ArrayPool<byte>.Shared.Return(buffer)
    public static async Task<byte[]?> AsstGetImageBgrDataAsync(AsstHandle handle)
    {
        return await Task.Run(() => AsstGetImageBgrData(handle));
    }

    // 需要外部调用 ArrayPool<byte>.Shared.Return(buffer)
    public async Task<byte[]?> AsstGetImageBgrDataAsync()
    {
        return await AsstGetImageBgrDataAsync(_handle);
    }

    public async Task<byte[]?> AsstGetImageBgrDataAsync(bool forceScreencap)
    {
        if (forceScreencap)
        {
            MaaService.AsstAsyncScreencap(_handle, true);
        }

        return await AsstGetImageBgrDataAsync(_handle);
    }

    // 需要外部调用 ArrayPool<byte>.Shared.Return(buffer)
    public async Task<byte[]?> AsstGetFreshImageBgrDataAsync()
    {
        return await AsstGetImageBgrDataAsync(forceScreencap: true);
    }

    public static WriteableBitmap WriteBgrToBitmap(byte[] bgrData, WriteableBitmap? targetBitmap)
    {
        const int Width = 1280, Height = 720;
        const int Stride = Width * 3;

        targetBitmap ??= new(Width, Height, 96, 96, PixelFormats.Bgr24, null);
        targetBitmap.Lock();
        targetBitmap.WritePixels(
            new(0, 0, Width, Height),
            bgrData,
            Stride,
            0);
        targetBitmap.Unlock();
        return targetBitmap;
    }

    public const int ScreencapWidth = 1280;
    public const int ScreencapHeight = 720;
    public const int ScreencapChannels = 3;

    public static BitmapSource? CreateBgrBitmapSourceScaled(byte[] bgrData, int targetWidth, int targetHeight)
    {
        if (targetWidth <= 0 || targetHeight <= 0)
        {
            _logger.Warning("Invalid target size: {Width}x{Height}", targetWidth, targetHeight);
            return null;
        }

        const int SrcWidth = ScreencapWidth;
        const int SrcHeight = ScreencapHeight;
        const int SrcStride = SrcWidth * ScreencapChannels;

        if (bgrData.Length < SrcHeight * SrcStride)
        {
            _logger.Warning("Invalid bgrData size: {Size}, expected at least {ExpectedSize}", bgrData.Length, SrcHeight * SrcStride);
            return null;
        }

        int dstStride = targetWidth * ScreencapChannels;

        // 直接生成小图，避免先构建大图再缩放导致不必要的内存峰值。
        var dst = new byte[targetHeight * dstStride];
        for (int y = 0; y < targetHeight; y++)
        {
            int srcY = y * SrcHeight / targetHeight;
            int srcRow = srcY * SrcStride;
            int dstRow = y * dstStride;
            for (int x = 0; x < targetWidth; x++)
            {
                int srcX = x * SrcWidth / targetWidth;
                int srcIndex = srcRow + (srcX * ScreencapChannels);
                int dstIndex = dstRow + (x * ScreencapChannels);
                dst[dstIndex] = bgrData[srcIndex];
                dst[dstIndex + 1] = bgrData[srcIndex + 1];
                dst[dstIndex + 2] = bgrData[srcIndex + 2];
            }
        }

        var bitmap = BitmapSource.Create(
            targetWidth,
            targetHeight,
            96,
            96,
            PixelFormats.Bgr24,
            null,
            dst,
            dstStride);
        bitmap.Freeze();
        return bitmap;
    }

    private readonly MaaService.CallbackDelegate _callback;

    /// <summary>
    /// Initializes a new instance of the <see cref="AsstProxy"/> class.
    /// </summary>
    public AsstProxy()
    {
        _callback = CallbackFunction;
        _runningState = RunningState.Instance;
        _tasksStatus.CollectionChanged += (in NotifyCollectionChangedEventArgs<KeyValuePair<AsstTaskId, (TaskType, TaskStatus)>> args) => {
            if (args.Action == NotifyCollectionChangedAction.Reset)
            {
                TaskSettingVisibilityInfo.Instance.NotifyOfTaskStatus();
            }
        };

        AsstSetUserDir(PathsHelper.BaseDir);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="AsstProxy"/> class.
    /// </summary>
    ~AsstProxy()
    {
        if (_handle != AsstHandle.Zero)
        {
            AsstDestroy();
        }
    }

    /// <summary>
    /// 加载全局资源。新版 core 全惰性加载资源，所以可以无脑调用
    /// </summary>
    /// <returns>是否成功。</returns>
    public bool LoadResource()
    {
        _logger.Information("Load Resource");

        string clientType = SettingsViewModel.GameSettings.ClientType;

        string mainRes = PathsHelper.ResourceDir;
        string globalRes = Path.Combine(mainRes, "global", clientType, "resource");
        string mainCacheRes = PathsHelper.CacheResourceDir;
        string globalCacheRes = Path.Combine(mainCacheRes, "global", clientType, "resource");

        bool loaded;
        if (clientType is "" or "Official" or "Bilibili")
        {
            // Read resources first, then read cache
            CopyTasksJson(mainCacheRes);
            loaded = LoadResIfExists(mainRes);
            loaded &= LoadResIfExists(mainCacheRes);
        }
        else
        {
            // Read resources first, then read cache
            CopyTasksJson(mainCacheRes);
            CopyTasksJson(globalCacheRes);
            loaded = LoadResIfExists(mainRes) && LoadResIfExists(mainCacheRes);
            loaded &= LoadResIfExists(globalRes) && LoadResIfExists(globalCacheRes);
        }

        // 使用窗口绑定模式时，额外加载 PC 平台差异资源
        if (SettingsViewModel.ConnectSettings.UseAttachWindow)
        {
            string pcPlatformRes = Path.Combine(mainRes, "platform_diff", "PC", "resource");
            loaded &= LoadResIfExists(pcPlatformRes);
        }

        return loaded;

        static bool LoadResIfExists(string path)
        {
            if (!Directory.Exists(path))
            {
                _logger.Warning("Resource not found: {Path}", path);
                return true;
            }

            _logger.Information("Load resource: {Path}", path);

            // AsstLoadResource 需要的是 resource 的上级目录
            var parent = Directory.GetParent(path)?.FullName ?? string.Empty;
            if (string.IsNullOrEmpty(parent))
            {
                _logger.Warning("Resource path invalid: {Path}", path);
                return false;
            }

            return AsstLoadResource(parent);
        }

        // 新的目录结构为 tasks/tasks.json，api 为了兼容，仍然存在 resource/tasks.json
        static void CopyTasksJson(string oldResPath)
        {
            try
            {
                string tasksJsonPath = Path.Combine(oldResPath, @"tasks.json");
                string tasksFolderPath = Path.Combine(oldResPath, @"tasks");
                string newTasksJsonPath = Path.Combine(tasksFolderPath, "tasks.json");

                if (!File.Exists(tasksJsonPath))
                {
                    return;
                }

                if (!Directory.Exists(tasksFolderPath))
                {
                    Directory.CreateDirectory(tasksFolderPath);
                    _logger.Information("Created directory: {TasksFolderPath}", tasksFolderPath);
                }

                File.Copy(tasksJsonPath, newTasksJsonPath, true);
                _logger.Information("Moved {TasksJsonPath} to {NewTasksJsonPath}", tasksJsonPath, newTasksJsonPath);
            }
            catch (Exception ex)
            {
                _logger.Error("Failed to move tasks.json: {ExMessage}", ex.Message);
            }
        }
    }

    /// <summary>
    /// 异步加载资源
    /// </summary>
    /// <returns>是否成功。</returns>
    public async Task<bool> LoadResourceAsync()
    {
        return await Task.Run(() => LoadResource());
    }

    /// <summary>
    /// 等待系统空闲时异步加载资源，并返回操作是否成功的值。
    /// Asynchronously loads the resource when the system is idle and returns a value indicating whether the operation succeeded.
    /// </summary>
    /// <returns>
    /// 表示在系统空闲时异步加载操作的任务。如果资源加载成功，任务结果为 <see langword="true"/>；否则为 <see langword="false"/>。
    /// A task that represents the asynchronous load operation when the system is idle. The task result is <see langword="true"/> if the
    /// resource was loaded successfully; otherwise, <see langword="false"/>.
    /// </returns>
    public async Task<bool> LoadResourceWhenIdleAsync()
    {
        await _runningState.UntilIdleAsync();
        return await LoadResourceAsync();
    }

    /// <summary>
    /// 初始化。
    /// </summary>
    public void Init()
    {
        if (GpuOption.GetCurrent() is GpuOption.EnableOption x)
        {
            var info = x.GpuInfo;
            var description = info?.Description;
            var version = info?.DriverVersion;
            var date = info?.DriverDate?.ToString("yyyy-MM-dd");

            if (x.IsDeprecated)
            {
                Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("GpuDeprecatedMessage"), description), UiLogColor.Warning);
                _logger.Warning("Using deprecated GPU {0} (Driver {1} {2})", description, version, date);
            }
            else
            {
                _logger.Information("Using GPU {0} (Driver {1} {2})", description, version, date);
            }

            // Check if driver date is over two years old
            if (info is { DriverDate: { } driverDate })
            {
                var twoYearsAgo = DateTime.Now.AddYears(-2);
                if (driverDate < twoYearsAgo)
                {
                    var dateStr = driverDate.ToString("yyyy-MM-dd");
                    var message = string.Format(LocalizationHelper.GetString("GpuDriverOutdatedMessage"), description, version ?? "Unknown", dateStr);
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.Warning);
                    _logger.Warning("Using GPU {0} with outdated driver {1} (release date: {2}, over 2 years old)", description, version, dateStr);
                }
            }

            AsstSetStaticOption(AsstStaticOptionKey.GpuOCR, x.Index.ToString());
        }

        bool loaded = LoadResource();

        _handle = MaaService.AsstCreateEx(_callback, AsstHandle.Zero);

        if (loaded == false || _handle == AsstHandle.Zero)
        {
            Execute.OnUIThreadAsync(
                () => {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("ResourceBroken"), LocalizationHelper.GetString("Error"), iconKey: ResourceToken.FatalGeometry, iconBrushKey: ResourceToken.DangerBrush);
                    Bootstrapper.Shutdown();
                });
        }

        _runningState.SetInit(true);
        AsstSetInstanceOption(InstanceOptionKey.TouchMode, SettingsViewModel.ConnectSettings.TouchMode);
        AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, SettingsViewModel.GameSettings.DeploymentWithPause ? "1" : "0");
        AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, SettingsViewModel.ConnectSettings.AdbLiteEnabled ? "1" : "0");

        // TODO: 之后把这个 OnUIThread 拆出来
        // ReSharper disable once AsyncVoidLambda
        Execute.OnUIThread(
            async () => {
                if (SettingsViewModel.StartSettings.RunDirectly)
                {
                    // 如果是直接运行模式，就先让按钮显示为运行
                    _runningState.SetIdle(false);
                }

                await Task.Run(() => SettingsViewModel.StartSettings.TryToStartEmulator(true));

                // 一般是点了“停止”按钮了
                if (_runningState.GetStopping())
                {
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                // ReSharper disable once InvertIf
                if (SettingsViewModel.StartSettings.RunDirectly)
                {
                    // 重置按钮状态，不影响LinkStart判断
                    _runningState.SetIdle(true);
                    await Instances.TaskQueueViewModel.LinkStart();
                }
            });
    }

    /// <summary>
    /// Determines the length of the specified string (not including the terminating null character).
    /// </summary>
    /// <param name="ptr">The null-terminated string to be checked.</param>
    /// <returns>
    /// The function returns the length of the string, in characters.
    /// If <paramref name="ptr"/> is <see cref="AsstHandle.Zero"/>, the function returns <c>0</c>.
    /// </returns>
    [DllImport("ucrtbase.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int strlen(AsstHandle ptr);

    private static string? PtrToStringCustom(AsstHandle ptr, Encoding enc)
    {
        if (ptr == AsstHandle.Zero)
        {
            return null;
        }

        int len = strlen(ptr);

        if (len == 0)
        {
            return string.Empty;
        }

        byte[] bytes = new byte[len];
        Marshal.Copy(ptr, bytes, 0, len);
        return enc.GetString(bytes);
    }

    private void CallbackFunction(int msg, AsstHandle jsonBuffer, AsstHandle customArg)
    {
        var jsonStr = PtrToStringCustom(jsonBuffer, Encoding.UTF8);

        // Console.WriteLine(json_str);
        var json = (JObject?)JsonConvert.DeserializeObject(jsonStr ?? string.Empty);
        MaaService.ProcCallbackMsg dlg = ProcMsg;
        Execute.OnUIThread(
            () => {
                dlg((AsstMsg)msg, json);
            });
    }

    private AsstHandle _handle;

    private void ProcMsg(AsstMsg msg, JObject details)
    {
        switch (msg)
        {
            case AsstMsg.InternalError:
                break;

            case AsstMsg.InitFailed:
                MessageBoxHelper.Show(LocalizationHelper.GetString("InitializationError"), LocalizationHelper.GetString("Error"), iconKey: ResourceToken.FatalGeometry, iconBrushKey: ResourceToken.DangerBrush);
                Bootstrapper.Shutdown();
                break;

            case AsstMsg.ConnectionInfo:
                ProcConnectInfo(details);
                break;

            case AsstMsg.TaskChainStart:
                Instances.TaskQueueViewModel.Running = true;
                goto case AsstMsg.TaskChainExtraInfo; // fallthrough
            case AsstMsg.AllTasksCompleted:
            case AsstMsg.AsyncCallInfo:
            case AsstMsg.Destroyed:
            case AsstMsg.TaskChainError:
            case AsstMsg.TaskChainCompleted:
            case AsstMsg.TaskChainStopped:
                Instances.TaskQueueViewModel.Running = false;
                goto case AsstMsg.TaskChainExtraInfo; // fallthrough
            case AsstMsg.TaskChainExtraInfo:
                ProcTaskChainMsg(msg, details);
                break;

            case AsstMsg.SubTaskError:
            case AsstMsg.SubTaskStart:
            case AsstMsg.SubTaskCompleted:
            case AsstMsg.SubTaskExtraInfo:
                ProcSubTaskMsg(msg, details);
                TaskQueueViewModel.InvokeProcSubTaskMsg(msg, details);
                break;

            case AsstMsg.SubTaskStopped:
                break;

            case AsstMsg.ReportRequest:
                _ = ProcReportRequest(details);
                break;

            default:
                throw new ArgumentOutOfRangeException(nameof(msg), msg, null);
        }
    }

    public bool Connected { get; set; }

    private string _connectedAdb = string.Empty;
    private string _connectedAddress = string.Empty;

    private void ProcConnectInfo(JObject details)
    {
        var what = details["what"]?.ToString() ?? string.Empty;
        switch (what)
        {
            case "Connected":
                Connected = true;
                _connectedAdb = details["details"]!["adb"]!.ToString();
                _connectedAddress = details["details"]!["address"]!.ToString();
                SettingsViewModel.ConnectSettings.ConnectAddress = _connectedAddress;
                break;

            case "UnsupportedResolution":
                Connected = false;
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ResolutionNotSupported"), UiLogColor.Error);
                break;

            case "ResolutionError":
                Connected = false;
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ResolutionAcquisitionFailure"), UiLogColor.Error);
                break;

            case "Reconnecting":
                Instances.TaskQueueViewModel.AddLog($"{LocalizationHelper.GetString("TryToReconnect")} ({Convert.ToUInt32(details!["details"]!["times"]) + 1})", UiLogColor.Error);
                break;

            case "Reconnected":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReconnectSuccess"));
                break;

            case "Disconnect":
                Connected = false;
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReconnectFailed"), UiLogColor.Error);
                if (_runningState.GetIdle())
                {
                    break;
                }

                if (!AsstStop())
                {
                    _logger.Warning("Failed to stop Asst");
                }

                break;

            case "ScreencapFailed":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ScreencapFailed"), UiLogColor.Error);
                break;

            case "TouchModeNotAvailable":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TouchModeNotAvailable"), UiLogColor.Error);
                Connected = false;
                break;

            case "FastestWayToScreencap":
                {
                    string costString = details["details"]?["cost"]?.ToString() ?? "???";
                    string method = details["details"]?["method"]?.ToString() ?? "???";
                    SettingsViewModel.ConnectSettings.ScreencapMethod = method;

                    List<(string Method, string Cost)>? screencapAlternatives = null;
                    var alternativesToken = details["details"]?["alternatives"];
                    if (alternativesToken is JArray { Count: > 1 } arr)
                    {
                        screencapAlternatives = arr.Select(item => {
                            string method1 = item?["method"]?.ToString() ?? "???";
                            string cost1 = item?["cost"]?.ToString() ?? "???";
                            return (method1, cost1);
                        }).ToList();
                    }

                    StringBuilder fastestScreencapStringBuilder = new();
                    string color = UiLogColor.Trace;
                    if (int.TryParse(costString, out var timeCost))
                    {
                        switch (timeCost)
                        {
                            case > 800:
                                costString = timeCost.ToString("#,#");
                                color = UiLogColor.Warning;
                                break;

                            case > 400:
                                color = UiLogColor.Warning;
                                break;
                        }
                    }
                    else
                    {
                        color = UiLogColor.Error;
                    }

                    var needToStop = false;
                    switch (SettingsViewModel.ConnectSettings.ConnectConfig)
                    {
                        case "MuMuEmulator12":
                            if (!SettingsViewModel.ConnectSettings.MuMuEmulator12Extras.Enable)
                            {
                                break;
                            }

                            if (method != "MumuExtras")
                            {
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage"), UiLogColor.Error);
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage"), UiLogColor.Error, showTime: false);
                                needToStop = true;
                            }
                            else if (timeCost < 100)
                            {
                                color = UiLogColor.MuMuSpecialScreenshot;
                            }

                            break;

                        case "LDPlayer":
                            if (!SettingsViewModel.ConnectSettings.LdPlayerExtras.Enable)
                            {
                                break;
                            }

                            if (method != "LDExtras")
                            {
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("LdExtrasNotEnabledMessage"), UiLogColor.Error);
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("LdExtrasNotEnabledMessage"), UiLogColor.Error, showTime: false);
                                needToStop = true;
                            }
                            else if (timeCost < 100)
                            {
                                color = UiLogColor.LdSpecialScreenshot;
                            }

                            break;
                    }

                    fastestScreencapStringBuilder.Insert(0, string.Format(LocalizationHelper.GetString("FastestWayToScreencap"), costString, method));
                    var fastestScreencapString = fastestScreencapStringBuilder.ToString();
                    SettingsViewModel.ConnectSettings.ScreencapTestCost = fastestScreencapString;
                    Instances.TaskQueueViewModel.AddLog(fastestScreencapString, color, toolTip: screencapAlternatives.CreateScreencapTooltip());
                    Instances.CopilotViewModel.AddLog(fastestScreencapString, color, showTime: false);

                    // 截图增强未生效禁止启动
                    if (needToStop)
                    {
                        Execute.OnUIThreadAsync(async () => {
                            Connected = false;
                            await Instances.TaskQueueViewModel.Stop();
                            Instances.TaskQueueViewModel.SetStopped();
                        });
                    }
                }

                break;

            case "ScreencapCost":
                var screencapCostMin = details["details"]?["min"]?.ToString() ?? "???";
                var screencapCostAvg = details["details"]?["avg"]?.ToString() ?? "???";
                var screencapCostMax = details["details"]?["max"]?.ToString() ?? "???";
                var currentTime = DateTimeOffset.Now.ToString("HH:mm:ss");
                SettingsViewModel.ConnectSettings.ScreencapCost = string.Format(LocalizationHelper.GetString("ScreencapCost"), screencapCostMin, screencapCostAvg, screencapCostMax, currentTime);
                if (!HasPrintedScreencapWarning && int.TryParse(screencapCostAvg, out var screencapCostAvgInt))
                {
                    static void AddLog(string message, string color)
                    {
                        Instances.TaskQueueViewModel.AddLog(message, color);
                        Instances.CopilotViewModel.AddLog(message, color, showTime: false);
                        HasPrintedScreencapWarning = true;
                    }

                    switch (screencapCostAvgInt)
                    {
                        // 日志提示
                        case >= 800:
                            AddLog(string.Format(LocalizationHelper.GetString("FastestWayToScreencapErrorTip"), screencapCostAvgInt), UiLogColor.Warning);
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.SnapshotChallenge1);
                            break;

                        case >= 400:
                            AddLog(string.Format(LocalizationHelper.GetString("FastestWayToScreencapWarningTip"), screencapCostAvgInt), UiLogColor.Warning);
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.SnapshotChallenge2);
                            break;

                        default:
                            {
                                AchievementTrackerHelper.Instance.Unlock(AchievementIds.SnapshotChallenge3);

                                if (screencapCostAvgInt < 100)
                                {
                                    AchievementTrackerHelper.Instance.Unlock(AchievementIds.SnapshotChallenge4);
                                }

                                if (screencapCostAvgInt < 10)
                                {
                                    AchievementTrackerHelper.Instance.Unlock(AchievementIds.SnapshotChallenge5);
                                }

                                if (screencapCostAvgInt < 5)
                                {
                                    AchievementTrackerHelper.Instance.Unlock(AchievementIds.SnapshotChallenge6);
                                }

                                break;
                            }
                    }
                }

                break;
        }
    }

    private DispatcherTimer? _toastNotificationTimer;

    private void OnToastNotificationTimerTick(object? sender, EventArgs e)
    {
        if (FightTask.SanityReport is not null)
        {
            var sanityReport = LocalizationHelper.GetString("SanityReport");
            var recoveryTime = FightTask.SanityReport.ReportTime.AddMinutes(FightTask.SanityReport.SanityCurrent < FightTask.SanityReport.SanityMax ? (FightTask.SanityReport.SanityMax - FightTask.SanityReport.SanityCurrent) * 6 : 0);
            sanityReport = sanityReport.Replace("{DateTime}", recoveryTime.ToString("yyyy-MM-dd HH:mm")).Replace("{TimeDiff}", (recoveryTime - DateTimeOffset.Now).ToString(@"h\h\ m\m"));
            ToastNotification.ShowDirect(sanityReport);
        }

        DisposeTimer();
    }

    public void DisposeTimer()
    {
        if (_toastNotificationTimer is null)
        {
            return;
        }

        _toastNotificationTimer.Stop();
        _toastNotificationTimer.Tick -= OnToastNotificationTimerTick;
        _toastNotificationTimer = null;
    }

    private void ProcTaskChainMsg(AsstMsg msg, JObject details)
    {
        string taskChain = details["taskchain"]?.ToString() ?? string.Empty;
        AsstTaskId taskId = details["taskid"]?.ToObject<AsstTaskId>() ?? 0;
        switch (taskChain)
        {
            case "CloseDown":
                return;

            case "Recruit":
                {
                    if (msg == AsstMsg.TaskChainError)
                    {
                        Instances.ToolboxViewModel.RecruitInfo = LocalizationHelper.GetString("IdentifyTheMistakes");
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("IdentifyTheMistakes"));
                    }

                    break;
                }
        }

        bool isCopilotTaskChain = taskChain is "Copilot" or "SSSCopilot"; /* or "VideoRecognition"; */

        switch (msg)
        {
            case AsstMsg.TaskChainStopped:
                Instances.TaskQueueViewModel.SetStopped();
                UpdateTaskStatus(taskId, TaskStatus.Completed);
                foreach (var i in Instances.TaskQueueViewModel.TaskItemViewModels)
                {
                    i.TaskId = 0;
                    i.Status = (int)TaskStatus.Idle;
                }
                _tasksStatus.Clear();
                break;

            case AsstMsg.TaskChainError:
                {
                    UpdateTaskStatus(taskId, TaskStatus.Error);
                    _tasksStatus.TryGetValue(taskId, out var value);

                    var log = LocalizationHelper.GetString("TaskError") + LocalizationHelper.GetString(taskChain);
                    Instances.TaskQueueViewModel.AddLog(log, UiLogColor.Error, updateCardImage: true, fetchLatestImage: true, useCardImageAsToolTip: true);

                    ToastNotification.ShowDirect(log);
                    if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenError)
                    {
                        ExternalNotificationService.Send(log, log);
                    }

                    if (value is { Type: TaskType.Copilot })
                    {
                        Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("CombatError"), UiLogColor.Error);
                        AchievementTrackerHelper.Instance.Unlock(AchievementIds.CopilotError);
                    }

                    break;
                }

            case AsstMsg.TaskChainStart:
                {
                    var taskIndex = Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(t => t.TaskId == taskId)?.Index ?? -1;
                    var task = taskIndex >= 0 && taskIndex < ConfigFactory.CurrentConfig.TaskQueue.Count
                        ? ConfigFactory.CurrentConfig.TaskQueue[taskIndex]
                        : null;
                    var taskName = task?.Name ?? $"({LocalizationHelper.GetString(taskChain)})";
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartTask") + taskName, splitMode: TaskQueueViewModel.LogCardSplitMode.Before);
                    _logger.Information("Start Task Chain: {TaskChain}, Task ID: {TaskId}", taskChain, taskId);
                    UpdateTaskStatus(taskId, TaskStatus.InProgress);

                    // LinkStart 按钮也会修改，但小工具中的日志源需要在这里修改
                    Instances.OverlayViewModel.LogItemsSource = (taskChain is "Copilot" or "SSSCopilot") /* or "VideoRecognition") */
                        ? Instances.CopilotViewModel.LogItemViewModels
                        : Instances.TaskQueueViewModel.LogItemViewModels;

                    break;
                }

            case AsstMsg.TaskChainCompleted:
                {
                    // 判断 _latestTaskId 中是否有元素的值和 details["taskid"] 相等，如果有再判断这个 id 对应的任务是否在 _mainTaskTypes 中
                    UpdateTaskStatus(taskId, TaskStatus.Completed);
                    if (_tasksStatus.TryGetValue(taskId, out var taskInfo))
                    {
                        if (_mainTaskTypes.Contains(taskInfo.Type))
                        {
                            Instances.TaskQueueViewModel.UpdateMainTasksProgress();
                        }
                    }

                    var taskIndex = Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(t => t.TaskId == taskId)?.Index ?? -1;
                    var task = taskIndex >= 0 && taskIndex < ConfigFactory.CurrentConfig.TaskQueue.Count
                        ? ConfigFactory.CurrentConfig.TaskQueue[taskIndex]
                        : null;
                    switch (taskChain)
                    {
                        case "Infrast":
                            {
                                if (task is InfrastTask infrastTask)
                                {
                                    InfrastSettingsUserControlModel.IncreaseCustomInfrastPlanIndex(infrastTask);
                                }
                                break;
                            }
                    }

                    var taskName = task?.Name ?? $"({LocalizationHelper.GetString(taskChain)})";
                    if (taskChain == "Fight" && FightTask.SanityReport is not null)
                    {
                        var sanityLog = "\n" + string.Format(LocalizationHelper.GetString("CurrentSanity"), FightTask.SanityReport.SanityCurrent, FightTask.SanityReport.SanityMax);
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + taskName + sanityLog);

                        if (FightTask.SanityReport.SanityCurrent == 0)
                        {
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.SanityPlanner);
                        }
                    }
                    else
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + taskName);
                    }

                    _logger.Information("Completed Task Chain: {TaskChain}, Task ID: {TaskId}", taskChain, taskId);

                    if (isCopilotTaskChain)
                    {
                        AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.UseCopilotGroup);
                    }

                    break;
                }

            case AsstMsg.TaskChainExtraInfo:
                {
                    var what = details["what"]?.ToString();
                    var why = details["why"]?.ToString();

                    switch (what)
                    {
                        case "RoutingRestart":
                            string msgText = string.Empty;
                            switch (why)
                            {
                                case "TooManyBattlesAhead":
                                    var cost = details["node_cost"]?.ToString() ?? "?";
                                    msgText = string.Format(LocalizationHelper.GetString("RoutingRestartTooManyBattles"), cost);
                                    break;
                            }

                            Instances.TaskQueueViewModel.AddLog(msgText, UiLogColor.Warning);
                            break;
                    }

                    break;
                }

            case AsstMsg.AllTasksCompleted:
                bool isMainTaskQueueAllCompleted = false;
                var taskList = details["finished_tasks"]?.ToObject<AsstTaskId[]>();
                if (taskList?.Length > 0)
                {
                    var latestMainTaskIds = _tasksStatus.Where(i => _mainTaskTypes.Contains(i.Value.Type)).Select(i => i.Key);
                    isMainTaskQueueAllCompleted = taskList.Any(i => latestMainTaskIds.Contains(i));
                }

                if (_tasksStatus.Any(t => t.Value.Type == TaskType.Copilot))
                {
                    if (SettingsViewModel.GameSettings.CopilotWithScript)
                    {
                        Task.Run(() => SettingsViewModel.GameSettings.RunScript("EndsWithScript", showLog: false));
                        if (!string.IsNullOrWhiteSpace(SettingsViewModel.GameSettings.EndsWithScript))
                        {
                            Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("EndsWithScript"));
                        }
                    }
                }

                bool buyWine = _tasksStatus.Any(t => t.Value.Type == TaskType.Mall) && Instances.SettingsViewModel.DidYouBuyWine();
                foreach (var i in Instances.TaskQueueViewModel.TaskItemViewModels)
                {
                    i.TaskId = 0;
                    i.Status = (int)TaskStatus.Idle;
                }
                _tasksStatus.Clear();

                Instances.TaskQueueViewModel.ResetAllTemporaryVariable();
                _runningState.SetIdle(true);

                if (isMainTaskQueueAllCompleted)
                {
                    var dateTimeNow = DateTimeOffset.Now;
                    var diffTaskTime = (dateTimeNow - StartTaskTime).ToString(@"h\h\ m\m\ s\s");

                    var allTaskCompleteTitle = string.Format(LocalizationHelper.GetString("AllTasksComplete"), diffTaskTime);
                    var allTaskCompleteMessage = LocalizationHelper.GetString("AllTaskCompleteContent");
                    var sanityReport = LocalizationHelper.GetString("SanityReport");

                    var configurationPreset = ConfigurationHelper.GetCurrentConfiguration();

                    allTaskCompleteMessage = allTaskCompleteMessage
                        .Replace("{DateTime}", dateTimeNow.ToString("yyyy-MM-dd HH:mm:ss"))
                        .Replace("{Preset}", configurationPreset)
                        .Replace("{TimeDiff}", diffTaskTime);

                    var allTaskCompleteLog = string.Format(LocalizationHelper.GetString("AllTasksComplete"), diffTaskTime);

                    if (FightTask.SanityReport is not null)
                    {
                        var recoveryTime = FightTask.SanityReport.ReportTime.AddMinutes(FightTask.SanityReport.SanityCurrent < FightTask.SanityReport.SanityMax ? (FightTask.SanityReport.SanityMax - FightTask.SanityReport.SanityCurrent) * 6 : 0);
                        sanityReport = sanityReport.Replace("{DateTime}", recoveryTime.ToString("yyyy-MM-dd HH:mm")).Replace("{TimeDiff}", (recoveryTime - DateTimeOffset.Now).ToString(@"h\h\ m\m"));

                        allTaskCompleteLog = allTaskCompleteLog + Environment.NewLine + sanityReport;
                        Instances.TaskQueueViewModel.AddLog(allTaskCompleteLog, splitMode: TaskQueueViewModel.LogCardSplitMode.Both);

                        if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenComplete)
                        {
                            var logs = SettingsViewModel.ExternalNotificationSettings.ExternalNotificationEnableDetails
                                ? Instances.TaskQueueViewModel.LogItemViewModels.Aggregate(string.Empty, (current, logItem) => current + $"[{logItem.Time}][{logItem.Color}]{logItem.Content}\n")
                                : string.Empty;
                            logs += allTaskCompleteMessage;

                            ExternalNotificationService.Send(allTaskCompleteTitle, logs + Environment.NewLine + sanityReport);
                        }

                        if (_toastNotificationTimer is not null)
                        {
                            DisposeTimer();
                        }

                        var interval = recoveryTime - DateTimeOffset.Now.AddMinutes(6);
                        if (interval > TimeSpan.Zero)
                        {
                            _toastNotificationTimer = new DispatcherTimer {
                                Interval = interval,
                            };
                            _toastNotificationTimer.Tick += OnToastNotificationTimerTick;
                            _toastNotificationTimer.Start();
                        }
                    }
                    else
                    {
                        Instances.TaskQueueViewModel.AddLog(allTaskCompleteLog, splitMode: TaskQueueViewModel.LogCardSplitMode.Both);

                        if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenComplete)
                        {
                            var logs = SettingsViewModel.ExternalNotificationSettings.ExternalNotificationEnableDetails
                                ? Instances.TaskQueueViewModel.LogItemViewModels.Aggregate(string.Empty, (current, logItem) => current + $"[{logItem.Time}][{logItem.Color}]{logItem.Content}\n")
                                : string.Empty;
                            logs += allTaskCompleteMessage;

                            ExternalNotificationService.Send(allTaskCompleteTitle, logs);
                        }
                    }

                    using (var toast = new ToastNotification(allTaskCompleteTitle))
                    {
                        if (FightTask.SanityReport is not null)
                        {
                            toast.AppendContentText(sanityReport);
                        }

                        toast.Show();
                    }

                    if (DateTime.UtcNow.ToYjDate().IsAprilFoolsDay())
                    {
                        if (Application.Current.MainWindow?.DataContext is RootViewModel rvm)
                        {
                            rvm.GifVisibility = true;
                            rvm.ChangeGif();
                        }
                    }

                    // Instances.TaskQueueViewModel.CheckAndShutdown();
                    _ = Instances.TaskQueueViewModel.CheckAfterCompleted();

                    if (Instances.OverlayViewModel.IsCreated)
                    {
                        AchievementTrackerHelper.Instance.Unlock(AchievementIds.LogSupervisor);
                    }
                }
                else if (isCopilotTaskChain)
                {
                    ToastNotification.ShowDirect(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString(taskChain));
                }

                if (buyWine)
                {
                    Instances.SettingsViewModel.LastBuyWineTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                    var result = MessageBoxHelper.Show(
                        LocalizationHelper.GetString("DrunkAndStaggering"),
                        LocalizationHelper.GetString("Burping"),
                        iconKey: "DrunkAndStaggeringGeometry",
                        iconBrushKey: "PallasBrush");
                    if (result == MessageBoxResult.OK)
                    {
                        Instances.SettingsViewModel.Cheers = true;
                        Bootstrapper.ShutdownAndRestartWithoutArgs();
                    }
                }

                break;

            case AsstMsg.InternalError:
                break;

            case AsstMsg.InitFailed:
                break;

            case AsstMsg.ConnectionInfo:
                break;

            case AsstMsg.AsyncCallInfo:
                break;

            case AsstMsg.Destroyed:
                break;

            case AsstMsg.SubTaskError:
                break;

            case AsstMsg.SubTaskStart:
                break;

            case AsstMsg.SubTaskCompleted:
                break;

            case AsstMsg.SubTaskExtraInfo:
                break;

            case AsstMsg.SubTaskStopped:
                break;

            default:
                throw new ArgumentOutOfRangeException(nameof(msg), msg, null);
        }
    }

    private static void ProcSubTaskMsg(AsstMsg msg, JObject details)
    {
        // 下面几行注释暂时没用到，先注释起来...
        // string taskChain = details["taskchain"].ToString();
        // string classType = details["class"].ToString();
        switch (msg)
        {
            case AsstMsg.SubTaskError:
                ProcSubTaskError(details);
                break;

            case AsstMsg.SubTaskStart:
                ProcSubTaskStart(details);
                break;

            case AsstMsg.SubTaskCompleted:
                ProcSubTaskCompleted(details);
                break;

            case AsstMsg.SubTaskExtraInfo:
                ProcSubTaskExtraInfo(details);
                break;

            case AsstMsg.InternalError:
                break;

            case AsstMsg.InitFailed:
                break;

            case AsstMsg.ConnectionInfo:
                break;

            case AsstMsg.AllTasksCompleted:
                break;

            case AsstMsg.TaskChainError:
                break;

            case AsstMsg.TaskChainStart:
                break;

            case AsstMsg.TaskChainCompleted:
                break;

            case AsstMsg.TaskChainExtraInfo:
                break;

            case AsstMsg.TaskChainStopped:
                break;

            case AsstMsg.SubTaskStopped:
                break;

            default:
                throw new ArgumentOutOfRangeException(nameof(msg), msg, null);
        }
    }

    private static void ProcSubTaskError(JObject details)
    {
        string subTask = details["subtask"]?.ToString() ?? string.Empty;
        AsstTaskId taskId = details["taskid"]?.ToObject<AsstTaskId>() ?? 0;
        switch (subTask)
        {
            case "StartGameTask":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FailedToOpenClient"), UiLogColor.Error);
                break;

            case "StopGameTask":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
                break;

            case "AutoRecruitTask":
                {
                    var whyStr = details.TryGetValue("why", out var why) ? why.ToString() : LocalizationHelper.GetString("ErrorOccurred");
                    Instances.TaskQueueViewModel.AddLog(whyStr + ", " + LocalizationHelper.GetString("HasReturned"), UiLogColor.Error);
                    break;
                }

            case "RecognizeDrops":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DropRecognitionError"), UiLogColor.Error);
                break;

            case "ReportToPenguinStats":
                {
                    var why = details["why"]!.ToString();

                    // 剿灭放弃上传企鹅物流的特殊处理
                    Instances.AsstProxy.TasksStatus.TryGetValue(taskId, out var value);
                    if (value is { Type: TaskType.Fight }
                        && (Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(t => t.TaskId == taskId)?.Index ?? -1) is int index and > -1
                        && index <= ConfigFactory.CurrentConfig.TaskQueue.Count
                        && ConfigFactory.CurrentConfig.TaskQueue[index] is Configuration.Single.MaaTask.FightTask fight
                        && FightTask.GetFightStage(fight) is "Annihilation")
                    {
                        Instances.TaskQueueViewModel.AddLog("AnnihilationStage, " + LocalizationHelper.GetString("GiveUpUploadingPenguins"));
                        break;
                    }

                    Instances.TaskQueueViewModel.AddLog(why + ", " + LocalizationHelper.GetString("GiveUpUploadingPenguins"), UiLogColor.Warning);
                    break;
                }

            case "CheckStageValid":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TheEx"), UiLogColor.Error);
                break;

            case "BattleFormationTask":
                {
                    var why = details.TryGetValue("why", out var whyObj) ? whyObj.ToString() : string.Empty;
                    if (why == "OperatorMissing")
                    {
                        var missingOpers = details["details"]?["opers"]?.ToObject<Dictionary<string, JArray>>();
                        if (missingOpers is not null && missingOpers.Count > 0)
                        {
                            var str = new StringBuilder();
                            str.AppendLine();
                            foreach (var (groupName, opers) in missingOpers)
                            {
                                if (opers.Count == 1)
                                {
                                    str.AppendLine($"{groupName}");
                                    continue;
                                }
                                else
                                {
                                    var operList = opers.Cast<dynamic>().ToList(); // 确保 opers 是动态类型
                                    str.AppendLine($"{groupName}=> {string.Join(" / ", operList.Select(i => i.name).ToList())}");
                                }
                            }

                            Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MissingOperators") + str.ToString().TrimEnd(), UiLogColor.Error);
                        }
                        else
                        {
                            Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MissingOperators"), UiLogColor.Error);
                        }

                        if (missingOpers is not null && missingOpers.Count >= 2)
                        {
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.Irreplaceable);
                        }
                    }
                    break;
                }
            case "CopilotTask":
                {
                    var what = details["what"]?.ToString() ?? string.Empty;
                    if (what == "UserAdditionalOperInvalid")
                    {
                        var operName = details["details"]?["name"]?.ToString();
                        Instances.CopilotViewModel.AddLog(LocalizationHelper.GetStringFormat("CopilotUserAdditionalNameInvalid", operName ?? string.Empty), UiLogColor.Error);
                    }
                    break;
                }
        }
    }

    private static void ProcSubTaskStart(JObject details)
    {
        string subTask = details["subtask"]?.ToString() ?? string.Empty;
        switch (subTask)
        {
            case "ProcessTask":
                {
                    string taskName = details!["details"]!["task"]!.ToString();
                    int execTimes = (int)details!["details"]!["exec_times"]!;

                    switch (taskName)
                    {
                        case "StartButton2":
                        case "AnnihilationConfirm":
                            StringBuilder missionStartLogBuilder = new();
                            if (FightTask.FightReport is null)
                            {
                                missionStartLogBuilder.AppendLine(string.Format(LocalizationHelper.GetString("MissionStart.FightTask"), "???", "???"));
                            }
                            else
                            {
                                var times = FightTask.FightReport.Series == 1 ? $"{FightTask.FightReport.TimesFinished + 1}" : $"{FightTask.FightReport.TimesFinished + 1}~{FightTask.FightReport.TimesFinished + FightTask.FightReport.Series}";
                                missionStartLogBuilder.AppendLine(string.Format(LocalizationHelper.GetString("MissionStart.FightTask"), times, FightTask.FightReport.SanityCost));
                            }

                            if (FightTask.SanityReport is not null)
                            {
                                missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("CurrentSanity"), FightTask.SanityReport.SanityCurrent, FightTask.SanityReport.SanityMax);
                            }

                            if (ExpiringMedicineUsedTimes > 0)
                            {
                                missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("MedicineUsedTimesWithExpiring"), MedicineUsedTimes, ExpiringMedicineUsedTimes);
                            }
                            else if (MedicineUsedTimes > 0)
                            {
                                missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("MedicineUsedTimes"), MedicineUsedTimes);
                            }

                            if (StoneUsedTimes > 0)
                            {
                                missionStartLogBuilder.AppendFormat(LocalizationHelper.GetString("StoneUsedTimes"), StoneUsedTimes);
                            }

                            Instances.TaskQueueViewModel.AddLog(missionStartLogBuilder.ToString().TrimEnd(), UiLogColor.Info, splitMode: TaskQueueViewModel.LogCardSplitMode.Before);
                            break;

                        case "StoneConfirm":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StoneUsed") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                            StoneUsedTimes++;
                            break;

                        case "AbandonAction":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ActingCommandError"), UiLogColor.Error);
                            break;

                        case "FightMissionFailedAndStop":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightMissionFailedAndStop"), UiLogColor.Error);
                            ToastNotification.ShowDirect(LocalizationHelper.GetString("FightMissionFailedAndStop"));
                            break;

                        case "RecruitRefreshConfirm":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("LabelsRefreshed"), UiLogColor.Info);
                            break;

                        case "RecruitConfirm":
                            RecruitConfirmTime++;
                            if (RecruitConfirmTime > AchievementTrackerHelper.Instance.GetProgressToGroup(AchievementIds.HrManager))
                            {
                                AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.HrManager);
                            }

                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RecruitConfirm"), UiLogColor.Info);
                            break;

                        case "InfrastDormDoubleConfirmButton":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("InfrastDormDoubleConfirmed"), UiLogColor.Error);
                            break;

                        /* 肉鸽相关 */
                        case "ExitThenAbandon":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ExplorationAbandoned"), UiLogColor.ExplorationAbandonedIS);
                            AchievementTrackerHelper.Instance.AddProgress(AchievementIds.RoguelikeRetreat);
                            break;

                        // case "StartAction":
                        //    Instances.TaskQueueViewModel.AddLog("开始战斗");
                        //    break;
                        case "MissionCompletedFlag":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightCompleted"), UiLogColor.SuccessIS, updateCardImage: true);
                            break;

                        case "MissionFailedFlag":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FightFailed"), UiLogColor.Error, updateCardImage: true);
                            break;

                        case "StageTrader":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Trader"), UiLogColor.TraderIS);
                            break;

                        case "StageSafeHouse":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("SafeHouse"), UiLogColor.SafehouseIS);
                            break;

                        case "StageFilterTruth":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FilterTruth"), UiLogColor.TruthIS);
                            break;

                        // case "StageBoonsEnter":
                        //    Instances.TaskQueueViewModel.AddLog("古堡馈赠");
                        //    break;
                        case "StageCombatOps":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CombatOps"), UiLogColor.CombatIS);
                            break;

                        case "StageEmergencyOps":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("EmergencyOps"), UiLogColor.EmergencyIS);
                            break;

                        case "StageDreadfulFoe":
                        case "StageDreadfulFoe-5":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DreadfulFoe"), UiLogColor.BossIS);
                            break;

                        case "StageTraderInvestSystemFull":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("UpperLimit"), UiLogColor.Info);
                            break;

                        case "OfflineConfirm":
                            if (TaskQueueViewModel.FightTask.AutoRestartOnDrop)
                            {
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("GameDrop"), UiLogColor.Warning);
                            }
                            else
                            {
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("GameDropNoRestart"), UiLogColor.Warning);
                                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameDropNoRestart"));
                                _ = Instances.TaskQueueViewModel.Stop();
                            }

                            break;

                        case "GamePass":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeGamePass"), UiLogColor.RareOperator);
                            AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.RoguelikeGamePassGroup);
                            break;

                        case "BattleStartAll":
                            Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("MissionStart"), UiLogColor.Info);
                            break;

                        case "StageDrops-Stars-3":
                        case "StageDrops-Stars-Adverse":
                            {
                                Instances.CopilotViewModel.CopilotTaskSuccess();
                                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("CompleteCombat"), UiLogColor.Info);
                                break;
                            }

                        case "StageTraderSpecialShoppingAfterRefresh":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeSpecialItemBought"), UiLogColor.RareOperator);
                            break;

                        case "DeepExplorationNotUnlockedComplain":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DeepExplorationNotUnlockedComplain"), UiLogColor.Warning);
                            break;

                        case "PNS-Resume":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReclamationPnsModeError"), UiLogColor.Error);
                            break;

                        case "PIS-Commence":
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ReclamationPisModeError"), UiLogColor.Error);
                            break;
                    }

                    break;
                }

            case "CombatRecordRecognitionTask":
                {
                    var what = details["what"]?.ToString();
                    if (!string.IsNullOrEmpty(what))
                    {
                        Instances.CopilotViewModel.AddLog(what);
                    }

                    break;
                }
        }
    }

    private static void ProcSubTaskCompleted(JObject details)
    {
        string subTask = details["subtask"]?.ToString() ?? string.Empty;
        switch (subTask)
        {
            case "ProcessTask":
                var taskName = details["details"]?["task"]?.ToString();
                var taskChain = details["taskchain"]?.ToString();
                AsstTaskId taskId = details["taskid"]?.ToObject<AsstTaskId>() ?? 0;
                switch (taskChain)
                {
                    case "Infrast":
                        {
                            switch (taskName)
                            {
                                case "UnlockClues":
                                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ClueExchangeUnlocked"));
                                    AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.ClueUseGroup);
                                    AchievementTrackerHelper.Instance.ClueObsessionAdd();
                                    break;

                                case "SendClues":
                                    AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.ClueSendGroup);
                                    break;
                            }

                            break;
                        }

                    case "Roguelike":
                        {
                            int execTimes = (int)details!["details"]!["exec_times"]!;

                            if (taskName == "StartExplore")
                            {
                                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("BegunToExplore") + $" {execTimes} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info, splitMode: TaskQueueViewModel.LogCardSplitMode.Before);
                            }

                            break;
                        }

                    case "Mall":
                        {
                            switch (taskName)
                            {
                                case "EndOfActionThenStop":
                                    {
                                        var index = Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(t => t.TaskId == taskId)?.Index ?? -1;
                                        if (index >= 0 && index < ConfigFactory.CurrentConfig.TaskQueue.Count && ConfigFactory.CurrentConfig.TaskQueue[index] is MallTask mall)
                                        {
                                            mall.CreditFightLastTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                                        }
                                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString("CreditFight"));
                                        AchievementTrackerHelper.Instance.AddProgress(AchievementIds.MosquitoLeg);
                                        break;
                                    }

                                case "VisitLimited" or "VisitNextBlack":
                                    {
                                        var index = Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(t => t.TaskId == taskId)?.Index ?? -1;
                                        if (index >= 0 && index < ConfigFactory.CurrentConfig.TaskQueue.Count && ConfigFactory.CurrentConfig.TaskQueue[index] is MallTask mall)
                                        {
                                            mall.VisitFriendsLastTime = DateTime.UtcNow.ToYjDate().ToFormattedString();
                                        }
                                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString("Visiting"));
                                        break;
                                    }
                            }

                            break;
                        }
                }

                break;
        }
    }

    private static void ProcSubTaskExtraInfo(JObject details)
    {
        string taskChain = details["taskchain"]?.ToString() ?? string.Empty;
        switch (taskChain)
        {
            case "Recruit":
                ProcRecruitCalcMsg(details);
                break;

                /*
                case "VideoRecognition":
                    ProcVideoRecMsg(details);
                    break;
                */
        }

        var subTaskDetails = details["details"];
        switch (taskChain)
        {
            case "Depot":
                Instances.ToolboxViewModel.DepotParse((JObject?)subTaskDetails, updateSyncTime: true);
                break;

            case "OperBox":
                Instances.ToolboxViewModel.OperBoxParse((JObject?)subTaskDetails);
                break;
        }

        string what = details["what"]?.ToString() ?? string.Empty;
        switch (what)
        {
            case "StageDrops":
                {
                    string allDrops = string.Empty;
                    var statistics = subTaskDetails!["stats"] ?? new JArray();
                    var stageInfo = subTaskDetails!["stage"] ?? new JObject();
                    int curTimes = (int)(subTaskDetails["cur_times"] ?? -1);
                    var drops = new List<(string ItemId, string ItemName, int Total, int Add)>();

                    foreach (var item in statistics)
                    {
                        var itemId = item["itemId"]?.ToString() ?? string.Empty;
                        var itemName = item["itemName"]?.ToString() ?? string.Empty;
                        if (itemName == "furni")
                        {
                            itemName = LocalizationHelper.GetString("FurnitureDrop");
                            itemId = "3401";
                        }

                        int totalQuantity = (int)(item["quantity"] ?? -1);
                        int addQuantity = (int)(item["addQuantity"] ?? -1);

                        drops.Add((itemId, itemName, totalQuantity, addQuantity));
                    }

                    // 先按新增数量降序，再按总数量降序
                    drops = [.. drops.OrderByDescending(x => x.Add).ThenByDescending(x => x.Total)];

                    foreach (var (_, itemName, totalQuantity, addQuantity) in drops)
                    {
                        allDrops += $"{itemName} : {totalQuantity.FormatNumber(false)}";
                        if (addQuantity > 0)
                        {
                            allDrops += $" (+{addQuantity.FormatNumber(false)})";
                        }

                        allDrops += "\n";
                    }

                    var stageCode = stageInfo["stageCode"]?.ToString();
                    allDrops = allDrops.EndsWith('\n') ? allDrops.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");

                    var dropsForTooltip = drops.Where(x => !string.IsNullOrEmpty(x.ItemId)).ToList();

                    Instances.TaskQueueViewModel.AddLog(
                        $"{stageCode} {LocalizationHelper.GetString("TotalDrop")}\n" +
                        $"{allDrops}{(curTimes >= 0
                            ? $"\n{LocalizationHelper.GetString("CurTimes")} : {curTimes}"
                            : string.Empty)}",
                        toolTip: dropsForTooltip.CreateMaterialDropTooltip(),
                        updateCardImage: true);

                    AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.SanitySpenderGroup, curTimes > 0 ? curTimes : 1);

                    // 联动更新 Depot 数据
                    Instances.ToolboxViewModel.UpdateDepotFromDrops(drops);

                    break;
                }

            case "EnterFacility":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ThisFacility") +
                                                    LocalizationHelper.GetString($"{subTaskDetails?["facility"]}") + " " +
                                                    ((int)(subTaskDetails?["index"] ?? -2) + 1).ToString("D2"),
                                                    splitMode: TaskQueueViewModel.LogCardSplitMode.Before);
                break;

            case "ProductIncorrect":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ProductIncorrect"), UiLogColor.Error);
                break;

            case "ProductUnknown":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ProductUnknown"), UiLogColor.Error);
                break;

            case "ProductChanged":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ProductChanged"), UiLogColor.Info);
                break;

            case "InfrastConfirmButton":
                Instances.TaskQueueViewModel.AddLog(string.Empty, updateCardImage: true, fetchLatestImage: true);
                break;

            case "RecruitTagsDetected":
                {
                    var tags = subTaskDetails!["tags"] ?? new JArray();
                    string logContent = tags.Select(tagName => tagName.ToString())
                        .Aggregate(string.Empty, (current, tagStr) => current + (tagStr + "\n"));

                    logContent = logContent.EndsWith('\n') ? logContent.TrimEnd('\n') : LocalizationHelper.GetString("Error");
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RecruitingResults") + "\n" + logContent, splitMode: TaskQueueViewModel.LogCardSplitMode.Before, updateCardImage: true);

                    break;
                }

            case "RecruitSpecialTag":
                {
                    string special = subTaskDetails!["tag"]!.ToString();
                    if (special == "支援机械" && TaskQueueViewModel.RecruitTask.NotChooseLevel1 == false)
                    {
                        break;
                    }

                    using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                    toast.AppendContentText(special).ShowRecruit();

                    break;
                }

            case "RecruitRobotTag":
                {
                    string special = subTaskDetails!["tag"]!.ToString();
                    using var toast = new ToastNotification(LocalizationHelper.GetString("RecruitingTips"));
                    toast.AppendContentText(special).ShowRecruitRobot();

                    break;
                }

            case "RecruitResult":
                {
                    int level = (int)subTaskDetails!["level"]!;
                    var tooltip = Instances.ToolboxViewModel.RecruitResultInlines.CreateTooltip(PlacementMode.Center);
                    if (level >= 5)
                    {
                        using (var toast = new ToastNotification(string.Format(LocalizationHelper.GetString("RecruitmentOfStar"), level)))
                        {
                            toast.AppendContentText(new string('★', level)).ShowRecruit(row: 2);
                        }

                        Instances.TaskQueueViewModel.AddLog(level + " ★ Tags", UiLogColor.RareOperator, "Bold", toolTip: tooltip);
                    }
                    else
                    {
                        Instances.TaskQueueViewModel.AddLog(level + " ★ Tags", UiLogColor.Info, toolTip: tooltip);
                    }

                    if (level == 6)
                    {
                        AchievementTrackerHelper.Instance.SetProgress(AchievementIds.RecruitNoSixStarStreak, 1);
                    }
                    else
                    {
                        AchievementTrackerHelper.Instance.AddProgress(AchievementIds.RecruitNoSixStar); // 累计
                        AchievementTrackerHelper.Instance.AddProgress(AchievementIds.RecruitNoSixStarStreak); // 连续
                    }

                    /*
                    bool robot = (bool)subTaskDetails["robot"];
                    if (robot)
                    {
                        using (var toast = new ToastNotification(Localization.GetString("RecruitmentOfBot")))
                        {
                            toast.AppendContentText(new string('★', 1)).ShowRecruitRobot(row: 2);
                        }

                        Instances.TaskQueueViewModel.AddLog(1 + " ★ Tag", LogColor.RobotOperator, "Bold");
                    }
                    */

                    break;
                }

            case "RecruitSupportOperator":
                {
                    var name = subTaskDetails!["name"]!.ToString();
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RecruitSupportOperator"), name), UiLogColor.Info);
                    break;
                }

            case "RecruitTagsSelected":
                {
                    var selected = subTaskDetails!["tags"] ?? new JArray();
                    string selectedLog = selected.Aggregate(string.Empty, (current, tag) => current + (tag + "\n"));

                    selectedLog = selectedLog.EndsWith('\n') ? selectedLog.TrimEnd('\n') : LocalizationHelper.GetString("NoDrop");

                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Choose") + " Tags：\n" + selectedLog);

                    break;
                }

            case "RecruitTagsRefreshed":
                {
                    int refreshCount = (int)subTaskDetails!["count"]!;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Refreshed") + refreshCount + LocalizationHelper.GetString("UnitTime"));
                    AchievementTrackerHelper.Instance.AddProgress(AchievementIds.RecruitGambler);
                    break;
                }

            case "RecruitNoPermit":
                {
                    bool continueRefresh = (bool)subTaskDetails!["continue"]!;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString(continueRefresh ? "ContinueRefresh" : "NoRecruitmentPermit"));
                    break;
                }

            case "NotEnoughStaff":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NotEnoughStaff"), UiLogColor.Error);
                break;

            case "CreditFullOnlyBuyDiscount":
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CreditFullOnlyBuyDiscount") + subTaskDetails!["credit"], UiLogColor.Message);
                    break;
                }

            /* Roguelike */
            case "StageInfo":
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartCombat") + subTaskDetails!["name"]);
                    if (TaskQueueViewModel.RoguelikeTask.RoguelikeDelayAbortUntilCombatComplete)
                    {
                        Instances.TaskQueueViewModel.RoguelikeInCombatAndShowWait = true;
                    }

                    break;
                }

            case "StageInfoError":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageInfoError"), UiLogColor.Error, splitMode: TaskQueueViewModel.LogCardSplitMode.Both, updateCardImage: true);
                break;

            case "BattleFormation":
                Instances.CopilotViewModel.AddLog(
                    LocalizationHelper.GetString("BattleFormation") +
                    "\n[" +
                    string.Join(
                        ", ",
                        (subTaskDetails!["formation"]?.ToObject<List<string?>>() ?? [])
                            .Select(oper => DataHelper.GetLocalizedCharacterName(oper) ?? oper)
                            .Where(oper => !string.IsNullOrEmpty(oper))) + "]");
                break;

            case "BattleFormationParseFailed":
                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormationParseFailed"));
                break;

            case "BattleFormationSelected":
                {
                    var oper_name = DataHelper.GetLocalizedCharacterName(subTaskDetails!["selected"]?.ToString());
                    var group_name = subTaskDetails!["group_name"]?.ToString();
                    if (group_name is not null && oper_name != group_name)
                    {
                        oper_name = $"{group_name} => {oper_name}";
                    }

                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("BattleFormationSelected") + oper_name);
                    break;
                }

            case "BattleFormationOperUnavailable":
                {
                    Instances.CopilotViewModel.HasRequirementIgnored = true;
                    var oper_name = DataHelper.GetLocalizedCharacterName(subTaskDetails!["oper_name"]?.ToString());
                    var type = subTaskDetails["requirement_type"]?.ToString() ?? "Unknown Type";
                    bool isError = !Instances.CopilotViewModel.IgnoreRequirements;
                    switch (type)
                    {
                        case "elite":
                            type = LocalizationHelper.GetString("BattleFormationOperUnavailable.Elite");
                            isError = true;
                            break;
                        case "level":
                            type = LocalizationHelper.GetString("BattleFormationOperUnavailable.Level");
                            break;
                        case "skill_level":
                            type = LocalizationHelper.GetString("BattleFormationOperUnavailable.SkillLevel");
                            break;
                        case "module":
                            type = LocalizationHelper.GetString("BattleFormationOperUnavailable.Module");
                            break;
                        default:
                            break;
                    }

                    Instances.CopilotViewModel.AddLog(LocalizationHelper.GetStringFormat("BattleFormationOperUnavailable", oper_name ?? string.Empty, type), isError ? UiLogColor.Error : UiLogColor.Warning);
                    break;
                }

            case "CopilotAction":
                {
                    var doc = subTaskDetails!["doc"]?.ToString();
                    if (!string.IsNullOrEmpty(doc))
                    {
                        var color = subTaskDetails["doc_color"]?.ToString();
                        Instances.CopilotViewModel.AddLog(doc, string.IsNullOrEmpty(color) ? UiLogColor.Message : color);
                    }

                    var target = subTaskDetails["target"]?.ToString();
                    var actionToken = subTaskDetails?["action"];
                    var actionString = actionToken?.ToString() ?? "UnknownAction";
                    Instances.CopilotViewModel.AddLog(
                        string.Format(
                            LocalizationHelper.GetString("CurrentSteps"),
                            LocalizationHelper.GetString(actionString),
                            DataHelper.GetLocalizedCharacterName(target) ?? target));

                    var elapsed_time_str = subTaskDetails!["elapsed_time"]?.ToString();
                    if (int.TryParse(elapsed_time_str, out int elapsed_time_int) && elapsed_time_int >= 0)
                    {
                        Instances.CopilotViewModel.AddLog(string.Format(LocalizationHelper.GetString("ElapsedTime"), elapsed_time_int), UiLogColor.Message);
                    }

                    break;
                }

            case "CopilotListLoadTaskFileSuccess":
                Instances.CopilotViewModel.AddLog($"Parse {subTaskDetails!["file_name"]}[{subTaskDetails["stage_name"]}] Success");
                Instances.CopilotViewModel.HasRequirementIgnored = false;
                break;

            case "SSSStage":
                Instances.CopilotViewModel.AddLog(string.Format(LocalizationHelper.GetString("CurrentStage"), subTaskDetails!["stage"]), UiLogColor.Info);
                break;

            case "SSSSettlement":
                Instances.CopilotViewModel.AddLog($"{details["why"]}", UiLogColor.Info);
                break;

            case "SSSGamePass":
                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("SSSGamePass"), UiLogColor.RareOperator);
                break;

            case "UnsupportedLevel":
                Instances.CopilotViewModel.AddLog(LocalizationHelper.GetString("UnsupportedLevel") + subTaskDetails!["level"], UiLogColor.Error);
                _ = ResourceUpdater.ResourceUpdateAndReloadAsync();
                break;

            case "CustomInfrastRoomGroupsMatch":
                // 选用xxx组编组
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomGroupsMatch") + subTaskDetails!["group"]);
                break;

            case "CustomInfrastRoomGroupsMatchFailed":
                // 干员编组匹配失败
                var groups = (JArray?)subTaskDetails!["groups"];
                if (groups != null)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomGroupsMatchFailed") + string.Join(", ", groups));
                }

                break;

            case "CustomInfrastRoomOperators":
                string nameStr = (subTaskDetails!["names"] ?? new JArray())
                    .Aggregate(string.Empty, (current, name) => current + DataHelper.GetLocalizedCharacterName(name.ToString()) + ", ");

                if (nameStr != string.Empty)
                {
                    nameStr = nameStr.Remove(nameStr.Length - 2);
                }

                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoomOperators") + nameStr);
                break;

            case "InfrastTrainingIdle":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TrainingIdle"));
                break;

            case "InfrastTrainingCompleted":
                {
                    var operatorName = DataHelper.GetLocalizedCharacterName(subTaskDetails!["operator"]?.ToString()) ?? "UnKnown";
                    var skillName = subTaskDetails["skill"]?.ToString() ?? "UnKnown";
                    Instances.TaskQueueViewModel.AddLog(
                        $"[{operatorName}] {skillName}\n" +
                        $"{LocalizationHelper.GetString("TrainingLevel")}: {(int)(subTaskDetails["level"] ?? -1)} {LocalizationHelper.GetString("TrainingCompleted")}",
                        UiLogColor.Info);
                    break;
                }

            case "InfrastTrainingTimeLeft":
                {
                    var operatorName = DataHelper.GetLocalizedCharacterName(subTaskDetails!["operator"]?.ToString()) ?? "UnKnown";
                    var skillName = subTaskDetails["skill"]?.ToString() ?? "UnKnown";
                    Instances.TaskQueueViewModel.AddLog(
                        $"[{operatorName}] {skillName}\n" +
                        $"{LocalizationHelper.GetString("TrainingLevel")}: {(int)(subTaskDetails["level"] ?? -1)}\n" +
                        $"{LocalizationHelper.GetString("TrainingTimeLeft")}: {subTaskDetails["time"]}",
                        UiLogColor.Info);
                    break;
                }

            /* 生息演算 */
            case "ReclamationReport":
                Instances.TaskQueueViewModel.AddLog(
                    LocalizationHelper.GetString("AlgorithmFinish") + "\n" +
                    LocalizationHelper.GetString("AlgorithmBadge") + ": " + $"{(int)(subTaskDetails!["total_badges"] ?? -1)}(+{(int)(subTaskDetails["badges"] ?? -1)})" + "\n" +
                    LocalizationHelper.GetString("AlgorithmConstructionPoint") + ": " + $"{(int)(subTaskDetails["total_construction_points"] ?? -1)}(+{(int)(subTaskDetails["construction_points"] ?? -1)})");
                break;

            case "ReclamationProcedureStart":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MissionStart") + $" {(int)(subTaskDetails!["times"] ?? -1)} " + LocalizationHelper.GetString("UnitTime"), UiLogColor.Info);
                break;

            case "ReclamationSmeltGold":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AlgorithmDoneSmeltGold") + $" {(int)(subTaskDetails!["times"] ?? -1)} " + LocalizationHelper.GetString("UnitTime"));
                break;

            case "SanityBeforeStage":
                {
                    FightTask.SanityReport = null;
                    if (subTaskDetails?.ToObject<FightSettingsUserControlModel.SanityInfo>() is { SanityMax: > 0 } report)
                    {
                        FightTask.SanityReport = report;
                    }

                    break;
                }

            case "FightTimes":
                {
                    FightTask.FightReport = null;
                    if ((subTaskDetails?.Children())?.Any() is true)
                    {
                        FightTask.FightReport = subTaskDetails.ToObject<FightTask.FightTimes>()!;
                        if (FightTask.FightReport.TimesFinished > 0)
                        {
                            AchievementTrackerHelper.Instance.SetProgress(AchievementIds.OverLimitAgent, FightTask.FightReport.TimesFinished);
                        }

                        if (FightTask.Instance.HasTimesLimited != false && FightTask.FightReport.IsFinished && FightTask.FightReport.TimesFinished < FightTask.Instance.MaxTimes)
                        {
                            Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("FightTimesUnused"), FightTask.FightReport.TimesFinished, FightTask.FightReport.Series, FightTask.FightReport.TimesFinished + FightTask.FightReport.Series, FightTask.Instance.MaxTimes), UiLogColor.Warning);
                        }
                    }

                    break;
                }

            case "UseMedicine":
                var medicineReport = (JObject?)subTaskDetails;
                if (medicineReport is null || !medicineReport.ContainsKey("is_expiring") || !medicineReport.ContainsKey("count"))
                {
                    break;
                }

                var isExpiringMedicine = medicineReport.TryGetValue("is_expiring", out var isExpiringMedicineToken) && (bool)isExpiringMedicineToken;
                int medicineCount = medicineReport.TryGetValue("count", out var medicineCountToken) ? (int)medicineCountToken : -1;

                if (medicineCount == -1)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MedicineUsed") + " Unknown times", UiLogColor.Error);
                    break;
                }

                string medicineLog;
                if (!isExpiringMedicine)
                {
                    MedicineUsedTimes += medicineCount;
                    medicineLog = LocalizationHelper.GetString("MedicineUsed") + $" {MedicineUsedTimes}(+{medicineCount})";
                    AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.SanitySaverGroup, medicineCount);
                }
                else
                {
                    ExpiringMedicineUsedTimes += medicineCount;
                    medicineLog = LocalizationHelper.GetString("ExpiringMedicineUsed") + $" {ExpiringMedicineUsedTimes}(+{medicineCount})";
                    AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.SanitySaverGroup, medicineCount);
                    AchievementTrackerHelper.Instance.SetProgress(AchievementIds.SanityExpire, ExpiringMedicineUsedTimes);
                }

                Instances.TaskQueueViewModel.AddLog(medicineLog, UiLogColor.Info);
                break;

            case "StageQueueUnableToAgent":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageQueue") + $" {subTaskDetails!["stage_code"]} " + LocalizationHelper.GetString("UnableToAgent"), UiLogColor.Info);
                break;

            case "StageQueueMissionCompleted":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StageQueue") + $" {subTaskDetails!["stage_code"]} - {subTaskDetails["stars"]} ★", UiLogColor.Info);
                break;
        }
    }

    private static void ProcRecruitCalcMsg(JObject details)
    {
        Instances.ToolboxViewModel.ProcRecruitMsg(details);
    }

    private static void ProcVideoRecMsg(JObject details)
    {
        string what = details["what"]?.ToString() ?? string.Empty;
        switch (what)
        {
            case "Finished":
                var filename = details["details"]?["filename"];
                Instances.CopilotViewModel.AddLog("Save to: " + filename, UiLogColor.Info, showTime: false);

                // string p = @"C:\tmp\this path contains spaces, and,commas\target.txt";
                string args = $"/e, /select, \"{filename}\"";

                ProcessStartInfo info = new() {
                    FileName = "explorer",
                    Arguments = args,
                };
                Process.Start(info);
                break;
        }
    }

    private static async Task ProcReportRequest(JObject details)
    {
        string? url = (string?)details["url"];
        if (string.IsNullOrEmpty(url))
        {
            _logger.Error("Report request received with empty URL.");
            return;
        }

        string subTask = details["subtask"]?.ToString() ?? string.Empty;

        // 企鹅物流不收集 txwy 客户端数据
        if (SettingsViewModel.GameSettings.ClientType == ClientType.Txwy && (subTask == "ReportToPenguinStats"))
        {
            _logger.Information("PenguinStats report skipped for txwy client type.");
            return;
        }

        var headersToken = details["headers"];
        Dictionary<string, string> headers = [];
        if (headersToken is JObject headersObj)
        {
            foreach (var prop in headersObj.Properties())
            {
                headers[prop.Name] = prop.Value.ToString();
            }
        }

        string? body = (string?)details["body"];
        if (string.IsNullOrEmpty(body))
        {
            _logger.Error("Report request received with empty body.");
            return;
        }

        var content = new StringContent(body, Encoding.UTF8, "application/json");

        bool success = false;
        try
        {
            success = await GameDataReportService.PostWithRetryAsync(url, content, headers, subTask, penguinId => {
                if (string.IsNullOrWhiteSpace(SettingsViewModel.GameSettings.PenguinId))
                {
                    SettingsViewModel.GameSettings.PenguinId = penguinId;
                }

                _logger.Information("New PenguinId got: {PenguinId}", penguinId);
            });
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to report: {Url}", url);
        }

        if (!success)
        {
            Instances.TaskQueueViewModel.AddLog("Failed to report, " + LocalizationHelper.GetString("GiveUpUploadingPenguins"), UiLogColor.Warning);
        }
    }

    public bool AsstSetInstanceOption(InstanceOptionKey key, string value)
    {
        return AsstSetInstanceOption(_handle, (AsstInstanceOptionKey)key, value);
    }

    public bool AsstSetStaticOption(AsstStaticOptionKey key, string value)
    {
        return MaaService.AsstSetStaticOption(key, value);
    }

    /// <summary>
    /// 使用 TCP 或 adb devices 命令检查连接。TCP 检测相比 adb devices 更快，但不支持实体机。
    /// </summary>
    /// <param name="adbPath">adb path，用于实体机检测</param>
    /// <param name="address">连接地址</param>
    /// <returns>设备是否在线</returns>
    private static bool CheckConnection(string adbPath, string? address)
    {
        if (string.IsNullOrEmpty(address))
        {
            return false;
        }

        // 实体机可能设备名 -> [host]
        if (!address.Contains(':') && !address.Contains('-'))
        {
            return WinAdapter.GetAdbAddresses(adbPath).Contains(address);
        }

        // normal -> [host]:[port]
        // LdPlayer -> emulator-[port]
        string[] hostAndPort = address.Split([':', '-'], StringSplitOptions.RemoveEmptyEntries);

        if (hostAndPort.Length != 2 || !int.TryParse(hostAndPort[1], out var port))
        {
            return false;
        }

        string host = hostAndPort[0];
        if (host.StartsWith("emulator"))
        {
            host = "127.0.0.1";
            port += 1;
        }

        using var client = new TcpClient();
        try
        {
            IAsyncResult result = client.BeginConnect(host, port, null, null);
            bool success = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(0.5));

            if (success)
            {
                client.EndConnect(result);
                return true;
            }

            client.Close();
            return false;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// 连接模拟器。
    /// </summary>
    /// <param name="error">具体的连接错误。</param>
    /// <returns>是否成功。</returns>
    public bool AsstConnect(ref string error)
    {
        // 如果启用了 AttachWindow 模式，则使用窗口绑定而非 ADB 连接
        if (SettingsViewModel.ConnectSettings.UseAttachWindow)
        {
            return AsstAttachWindowConnect(ref error);
        }

        return AsstAdbConnect(ref error);
    }

    /// <summary>
    /// 搜索指定窗口标题的窗口句柄。
    /// </summary>
    /// <param name="windowName">窗口标题（完全匹配）。</param>
    /// <returns>找到的窗口句柄列表。</returns>
    private static List<IntPtr> FindWindowsByName(string windowName)
    {
        var results = new List<IntPtr>();

        PInvoke.EnumWindows((hWnd, lParam) => {
            if (!PInvoke.IsWindowVisible(hWnd))
            {
                return true;
            }

            var len = PInvoke.GetWindowTextLength(hWnd);
            if (len <= 0)
            {
                return true;
            }

            Span<char> buffer = len <= 1024 ? stackalloc char[len + 1] : new char[len + 1];
            int written = PInvoke.GetWindowText(hWnd, buffer);
            var title = new string(buffer[..Math.Max(0, Math.Min(written, buffer.Length))]);

            if (title == windowName)
            {
                results.Add((IntPtr)hWnd);
            }

            return true;
        }, IntPtr.Zero);

        return results;
    }

    /// <summary>
    /// 通过 AttachWindow 绑定 Win32 窗口。
    /// 自动搜索 "明日方舟" 窗口。
    /// </summary>
    /// <param name="error">具体的连接错误。</param>
    /// <returns>是否成功。</returns>
    private bool AsstAttachWindowConnect(ref string error)
    {
        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("UseAttachWindowWarning"), UiLogColor.Warning);

        const string TargetWindowName = "明日方舟";
        var foundWindows = FindWindowsByName(TargetWindowName);

        if (foundWindows.Count == 0)
        {
            error = string.Format(LocalizationHelper.GetString("AttachWindowNotFound"), TargetWindowName);
            Instances.TaskQueueViewModel.AddLog(error, UiLogColor.Error);
            _logger.Warning("AttachWindow: No window found with name {WindowName}", TargetWindowName);
            return false;
        }

        var hwnd = foundWindows[0];

        if (foundWindows.Count > 1)
        {
            // 找到多个窗口，使用第一个并记录日志
            var multipleMsg = string.Format(LocalizationHelper.GetString("AttachWindowMultipleFound"), foundWindows.Count, TargetWindowName);
            Instances.TaskQueueViewModel.AddLog(multipleMsg, UiLogColor.Info);
            _logger.Warning("AttachWindow: Multiple windows found with name {WindowName}, count: {Count}, using first one: {Hwnd}", TargetWindowName, foundWindows.Count, hwnd);
        }
        else
        {
            var foundMsg = string.Format(LocalizationHelper.GetString("AttachWindowFound"), TargetWindowName);
            Instances.TaskQueueViewModel.AddLog(foundMsg, UiLogColor.Info);
            _logger.Information("AttachWindow: Found window \"{WindowName}\" with HWND: {Hwnd}", TargetWindowName, hwnd);
        }

        if (!ulong.TryParse(SettingsViewModel.ConnectSettings.AttachWindowScreencapMethod, out var screencapMethod))
        {
            screencapMethod = 2; // 默认 FramePool
        }

        if (!ulong.TryParse(SettingsViewModel.ConnectSettings.AttachWindowMouseMethod, out var mouseMethod))
        {
            mouseMethod = 64; // 默认 PostMessageWithCursorPos
        }

        if (!ulong.TryParse(SettingsViewModel.ConnectSettings.AttachWindowKeyboardMethod, out var keyboardMethod))
        {
            keyboardMethod = 64; // 默认 PostMessageWithCursorPos
        }

        bool ret = AsstAttachWindow(_handle, hwnd, screencapMethod, mouseMethod, keyboardMethod);

        if (!ret)
        {
            error = LocalizationHelper.GetString("AttachWindowFailed");
            Instances.TaskQueueViewModel.AddLog(error, UiLogColor.Error);
        }

        return ret;
    }

    /// <summary>
    /// 通过 ADB 连接模拟器。
    /// </summary>
    /// <param name="error">具体的连接错误。</param>
    /// <returns>是否成功。</returns>
    private bool AsstAdbConnect(ref string error)
    {
        switch (SettingsViewModel.ConnectSettings.ConnectConfig)
        {
            case "MuMuEmulator12":
                AsstSetConnectionExtrasMuMu12(SettingsViewModel.ConnectSettings.MuMuEmulator12Extras.Config);
                break;

            case "LDPlayer":
                AsstSetConnectionExtrasLdPlayer(SettingsViewModel.ConnectSettings.LdPlayerExtras.Config);
                break;
        }

        if (SettingsViewModel.ConnectSettings.AutoDetectConnection)
        {
            if (!AutoDetectConnection(ref error))
            {
                return false;
            }
        }

        if (Connected && _connectedAdb == SettingsViewModel.ConnectSettings.AdbPath &&
            _connectedAddress == SettingsViewModel.ConnectSettings.ConnectAddress)
        {
            var actualConnectionStatus = CheckConnection(_connectedAdb, _connectedAddress);
            if (!actualConnectionStatus)
            {
                Connected = false;
                _logger.Information("Connection lost to {ConnectedAdb} {ConnectedAddress}", _connectedAdb, _connectedAddress);
                error = "Connection lost";
            }
            else
            {
                _logger.Information("Already connected to {ConnectedAdb} {ConnectedAddress}", _connectedAdb, _connectedAddress);
                if (!Instances.TaskQueueViewModel.EnableAutoReload)
                {
                    return true;
                }

                _logger.Information("Forced reload resource");
                if (!LoadResource())
                {
                    error = "Load Resource Failed";
                    return false;
                }

                ToastNotification.ShowDirect("Auto Reload");

                return true;
            }
        }

        bool ret = AsstConnect(_handle, SettingsViewModel.ConnectSettings.AdbPath, SettingsViewModel.ConnectSettings.ConnectAddress, SettingsViewModel.ConnectSettings.ConnectConfig);

        // 尝试默认的备选端口
        if (!ret && SettingsViewModel.ConnectSettings.AutoDetectConnection)
        {
            if (SettingsViewModel.ConnectSettings.DefaultAddress.TryGetValue(SettingsViewModel.ConnectSettings.ConnectConfig, out var value))
            {
                foreach (var address in value
                             .TakeWhile(_ => !_runningState.GetIdle()))
                {
                    ret = AsstConnect(_handle, SettingsViewModel.ConnectSettings.AdbPath, address, SettingsViewModel.ConnectSettings.ConnectConfig);
                    if (!ret)
                    {
                        continue;
                    }

                    SettingsViewModel.ConnectSettings.ConnectAddress = address;
                    break;
                }
            }
            else
            {
                Execute.OnUIThreadAsync(
                    () => {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AutoDetectConnectionNotSupported"), UiLogColor.Error);
                    });
            }
        }

        if (!ret)
        {
            error = LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("CheckSettings");
        }
        else if (SettingsViewModel.ConnectSettings.AutoDetectConnection && !SettingsViewModel.ConnectSettings.AlwaysAutoDetectConnection)
        {
            SettingsViewModel.ConnectSettings.AutoDetectConnection = false;
        }

        return ret;
    }

    private static bool AutoDetectConnection(ref string error)
    {
        var adbPath = SettingsViewModel.ConnectSettings.AdbPath;
        bool adbResult = !string.IsNullOrEmpty(adbPath) &&
                         File.Exists(adbPath) &&
                         Path.GetFileName(adbPath).Contains("adb", StringComparison.InvariantCultureIgnoreCase) &&
                         CheckConnection(adbPath, SettingsViewModel.ConnectSettings.ConnectAddress);

        if (adbResult)
        {
            error = string.Empty;
            return true;
        }

        // 蓝叠的特殊处理
        {
            string bsHvAddress = SettingsViewModel.ConnectSettings.TryToSetBlueStacksHyperVAddress() ?? string.Empty;
            bool bsResult = CheckConnection(adbPath, bsHvAddress);
            if (bsResult)
            {
                error = string.Empty;
                if (string.IsNullOrEmpty(SettingsViewModel.ConnectSettings.AdbPath) && SettingsViewModel.ConnectSettings.DetectAdbConfig(ref error))
                {
                    return string.IsNullOrEmpty(error);
                }

                SettingsViewModel.ConnectSettings.ConnectAddress = bsHvAddress;
                return true;
            }
        }

        if (SettingsViewModel.ConnectSettings.DetectAdbConfig(ref error))
        {
            // https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8547
            // DetectAdbConfig 会把 ConnectAddress 变成第一个不是 emulator 开头的地址，可能会存在多开问题
            error = string.Empty;
            return true;
        }

        return false;
    }

    private AsstTaskId AsstAppendTaskWithEncoding(AsstTaskType type, JObject? taskParams = null)
    {
        taskParams ??= [];
        return AsstAppendTask(_handle, type.ToString(), JsonConvert.SerializeObject(taskParams));
    }

    private bool AsstSetTaskParamsWithEncoding(AsstTaskId id, JObject? taskParams = null)
    {
        if (id == 0)
        {
            return false;
        }

        taskParams ??= [];
        return AsstSetTaskParams(_handle, id, JsonConvert.SerializeObject(taskParams));
    }

    /// <summary>
    /// WPF 区分任务的类型
    /// </summary>
    public enum TaskType
    {
        /// <summary>开始唤醒</summary>
        StartUp,

        /// <summary>关闭游戏</summary>
        CloseDown,

        /// <summary>理智作战</summary>
        Fight,

        /// <summary>自动公招</summary>
        Recruit,

        /// <summary>基建</summary>
        Infrast,

        /// <summary>获取信用点/访问好友/信用商店</summary>
        Mall,

        /// <summary>领奖励/邮箱/幸运墙等</summary>
        Award,

        /// <summary>自动肉鸽</summary>
        Roguelike,

        /// <summary>公招识别</summary>
        RecruitCalc,

        /// <summary>自动战斗</summary>
        Copilot,

        /*
        /// <summary>视频识别（真有人用吗）</summary>
        VideoRec,
        */

        /// <summary>仓库识别</summary>
        Depot,

        /// <summary>干员识别</summary>
        OperBox,

        /// <summary>抽卡</summary>
        Gacha,

        /// <summary>生息演算</summary>
        Reclamation,

        /// <summary>小游戏</summary>
        MiniGame,

        /// <summary>自定义任务s</summary>
        Custom,
    }

    private readonly HashSet<TaskType> _mainTaskTypes =
    [
        TaskType.StartUp,
        TaskType.Fight,
        TaskType.Recruit,
        TaskType.Infrast,
        TaskType.Mall,
        TaskType.Award,
        TaskType.Roguelike,
        TaskType.Reclamation,
    ];

    private readonly ObservableDictionary<AsstTaskId, (TaskType Type, TaskStatus Status)> _tasksStatus = [];

    public IReadOnlyDictionary<AsstTaskId, (TaskType Type, TaskStatus Status)> TasksStatus => new Dictionary<AsstTaskId, (TaskType, TaskStatus)>(_tasksStatus);

    private bool UpdateTaskStatus(AsstTaskId id, TaskStatus status)
    {
        if (id <= 0)
        {
            return false;
        }

        if (!_tasksStatus.TryGetValue(id, out var value))
        {
            _logger.Error("Task ID {TaskId} not found in _tasksStatus", id);
            return false;
        }

        if (value.Status == TaskStatus.Idle && status == TaskStatus.InProgress)
        {
            RunningState.Instance.ResetTimeout(); // 进入新任务时重置超时计时
        }

        _tasksStatus[id] = (value.Type, status);
        Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(item => item.TaskId == id)?.Status = (int)status;
        if (status == TaskStatus.InProgress)
        {
            TaskSettingVisibilityInfo.Instance.NotifyOfTaskStatus();
        }

        return true;
    }

    public bool AsstAppendCloseDown(string clientType)
    {
        if (!AsstStop())
        {
            _logger.Warning("Failed to stop Asst");
        }

        var (type, param) = new AsstCloseDownTask() { ClientType = clientType }.Serialize();
        return AsstAppendTaskWithEncoding(TaskType.CloseDown, type, param);
    }

    /// <summary>
    /// <c>CloseDown</c> 任务。
    /// </summary>
    /// <param name="clientType">客户端版本。</param>
    /// <returns>是否成功。</returns>
    public bool AsstStartCloseDown(string clientType)
    {
        return AsstAppendCloseDown(clientType) && AsstStart();
    }

    public bool AsstBackToHome()
    {
        return MaaService.AsstBackToHome(_handle);
    }

    /// <summary>
    /// 仓库识别。
    /// </summary>
    /// <returns>是否成功。</returns>
    public bool AsstStartDepot()
    {
        return AsstAppendTaskWithEncoding(TaskType.Depot, AsstTaskType.Depot) && AsstStart();
    }

    /// <summary>
    /// 干员识别。
    /// </summary>
    /// <returns>是否成功。</returns>
    public bool AsstStartOperBox()
    {
        return AsstAppendTaskWithEncoding(TaskType.OperBox, AsstTaskType.OperBox) && AsstStart();
    }

    /// <summary>
    /// 牛牛抽卡。
    /// </summary>
    /// <param name="once">是否为单抽，默认为 true</param>
    /// <returns>是否成功。</returns>
    public bool AsstStartGacha(bool once = true)
    {
        var task = new AsstCustomTask() {
            CustomTasks = [once ? "GachaOnce" : "GachaTenTimes"],
        };
        var (type, param) = task.Serialize();
        return AsstAppendTaskWithEncoding(TaskType.Gacha, type, param) && AsstStart();
    }

    /// <summary>
    /// 小游戏。
    /// </summary>
    /// <param name="taskName">任务名（tasks.json 中的 key）</param>
    /// <returns>是否成功。</returns>
    public bool AsstMiniGame(string taskName)
    {
        var task = new AsstCustomTask() {
            CustomTasks = [taskName],
        };
        var (type, param) = task.Serialize();
        return AsstAppendTaskWithEncoding(TaskType.MiniGame, type, param) && AsstStart();
    }

    /*
    /// <summary>
    /// 视频识别。
    /// </summary>
    /// <param name="filename">文件路径</param>
    /// <returns>是否成功。</returns>
    public bool AsstStartVideoRec(string filename)
    {
        var taskParams = new JObject {
            ["filename"] = filename,
        };
        AsstTaskId id = AsstAppendTaskWithEncoding(AsstTaskType.VideoRecognition, taskParams);
        _tasksStatus.Add(id, (TaskType.Copilot, TaskStatus.Idle));
        return id != 0 && AsstStart();
    }
    */

    public bool AsstAppendTaskWithEncoding(TaskType wpfTasktype, AsstBaseTask task)
    {
        return AsstAppendTaskWithEncoding(wpfTasktype, task.Serialize());
    }

    public bool AsstAppendTaskWithEncoding(TaskType wpfTaskType, (AsstTaskType Type, JObject? TaskParams) task)
    {
        return AsstAppendTaskWithEncoding(wpfTaskType, task.Type, task.TaskParams);
    }

    public bool AsstAppendTaskWithEncoding(TaskType wpfTaskType, AsstTaskType type, JObject? taskParams = null)
    {
        taskParams ??= [];
        AsstTaskId id = AsstAppendTask(_handle, type.ToString(), JsonConvert.SerializeObject(taskParams));
        if (id == 0)
        {
            return false;
        }

        _tasksStatus.Add(id, (wpfTaskType, TaskStatus.Idle));
        return true;
    }

    public bool AsstSetTaskParamsEncoded(AsstTaskId id, AsstBaseTask task)
    {
        return AsstSetTaskParamsEncoded(id, task.Serialize().Params);
    }

    public bool AsstSetTaskParamsEncoded(AsstTaskId id, JObject? taskParams = null)
    {
        if (id == 0)
        {
            return false;
        }

        taskParams ??= [];
        return AsstSetTaskParams(_handle, id, JsonConvert.SerializeObject(taskParams));
    }

    /// <summary>
    /// 启动。
    /// </summary>
    /// <returns>是否成功。</returns>
    public bool AsstStart()
    {
        return MaaService.AsstStart(_handle);
    }

    /// <summary>
    /// 运行中。
    /// </summary>
    /// <returns>是否正在运行。</returns>
    public bool AsstRunning()
    {
        return MaaService.AsstRunning(_handle);
    }

    /// <summary>
    /// 停止。
    /// </summary>
    /// <returns>是否成功。</returns>
    public bool AsstStop()
    {
        return MaaService.AsstStop(_handle);
    }

    /// <summary>
    /// 销毁。
    /// </summary>
    public void AsstDestroy()
    {
        MaaService.AsstDestroy(_handle);
    }
}

/// <summary>
/// MaaCore 消息。
/// </summary>
[SuppressMessage("ReSharper", "UnusedMember.Global")]
public enum AsstMsg
{
    /* Global Info */

    /// <summary>
    /// 内部错误。
    /// </summary>
    InternalError = 0,

    /// <summary>
    /// 初始化失败。
    /// </summary>
    InitFailed,

    /// <summary>
    /// 连接相关错误。
    /// </summary>
    ConnectionInfo,

    /// <summary>
    /// 全部任务完成。
    /// </summary>
    AllTasksCompleted,

    /// <summary>
    /// 外部异步调用信息
    /// </summary>
    AsyncCallInfo,

    /// <summary>
    /// 实例已销毁
    /// </summary>
    Destroyed,

    /* TaskChain Info */

    /// <summary>
    /// 任务链执行/识别错误。
    /// </summary>
    TaskChainError = 10000,

    /// <summary>
    /// 任务链开始。
    /// </summary>
    TaskChainStart,

    /// <summary>
    /// 任务链完成。
    /// </summary>
    TaskChainCompleted,

    /// <summary>
    /// 任务链额外信息。
    /// </summary>
    TaskChainExtraInfo,

    /// <summary>
    /// 任务链手动停止
    /// </summary>
    TaskChainStopped,

    /* SubTask Info */

    /// <summary>
    /// 原子任务执行/识别错误。
    /// </summary>
    SubTaskError = 20000,

    /// <summary>
    /// 原子任务开始。
    /// </summary>
    SubTaskStart,

    /// <summary>
    /// 原子任务完成。
    /// </summary>
    SubTaskCompleted,

    /// <summary>
    /// 原子任务额外信息。
    /// </summary>
    SubTaskExtraInfo,

    /// <summary>
    /// 原子任务手动停止
    /// </summary>
    SubTaskStopped,

    /// <summary>
    /// 上报请求
    /// </summary>
    ReportRequest = 30000,
}

/// <summary>
/// 任务状态
/// </summary>
public enum TaskStatus
{
    /// <summary>
    /// 未开始
    /// </summary>
    Idle = 0,

    /// <summary>
    /// 进行中
    /// </summary>
    InProgress = 1,

    /// <summary>
    /// 已完成
    /// </summary>
    Completed = 2,

    /// <summary>
    /// 错误终止
    /// </summary>
    Error = 3,
}

public enum AsstStaticOptionKey
{
    /// <summary>
    /// 无效
    /// </summary>
    Invalid,

    /// <summary>
    /// 用CPU进行OCR
    /// </summary>
    CpuOCR,

    /// <summary>
    /// 用GPU进行OCR
    /// </summary>
    GpuOCR,
}

public enum InstanceOptionKey
{
    /* Deprecated */ // MinitouchEnabled = 1,

    /// <summary>
    /// Indicates the touch mode.
    /// </summary>
    TouchMode = 2,

    /// <summary>
    /// Indicates whether the deployment should be paused.
    /// </summary>
    DeploymentWithPause = 3,

    /// <summary>
    /// Indicates whether AdbLite is used.
    /// </summary>
    AdbLiteEnabled = 4,

    /// <summary>
    /// Indicates whether the ADB server process should be killed when the instance is exited.
    /// </summary>
    KillAdbOnExit = 5,
}
