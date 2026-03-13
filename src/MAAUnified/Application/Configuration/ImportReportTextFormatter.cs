using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

public static class ImportReportTextFormatter
{
    public static IReadOnlyList<ImportReportLogLine> BuildLogLines(ImportReport report, bool manualImport)
    {
        var lines = new List<ImportReportLogLine>();
        if (report is null)
        {
            return lines;
        }

        var primary = BuildPrimaryLine(report, manualImport);
        if (primary is not null)
        {
            lines.Add(primary.Value);
        }

        if (report.MissingFiles.Count > 0)
        {
            lines.Add(new ImportReportLogLine(
                "WARN",
                $"未找到或未选择：{JoinFiles(report.MissingFiles)}，对应项保持默认。"));
        }

        if (report.DamagedFiles.Count > 0)
        {
            lines.Add(new ImportReportLogLine(
                report.AppliedConfig ? "WARN" : "ERROR",
                $"文件损坏：{JoinFiles(report.DamagedFiles)}。"));
        }

        foreach (var warning in report.Warnings.Where(static warning => !string.IsNullOrWhiteSpace(warning)))
        {
            lines.Add(new ImportReportLogLine("WARN", warning.Trim()));
        }

        return lines;
    }

    public static string BuildStatusMessage(ImportReport report, bool manualImport)
    {
        if (report.CreatedDefaultConfig)
        {
            return "未找到旧配置，已自动创建默认配置。";
        }

        if (report.AppliedConfig)
        {
            if (report.DamagedFiles.Count > 0)
            {
                return manualImport ? "已导入可用内容。" : "已加载可用旧配置内容。";
            }

            if (manualImport && report.MissingFiles.Count > 0)
            {
                return "已强行导入旧配置。";
            }

            return manualImport ? "旧配置导入完成。" : "旧配置自动加载完成。";
        }

        if (report.DamagedFiles.Count > 0)
        {
            return "旧配置导入未应用，存在损坏文件。";
        }

        return "旧配置导入失败。";
    }

    private static ImportReportLogLine? BuildPrimaryLine(ImportReport report, bool manualImport)
    {
        if (!report.AppliedConfig)
        {
            if (report.DamagedFiles.Count > 0)
            {
                return new ImportReportLogLine(
                    "WARN",
                    $"检测到损坏文件：{JoinFiles(report.DamagedFiles)}，本次导入尚未应用。");
            }

            if (report.ImportedFiles.Count == 0 && report.MissingFiles.Count > 0)
            {
                return new ImportReportLogLine(
                    "WARN",
                    $"未找到可导入的旧配置文件：{JoinFiles(report.MissingFiles)}。");
            }

            return new ImportReportLogLine("WARN", "旧配置导入尚未应用。");
        }

        if (report.CreatedDefaultConfig)
        {
            return new ImportReportLogLine(
                "INFO",
                "未找到 gui.new.json / gui.json，已自动创建默认配置 avalonia.json。");
        }

        if (report.ImportedFiles.Count == 0)
        {
            if (report.DamagedFiles.Count > 0)
            {
                return new ImportReportLogLine(
                    "ERROR",
                    $"未导入任何旧配置，损坏文件：{JoinFiles(report.DamagedFiles)}。");
            }

            if (report.MissingFiles.Count > 0)
            {
                return new ImportReportLogLine(
                    "WARN",
                    $"未找到可导入的旧配置文件：{JoinFiles(report.MissingFiles)}。");
            }

            return null;
        }

        if (manualImport)
        {
            if (report.DamagedFiles.Count > 0)
            {
                return new ImportReportLogLine(
                    "WARN",
                    $"已导入可用内容：{JoinFiles(report.ImportedFiles)}。");
            }

            if (report.MissingFiles.Count > 0)
            {
                return new ImportReportLogLine(
                    "INFO",
                    $"已强行导入旧配置：{JoinFiles(report.ImportedFiles)}。");
            }

            return new ImportReportLogLine(
                "INFO",
                $"已导入旧配置：{JoinFiles(report.ImportedFiles)}。");
        }

        if (report.ImportedFiles.Count == 2)
        {
            return new ImportReportLogLine(
                "INFO",
                $"已自动加载并转换 {JoinFiles(report.ImportedFiles)}。");
        }

        return new ImportReportLogLine(
            report.DamagedFiles.Count > 0 ? "WARN" : "INFO",
            $"已自动加载并转换 {JoinFiles(report.ImportedFiles)}。");
    }

    private static string JoinFiles(IEnumerable<string> files)
    {
        return string.Join("、", files
            .Where(static file => !string.IsNullOrWhiteSpace(file))
            .Distinct(StringComparer.OrdinalIgnoreCase));
    }
}

public readonly record struct ImportReportLogLine(string Level, string Message);
