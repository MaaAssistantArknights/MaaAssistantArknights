// <copyright file="AuthenticodeHelper.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1310 // 常量沿用 wintrust.h 原名（WTD_* / TRUST_E_*），便于与 SDK 定义对照
#pragma warning disable SA1401 // Win32 结构体按原始布局映射，字段须为公共字段

#nullable enable

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

namespace MaaWpfGui.Helper;

/// <summary>
/// 基于 WinVerifyTrust 的 Authenticode 签名验证，并对签名者做固定发布者校验。
/// </summary>
internal static class AuthenticodeHelper
{
    /// <summary>
    /// 允许的签名者证书发布者名称（证书 SimpleName，与 CI 校验一致，区分大小写）。
    /// SignPath Foundation 的开源项目证书均以此名称签发，每个项目证书互不相同，
    /// 因此除名称外还需比对证书指纹。
    /// </summary>
    private const string TrustedSignerSimpleName = "SignPath Foundation";

    /// <summary>
    /// 允许的签名者证书 SHA-256 指纹列表（与仓库 Actions 变量 SIGNPATH_RELEASE_CERTIFICATE_SHA256 同值，
    /// 由 .github/scripts/windows-signing.ps1 在 CI 侧使用）。SignPath 更换证书时在此追加新指纹
    /// （保留旧指纹，存量旧包的签名仍应验证通过）；列表为空时退化为仅校验发布者名称。
    /// </summary>
    private static readonly HashSet<string> _trustedSignerCertificateSha256Thumbprints = new(StringComparer.OrdinalIgnoreCase)
    {
    };

    // WINTRUST_ACTION_GENERIC_VERIFY_V2（softpub.h），即 Authenticode 校验策略
    private static readonly Guid _winTrustActionGenericVerifyV2 = new("00AAC56B-CD44-11D0-8CC2-00C04FC295EE");

    private const uint WTD_UI_NONE = 2;

    private const uint WTD_REVOKE_NONE = 0x00000000;

    private const uint WTD_CHOICE_FILE = 1;

    private const uint WTD_STATEACTION_VERIFY = 0x00000001;

    private const uint WTD_STATEACTION_CLOSE = 0x00000002;

    private const int TRUST_E_NOSIGNATURE = unchecked((int)0x800B0100);

    internal enum SignatureVerificationStatus
    {
        /// <summary>签名链有效且签名者匹配固定发布者。</summary>
        Trusted,

        /// <summary>文件未包含 Authenticode 签名。</summary>
        Unsigned,

        /// <summary>签名链验证失败，或签名者与固定发布者不符。</summary>
        Invalid,
    }

    /// <summary>
    /// 验证文件的 Authenticode 签名，并要求签名者证书与固定发布者匹配。
    /// </summary>
    /// <param name="filePath">待验证文件路径。</param>
    /// <param name="failureDetail">验证未通过时的细节描述（错误码或签名者信息），供日志使用。</param>
    /// <returns>验证结果三态。</returns>
    internal static SignatureVerificationStatus VerifyFile(string filePath, out string? failureDetail)
    {
        failureDetail = null;

        Guid actionId = _winTrustActionGenericVerifyV2;
        IntPtr fileInfoPtr = Marshal.AllocHGlobal(Marshal.SizeOf<WinTrustFileInfo>());
        WinTrustData trustData = default;
        try
        {
            Marshal.StructureToPtr(
                new WinTrustFileInfo
                {
                    CbStruct = (uint)Marshal.SizeOf<WinTrustFileInfo>(),
                    PcwszFilePath = filePath,
                },
                fileInfoPtr,
                fDeleteOld: false);

            trustData.CbStruct = (uint)Marshal.SizeOf<WinTrustData>();
            trustData.DwUIChoice = WTD_UI_NONE;
            trustData.FdwRevocationChecks = WTD_REVOKE_NONE;
            trustData.DwUnionChoice = WTD_CHOICE_FILE;
            trustData.PFile = fileInfoPtr;
            trustData.DwStateAction = WTD_STATEACTION_VERIFY;

            int error = WinVerifyTrust(IntPtr.Zero, ref actionId, ref trustData);
            if (error != 0)
            {
                failureDetail = $"WinVerifyTrust error 0x{error:X8}";
                return error == TRUST_E_NOSIGNATURE ? SignatureVerificationStatus.Unsigned : SignatureVerificationStatus.Invalid;
            }

            return VerifySignerPublisher(trustData.HWVTStateData, out failureDetail);
        }
        finally
        {
            CloseStateData(ref actionId, ref trustData);

            // StructureToPtr 为 string 字段另行分配的非托管副本须先销毁，否则每次调用泄漏
            Marshal.DestroyStructure<WinTrustFileInfo>(fileInfoPtr);
            Marshal.FreeHGlobal(fileInfoPtr);
        }
    }

