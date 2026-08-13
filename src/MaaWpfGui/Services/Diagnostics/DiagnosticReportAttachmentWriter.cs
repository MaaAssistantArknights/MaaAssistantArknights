// <copyright file="DiagnosticReportAttachmentWriter.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1636
#nullable enable

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using MaaWpfGui.Models.Diagnostics;

namespace MaaWpfGui.Services.Diagnostics;

internal sealed class DiagnosticReportAttachmentWriter
{
    private const long MaxPartBytes = 20L * 1024L * 1024L;
    private readonly string _baseDirectory;

    internal DiagnosticReportAttachmentWriter(string baseDirectory)
    {
        _baseDirectory = baseDirectory;
    }

    internal DiagnosticAttachmentBuildResult Build(
        string transactionDirectory,
        string reportFileStem,
        DiagnosticFailure failure,
        DiagnosticReportOptions options)
    {
        string stagingDirectory = Path.Combine(transactionDirectory, "attachments");
        Directory.CreateDirectory(stagingDirectory);
        var collection = new DiagnosticCollectionStatus();
        var candidates = new List<AttachmentCandidate>();
        DiscoverCategory(
            collection.Screenshots,
            options.IncludeScreenshots,
            Path.Combine(_baseDirectory, "debug"),
            "screenshots",
            failure.TimestampUtc,
            new HashSet<string>([".png", ".jpg", ".jpeg"], StringComparer.OrdinalIgnoreCase),
            candidates);
        DiscoverCategory(
            collection.Dumps,
            options.IncludeDumps,
            Path.Combine(_baseDirectory, "debug", "dumps"),
            "dumps",
            failure.TimestampUtc,
            new HashSet<string>([".dmp"], StringComparer.OrdinalIgnoreCase),
            candidates);
        DiscoverCategory(
            collection.Dumps,
            options.IncludeDumps,
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "CrashDumps"),
            "dumps/windows-crash-dumps",
            failure.TimestampUtc,
            new HashSet<string>([".dmp"], StringComparer.OrdinalIgnoreCase),
            candidates,
            "MAA*.dmp");

        var staged = new List<StagedAttachment>();
        int attachmentIndex = 0;
        foreach (var candidate in candidates
                     .DistinctBy(static item => item.SourcePath, StringComparer.OrdinalIgnoreCase)
                     .OrderBy(static item => item.EntryName, StringComparer.OrdinalIgnoreCase))
        {
            var itemStatus = new DiagnosticAttachmentStatus
            {
                EntryName = candidate.EntryName,
                Size = candidate.Length,
                Status = "error",
            };
            collection.Attachments.Add(itemStatus);
            string stagedPath = Path.Combine(stagingDirectory, $"{attachmentIndex++:D6}.bin");
            try
            {
                // 先完整复制并刷盘，再让附件进入分卷；源文件被占用或中途读取失败时不会留下半截 ZIP 条目。
                using var source = new FileStream(candidate.SourcePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
                using var destination = new FileStream(stagedPath, FileMode.CreateNew, FileAccess.Write, FileShare.None);
                source.CopyTo(destination);
                destination.Flush(flushToDisk: true);
                staged.Add(new(stagedPath, candidate.EntryName, destination.Length, itemStatus));
                itemStatus.Size = destination.Length;
                itemStatus.Status = "staged";
            }
            catch (Exception ex)
            {
                DeleteBestEffort(stagedPath);
                itemStatus.ExceptionType = ex.GetType().FullName ?? ex.GetType().Name;
                itemStatus.ExceptionMessage = ex.Message;
            }
        }

        var partPaths = new List<string>();
        int partNumber = 2;
        int index = 0;
        while (index < staged.Count)
        {
            var partItems = new List<StagedAttachment>();
            long currentBytes = 0;
            while (index < staged.Count)
            {
                var next = staged[index];
                if (partItems.Count > 0 && currentBytes + next.Length > MaxPartBytes)
                {
                    break;
                }

                partItems.Add(next);
                currentBytes += next.Length;
                index++;
                if (next.Length > MaxPartBytes)
                {
                    break;
                }
            }

            string partFileName = $"{reportFileStem}_part{partNumber++:00}.zip";
            string partPath = Path.Combine(transactionDirectory, partFileName);
            using (var archive = ZipFile.Open(partPath, ZipArchiveMode.Create))
            {
                foreach (var item in partItems)
                {
                    archive.CreateEntryFromFile(item.StagedPath, item.EntryName, CompressionLevel.SmallestSize);
                    item.Status.Status = "included";
                    item.Status.PartFileName = partFileName;
                }
            }

            ValidateArchive(partPath);
            partPaths.Add(partPath);
        }

        DeleteDirectoryBestEffort(stagingDirectory);
        UpdateSummary(collection.Screenshots, collection.Attachments, "screenshots/");
        UpdateSummary(collection.Dumps, collection.Attachments, "dumps/");
        return new(partPaths, collection);
    }

