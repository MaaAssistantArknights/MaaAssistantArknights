// <copyright file="PendingUpdateApplier.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.WineCompat;
using Newtonsoft.Json.Linq;
using Semver;
using Serilog;

namespace MaaWpfGui.Services;

/// <summary>
/// Handles the application of pending update packages for the application. This includes validating, extracting, and applying both OTA and full update packages that have been registered for installation on the next application launch.
/// </summary>
internal static partial class PendingUpdateApplier
{
    private static readonly ILogger _logger = Log.ForContext(typeof(PendingUpdateApplier));

    [GeneratedRegex(@"^MAAComponent-OTA-(?<from>v.+?|DEBUG_VERSION)_(?<to>v.+?)-win-(?<arch>x64|arm64)\.zip$", RegexOptions.IgnoreCase | RegexOptions.Compiled | RegexOptions.CultureInvariant)]
    private static partial Regex OtaPackageNameRegex();

    [GeneratedRegex(@"^MAA-(?<version>v.+?)-win-(?<arch>x64|arm64)\.zip$", RegexOptions.IgnoreCase | RegexOptions.Compiled | RegexOptions.CultureInvariant)]
    private static partial Regex FullPackageNameRegex();

    private static readonly HashSet<string> _controlFiles = new(StringComparer.OrdinalIgnoreCase)
    {
        "removelist.txt",
        "changes.json",
    };

    private static readonly HashSet<string> _fullPackagePreservedEntries = new(StringComparer.OrdinalIgnoreCase)
    {
        "achievement",
        "cache",
        "config",
        "data",
        "debug",
        "MAA.Updater.exe",
    };

    // 更新包签名验证的文件集，与 CI 签名白名单 .signpath/windows.xml 保持一致；
    // 解压目录中存在才验，纯资源 OTA 等不含这些文件的包不受影响
    private static readonly HashSet<string> _signatureVerificationFileNames = new(StringComparer.OrdinalIgnoreCase)
    {
        "MAA.exe",
        "MAA.dll",
        "MAA.Updater.exe",
        "MaaCore.dll",
        "MaaUtils.dll",
    };

    // 完整包清理时的嵌套保留目录（相对安装根目录，反斜杠分隔）：
    // 用于随包发布、同时允许用户存放自有文件的目录（如壁纸目录 Res\Backgrounds\Wallpapers），
    // 不随完整包更新清理，其上级目录（如 Res）下的其他内容仍正常清理
    private static readonly HashSet<string> _fullPackagePreservedNestedEntries = new(StringComparer.OrdinalIgnoreCase)
    {
        @"Res\Backgrounds\Wallpapers",
    };

    private static string DelegatedUpdateSuccessStatusFilePath => Path.Combine(PathsHelper.BaseDir, "pending-update-success.txt");

    private static string DelegatedUpdateFailureStatusFilePath => Path.Combine(PathsHelper.BaseDir, "pending-update-failure.txt");

    public enum LocalPackageImportStatus
    {
        /// <summary>
        /// Represents an unsupported operation or value.
        /// </summary>
        Unsupported,

        /// <summary>
        /// Indicates that a full update package has been successfully registered for installation.
        /// </summary>
        FullPackageRegistered,

        /// <summary>
        /// Indicates that an OTA update package has been successfully registered for installation.
        /// </summary>
        OtaPackageRegistered,
    }

    public sealed record LocalPackageImportResult(
        LocalPackageImportStatus Status,
        string? SourceVersion = null,
        string? TargetVersion = null);

    public enum PackageInspectionStatus
    {
        /// <summary>文件不存在。</summary>
        MissingFile,

        /// <summary>文件名不匹配任何已知更新包模式（完整包 / OTA 包）。</summary>
        NotMatched,

        /// <summary>完整包：架构或版本升级方向不符合要求。</summary>
        FullRejected,

        /// <summary>完整包：架构、版本方向均符合要求。</summary>
        FullSupported,

        /// <summary>OTA 包：架构、源版本或版本升级方向不符合要求。</summary>
        OtaRejected,

        /// <summary>OTA 包：架构、源版本、版本方向均符合要求。</summary>
        OtaSupported,
    }

    public sealed record PackageInspectionResult(
        PackageInspectionStatus Status,
        string? SourceVersion = null,
        string? TargetVersion = null)
    {
        public bool IsSupported =>
            Status is PackageInspectionStatus.FullSupported or PackageInspectionStatus.OtaSupported;

        /// <summary>是否匹配了任意一种版本更新包文件名模式（完整包或 OTA 包）。</summary>
        public bool MatchedPattern =>
            Status is not (PackageInspectionStatus.MissingFile or PackageInspectionStatus.NotMatched);
    }

    public static bool HasPendingUpdatePackage()
    {
        string updateTag = ConfigFactory.Root.Update.Name;
        string updatePackageName = ConfigFactory.Root.Update.UpdatePackage;
        return updateTag != string.Empty && updatePackageName != string.Empty && File.Exists(updatePackageName);
    }

    public static LocalPackageImportResult TryRegisterLocalPackage(
        string packagePath,
        string currentVersion,
        string architecture,
        PackageInspectionResult? inspection,
        bool allowSameVersion = false)
    {
        inspection ??= InspectLocalUpdatePackage(packagePath, currentVersion, architecture, allowSameVersion);

        switch (inspection.Status)
        {
            case PackageInspectionStatus.MissingFile:
                _logger.Warning("Dropped update package does not exist: {PackagePath}", packagePath);
                return new(LocalPackageImportStatus.Unsupported);

            case PackageInspectionStatus.FullSupported:
                return RegisterSupportedLocalFullPackage(packagePath, inspection.TargetVersion);

            case PackageInspectionStatus.OtaSupported:
                return RegisterSupportedLocalOtaPackage(packagePath, inspection.SourceVersion, inspection.TargetVersion);

            // FullRejected / OtaRejected / NotMatched
            default:
                _logger.Warning(
                    "Dropped package rejected or unrecognized: status={Status}, sourceVersion={SourceVersion}, targetVersion={TargetVersion}",
                    inspection.Status,
                    inspection.SourceVersion,
                    inspection.TargetVersion);
                return new(LocalPackageImportStatus.Unsupported, inspection.SourceVersion, inspection.TargetVersion);
        }
    }