    /// <summary>
    /// 校验签名者证书是否与固定发布者匹配（SimpleName 区分大小写比对、SHA-256 指纹忽略大小写比对，
    /// 与 CI 签名校验脚本保持一致）。时间戳不单独校验：WinVerifyTrust 通过即代表
    /// 签名时刻（含时间戳时间）的证书链有效，带有效时间戳的签名在证书过期后仍可通过。
    /// </summary>
    /// <param name="stateData">WinVerifyTrust VERIFY 状态句柄。</param>
    /// <param name="failureDetail">校验未通过时的细节描述，供日志使用。</param>
    /// <returns>签名者匹配返回 <see cref="SignatureVerificationStatus.Trusted"/>，否则 <see cref="SignatureVerificationStatus.Invalid"/>。</returns>
    private static SignatureVerificationStatus VerifySignerPublisher(IntPtr stateData, out string? failureDetail)
    {
        failureDetail = null;

        IntPtr provData = WTHelperProvDataFromStateData(stateData);
        if (provData == IntPtr.Zero)
        {
            failureDetail = "WTHelperProvDataFromStateData returned null";
            return SignatureVerificationStatus.Invalid;
        }

        IntPtr provSigner = WTHelperGetProvSignerFromChain(provData, idxSigner: 0, fCounterSigner: false, idxCounterSigner: 0);
        if (provSigner == IntPtr.Zero)
        {
            failureDetail = "WTHelperGetProvSignerFromChain returned null";
            return SignatureVerificationStatus.Invalid;
        }

        var signer = Marshal.PtrToStructure<CryptProviderSgnr>(provSigner);
        if (signer.CsCertChain == 0 || signer.PasCertChain == IntPtr.Zero)
        {
            failureDetail = "Signer certificate chain is empty";
            return SignatureVerificationStatus.Invalid;
        }

        // 链首即签名者证书（后续为颁发链，发布者校验只针对签名者本身）
        var signerCertificateEntry = Marshal.PtrToStructure<CryptProviderCert>(signer.PasCertChain);
        if (signerCertificateEntry.PCert == IntPtr.Zero)
        {
            failureDetail = "Signer certificate context is null";
            return SignatureVerificationStatus.Invalid;
        }

        // X509Certificate2(IntPtr) 会复制证书上下文并自行管理引用计数，不影响 provider 侧生命周期
        using var certificate = new X509Certificate2(signerCertificateEntry.PCert);
        string simpleName = certificate.GetNameInfo(X509NameType.SimpleName, false);
        if (!string.Equals(simpleName, TrustedSignerSimpleName, StringComparison.Ordinal))
        {
            failureDetail = $"Unexpected signer simple name '{simpleName}'";
            return SignatureVerificationStatus.Invalid;
        }

        string certificateSha256 = certificate.GetCertHashString(HashAlgorithmName.SHA256);
        if (_trustedSignerCertificateSha256Thumbprints.Count > 0 && !_trustedSignerCertificateSha256Thumbprints.Contains(certificateSha256))
        {
            failureDetail = $"Unexpected signer certificate SHA-256 {certificateSha256}";
            return SignatureVerificationStatus.Invalid;
        }

        return SignatureVerificationStatus.Trusted;
    }

    private static void CloseStateData(ref Guid actionId, ref WinTrustData trustData)
    {
        if (trustData.HWVTStateData == IntPtr.Zero)
        {
            return;
        }

        trustData.DwStateAction = WTD_STATEACTION_CLOSE;
        _ = WinVerifyTrust(IntPtr.Zero, ref actionId, ref trustData);
        trustData.HWVTStateData = IntPtr.Zero;
    }

    [DllImport("wintrust.dll", ExactSpelling = true)]
    private static extern int WinVerifyTrust(IntPtr hwnd, ref Guid pgActionId, ref WinTrustData pWVTData);

    [DllImport("wintrust.dll", ExactSpelling = true)]
    private static extern IntPtr WTHelperProvDataFromStateData(IntPtr hStateData);

    [DllImport("wintrust.dll", ExactSpelling = true)]
    private static extern IntPtr WTHelperGetProvSignerFromChain(IntPtr pProvData, uint idxSigner, [MarshalAs(UnmanagedType.Bool)] bool fCounterSigner, uint idxCounterSigner);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct WinTrustFileInfo
    {
        public uint CbStruct;
        public string PcwszFilePath;
        public IntPtr HFile;
        public IntPtr PgKnownSubject;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct WinTrustData
    {
        public uint CbStruct;
        public IntPtr PPolicyCallbackData;
        public IntPtr PSIPClientData;
        public uint DwUIChoice;
        public uint FdwRevocationChecks;
        public uint DwUnionChoice;
        public IntPtr PFile;
        public uint DwStateAction;
        public IntPtr HWVTStateData;
        public IntPtr PwszURLReference;
        public uint DwProvFlags;
        public uint DwUIContext;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct CryptProviderSgnr
    {
        public uint CbStruct;

        // FILETIME 拆为两个 uint：wintrust.h 该区域 pack(8) 但 FILETIME 自身按 4 对齐、
        // 位于 cbStruct 后的 offset 4，合并成 long 会引入 8 字节对齐使后续字段全部错位
        public uint SftVerifyAsOfLow;
        public uint SftVerifyAsOfHigh;
        public uint CsCertChain;
        public IntPtr PasCertChain;
        public uint DwSignerType;
        public IntPtr PsSigner;
        public uint DwError;
        public uint CsCounterSigners;
        public IntPtr PasCounterSigners;
        public IntPtr PChainContext;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct CryptProviderCert
    {
        public uint CbStruct;
        public IntPtr PCert;
    }
}
