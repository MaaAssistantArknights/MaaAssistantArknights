namespace MAAUnified.Application.Models.TaskParams;

public sealed class ReclamationTaskParamsDto
{
    public string Theme { get; set; } = "Tales";

    public int Mode { get; set; } = 1;

    public int IncrementMode { get; set; }

    public int NumCraftBatches { get; set; } = 1;

    public List<string> ToolsToCraft { get; set; } = [];

    public bool ClearStore { get; set; } = true;
}