    /// <summary>
    /// 通过文件名检测拖入的压缩包是完整更新包还是 OTA 更新包，并校验架构与版本方向。
    /// 两种正则都不匹配时返回 <see cref="PackageInspectionStatus.NotMatched"/>（可能是资源包或无关文件）。
    /// </summary>
    /// <param name="packagePath">拖入的压缩包路径。</param>
    /// <param name="currentVersion">当前 MAA 版本号（如 v5.10.0）。</param>
    /// <param name="architecture">当前系统架构（如 x64、arm64）。</param>
    /// <param name="allowSameVersion">允许目标版本与当前版本相同（用于文件完整性修复的重装场景）。</param>
    /// <returns>检测结果，包含状态、源版本（OTA）、目标版本。</returns>
    public static PackageInspectionResult InspectLocalUpdatePackage(string packagePath, string currentVersion, string architecture, bool allowSameVersion = false)
    {
        if (!File.Exists(packagePath))
        {
            return new(PackageInspectionStatus.MissingFile);
        }

        string fullPackagePath = Path.GetFullPath(packagePath);
        string fileName = Path.GetFileName(fullPackagePath);
        string normalizedArchitecture = NormalizeArchitecture(architecture);
        _logger.Information(
            "Checking dropped update package: {PackageName}, currentVersion={CurrentVersion}, architecture={Architecture}",
            fileName,
            currentVersion,
            normalizedArchitecture);

        // ① 完整包：MAA-vX.X.X-win-x64.zip
        Match fullPackageMatch = FullPackageNameRegex().Match(fileName);
        if (fullPackageMatch.Success)
        {
            string targetVersion = fullPackageMatch.Groups["version"].Value;
            string packageArchitecture = fullPackageMatch.Groups["arch"].Value;
            bool architectureMatched = string.Equals(normalizedArchitecture, packageArchitecture, StringComparison.OrdinalIgnoreCase);
            bool isUpgradeTarget = IsUpgradeTarget(currentVersion, targetVersion)
                || (allowSameVersion && VersionsMatch(currentVersion, targetVersion));

            _logger.Information(
                "Dropped package matched full package pattern: targetVersion={TargetVersion}, packageArchitecture={PackageArchitecture}",
                targetVersion,
                packageArchitecture);

            if (!architectureMatched || !isUpgradeTarget)
            {
                _logger.Warning(
                    "Dropped full package rejected: architectureMatched={ArchitectureMatched}, isUpgradeTarget={IsUpgradeTarget}",
                    architectureMatched,
                    isUpgradeTarget);
                return new(PackageInspectionStatus.FullRejected, TargetVersion: targetVersion);
            }

            return new(PackageInspectionStatus.FullSupported, TargetVersion: targetVersion);
        }

        // ② OTA 包：MAAComponent-OTA-vFROM_vTO-win-x64.zip
        Match otaMatch = OtaPackageNameRegex().Match(fileName);
        if (otaMatch.Success)
        {
            string sourceVersion = otaMatch.Groups["from"].Value;
            string targetVersion = otaMatch.Groups["to"].Value;
            string packageArchitecture = otaMatch.Groups["arch"].Value;

            _logger.Information(
                "Dropped package matched OTA pattern: sourceVersion={SourceVersion}, targetVersion={TargetVersion}, packageArchitecture={PackageArchitecture}",
                sourceVersion,
                targetVersion,
                packageArchitecture);

            bool architectureMatched = string.Equals(normalizedArchitecture, packageArchitecture, StringComparison.OrdinalIgnoreCase);
            bool sourceVersionMatched = VersionsMatch(sourceVersion, currentVersion);
            bool isUpgradeTarget = IsUpgradeTarget(sourceVersion, targetVersion);

            if (!architectureMatched || !sourceVersionMatched || !isUpgradeTarget)
            {
                _logger.Warning(
                    "Dropped OTA package rejected: architectureMatched={ArchitectureMatched}, sourceVersionMatched={SourceVersionMatched}, isUpgradeTarget={IsUpgradeTarget}",
                    architectureMatched,
                    sourceVersionMatched,
                    isUpgradeTarget);
                return new(PackageInspectionStatus.OtaRejected, sourceVersion, targetVersion);
            }

            return new(PackageInspectionStatus.OtaSupported, sourceVersion, targetVersion);
        }

        // ③ 都不匹配（资源包或无关文件），交给调用方决定后续处理
        return new(PackageInspectionStatus.NotMatched);
    }

    private static LocalPackageImportResult RegisterSupportedLocalFullPackage(string packagePath, string? targetVersion)
    {
        string fullPackagePath = Path.GetFullPath(packagePath);
        RegisterPendingUpdatePackage(targetVersion ?? string.Empty, fullPackagePath);
        _logger.Information(
            "Dropped full package registered successfully: packagePath={PackagePath}, targetVersion={TargetVersion}",
            fullPackagePath,
            targetVersion);
        return new(LocalPackageImportStatus.FullPackageRegistered, null, targetVersion);
    }

    private static LocalPackageImportResult RegisterSupportedLocalOtaPackage(string packagePath, string? sourceVersion, string? targetVersion)
    {
        string fullPackagePath = Path.GetFullPath(packagePath);
        RegisterPendingUpdatePackage(targetVersion ?? string.Empty, fullPackagePath);
        _logger.Information(
            "Dropped OTA package registered successfully: packagePath={PackagePath}, sourceVersion={SourceVersion}, targetVersion={TargetVersion}",
            fullPackagePath,
            sourceVersion,
            targetVersion);
        return new(LocalPackageImportStatus.OtaPackageRegistered, sourceVersion, targetVersion);
    }

