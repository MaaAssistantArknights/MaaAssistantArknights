// <copyright file="PendingUpdateApplier.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
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

    private static readonly HashSet<string> s_controlFiles = new(StringComparer.OrdinalIgnoreCase)
    {
        "removelist.txt",
        "changes.json",
    };

    private static readonly HashSet<string> s_fullPackagePreservedEntries = new(StringComparer.OrdinalIgnoreCase)
    {
        "achievement",
        "background",
        "cache",
        "config",
        "data",
        "debug",
        "MAA.Updater.exe",
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

    public enum FullPackageInspectionStatus
    {
        MissingFile,
        NotMatched,
        Rejected,
        Supported,
    }

    public sealed record FullPackageInspectionResult(
        FullPackageInspectionStatus Status,
        string? TargetVersion = null)
    {
        public bool IsSupported => Status == FullPackageInspectionStatus.Supported;

        public bool MatchedPattern =>
            Status == FullPackageInspectionStatus.Rejected || Status == FullPackageInspectionStatus.Supported;
    }

    public static bool HasPendingUpdatePackage()
    {
        string updateTag = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionName, string.Empty);
        string updatePackageName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
        return updateTag != string.Empty && updatePackageName != string.Empty && File.Exists(updatePackageName);
    }

    public static LocalPackageImportResult TryRegisterLocalPackage(string packagePath, string currentVersion, string architecture)
    {
        return TryRegisterLocalPackage(packagePath, currentVersion, architecture, null);
    }

    public static LocalPackageImportResult TryRegisterLocalPackage(
        string packagePath,
        string currentVersion,
        string architecture,
        FullPackageInspectionResult? fullPackageInspection)
    {
        FullPackageInspectionResult inspection = fullPackageInspection ?? InspectSupportedLocalFullPackage(packagePath, currentVersion, architecture);
        if (inspection.Status == FullPackageInspectionStatus.MissingFile)
        {
            _logger.Warning("Dropped update package does not exist: {PackagePath}", packagePath);
            return new(LocalPackageImportStatus.Unsupported);
        }

        if (inspection.IsSupported)
        {
            return RegisterSupportedLocalFullPackage(packagePath, inspection.TargetVersion);
        }

        if (inspection.MatchedPattern)
        {
            return new(LocalPackageImportStatus.Unsupported, null, inspection.TargetVersion);
        }

        return TryRegisterNonFullLocalPackage(packagePath, currentVersion, architecture);
    }

    public static FullPackageInspectionResult InspectSupportedLocalFullPackage(string packagePath, string currentVersion, string architecture)
    {
        if (!File.Exists(packagePath))
        {
            return new(FullPackageInspectionStatus.MissingFile);
        }

        string fullPackagePath = Path.GetFullPath(packagePath);
        string fileName = Path.GetFileName(fullPackagePath);
        string normalizedArchitecture = NormalizeArchitecture(architecture);
        _logger.Information(
            "Checking dropped update package: {PackageName}, currentVersion={CurrentVersion}, architecture={Architecture}",
            fileName,
            currentVersion,
            normalizedArchitecture);

        Match fullPackageMatch = FullPackageNameRegex().Match(fileName);
        if (!fullPackageMatch.Success)
        {
            return new(FullPackageInspectionStatus.NotMatched);
        }

        string targetVersion = fullPackageMatch.Groups["version"].Value;
        string packageArchitecture = fullPackageMatch.Groups["arch"].Value;
        bool architectureMatched = string.Equals(normalizedArchitecture, packageArchitecture, StringComparison.OrdinalIgnoreCase);
        bool isUpgradeTarget = IsUpgradeTarget(currentVersion, targetVersion);

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
            return new(FullPackageInspectionStatus.Rejected, targetVersion);
        }

        return new(FullPackageInspectionStatus.Supported, targetVersion);
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

    private static LocalPackageImportResult TryRegisterNonFullLocalPackage(string packagePath, string currentVersion, string architecture)
    {
        string fullPackagePath = Path.GetFullPath(packagePath);
        string fileName = Path.GetFileName(fullPackagePath);
        string normalizedArchitecture = NormalizeArchitecture(architecture);

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
                return new(LocalPackageImportStatus.Unsupported, sourceVersion, targetVersion);
            }

            RegisterPendingUpdatePackage(targetVersion, fullPackagePath);
            _logger.Information(
                "Dropped OTA package registered successfully: packagePath={PackagePath}, targetVersion={TargetVersion}",
                fullPackagePath,
                targetVersion);
            return new(LocalPackageImportStatus.OtaPackageRegistered, sourceVersion, targetVersion);
        }

        _logger.Warning("Dropped package did not match any supported update package pattern: {PackageName}", fileName);
        return new(LocalPackageImportStatus.Unsupported);
    }

    public static PendingUpdateApplyResult TryApplyPendingUpdatePackage()
    {
        string updateTag = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionName, string.Empty);
        string updatePackageName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
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

    public static bool TryConsumeDelegatedUpdateFailure(out string? failureReason)
    {
        failureReason = null;
        if (!File.Exists(DelegatedUpdateFailureStatusFilePath))
        {
            return false;
        }

        try
        {
            failureReason = File.ReadAllText(DelegatedUpdateFailureStatusFilePath).Trim();
            return true;
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to read delegated update failure state: {FailureStateFilePath}", DelegatedUpdateFailureStatusFilePath);
            return true;
        }
        finally
        {
            ClearPendingUpdatePackageState();
            SafeDeleteFile(DelegatedUpdateFailureStatusFilePath);
        }
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
        bool showUpdaterConsole = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ShowUpdaterConsole, false);
        string planPath = Path.Combine(context.RootDir, $"maa-pending-update-{Guid.NewGuid():N}.json");
        string updaterExecutablePath = PrepareDelegatedUpdaterExecutable(context);
        string relaunchExecutablePath = Path.Combine(context.RootDir, "MAA.exe");

        File.WriteAllText(planPath, CreatePendingUpdatePlan(packageType, removeEntries, moveEntries));

        var startInfo = new ProcessStartInfo
        {
            FileName = updaterExecutablePath,
            UseShellExecute = false,
            CreateNoWindow = !showUpdaterConsole,
            WorkingDirectory = context.RootDir,
        };

        // Args: <ParentPid> <RootDir> <ExtractDir> <BackupDir>
        //       <PackagePath> <SuccessStatusFile> <FailureStatusFile>
        //       <RelaunchExecutablePath> <PlanFile>
        //       [--mutex-name <name>] [--show-console]
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

        _logger.Information(
            "Delegating pending update apply to external updater: packageType={PackageType}, rootDir={RootDir}, extractDir={ExtractDir}, packagePath={PackagePath}, showUpdaterConsole={ShowUpdaterConsole}",
            packageType,
            context.RootDir,
            context.ExtractDir,
            context.PackagePath,
            showUpdaterConsole);

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

    private static bool IsMissingDelegatedUpdaterExecutable(FileNotFoundException ex)
    {
        return string.Equals(Path.GetFileName(ex.FileName), "MAA.Updater.exe", StringComparison.OrdinalIgnoreCase);
    }

    private static string CreatePendingUpdatePlan(string packageType, IReadOnlyList<string> removeEntries, IReadOnlyList<string> moveEntries)
    {
        return new JObject
        {
            ["packageType"] = packageType,
            ["removeList"] = JArray.FromObject(removeEntries),
            ["moveList"] = JArray.FromObject(moveEntries),
        }.ToString();
    }

    private static string[] GetFullPackageRemoveEntries(PendingUpdateContext context)
    {
        HashSet<string> preservedEntries = CreateFullPackagePreservedEntries(context);
        return [.. Directory.GetFileSystemEntries(context.RootDir)
            .Select(Path.GetFileName)
            .OfType<string>()
            .Where(entry => !string.IsNullOrWhiteSpace(entry) && !preservedEntries.Contains(entry))
            .Distinct(StringComparer.OrdinalIgnoreCase)];
    }

    private static string[] GetFullPackageMoveEntries(string extractDir)
    {
        return [.. GetTopLevelExtractEntries(extractDir)
            .Where(entry => !IsFullPackagePreservedEntry(entry))
            .Distinct(StringComparer.OrdinalIgnoreCase)];
    }

    private static string[] GetTopLevelExtractEntries(string extractDir)
    {
        return [.. Directory.GetFileSystemEntries(extractDir)
            .Select(entry => Path.GetRelativePath(extractDir, entry))
            .Select(entry => entry.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
            .Where(entry => !string.IsNullOrWhiteSpace(entry) && !IsControlFile(entry))
            .Distinct(StringComparer.OrdinalIgnoreCase)];
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
        var preservedEntries = new HashSet<string>(s_fullPackagePreservedEntries, StringComparer.OrdinalIgnoreCase)
        {
            Path.GetFileName(context.ExtractDir),
            Path.GetFileName(context.BackupDir),
            Path.GetFileName(context.PackagePath),
        };

        return preservedEntries;
    }

    private static bool IsFullPackagePreservedEntry(string relativePath)
    {
        return s_fullPackagePreservedEntries.Contains(NormalizeRelativePath(relativePath));
    }

    private static string NormalizeRelativePath(string relativePath)
    {
        return relativePath.Trim().Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
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
        return relativePath.IndexOf(Path.DirectorySeparatorChar) < 0 && s_controlFiles.Contains(relativePath);
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

    private static void SafeDeleteFile(string filePath)
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
            _logger.Warning(ex, "Failed to delete pending update package: {FilePath}", filePath);
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
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionUpdateIsFirstBoot, bool.TrueString);
    }

    internal static bool ShouldPreserveExistingUpdateBody(string updateTag)
    {
        if (string.IsNullOrWhiteSpace(updateTag))
        {
            return false;
        }

        string existingUpdateTag = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionName, string.Empty);
        string existingUpdateBody = MarkdownDataHelper.Get("CHANGELOG");
        return !string.IsNullOrWhiteSpace(existingUpdateBody) && VersionsMatch(existingUpdateTag, updateTag);
    }

    private static void RegisterPendingUpdatePackage(string updateTag, string packagePath)
    {
        bool preserveExistingUpdateBody = ShouldPreserveExistingUpdateBody(updateTag);
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionName, updateTag);
        if (!preserveExistingUpdateBody)
        {
            MarkdownDataHelper.Delete("CHANGELOG");
        }

        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionUpdatePackage, packagePath);
    }

    private static void ClearPendingUpdatePackageState()
    {
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
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
