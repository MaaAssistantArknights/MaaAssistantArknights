// <copyright file="InferenceBackend.cs" company="MaaAssistantArknights">
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
using System;

namespace MaaWpfGui.Constants.Enums;

public enum InferenceBackend
{
    /// <summary>
    /// 自动选择最成熟稳定的后端（推荐）。
    /// </summary>
    Auto = 0,

    /// <summary>
    /// Microsoft DirectML 后端。
    /// </summary>
    DirectML = 1,

    /// <summary>
    /// Google Dawn WebGPU 后端（实验性）。
    /// </summary>
    WebGPU = 2,

    /// <summary>
    /// Intel OpenVINO 后端，使用 CPU 设备。
    /// </summary>
    OpenVINOCpu = 3,

    /// <summary>
    /// Intel OpenVINO 后端，使用 NPU 设备。
    /// </summary>
    OpenVINONpu = 4,

    /// <summary>
    /// Intel OpenVINO 混合后端：OCR 使用 CPU，ONNX 会话使用 NPU。
    /// NPU 只加速 ONNX 会话，并非全链路加速。
    /// </summary>
    OpenVINOHybrid = 5,
}

/// <summary>
/// 推理后端的一级分类，用于两级下拉布局。
/// </summary>
public enum BackendFamily
{
    /// <summary>自动选择。</summary>
    Auto = 0,

    /// <summary>Microsoft DirectML。</summary>
    DirectML = 1,

    /// <summary>Google Dawn WebGPU（实验性）。</summary>
    WebGPU = 2,

    /// <summary>Intel OpenVINO（实验性），设备由二级下拉选择。</summary>
    OpenVINO = 3,
}

/// <summary>
/// OpenVINO 的设备选择（二级下拉）。
/// </summary>
public enum OpenVinoDeviceChoice
{
    /// <summary>OCR 与 ONNX 会话均使用 CPU。</summary>
    CPU = 0,

    /// <summary>OCR 与 ONNX 会话均使用 NPU。</summary>
    NPU = 1,

    /// <summary>OCR 使用 CPU，ONNX 会话使用 NPU。</summary>
    Hybrid = 2,
}
