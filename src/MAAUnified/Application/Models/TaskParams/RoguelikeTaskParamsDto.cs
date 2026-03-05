namespace MAAUnified.Application.Models.TaskParams;

public sealed class RoguelikeTaskParamsDto
{
    public int Mode { get; set; }

    public string Theme { get; set; } = "JieGarden";

    public int Difficulty { get; set; } = int.MaxValue;

    public int StartsCount { get; set; } = 999999;

    public bool InvestmentEnabled { get; set; } = true;

    public bool InvestmentWithMoreScore { get; set; }

    public int InvestmentsCount { get; set; } = 999;

    public bool StopWhenInvestmentFull { get; set; }

    public string Squad { get; set; } = string.Empty;

    public string Roles { get; set; } = string.Empty;

    public string CoreChar { get; set; } = string.Empty;

    public bool UseSupport { get; set; }

    public bool UseNonfriendSupport { get; set; }

    public bool RefreshTraderWithDice { get; set; }

    public bool StopAtFinalBoss { get; set; }

    public bool StopAtMaxLevel { get; set; }

    public bool CollectibleModeShopping { get; set; }

    public string CollectibleModeSquad { get; set; } = string.Empty;

    public bool StartWithEliteTwo { get; set; }

    public bool OnlyStartWithEliteTwo { get; set; }

    public RoguelikeCollectibleStartListDto CollectibleModeStartList { get; set; } = new();

    public bool MonthlySquadAutoIterate { get; set; } = true;

    public bool MonthlySquadCheckComms { get; set; } = true;

    public bool DeepExplorationAutoIterate { get; set; } = true;

    public int FindPlayTimeTarget { get; set; } = 1;

    public string FirstFloorFoldartal { get; set; } = string.Empty;

    public List<string> StartFoldartalList { get; set; } = [];

    public List<string> ExpectedCollapsalParadigms { get; set; } = [];

    public string StartWithSeed { get; set; } = string.Empty;
}

public sealed class RoguelikeCollectibleStartListDto
{
    public bool HotWater { get; set; }

    public bool Shield { get; set; }

    public bool Ingot { get; set; }

    public bool Hope { get; set; }

    public bool Random { get; set; }

    public bool Key { get; set; }

    public bool Dice { get; set; }

    public bool Ideas { get; set; }

    public bool Ticket { get; set; }
}