    /// <summary>
    /// 应用待处理的更新包：委托外部 updater 或进程内应用。
    /// 签名验证发现需用户确认的问题时按 <paramref name="interactive"/> 分流：允许交互则弹连续确认弹窗，
    /// 不允许（启动早期，主窗口未创建，弹窗是唯一窗口）则不动 pending 状态与包文件直接返回
    /// <see cref="PendingUpdateApplyResult.StatusKind.SignatureIssueDeferred"/>，由调用方在主窗口加载后重入本流程。
    /// </summary>
    /// <param name="interactive">是否允许弹窗交互。</param>
    /// <returns>应用结果。</returns>
    public static PendingUpdateApplyResult TryApplyPendingUpdatePackage(bool interactive = true)
    {
        string updateTag = ConfigFactory.Root.Update.Name;
        string updatePackageName = ConfigFactory.Root.Update.UpdatePackage;
        if (updateTag == string.Empty || updatePackageName == string.Empty || !File.Exists(updatePackageName))
        {
            return new(PendingUpdateApplyResult.StatusKind.NoPendingPackage);
        }

        var context = new PendingUpdateContext(
            PathsHelper.BaseDir,
            updatePackageName,
            Path.Combine(PathsHelper.BaseDir, "NewVersionExtract"),
            Path.Combine(PathsHelper.BaseDir, ".old"));

        bool installationChanged = false;
        bool clearPendingPackageState = false;
        bool keepExtractDirectory = false;

        try
        {
            PrepareExtractDirectory(context.ExtractDir);

            try
            {
                ExtractPendingUpdatePackage(context.PackagePath, context.ExtractDir);
            }
            catch (InvalidDataException ex)
            {
                _logger.Warning(ex, "Pending update package is invalid: {PackagePath}", context.PackagePath);
                ClearPendingUpdatePackageState();
                return new(PendingUpdateApplyResult.StatusKind.InvalidPackage, FailureReason: ex.Message);
            }

            // 签名验证位于三分支判定（完整包委派 / OTA 委派 / 进程内 OTA）之前，
            // 同时先于 delegated updater 同名 exe 的覆盖，验证对象即后续被复制/移动的解压产物本身
            if (TryCollectSignatureIssue(context, out var overallStatus, out string? failureDetail))
            {
                if (!interactive)
                {
                    _logger.Information(
                        "Signature issue in pending update package deferred until interaction is allowed: {PackagePath}",
                        context.PackagePath);
                    return new(PendingUpdateApplyResult.StatusKind.SignatureIssueDeferred);
                }

                if (!ConfirmApplyPackageWithSignatureIssue(overallStatus, failureDetail))
                {
                    ClearPendingUpdatePackageState();
                    return new(PendingUpdateApplyResult.StatusKind.UserDeclinedSignature);
                }
            }

            if (!PendingUpdateManifest.HasOtaMetadata(context.ExtractDir))
            {
                keepExtractDirectory = true;
                string[] fullPackageMoveEntries = GetFullPackageMoveEntries(context.ExtractDir);
                return DelegatePendingUpdateApply(
                    context,
                    "full",
                    "full package always replaces runtime files",
                    GetFullPackageRemoveEntries(context),
                    fullPackageMoveEntries);
            }

            var manifest = PendingUpdateManifest.Load(context.ExtractDir);
            if (ShouldDelegatePendingOtaApply(manifest, out string delegationReason))
            {
                keepExtractDirectory = true;
                return DelegatePendingUpdateApply(
                    context,
                    "ota",
                    delegationReason,
                    manifest.RemoveList,
                    manifest.PayloadFiles);
            }

            _logger.Information("Applying pending OTA package in current process because it only touches resource files.");
            ApplyOtaPackage(context, manifest, ref installationChanged);

            SafeDeleteFile(context.PackagePath);
            MarkPendingUpdateApplied();
            return new(PendingUpdateApplyResult.StatusKind.Succeeded);
        }
        catch (InvalidDataException ex)
        {
            _logger.Error(ex, "Pending update package was rejected: {PackagePath}", context.PackagePath);
            if (!installationChanged)
            {
                ClearPendingUpdatePackageState();
                return new(PendingUpdateApplyResult.StatusKind.InvalidPackage, FailureReason: ex.Message);
            }

            clearPendingPackageState = true;
            return new(PendingUpdateApplyResult.StatusKind.Failed, true, ex.Message);
        }
        catch (FileNotFoundException ex) when (!installationChanged && IsMissingDelegatedUpdaterExecutable(ex))
        {
            _logger.Error(ex, "External updater executable is missing while applying pending update package: {PackagePath}", context.PackagePath);
            clearPendingPackageState = true;
            return new(PendingUpdateApplyResult.StatusKind.MissingUpdaterExecutable, FailureReason: ex.Message);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to apply pending update package: {PackagePath}", context.PackagePath);
            if (installationChanged)
            {
                clearPendingPackageState = true;
                return new(PendingUpdateApplyResult.StatusKind.Failed, true, ex.Message);
            }

            clearPendingPackageState = true;
            return new(PendingUpdateApplyResult.StatusKind.Failed, FailureReason: ex.Message);
        }
        finally
        {
            if (clearPendingPackageState)
            {
                ClearPendingUpdatePackageState();
            }

            if (!keepExtractDirectory)
            {
                SafeDeleteDirectory(context.ExtractDir);
            }
        }
    }

    /// <summary>
    /// 判断委托更新失败标志文件是否存在（只读检查，无副作用）。
    /// 存在期间安装可能处于半更新状态，只允许完整包更新。
    /// </summary>
    /// <returns>失败标志文件存在时为 <c>true</c>。</returns>
    public static bool HasDelegatedUpdateFailure()
    {
        return File.Exists(DelegatedUpdateFailureStatusFilePath);
    }

    /// <summary>
    /// 读取外部更新器写入的失败状态。标志文件只读不删：须跨启动持久保留
    /// （避免用户忽略提示后重启导致半更新状态无人提醒），直至完整包安装时随根目录清场、
    /// 或注册新更新包时（注册即代表用户已着手修复，旧失败原因失效）移除。
    /// 读取同时会清空待应用更新包配置（沿用旧消费语义：失败后不再自动重试该包）。
    /// </summary>
    /// <param name="failureReason">更新器写入的 UTF-8 失败原因，读取失败时为 <c>null</c>。</param>
    /// <returns>失败标志文件存在（上次更新失败）时为 <c>true</c>。</returns>
    public static bool TryReadDelegatedUpdateFailure(out string? failureReason)
    {
        failureReason = null;
        if (!HasDelegatedUpdateFailure())
        {
            return false;
        }

        try
        {
            failureReason = File.ReadAllText(DelegatedUpdateFailureStatusFilePath).Trim();
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to read delegated update failure state: {FailureStateFilePath}", DelegatedUpdateFailureStatusFilePath);
        }

        ClearPendingUpdatePackageState();
        return true;
    }

