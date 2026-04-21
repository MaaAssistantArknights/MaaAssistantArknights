#pragma warning disable SA1633

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

    private static string DelegatedUpdateSuccessStatusFilePath => Path.Combine(PathsHelper.ConfigDir, "pending-update-success.txt");

    private static string DelegatedUpdateFailureStatusFilePath => Path.Combine(PathsHelper.ConfigDir, "pending-update-failure.txt");

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

    public static bool HasPendingUpdatePackage()
    {
        string updateTag = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionName, string.Empty);
        string updatePackageName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
        return updateTag != string.Empty && updatePackageName != string.Empty && File.Exists(updatePackageName);
    }

    public static LocalPackageImportResult TryRegisterLocalPackage(string packagePath, string currentVersion, string architecture)
    {
        if (!File.Exists(packagePath))
        {
            _logger.Warning("Dropped update package does not exist: {PackagePath}", packagePath);
            return new(LocalPackageImportStatus.Unsupported);
        }

        string fullPackagePath = Path.GetFullPath(packagePath);
        string fileName = Path.GetFileName(fullPackagePath);
        string normalizedArchitecture = NormalizeArchitecture(architecture);
        _logger.Information(
            "Checking dropped update package: {PackageName}, currentVersion={CurrentVersion}, architecture={Architecture}",
            fileName,
            currentVersion,
            normalizedArchitecture);

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

        Match fullPackageMatch = FullPackageNameRegex().Match(fileName);
        if (fullPackageMatch.Success)
        {
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
                return new(LocalPackageImportStatus.Unsupported, null, targetVersion);
            }

            RegisterPendingUpdatePackage(targetVersion, fullPackagePath);
            _logger.Information(
                "Dropped full package registered successfully: packagePath={PackagePath}, targetVersion={TargetVersion}",
                fullPackagePath,
                targetVersion);
            return new(LocalPackageImportStatus.FullPackageRegistered, null, targetVersion);
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
                return DelegatePendingUpdateApply(
                    context,
                    "full",
                    "full package always replaces runtime files",
                    Array.Empty<string>(),
                    GetTopLevelExtractEntries(context.ExtractDir));
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
        string scriptPath = Path.Combine(Path.GetTempPath(), $"maa-pending-update-{Guid.NewGuid():N}.ps1");
        string planPath = Path.Combine(Path.GetTempPath(), $"maa-pending-update-{Guid.NewGuid():N}.json");
        string relaunchExecutablePath = Path.Combine(context.RootDir, "MAA.exe");

        File.WriteAllText(scriptPath, CreatePendingUpdateScript());
        File.WriteAllText(planPath, CreatePendingUpdatePlan(removeEntries, moveEntries));

        var startInfo = new ProcessStartInfo
        {
            FileName = "powershell.exe",
            UseShellExecute = true,
            CreateNoWindow = true,
            WindowStyle = ProcessWindowStyle.Hidden,
            WorkingDirectory = context.RootDir,
        };

        startInfo.ArgumentList.Add("-NoProfile");
        startInfo.ArgumentList.Add("-ExecutionPolicy");
        startInfo.ArgumentList.Add("Bypass");
        startInfo.ArgumentList.Add("-File");
        startInfo.ArgumentList.Add(scriptPath);
        startInfo.ArgumentList.Add(Environment.ProcessId.ToString());
        startInfo.ArgumentList.Add(context.RootDir);
        startInfo.ArgumentList.Add(context.ExtractDir);
        startInfo.ArgumentList.Add(context.BackupDir);
        startInfo.ArgumentList.Add(context.PackagePath);
        startInfo.ArgumentList.Add(DelegatedUpdateSuccessStatusFilePath);
        startInfo.ArgumentList.Add(DelegatedUpdateFailureStatusFilePath);
        startInfo.ArgumentList.Add(relaunchExecutablePath);
        startInfo.ArgumentList.Add(planPath);
        startInfo.ArgumentList.Add(scriptPath);

        _logger.Information(
            "Delegating pending update apply to external updater: packageType={PackageType}, rootDir={RootDir}, extractDir={ExtractDir}, packagePath={PackagePath}",
            packageType,
            context.RootDir,
            context.ExtractDir,
            context.PackagePath);

        Process.Start(startInfo);
        return new(PendingUpdateApplyResult.StatusKind.Delegated);
    }

    private static void ExtractPendingUpdatePackage(string packagePath, string extractDir)
    {
        ZipFile.ExtractToDirectory(packagePath, extractDir, Encoding.Default, overwriteFiles: true);
    }

    private static string CreatePendingUpdatePlan(IReadOnlyList<string> removeEntries, IReadOnlyList<string> moveEntries)
    {
        return new JObject
        {
            ["removeList"] = JArray.FromObject(removeEntries),
            ["moveList"] = JArray.FromObject(moveEntries),
        }.ToString();
    }

    private static string[] GetTopLevelExtractEntries(string extractDir)
    {
        return [.. Directory.GetFileSystemEntries(extractDir)
            .Select(entry => Path.GetRelativePath(extractDir, entry))
            .Select(entry => entry.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
            .Where(entry => !string.IsNullOrWhiteSpace(entry) && !IsControlFile(entry))
            .Distinct(StringComparer.OrdinalIgnoreCase)];
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

    private static string CreatePendingUpdateScript()
    {
        return $$"""
param(
    [int]$ParentProcessId,
    [string]$RootDir,
    [string]$ExtractDir,
    [string]$BackupDir,
    [string]$PackagePath,
    [string]$SuccessStatusFile,
    [string]$FailureStatusFile,
    [string]$RelaunchExecutablePath,
    [string]$PlanFile,
    [string]$ScriptPath
)

$ErrorActionPreference = 'Stop'
$logFile = Join-Path $RootDir 'debug\pending-update-applier.log'
$shouldRelaunch = $false

function Write-Log {
    param([string]$Message)

    $timestamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff'
    $directory = Split-Path -Parent $logFile
    if (-not [string]::IsNullOrEmpty($directory)) {
        [System.IO.Directory]::CreateDirectory($directory) | Out-Null
    }

    Add-Content -LiteralPath $logFile -Value "[$timestamp] $Message"
}

function Ensure-TrailingSeparator {
    param([string]$Path)

    $separator = [string][System.IO.Path]::DirectorySeparatorChar
    if ($Path.EndsWith($separator)) {
        return $Path
    }

    return $Path + $separator
}

function Resolve-PathUnderRoot {
    param(
        [string]$RootPath,
        [string]$RelativePath
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        throw "Illegal path in update package: $RelativePath"
    }

    $normalizedRelativePath = $RelativePath.Trim().Replace([System.IO.Path]::AltDirectorySeparatorChar, [System.IO.Path]::DirectorySeparatorChar)
    if ([System.IO.Path]::IsPathRooted($normalizedRelativePath)) {
        throw "Illegal path in update package: $RelativePath"
    }

    $normalizedRoot = Ensure-TrailingSeparator ([System.IO.Path]::GetFullPath($RootPath))
    $candidateFullPath = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($RootPath, $normalizedRelativePath))
    if (-not $candidateFullPath.StartsWith($normalizedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Illegal path in update package: $RelativePath"
    }

    return $candidateFullPath
}

function Ensure-ParentDirectory {
    param([string]$Path)

    $parentDirectory = Split-Path -Parent $Path
    if (-not [string]::IsNullOrEmpty($parentDirectory)) {
        [System.IO.Directory]::CreateDirectory($parentDirectory) | Out-Null
    }
}

function Move-PathEntry {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    Ensure-ParentDirectory $DestinationPath

    if (Test-Path -LiteralPath $SourcePath -PathType Leaf) {
        [System.IO.File]::Move($SourcePath, $DestinationPath)
        return
    }

    if (Test-Path -LiteralPath $SourcePath -PathType Container) {
        [System.IO.Directory]::Move($SourcePath, $DestinationPath)
        return
    }

    throw "Path not found: $SourcePath"
}

function Create-ArchivedPath {
    param([string]$Path)

    $index = 0
    $currentDate = Get-Date -Format 'yyyyMMddHHmmss'
    $archivedPath = "$Path.$currentDate.$index"

    while (Test-Path -LiteralPath $archivedPath) {
        $index++
        $archivedPath = "$Path.$currentDate.$index"
    }

    return $archivedPath
}

function Prepare-BackupDestination {
    param([string]$BackupPath)

    Ensure-ParentDirectory $BackupPath
    if (-not (Test-Path -LiteralPath $BackupPath)) {
        return
    }

    $archivedPath = Create-ArchivedPath $BackupPath
    Move-PathEntry $BackupPath $archivedPath
}

function Move-ExistingPathToBackup {
    param(
        [string]$SourcePath,
        [string]$BackupPath
    )

    Prepare-BackupDestination $BackupPath
    Move-PathEntry $SourcePath $BackupPath
}

try {
    Wait-Process -Id $ParentProcessId -ErrorAction SilentlyContinue

    if (-not (Test-Path -LiteralPath $PlanFile)) {
        throw "Pending update plan not found: $PlanFile"
    }

    $plan = [System.IO.File]::ReadAllText($PlanFile, [System.Text.Encoding]::UTF8) | ConvertFrom-Json
    $removeList = if ($null -eq $plan.removeList) { @() } else { @($plan.removeList) }
    $moveList = if ($null -eq $plan.moveList) { @() } else { @($plan.moveList) }

    Write-Log "External pending updater started. PlanFile=$PlanFile ExtractDir=$ExtractDir"
    [System.IO.Directory]::CreateDirectory($BackupDir) | Out-Null

    foreach ($relativePath in $removeList) {
        $targetPath = Resolve-PathUnderRoot $RootDir $relativePath
        if (-not (Test-Path -LiteralPath $targetPath)) {
            continue
        }

        $backupPath = Resolve-PathUnderRoot $BackupDir $relativePath
        Move-ExistingPathToBackup $targetPath $backupPath
    }

    foreach ($relativePath in $moveList) {
        $sourcePath = Resolve-PathUnderRoot $ExtractDir $relativePath
        $targetPath = Resolve-PathUnderRoot $RootDir $relativePath
        $backupPath = Resolve-PathUnderRoot $BackupDir $relativePath

        if (Test-Path -LiteralPath $targetPath) {
            Move-ExistingPathToBackup $targetPath $backupPath
        }

        Move-PathEntry $sourcePath $targetPath
    }

    if (Test-Path -LiteralPath $PackagePath) {
        Remove-Item -LiteralPath $PackagePath -Force
    }

    if (Test-Path -LiteralPath $FailureStatusFile) {
        Remove-Item -LiteralPath $FailureStatusFile -Force
    }

    Ensure-ParentDirectory $SuccessStatusFile
    [System.IO.File]::WriteAllText($SuccessStatusFile, 'succeeded', [System.Text.Encoding]::UTF8)

    $shouldRelaunch = $true

    Write-Log 'External pending updater completed successfully.'
}
catch {
    Write-Log ("External pending updater failed: " + $_.Exception)
    Ensure-ParentDirectory $FailureStatusFile
    [System.IO.File]::WriteAllText($FailureStatusFile, $_.Exception.ToString(), [System.Text.Encoding]::UTF8)
    if (Test-Path -LiteralPath $SuccessStatusFile) {
        Remove-Item -LiteralPath $SuccessStatusFile -Force -ErrorAction SilentlyContinue
    }
}
finally {
    if (Test-Path -LiteralPath $ExtractDir) {
        Remove-Item -LiteralPath $ExtractDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (Test-Path -LiteralPath $PlanFile) {
        Remove-Item -LiteralPath $PlanFile -Force -ErrorAction SilentlyContinue
    }

    if ($shouldRelaunch -and (Test-Path -LiteralPath $RelaunchExecutablePath)) {
        Start-Process -FilePath $RelaunchExecutablePath -WorkingDirectory $RootDir
    }

    if (Test-Path -LiteralPath $ScriptPath) {
        Remove-Item -LiteralPath $ScriptPath -Force -ErrorAction SilentlyContinue
    }
}
""";
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

    private static void RegisterPendingUpdatePackage(string updateTag, string packagePath)
    {
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionName, updateTag);
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionUpdateBody, string.Empty);
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
                .Where(entry => !string.IsNullOrWhiteSpace(entry))
                .Distinct(StringComparer.OrdinalIgnoreCase)];

            return new PendingUpdateManifest(normalizedRemoveList, payloadFiles);
        }
    }
}
