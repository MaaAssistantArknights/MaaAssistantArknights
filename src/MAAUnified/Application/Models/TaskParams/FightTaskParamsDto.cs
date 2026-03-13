using System;

namespace MAAUnified.Application.Models.TaskParams;

public sealed class FightTaskParamsDto
{
    public string Stage { get; set; } = FightStageSelection.CurrentOrLast;

    public bool UseMedicine { get; set; }

    public int Medicine { get; set; }

    public bool UseStone { get; set; }

    public int Stone { get; set; }

    public bool EnableTimesLimit { get; set; }

    public int Times { get; set; } = int.MaxValue;

    public int Series { get; set; } = 1;

    public bool IsDrGrandet { get; set; }

    public bool UseExpiringMedicine { get; set; }

    public bool EnableTargetDrop { get; set; }

    public string DropId { get; set; } = string.Empty;

    public int DropCount { get; set; } = 1;

    public bool UseCustomAnnihilation { get; set; }

    public string AnnihilationStage { get; set; } = "Annihilation";

    public bool UseAlternateStage { get; set; }

    public bool HideUnavailableStage { get; set; } = true;

    public string StageResetMode { get; set; } = "Current";

    public bool HideSeries { get; set; }

    public bool AllowUseStoneSave { get; set; }
}

public static class FightStageSelection
{
    public const string CurrentOrLast = "$CurrentOrLast";

    public static bool IsCurrentOrLast(string? stage)
    {
        return string.IsNullOrWhiteSpace(stage)
            || string.Equals(stage.Trim(), CurrentOrLast, StringComparison.OrdinalIgnoreCase);
    }

    public static string NormalizeStoredValue(string? stage)
    {
        return IsCurrentOrLast(stage)
            ? CurrentOrLast
            : stage!.Trim();
    }

    public static string ToCoreStage(string? stage)
    {
        return IsCurrentOrLast(stage)
            ? string.Empty
            : stage!.Trim();
    }
}