    /// <summary>
    /// 写入更新失败状态文件，格式与外部更新器写入的一致（UTF-8 纯文本原因），
    /// 供进程内应用失败（RequiresManualRecovery）等场景持久化失败状态。
    /// </summary>
    /// <param name="failureReason">失败原因，原样写入状态文件</param>
    public static void MarkDelegatedUpdateFailure(string failureReason)
    {
        try
        {
            File.WriteAllText(DelegatedUpdateFailureStatusFilePath, failureReason, Encoding.UTF8);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to write delegated update failure state: {FailureStateFilePath}", DelegatedUpdateFailureStatusFilePath);
        }
    }

    /// <summary>
    /// 删除委托更新失败标志，供所有注册新更新包的路径统一调用。
    /// 注册即代表用户已着手修复（手动下载完整包、拖入本地包等），旧失败原因失效；
    /// 标志不删的话，下次启动 <see cref="TryReadDelegatedUpdateFailure"/> 会连刚注册的包一起清空，形成死循环。
    /// </summary>
    public static void ClearDelegatedUpdateFailureState()
    {
        SafeDeleteFile(DelegatedUpdateFailureStatusFilePath, "delegated update failure state");
    }

    /// <summary>
    /// 将更新器写入的英文失败原因映射为本地化的展示文案。
    /// </summary>
    /// <param name="failureReason">更新器写入的失败原因。</param>
    /// <returns>可直接展示给用户的文案；双语 reason（多实例互斥等）原样返回。</returns>
    public static string GetDelegatedUpdateFailureDescription(string? failureReason)
    {
        if (string.IsNullOrWhiteSpace(failureReason))
        {
            return LocalizationHelper.GetString("DelegatedUpdateFailureReasonUnknown");
        }

        // 文件搬移类失败：多为文件占用（安全软件扫描新解压的文件、其他程序锁定）
        if (failureReason.Contains("Failed to move to backup:", StringComparison.Ordinal) ||
            failureReason.Contains("Failed to back up existing entry:", StringComparison.Ordinal) ||
            failureReason.Contains("Failed to move file into place:", StringComparison.Ordinal))
        {
            return LocalizationHelper.GetString("DelegatedUpdateFailureReasonFileLocked");
        }

        // 计划文件/路径类失败：更新包或其解压产物异常
        if (failureReason.Contains("Plan file", StringComparison.Ordinal) ||
            failureReason.Contains("Failed to open file:", StringComparison.Ordinal) ||
            failureReason.Contains("Failed to read file:", StringComparison.Ordinal) ||
            failureReason.Contains("Illegal path", StringComparison.Ordinal) ||
            failureReason.Contains("Non-full package", StringComparison.Ordinal))
        {
            return LocalizationHelper.GetString("DelegatedUpdateFailureReasonPackageError");
        }

        // 更新器对用户可干预的场景（多实例互斥等）写入的是中英双语文案，含非 ASCII 字符，直接展示；
        // 其余未匹配的纯英文技术串不直接进弹窗，引导用户看日志
        return failureReason.Any(c => c > 0x7f)
            ? failureReason
            : LocalizationHelper.GetString("DelegatedUpdateFailureReasonUnknown");
    }

    public static bool TryConsumeDelegatedUpdateSuccess()
    {
        if (!File.Exists(DelegatedUpdateSuccessStatusFilePath))
        {
            return false;
        }

        MarkPendingUpdateApplied();
        SafeDeleteFile(DelegatedUpdateSuccessStatusFilePath);
        return true;
    }

    private static void ApplyOtaPackage(PendingUpdateContext context, PendingUpdateManifest manifest, ref bool installationChanged)
    {
        Directory.CreateDirectory(context.BackupDir);

        foreach (string relativePath in manifest.RemoveList)
        {
            string targetPath = GetPathUnderRoot(context.RootDir, relativePath);
            if (!PathExists(targetPath))
            {
                continue;
            }

            string backupPath = GetPathUnderRoot(context.BackupDir, relativePath);
            MoveExistingPathToBackup(targetPath, backupPath);
            installationChanged = true;
        }

        foreach (string relativePath in manifest.PayloadFiles)
        {
            string sourcePath = GetPathUnderRoot(context.ExtractDir, relativePath);
            string targetPath = GetPathUnderRoot(context.RootDir, relativePath);
            string backupPath = GetPathUnderRoot(context.BackupDir, relativePath);

            if (PathExists(targetPath))
            {
                MoveExistingPathToBackup(targetPath, backupPath);
                installationChanged = true;
            }

            EnsureParentDirectory(targetPath);
            File.Move(sourcePath, targetPath);
            installationChanged = true;
        }
    }

    private static PendingUpdateApplyResult DelegatePendingUpdateApply(
        PendingUpdateContext context,
        string packageType,
        string reason,
        IReadOnlyList<string> removeEntries,
        IReadOnlyList<string> moveEntries)
    {
        _logger.Information(
            "Delegating pending update apply to external updater: packageType={PackageType}, reason={Reason}",
            packageType,
            reason);
        return HandOffPendingUpdateApplyToExternalProcess(context, packageType, removeEntries, moveEntries);
    }

