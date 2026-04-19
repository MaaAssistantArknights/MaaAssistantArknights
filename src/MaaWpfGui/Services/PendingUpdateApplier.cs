#pragma warning disable SA1633

#nullable enable

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text.RegularExpressions;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
using Semver;
using Serilog;

namespace MaaWpfGui.Services;

internal static class PendingUpdateApplier
{
    private static readonly ILogger _logger = Log.ForContext(typeof(PendingUpdateApplier));

    private static readonly Regex s_otaPackageNameRegex = new(
        @"^MAAComponent-OTA-(?<from>v.+?|DEBUG_VERSION)_(?<to>v.+?)-win-(?<arch>x64|arm64)\.zip$",
        RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase);

    private static readonly Regex s_fullPackageNameRegex = new(
        @"^MAA-(?<version>v.+?)-win-(?<arch>x64|arm64)\.zip$",
        RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase);

    private static readonly HashSet<string> s_controlFiles = new(StringComparer.OrdinalIgnoreCase)
    {
        "removelist.txt",
        "changes.json",
    };

    public enum LocalPackageImportStatus
    {
        Unsupported,
        FullPackage,
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

        Match otaMatch = s_otaPackageNameRegex.Match(fileName);
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

        if (s_fullPackageNameRegex.IsMatch(fileName))
        {
            _logger.Information("Dropped package matched full package pattern: {PackageName}", fileName);
            return new(LocalPackageImportStatus.FullPackage);
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

        try
        {
            PrepareExtractDirectory(context.ExtractDir);

            try
            {
                ZipFile.ExtractToDirectory(context.PackagePath, context.ExtractDir);
            }
            catch (InvalidDataException ex)
            {
                _logger.Warning(ex, "Pending update package is invalid: {PackagePath}", context.PackagePath);
                SafeDeleteFile(context.PackagePath);
                ClearPendingUpdatePackageState();
                return new(PendingUpdateApplyResult.StatusKind.InvalidPackage, FailureReason: ex.Message);
            }

            var manifest = PendingUpdateManifest.Load(context.ExtractDir);
            if (manifest.IsOtaPackage)
            {
                ApplyOtaPackage(context, manifest, ref installationChanged);
            }
            else
            {
                ApplyFullPackage(context, manifest, ref installationChanged);
            }

            SafeDeleteFile(context.PackagePath);
            MarkPendingUpdateApplied();
            return new(PendingUpdateApplyResult.StatusKind.Succeeded);
        }
        catch (InvalidDataException ex)
        {
            _logger.Error(ex, "Pending update package was rejected: {PackagePath}", context.PackagePath);
            if (!installationChanged)
            {
                SafeDeleteFile(context.PackagePath);
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

            return new(PendingUpdateApplyResult.StatusKind.Failed, FailureReason: ex.Message);
        }
        finally
        {
            if (clearPendingPackageState)
            {
                ClearPendingUpdatePackageState();
            }

            SafeDeleteDirectory(context.ExtractDir);
        }
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

    private static void ApplyFullPackage(PendingUpdateContext context, PendingUpdateManifest manifest, ref bool installationChanged)
    {
        Directory.CreateDirectory(context.BackupDir);

        foreach (string relativePath in manifest.TopLevelEntries)
        {
            string sourcePath = GetPathUnderRoot(context.ExtractDir, relativePath);
            string targetPath = GetPathUnderRoot(context.RootDir, relativePath);
            string backupPath = GetPathUnderRoot(context.BackupDir, relativePath);

            if (PathExists(targetPath))
            {
                MoveExistingPathToBackup(targetPath, backupPath);
                installationChanged = true;
            }

            MovePath(sourcePath, targetPath);
            installationChanged = true;
        }
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
        bool IsOtaPackage,
        IReadOnlyList<string> RemoveList,
        IReadOnlyList<string> PayloadFiles,
        IReadOnlyList<string> TopLevelEntries)
    {
        public static PendingUpdateManifest Load(string extractDir)
        {
            string removeListFile = Path.Combine(extractDir, "removelist.txt");
            string changesFile = Path.Combine(extractDir, "changes.json");
            bool isOtaPackage = File.Exists(removeListFile) || File.Exists(changesFile);

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
                    removeList = jObject["deleted"]?.ToObject<List<string>>() ?? [];
                }
                catch (Exception ex)
                {
                    throw new InvalidDataException("Invalid changes.json in pending update package.", ex);
                }
            }

            string[] payloadFiles = Directory.GetFiles(extractDir, "*", SearchOption.AllDirectories)
                .Select(file => Path.GetRelativePath(extractDir, file))
                .Select(file => file.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
                .Where(file => !string.IsNullOrWhiteSpace(file) && !IsControlFile(file))
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();

            string[] topLevelEntries = Directory.GetFileSystemEntries(extractDir)
                .Select(entry => Path.GetRelativePath(extractDir, entry))
                .Select(entry => entry.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
                .Where(entry => !string.IsNullOrWhiteSpace(entry) && !IsControlFile(entry))
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();

            string[] normalizedRemoveList = removeList
                .Select(entry => entry.Trim().Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar))
                .Where(entry => !string.IsNullOrWhiteSpace(entry))
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();

            return new PendingUpdateManifest(isOtaPackage, normalizedRemoveList, payloadFiles, topLevelEntries);
        }
    }
}