    private static void DiscoverCategory(
        DiagnosticAttachmentCategoryStatus category,
        bool requested,
        string directory,
        string entryRoot,
        DateTimeOffset timestamp,
        IReadOnlySet<string> extensions,
        ICollection<AttachmentCandidate> destination,
        string searchPattern = "*")
    {
        category.Requested |= requested;
        if (!requested)
        {
            category.Status = "not-requested";
            return;
        }

        if (!Directory.Exists(directory))
        {
            return;
        }

        var start = timestamp.LocalDateTime.AddMinutes(-10);
        var end = timestamp.LocalDateTime.AddMinutes(10);
        try
        {
            foreach (string path in Directory.EnumerateFiles(directory, searchPattern, SearchOption.AllDirectories))
            {
                var info = new FileInfo(path);
                if (!extensions.Contains(info.Extension) || info.LastWriteTime < start || info.LastWriteTime > end)
                {
                    continue;
                }

                string relativePath = Path.GetRelativePath(directory, info.FullName).Replace('\\', '/');
                string entryName = NormalizeEntryName($"{entryRoot}/{relativePath}");
                destination.Add(new(info.FullName, entryName, info.Length));
                category.CandidatesFound++;
            }

            if (category.Status != "error")
            {
                category.Status = "success";
            }
        }
        catch (Exception ex)
        {
            category.Status = "error";
            category.ExceptionType = ex.GetType().FullName ?? ex.GetType().Name;
            category.ExceptionMessage = ex.Message;
        }
    }

    private static string NormalizeEntryName(string entryName)
    {
        string normalized = entryName.Replace('\\', '/').TrimStart('/');
        if (Path.IsPathRooted(normalized) || normalized.Split('/').Any(static segment => segment is ".." or "." or ""))
        {
            throw new InvalidDataException("An attachment entry contained an unsafe relative path.");
        }

        return normalized;
    }

    private static void UpdateSummary(
        DiagnosticAttachmentCategoryStatus category,
        IEnumerable<DiagnosticAttachmentStatus> attachments,
        string prefix)
    {
        var matching = attachments.Where(item => item.EntryName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)).ToList();
        category.Included = matching.Count(static item => item.Status == "included");
        category.Failed = matching.Count(static item => item.Status == "error");
    }

    private static void ValidateArchive(string path)
    {
        using var archive = ZipFile.OpenRead(path);
        foreach (var entry in archive.Entries)
        {
            using var stream = entry.Open();
            stream.CopyTo(Stream.Null);
        }
    }

    private static void DeleteBestEffort(string path)
    {
        try
        {
            File.Delete(path);
        }
        catch
        {
            // The transaction directory cleanup gets another chance to remove this file.
        }
    }

    private static void DeleteDirectoryBestEffort(string path)
    {
        try
        {
            Directory.Delete(path, recursive: true);
        }
        catch
        {
            // The transaction directory cleanup gets another chance to remove staged data.
        }
    }

    private sealed record AttachmentCandidate(string SourcePath, string EntryName, long Length);

    private sealed record StagedAttachment(
        string StagedPath,
        string EntryName,
        long Length,
        DiagnosticAttachmentStatus Status);
}