    private static PendingUpdateApplyResult HandOffPendingUpdateApplyToExternalProcess(
        PendingUpdateContext context,
        string packageType,
        IReadOnlyList<string> removeEntries,
        IReadOnlyList<string> moveEntries)
    {
        bool showUpdaterConsole = ConfigFactory.Root.Update.ShowUpdaterConsole;
        bool showUpdaterProgress = ConfigFactory.Root.Update.ShowUpdaterProgress;
        string planPath = Path.Combine(context.RootDir, $"maa-pending-update-{Guid.NewGuid():N}.json");
        string updaterExecutablePath = PrepareDelegatedUpdaterExecutable(context);
        string relaunchExecutablePath = Path.Combine(context.RootDir, "MAA.exe");

        // 将当前进程需要转发的启动参数写入 plan，供 Updater 原样传给下次 MAA.exe
        string[] relaunchArgs = Bootstrapper.GetForwardableRestartArgs();
        File.WriteAllText(planPath, CreatePendingUpdatePlan(packageType, removeEntries, moveEntries, relaunchArgs));

        var startInfo = new ProcessStartInfo {
            FileName = updaterExecutablePath,
            UseShellExecute = false,
            CreateNoWindow = !showUpdaterConsole,
            WorkingDirectory = context.RootDir,
        };

        // Args: <ParentPid> <RootDir> <ExtractDir> <BackupDir>
        //       <PackagePath> <SuccessStatusFile> <FailureStatusFile>
        //       <RelaunchExecutablePath> <PlanFile>
        //       [--mutex-name <name>] [--show-console] [--no-progress-ui]
        startInfo.ArgumentList.Add(Environment.ProcessId.ToString());
        startInfo.ArgumentList.Add(context.RootDir);
        startInfo.ArgumentList.Add(context.ExtractDir);
        startInfo.ArgumentList.Add(context.BackupDir);
        startInfo.ArgumentList.Add(context.PackagePath);
        startInfo.ArgumentList.Add(DelegatedUpdateSuccessStatusFilePath);
        startInfo.ArgumentList.Add(DelegatedUpdateFailureStatusFilePath);
        startInfo.ArgumentList.Add(relaunchExecutablePath);
        startInfo.ArgumentList.Add(planPath);
        startInfo.ArgumentList.Add("--mutex-name");
        startInfo.ArgumentList.Add(Bootstrapper.MutexName);
        if (showUpdaterConsole)
        {
            startInfo.ArgumentList.Add("--show-console");
        }

        if (!showUpdaterProgress)
        {
            startInfo.ArgumentList.Add("--no-progress-ui");
        }

        _logger.Information(
            "Delegating pending update apply to external updater: packageType={PackageType}, rootDir={RootDir}, extractDir={ExtractDir}, packagePath={PackagePath}, showUpdaterConsole={ShowUpdaterConsole}, showUpdaterProgress={ShowUpdaterProgress}",
            packageType,
            context.RootDir,
            context.ExtractDir,
            context.PackagePath,
            showUpdaterConsole,
            showUpdaterProgress);

        if (!File.Exists(updaterExecutablePath))
        {
            throw new FileNotFoundException("MAA.Updater.exe is missing.", updaterExecutablePath);
        }

        Process.Start(startInfo);
        return new(PendingUpdateApplyResult.StatusKind.Delegated);
    }

    private static void ExtractPendingUpdatePackage(string packagePath, string extractDir)
    {
        ZipFile.ExtractToDirectory(packagePath, extractDir, Encoding.Default, overwriteFiles: true);
    }

    /// <summary>
    /// 验证解压产物中签名验证文件集内各文件的 Authenticode 签名，判定是否存在需用户确认的签名问题（不含弹窗交互）。
    /// 多文件异常时取最严重结果（验签失败优先于无签名）作为整体判定；
    /// 无签名仅当包版本（按解压产物的 PE 版本属性判定）与用户更新通道均为 nightly 时静默放行
    /// （nightly 包不做签名，与 CI 一致），其余无签名情形一律需用户确认；
    /// 伪签（有签名却验签失败或发布者不符）恒按强警告处理。
    /// </summary>
    /// <param name="context">更新应用上下文。</param>
    /// <param name="overallStatus">需确认时的整体签名判定，供确认弹窗选择文案强度。</param>
    /// <param name="failureDetail">需确认时的验证细节描述，仅供日志。</param>
    /// <returns>存在需用户确认的签名问题时为 <c>true</c>；验证通过或可静默放行时为 <c>false</c>。</returns>
    private static bool TryCollectSignatureIssue(
        PendingUpdateContext context,
        out AuthenticodeHelper.SignatureVerificationStatus overallStatus,
        out string? failureDetail)
    {
        // Wine 的 wintrust 实现不完整，有效签名可能被误判为验证失败，跳过签名检查
        if (WineRuntimeInformation.IsRunningUnderWine)
        {
            _logger.Information("Skipping update package signature verification under Wine");
            overallStatus = AuthenticodeHelper.SignatureVerificationStatus.Trusted;
            failureDetail = null;
            return false;
        }

        overallStatus = AuthenticodeHelper.SignatureVerificationStatus.Trusted;
        failureDetail = null;
        string? packageProductVersion = null;
        int verifiedCount = 0;
        int absentCount = 0;
        foreach (string fileName in _signatureVerificationFileNames)
        {
            string filePath = Path.Combine(context.ExtractDir, fileName);
            if (!File.Exists(filePath))
            {
                absentCount++;
                continue;
            }

            // 验证文件的 PE ProductVersion 由 CI 保证同值（csproj Taggify 与 MAA_HASH_VERSION 同源写入），
            // 取首个存在且非空的即可
            packageProductVersion ??= FileVersionInfo.GetVersionInfo(filePath).ProductVersion;

            var fileStatus = AuthenticodeHelper.VerifyFile(filePath, out string? fileDetail);
            if (fileStatus == AuthenticodeHelper.SignatureVerificationStatus.Trusted)
            {
                verifiedCount++;
                _logger.Information("Authenticode signature verified: {FileName}", fileName);
                continue;
            }

            _logger.Warning(
                "Authenticode signature check failed: {FileName}, status={Status}, detail={Detail}",
                fileName,
                fileStatus,
                fileDetail);

            if (overallStatus != AuthenticodeHelper.SignatureVerificationStatus.Invalid)
            {
                overallStatus = fileStatus;
                failureDetail = $"{fileName}: {fileDetail ?? fileStatus.ToString()}";
            }
        }

        if (overallStatus == AuthenticodeHelper.SignatureVerificationStatus.Trusted)
        {
            _logger.Information(
                "Authenticode signature verification passed: {VerifiedCount} verified, {AbsentCount} absent",
                verifiedCount,
                absentCount);
            return false;
        }

        if (overallStatus == AuthenticodeHelper.SignatureVerificationStatus.Unsigned
            && IsNightlyVersion(packageProductVersion)
            && ConfigFactory.Root.Update.VersionType == VersionUpdateSettingsUserControlModel.UpdateVersionType.Nightly)
        {
            _logger.Information(
                "Unsigned update package accepted because both the package and the update channel are nightly: {PackagePath}, productVersion={ProductVersion}",
                context.PackagePath,
                packageProductVersion);
            return false;
        }

        return true;
    }

