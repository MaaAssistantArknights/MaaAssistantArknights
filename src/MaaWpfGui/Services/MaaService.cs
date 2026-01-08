// <copyright file="MaaService.cs" company="MaaAssistantArknights">
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

using System;
using System.Runtime.InteropServices;
using MaaWpfGui.Main;
using Newtonsoft.Json.Linq;
using AsstHandle = System.IntPtr;
using AsstInstanceOptionKey = System.Int32;
using AsstTaskId = System.Int32;

namespace MaaWpfGui.Services;

#pragma warning disable SA1601 // Partial elements should be documented
internal static partial class MaaService
{
    internal delegate void CallbackDelegate(int msg, IntPtr jsonBuffer, IntPtr customArg);

    internal delegate void ProcCallbackMsg(AsstMsg msg, JObject details);

    [LibraryImport("MaaCore.dll")]
    internal static partial AsstHandle AsstCreateEx(CallbackDelegate callback, IntPtr customArg);

    [LibraryImport("MaaCore.dll")]
    internal static partial void AsstDestroy(AsstHandle handle);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static unsafe partial bool AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, byte* value);

    [LibraryImport("MaaCore.dll", StringMarshalling = StringMarshalling.Utf8)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool AsstSetStaticOption(AsstStaticOptionKey key, string value);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static unsafe partial bool AsstSetUserDir(byte* dirname);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static unsafe partial bool AsstLoadResource(byte* dirname);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static unsafe partial bool AsstConnect(AsstHandle handle, byte* adbPath, byte* address, byte* config);

    [LibraryImport("MaaCore.dll")]
    internal static unsafe partial AsstTaskId AsstAppendTask(AsstHandle handle, byte* type, byte* taskParams);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static unsafe partial bool AsstSetTaskParams(AsstHandle handle, AsstTaskId id, byte* taskParams);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool AsstStart(AsstHandle handle);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool AsstRunning(AsstHandle handle);

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool AsstStop(AsstHandle handle);

    [LibraryImport("MaaCore.dll")]
    internal static unsafe partial Int32 AsstAsyncScreencap(AsstHandle handle, [MarshalAs(UnmanagedType.Bool)] bool block);

    [LibraryImport("MaaCore.dll")]
    internal static unsafe partial ulong AsstGetImage(AsstHandle handle, byte* buff, ulong buffSize);

    [LibraryImport("MaaCore.dll")]
    internal static unsafe partial ulong AsstGetImageBgr(AsstHandle handle, byte* buff, ulong buffSize);

    [LibraryImport("MaaCore.dll")]
    internal static partial ulong AsstGetNullSize();

    [LibraryImport("MaaCore.dll")]
    internal static partial IntPtr AsstGetVersion();

    [LibraryImport("MaaCore.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool AsstBackToHome(AsstHandle handle);

    [LibraryImport("MaaCore.dll")]
    internal static unsafe partial void AsstSetConnectionExtras(byte* name, byte* extras);
}
#pragma warning restore SA1601 // Partial elements should be documented

public enum AsstTaskType : byte
{
    /// <summary>
    /// 开始唤醒。
    /// </summary>
    StartUp = 0,

    /// <summary>
    /// 关闭明日方舟
    /// </summary>
    CloseDown,

    /// <summary>
    /// 理智作战
    /// </summary>
    Fight,

    /// <summary>
    /// 领取奖励
    /// </summary>
    Award,

    /// <summary>
    /// 信用商店
    /// </summary>
    Mall,

    /// <summary>
    /// 基建
    /// </summary>
    Infrast,

    /// <summary>
    /// 招募
    /// </summary>
    Recruit,

    /// <summary>
    /// 肉鸽
    /// </summary>
    Roguelike,

    /// <summary>
    /// 自动战斗
    /// </summary>
    Copilot,

    /// <summary>
    /// 自动战斗-保全ver
    /// </summary>
    SSSCopilot,

    /// <summary>
    /// 单步任务（目前仅支持战斗）
    /// </summary>
    SingleStep,

    /*
    /// <summary>
    /// 视频识别
    /// </summary>
    VideoRecognition,
    */

    /// <summary>
    /// 仓库识别
    /// </summary>
    Depot,

    /// <summary>
    /// 干员识别
    /// </summary>
    OperBox,

    /// <summary>
    /// 生息演算
    /// </summary>
    Reclamation,

    /// <summary>
    /// 自定义任务
    /// </summary>
    Custom,
}
