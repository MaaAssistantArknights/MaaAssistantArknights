namespace MAAUnified.Application.Models.TaskParams;

public sealed class RecruitTaskParamsDto
{
    public int Times { get; set; } = 4;

    public bool Refresh { get; set; } = true;

    public bool ForceRefresh { get; set; } = true;

    public bool UseExpedited { get; set; }

    public bool SkipRobot { get; set; } = true;

    public int ExtraTagsMode { get; set; }

    public List<string> FirstTags { get; set; } = [];

    public bool ChooseLevel3 { get; set; } = true;

    public bool ChooseLevel4 { get; set; } = true;

    public bool ChooseLevel5 { get; set; }

    public int Level3Time { get; set; } = 540;

    public int Level4Time { get; set; } = 540;

    public int Level5Time { get; set; } = 540;

    public bool SetTime { get; set; } = true;
}