    /// <summary>
    /// 弹窗告知更新包签名问题并要求连续确认：需连续 3 次选择 ｢仍要继续｣ 才放行，任意一次取消即中止本次更新。
    /// 只在主窗口在场后调用（主窗口加载前由 <see cref="TryApplyPendingUpdatePackage"/> 以
    /// <c>interactive: false</c> 挂起，不在无主窗口阶段弹此窗，避免弹窗成为唯一窗口致 WPF 直接退出）。
    /// </summary>
    /// <param name="status">整体签名判定，验签失败弹强警告文案，无签名弹中性文案。</param>
    /// <param name="failureDetail">验证未通过的细节描述，仅供日志。</param>
    /// <returns>用户连续确认后为 <c>true</c>；取消时为 <c>false</c>。</returns>
    private static bool ConfirmApplyPackageWithSignatureIssue(AuthenticodeHelper.SignatureVerificationStatus status, string? failureDetail)
    {
        bool strongWarning = status == AuthenticodeHelper.SignatureVerificationStatus.Invalid;
        string titleKey = strongWarning
            ? "VersionUpdate.SignatureVerification.Failed.Title"
            : "VersionUpdate.SignatureVerification.Unsigned.Title";
        string messageKey = strongWarning
            ? "VersionUpdate.SignatureVerification.Failed.Message"
            : "VersionUpdate.SignatureVerification.Unsigned.Message";

        int count = 3;
        while (count-- > 0)
        {
            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString(messageKey),
                LocalizationHelper.GetString(titleKey),
                MessageBoxButton.OKCancel,
                strongWarning ? MessageBoxImage.Error : MessageBoxImage.Warning,
                ok: LocalizationHelper.GetStringFormat("VersionUpdate.SignatureVerification.Continue", count + 1),
                cancel: LocalizationHelper.GetString("VersionUpdate.SignatureVerification.Cancel"));
            if (result == MessageBoxResult.Cancel)
            {
                _logger.Warning(
                    "User declined to apply update package with signature issue: status={Status}, detail={Detail}",
                    status,
                    failureDetail);
                return false;
            }
        }

