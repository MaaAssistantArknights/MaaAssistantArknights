using MAAUnified.Application.Configuration;

namespace MAAUnified.Application.Models;

public sealed record LegacyImportRequest(
    LegacyConfigSnapshot Snapshot,
    ImportSource Source,
    bool ManualImport,
    bool AllowPartialImport = true);
