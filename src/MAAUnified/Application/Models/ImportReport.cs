using MAAUnified.Application.Configuration;

namespace MAAUnified.Application.Models;

public sealed class ImportReport
{
    public ImportSource Source { get; set; }

    public DateTimeOffset StartedAt { get; set; } = DateTimeOffset.UtcNow;

    public DateTimeOffset FinishedAt { get; set; }

    public bool Success { get; set; }

    public bool ImportedGuiNew { get; set; }

    public bool ImportedGui { get; set; }

    public int MappedFieldCount { get; set; }

    public int DefaultFallbackCount { get; set; }

    public int ConflictCount { get; set; }

    public List<string> Warnings { get; set; } = [];

    public List<string> Errors { get; set; } = [];

    public string OutputConfigPath { get; set; } = string.Empty;

    public string ReportPath { get; set; } = string.Empty;

    public string Summary => $"mapped={MappedFieldCount}, fallback={DefaultFallbackCount}, conflicts={ConflictCount}";
}