        _logger.Warning(
            "User acknowledged signature issue and chose to continue: status={Status}, detail={Detail}",
            status,
            failureDetail);
        return true;
    }

    /// <summary>
    /// 按产品版本号字符串判断是否 nightly 构建。判定依据是解压产物自身的 PE 版本资源
    /// （CI 将完整版本 tag 写入 InformationalVersion，即 ProductVersion）而非包文件名，
    /// 避免通过重命名包文件伪装 nightly 通道绕过签名检查。
    /// </summary>
    /// <param name="productVersion">解压产物 PE 版本属性中的产品版本号。</param>
    /// <returns>nightly 构建时为 <c>true</c>。</returns>
    internal static bool IsNightlyVersion(string? productVersion)
    {
        return SemVersion.TryParse(productVersion, SemVersionStyles.AllowLowerV, out var semVersion) && IsNightlyVersion(semVersion);
    }

    /// <summary>
    /// 判断版本号是否 nightly 构建（prerelease 末段为 g 嵌 commit hash 形式）。
    /// </summary>
    /// <param name="version">待判断版本。</param>
    /// <returns>nightly 构建时为 <c>true</c>。</returns>
    internal static bool IsNightlyVersion(SemVersion version)
    {
        if (!version.IsPrerelease)
        {
            return false;
        }

        // v{Major}.{Minor}.{Patch}-{Prerelease}.{CommitDistance}.g{CommitHash}
        // v4.6.7-beta.2.1.g1234567
        // v4.6.8-5.g1234567
        var lastId = version.PrereleaseIdentifiers.LastOrDefault().ToString();
        return lastId.StartsWith('g') && lastId.Length >= 7;
    }

    private static bool IsMissingDelegatedUpdaterExecutable(FileNotFoundException ex)
    {
        return string.Equals(Path.GetFileName(ex.FileName), "MAA.Updater.exe", StringComparison.OrdinalIgnoreCase);
    }

    private static string CreatePendingUpdatePlan(
        string packageType,
        IReadOnlyList<string> removeEntries,
        IReadOnlyList<string> moveEntries,
        IReadOnlyList<string>? relaunchArgs = null)
    {
        var plan = new JObject {
            ["packageType"] = packageType,
            ["removeList"] = JArray.FromObject(removeEntries),
            ["moveList"] = JArray.FromObject(moveEntries),
        };

        // relaunchArgs：Updater 成功后传给 MAA.exe 的启动参数（如 --skip-startup-auto-run）
        if (relaunchArgs is { Count: > 0 })
        {
            plan["relaunchArgs"] = JArray.FromObject(relaunchArgs);
        }

        return plan.ToString();
    }

    private static string[] GetFullPackageRemoveEntries(PendingUpdateContext context)
    {
        HashSet<string> preservedEntries = CreateFullPackagePreservedEntries(context);

        // 旧安装侧：嵌套保留目录连同内容整体跳过，不进入 .old
        return [.. ExpandFullPackageEntries(context.RootDir, string.Empty, preservedEntries, includePreserved: false)
            .Distinct(StringComparer.OrdinalIgnoreCase)];
    }

    private static string[] GetFullPackageMoveEntries(string extractDir)
    {
        // 新包侧：嵌套保留目录只下钻输出其中的文件条目（逐文件覆盖、新增），不整体替换该目录
        return [.. ExpandFullPackageEntries(extractDir, string.Empty, _fullPackagePreservedEntries, includePreserved: true)
            .Where(entry => !IsControlFile(entry))
            .Distinct(StringComparer.OrdinalIgnoreCase)];
    }

    /// <summary>
    /// 枚举完整包应用计划中的条目（相对根目录路径）。
    /// 顶层保留条目（config、cache 等）整体跳过；
    /// 包含嵌套保留目录的路径不整体输出，下钻拆分，保证保留目录之外的兄弟内容正常清理/写入。
    /// </summary>
    /// <param name="dir">待枚举目录（旧安装根目录或新包解压目录）。</param>
    /// <param name="relativePrefix">当前目录相对根目录的前缀路径，根目录传空串。</param>
    /// <param name="preservedTopLevel">顶层保留条目名集合。</param>
    /// <param name="includePreserved">是否输出嵌套保留目录内部的文件条目（新包侧传 true）。</param>
    /// <param name="insidePreserved">当前是否已处于嵌套保留目录内部。</param>
    /// <returns>相对根目录的条目路径（反斜杠分隔）。</returns>
    private static IEnumerable<string> ExpandFullPackageEntries(
        string dir,
        string relativePrefix,
        HashSet<string> preservedTopLevel,
        bool includePreserved,
        bool insidePreserved = false)
    {
        bool topLevel = relativePrefix.Length == 0;
        foreach (string entryPath in Directory.EnumerateFileSystemEntries(dir))
        {
            string name = Path.GetFileName(entryPath);
            if (string.IsNullOrWhiteSpace(name))
            {
                continue;
            }

            string relativePath = topLevel ? name : relativePrefix + Path.DirectorySeparatorChar + name;

            if (topLevel && preservedTopLevel.Contains(name))
            {
                continue;
            }

            bool isDirectory = Directory.Exists(entryPath);
            if (!insidePreserved && isDirectory && _fullPackagePreservedNestedEntries.Contains(relativePath))
            {
                if (includePreserved)
                {
                    foreach (string child in ExpandFullPackageEntries(entryPath, relativePath, preservedTopLevel, includePreserved, insidePreserved: true))
                    {
                        yield return child;
                    }
                }

                continue;
            }

            if (insidePreserved)
            {
                // 保留目录内部只输出文件，目录继续下钻，避免目录条目触发整体替换
                if (isDirectory)
                {
                    foreach (string child in ExpandFullPackageEntries(entryPath, relativePath, preservedTopLevel, includePreserved, insidePreserved: true))
                    {
                        yield return child;
                    }
                }
                else
                {
                    yield return relativePath;
                }

                continue;
            }

            if (isDirectory && ContainsNestedPreservedEntry(relativePath))
            {
                // 目录下存在更深的保留路径，不能整体输出
                foreach (string child in ExpandFullPackageEntries(entryPath, relativePath, preservedTopLevel, includePreserved))
                {
                    yield return child;
                }

                continue;
            }

            yield return relativePath;
        }
    }

    private static bool ContainsNestedPreservedEntry(string relativePath)
    {
        string prefix = relativePath + Path.DirectorySeparatorChar;
        return _fullPackagePreservedNestedEntries.Any(entry => entry.StartsWith(prefix, StringComparison.OrdinalIgnoreCase));
    }

    private static string PrepareDelegatedUpdaterExecutable(PendingUpdateContext context)
    {
        string updaterExecutablePath = Path.Combine(context.RootDir, "MAA.Updater.exe");
        string extractedUpdaterPath = GetPathUnderRoot(context.ExtractDir, "MAA.Updater.exe");
        if (!File.Exists(extractedUpdaterPath))
        {
            return updaterExecutablePath;
        }

        EnsureParentDirectory(updaterExecutablePath);
        File.Copy(extractedUpdaterPath, updaterExecutablePath, overwrite: true);
        _logger.Information(
            "Pending update package contains MAA.Updater.exe. Replaced updater before delegation: {UpdaterExecutablePath}",
            updaterExecutablePath);
        return updaterExecutablePath;
    }

    private static HashSet<string> CreateFullPackagePreservedEntries(PendingUpdateContext context)
    {
        var preservedEntries = new HashSet<string>(_fullPackagePreservedEntries, StringComparer.OrdinalIgnoreCase)
        {
            Path.GetFileName(context.ExtractDir),
            Path.GetFileName(context.BackupDir),
            Path.GetFileName(context.PackagePath),
        };

        return preservedEntries;
    }

    private static bool ShouldDelegatePendingOtaApply(PendingUpdateManifest manifest, out string reason)
    {
        string? sensitivePath = manifest.AffectedPaths
            .FirstOrDefault(path => !IsSafeInProcessOtaPath(path));

        if (sensitivePath is not null)
        {
            reason = $"OTA touches runtime-sensitive path '{sensitivePath}'";
            return true;
        }

        reason = "OTA changes are limited to resource files";
        return false;
    }

    private static bool IsSafeInProcessOtaPath(string relativePath)
    {
        if (string.IsNullOrWhiteSpace(relativePath))
        {
            return false;
        }

        string normalizedRelativePath = relativePath.Trim().Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
        int separatorIndex = normalizedRelativePath.IndexOf(Path.DirectorySeparatorChar);
        string topLevelEntry = separatorIndex >= 0
            ? normalizedRelativePath[..separatorIndex]
            : normalizedRelativePath;

        return string.Equals(topLevelEntry, "resource", StringComparison.OrdinalIgnoreCase);
    }

    private static void MoveExistingPathToBackup(string sourcePath, string backupPath)
    {
        PrepareBackupDestination(backupPath);
        MovePath(sourcePath, backupPath);
    }

    private static void PrepareBackupDestination(string backupPath)
    {
        EnsureParentDirectory(backupPath);
        if (!PathExists(backupPath))
        {
            return;
        }

        string archivedPath = CreateArchivedPath(backupPath);
        MovePath(backupPath, archivedPath);
    }

    private static string CreateArchivedPath(string path)
    {
        int index = 0;
        string currentDate = DateTime.Now.ToString("yyyyMMddHHmmss");
        string archivedPath = $"{path}.{currentDate}.{index}";

        while (PathExists(archivedPath))
        {
            index++;
            archivedPath = $"{path}.{currentDate}.{index}";
        }

        return archivedPath;
    }

    private static void PrepareExtractDirectory(string extractDir)
    {
        if (Directory.Exists(extractDir))
        {
            Directory.Delete(extractDir, true);
        }
    }

    private static string GetPathUnderRoot(string rootPath, string relativePath)
    {
        if (!TryResolvePathUnderRoot(rootPath, relativePath, out string fullPath))
        {
            throw new InvalidDataException($"Illegal path in update package: {relativePath}");
        }

        return fullPath;
    }

    private static bool TryResolvePathUnderRoot(string rootPath, string relativePath, out string fullPath)
    {
        fullPath = string.Empty;
        if (string.IsNullOrWhiteSpace(relativePath))
        {
            return false;
        }

        string normalizedRelativePath = relativePath.Trim().Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
        if (Path.IsPathRooted(normalizedRelativePath))
        {
            return false;
        }

        string normalizedRoot = EnsureTrailingSeparator(Path.GetFullPath(rootPath));
        string candidateFullPath = Path.GetFullPath(Path.Combine(rootPath, normalizedRelativePath));
        if (!candidateFullPath.StartsWith(normalizedRoot, StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        fullPath = candidateFullPath;
        return true;
    }

    private static string EnsureTrailingSeparator(string path)
    {
        return path.EndsWith(Path.DirectorySeparatorChar)
            ? path
            : path + Path.DirectorySeparatorChar;
    }

    private static bool IsControlFile(string relativePath)
    {
        return relativePath.IndexOf(Path.DirectorySeparatorChar) < 0 && _controlFiles.Contains(relativePath);
    }

    private static bool PathExists(string path)
    {
        return File.Exists(path) || Directory.Exists(path);
    }

    private static void MovePath(string sourcePath, string destinationPath)
    {
        EnsureParentDirectory(destinationPath);

        if (File.Exists(sourcePath))
        {
            File.Move(sourcePath, destinationPath);
            return;
        }

        if (Directory.Exists(sourcePath))
        {
            Directory.Move(sourcePath, destinationPath);
            return;
        }

        throw new FileNotFoundException($"Path not found: {sourcePath}", sourcePath);
    }

    private static void EnsureParentDirectory(string path)
    {
        string? parentDirectory = Path.GetDirectoryName(path);
        if (!string.IsNullOrEmpty(parentDirectory))
        {
            Directory.CreateDirectory(parentDirectory);
        }
    }

    private static void SafeDeleteFile(string filePath, string fileDescription = "pending update package")
    {
        try
        {
            if (File.Exists(filePath))
            {
                File.Delete(filePath);
            }
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to delete {FileDescription}: {FilePath}", fileDescription, filePath);
        }
    }

    private static void SafeDeleteDirectory(string directoryPath)
    {
        try
        {
            if (Directory.Exists(directoryPath))
            {
                Directory.Delete(directoryPath, true);
            }
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to clean temporary update directory: {DirectoryPath}", directoryPath);
        }
    }

    private static void MarkPendingUpdateApplied()
    {
        ConfigFactory.Root.Update.UpdatePackage = string.Empty;
        ConfigFactory.Root.Update.IsFirstBoot = true;
    }

    internal static bool ShouldPreserveExistingUpdateBody(string updateTag)
    {
        if (string.IsNullOrWhiteSpace(updateTag))
        {
            return false;
        }

        string existingUpdateTag = ConfigFactory.Root.Update.Name;
        string existingUpdateBody = MarkdownDataHelper.Get("CHANGELOG");
        return !string.IsNullOrWhiteSpace(existingUpdateBody) && VersionsMatch(existingUpdateTag, updateTag);
    }

    private static void RegisterPendingUpdatePackage(string updateTag, string packagePath)
    {
        bool preserveExistingUpdateBody = ShouldPreserveExistingUpdateBody(updateTag);
        ConfigFactory.Root.Update.Name = updateTag;
        if (!preserveExistingUpdateBody)
        {
            MarkdownDataHelper.Delete("CHANGELOG");
        }

        ConfigFactory.Root.Update.UpdatePackage = packagePath;

        ClearDelegatedUpdateFailureState();
    }

    private static void ClearPendingUpdatePackageState()
    {
        ConfigFactory.Root.Update.UpdatePackage = string.Empty;
    }

    private static string NormalizeArchitecture(string architecture)
    {
        return architecture.StartsWith("arm", StringComparison.OrdinalIgnoreCase)
            ? "arm64"
            : "x64";
    }

    private static bool VersionsMatch(string leftVersion, string rightVersion)
    {
        if (SemVersion.TryParse(leftVersion, SemVersionStyles.AllowLowerV, out var leftSemVersion) &&
            SemVersion.TryParse(rightVersion, SemVersionStyles.AllowLowerV, out var rightSemVersion) &&
            leftSemVersion != null &&
            rightSemVersion != null)
        {
            return leftSemVersion.CompareSortOrderTo(rightSemVersion) == 0;
        }

        return string.Equals(leftVersion, rightVersion, StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsUpgradeTarget(string currentVersion, string targetVersion)
    {
        if (SemVersion.TryParse(currentVersion, SemVersionStyles.AllowLowerV, out var currentSemVersion) &&
            SemVersion.TryParse(targetVersion, SemVersionStyles.AllowLowerV, out var targetSemVersion) &&
            currentSemVersion != null &&
            targetSemVersion != null)
        {
            return currentSemVersion.CompareSortOrderTo(targetSemVersion) < 0;
        }

        return !string.Equals(currentVersion, targetVersion, StringComparison.OrdinalIgnoreCase);
    }

    private sealed record PendingUpdateContext(
        string RootDir,
        string PackagePath,
        string ExtractDir,
        string BackupDir);

    private sealed record PendingUpdateManifest(
        IReadOnlyList<string> RemoveList,
        IReadOnlyList<string> PayloadFiles)
    {
        public IEnumerable<string> AffectedPaths => RemoveList.Concat(PayloadFiles);

        public static bool HasOtaMetadata(string extractDir)
        {
            return File.Exists(Path.Combine(extractDir, "removelist.txt")) ||
                   File.Exists(Path.Combine(extractDir, "changes.json"));
        }

        public static PendingUpdateManifest Load(string extractDir)
        {
            string removeListFile = Path.Combine(extractDir, "removelist.txt");
            string changesFile = Path.Combine(extractDir, "changes.json");

            var removeList = new List<string>();
            if (File.Exists(removeListFile))
            {
                removeList.AddRange(File.ReadAllLines(removeListFile));
            }

            if (File.Exists(changesFile))
            {
                try
                {
                    string json = File.ReadAllText(changesFile);
                    var jObject = JObject.Parse(json);
                    removeList.AddRange(jObject["deleted"]?.ToObject<List<string>>() ?? []);
                }
                catch (Exception ex)
                {
                    throw new InvalidDataException("Invalid changes.json in pending update package.", ex);
                }
            }

            string[] payloadFiles = [.. Directory.GetFiles(extractDir, "*", SearchOption.AllDirectories)
                .Select(file => Path.GetRelativePath(extractDir, file))
                .Select(file => file.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
                .Where(file => !string.IsNullOrWhiteSpace(file) && !IsControlFile(file))
                .Distinct(StringComparer.OrdinalIgnoreCase)];

            string[] normalizedRemoveList = [.. removeList
                .Select(entry => entry.Trim().Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
                .Where(entry => !string.IsNullOrWhiteSpace(entry) && !IsDirectoryRemovalEntry(entry))
                .Distinct(StringComparer.OrdinalIgnoreCase)];

            return new PendingUpdateManifest(normalizedRemoveList, payloadFiles);
        }
    }

    private static bool IsDirectoryRemovalEntry(string relativePath)
    {
        return relativePath.EndsWith(Path.DirectorySeparatorChar) ||
               relativePath.EndsWith(Path.AltDirectorySeparatorChar);
    }
}
