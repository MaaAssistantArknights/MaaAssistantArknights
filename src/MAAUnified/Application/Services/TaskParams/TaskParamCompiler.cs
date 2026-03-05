using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;

namespace MAAUnified.Application.Services.TaskParams;

public sealed class TaskCompileOutput
{
    public required string NormalizedType { get; init; }

    public required JsonObject Params { get; init; }

    public required IReadOnlyList<TaskValidationIssue> Issues { get; init; }

    public bool HasBlockingIssues => Issues.Any(i => i.Blocking);
}

public static class TaskParamCompiler
{
    private const string UiUseAlternateStage = "_ui_use_alternate_stage";
    private const string UiHideUnavailableStage = "_ui_hide_unavailable_stage";
    private const string UiStageResetMode = "_ui_stage_reset_mode";
    private const string UiUseCustomAnnihilation = "_ui_use_custom_annihilation";
    private const string UiAnnihilationStage = "_ui_annihilation_stage";
    private static readonly Regex RoguelikeSeedRegex = new("^[0-9A-Za-z]+,rogue_\\d+,\\d+$", RegexOptions.Compiled);
    private static readonly HashSet<int> RoguelikeModes = [0, 1, 4, 5, 6, 7, 20001];
    private static readonly HashSet<string> RoguelikeThemes = new(StringComparer.OrdinalIgnoreCase) { "JieGarden", "Phantom", "Mizuki", "Sami", "Sarkaz" };
    private static readonly HashSet<string> ReclamationThemes = new(StringComparer.OrdinalIgnoreCase) { "Tales", "Fire" };
    private static readonly HashSet<int> ReclamationModes = [0, 1];
    private static readonly HashSet<string> RoguelikeProfessionalSquads = new(StringComparer.Ordinal)
    {
        "突击战术分队",
        "堡垒战术分队",
        "远程战术分队",
        "破坏战术分队",
    };
    private static readonly HashSet<string> CustomKnownTaskTypes = new(StringComparer.OrdinalIgnoreCase)
    {
        "StartUp",
        "CloseDown",
        "Fight",
        "Recruit",
        "Infrast",
        "Mall",
        "Award",
        "Roguelike",
        "Reclamation",
        "Custom",
        "PostAction",
    };

    public static string NormalizeTaskType(string? type)
    {
        if (string.IsNullOrWhiteSpace(type))
        {
            return "Unknown";
        }

        var normalized = type.Trim();
        if (normalized.EndsWith("Task", StringComparison.OrdinalIgnoreCase))
        {
            normalized = normalized[..^4];
        }

        return normalized switch
        {
            "StartUp" => "StartUp",
            "CloseDown" => "CloseDown",
            "Fight" => "Fight",
            "Recruit" => "Recruit",
            "Infrast" => "Infrast",
            "Mall" => "Mall",
            "Award" => "Award",
            "Roguelike" => "Roguelike",
            "Reclamation" => "Reclamation",
            "Custom" => "Custom",
            "PostAction" => "PostAction",
            _ => normalized,
        };
    }

    public static (string NormalizedType, JsonObject Params) NormalizeTypeAndCreateDefaultParams(
        string type,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        var normalizedType = NormalizeTaskType(type);

        return normalizedType switch
        {
            "StartUp" => (normalizedType, CompileStartUp(new StartUpTaskParamsDto(), profile, config).Params),
            "Fight" => (normalizedType, CompileFight(new FightTaskParamsDto(), profile, config).Params),
            "Recruit" => (normalizedType, CompileRecruit(new RecruitTaskParamsDto(), profile, config).Params),
            "Roguelike" => (normalizedType, CompileRoguelike(new RoguelikeTaskParamsDto(), profile, config).Params),
            "Reclamation" => (normalizedType, CompileReclamation(new ReclamationTaskParamsDto(), profile, config).Params),
            "Custom" => (normalizedType, CompileCustom(new CustomTaskParamsDto(), profile, config).Params),
            _ => (normalizedType, new JsonObject()),
        };
    }

    public static (StartUpTaskParamsDto Dto, IReadOnlyList<TaskValidationIssue> Issues) ReadStartUp(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var issues = new List<TaskValidationIssue>();
        var parameters = task.Params ?? new JsonObject();
        var profileClientType = ResolveStringSetting(profile, config, "ClientType");
        var clientType = !string.IsNullOrWhiteSpace(profileClientType)
            ? profileClientType
            : ReadString(parameters, "client_type", strict, issues, "start_up.client_type", "Official");
        var startGameEnabled = TryResolveBooleanSetting(profile, config, "StartGame", out var profileStartGame)
            ? profileStartGame
            : ReadBool(parameters, "start_game_enabled", strict, issues, "start_up.start_game_enabled", true);

        var dto = new StartUpTaskParamsDto
        {
            ClientType = clientType,
            StartGameEnabled = startGameEnabled,
            AccountName = ReadString(parameters, "account_name", strict, issues, "start_up.account_name", string.Empty),
            ConnectConfig = ResolveStringSetting(profile, config, "ConnectConfig") ?? "General",
            ConnectAddress = ResolveStringSetting(profile, config, "ConnectAddress") ?? "127.0.0.1:5555",
            AdbPath = ResolveStringSetting(profile, config, "AdbPath") ?? string.Empty,
            TouchMode = ResolveStringSetting(profile, config, "TouchMode") ?? "minitouch",
            AutoDetectConnection = ResolveBooleanSetting(profile, config, "AutoDetect", true),
            AttachWindowScreencapMethod = ResolveStringSetting(profile, config, "AttachWindowScreencapMethod") ?? "2",
            AttachWindowMouseMethod = ResolveStringSetting(profile, config, "AttachWindowMouseMethod") ?? "64",
            AttachWindowKeyboardMethod = ResolveStringSetting(profile, config, "AttachWindowKeyboardMethod") ?? "64",
        };

        return (dto, issues);
    }

    public static TaskCompileOutput CompileStartUp(
        StartUpTaskParamsDto dto,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        var issues = new List<TaskValidationIssue>();

        var clientType = (dto.ClientType ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(clientType))
        {
            issues.Add(new TaskValidationIssue("ClientTypeMissing", "start_up.client_type", "Client type cannot be empty."));
            clientType = "Official";
        }

        var startGameEnabled = dto.StartGameEnabled;
        if (string.Equals(dto.ConnectConfig, "PC", StringComparison.OrdinalIgnoreCase))
        {
            startGameEnabled = false;
        }

        var accountName = (dto.AccountName ?? string.Empty).Trim();
        if (!string.Equals(clientType, "Official", StringComparison.OrdinalIgnoreCase)
            && !string.Equals(clientType, "Bilibili", StringComparison.OrdinalIgnoreCase))
        {
            accountName = string.Empty;
        }

        return new TaskCompileOutput
        {
            NormalizedType = "StartUp",
            Params = new JsonObject
            {
                ["client_type"] = clientType,
                ["start_game_enabled"] = startGameEnabled,
                ["account_name"] = accountName,
            },
            Issues = issues,
        };
    }

    public static (FightTaskParamsDto Dto, IReadOnlyList<TaskValidationIssue> Issues) ReadFight(
        UnifiedTaskItem task,
        bool strict)
    {
        var issues = new List<TaskValidationIssue>();
        var parameters = task.Params ?? new JsonObject();

        var stage = ReadString(parameters, "stage", strict, issues, "fight.stage", string.Empty);
        var medicine = ReadInt(parameters, "medicine", strict, issues, "fight.medicine", 0);
        var stone = ReadInt(parameters, "stone", strict, issues, "fight.stone", 0);
        var times = ReadInt(parameters, "times", strict, issues, "fight.times", int.MaxValue);
        var series = ReadInt(parameters, "series", strict, issues, "fight.series", 1);

        string dropId = string.Empty;
        var dropCount = 1;
        if (parameters["drops"] is JsonObject drops)
        {
            var firstDrop = drops.FirstOrDefault();
            dropId = firstDrop.Key ?? string.Empty;
            if (firstDrop.Value is JsonValue value && value.TryGetValue(out int count))
            {
                dropCount = count;
            }
        }

        var dto = new FightTaskParamsDto
        {
            Stage = stage,
            Medicine = Math.Max(0, medicine),
            UseMedicine = medicine > 0,
            Stone = Math.Max(0, stone),
            UseStone = stone > 0,
            Times = times,
            EnableTimesLimit = times != int.MaxValue,
            Series = series,
            IsDrGrandet = ReadBool(parameters, "DrGrandet", false),
            UseExpiringMedicine = ReadInt(parameters, "expiring_medicine", false, issues, "fight.expiring_medicine", 0) > 0,
            EnableTargetDrop = !string.IsNullOrWhiteSpace(dropId),
            DropId = dropId,
            DropCount = Math.Max(1, dropCount),
            UseCustomAnnihilation = ReadBool(parameters, UiUseCustomAnnihilation, false),
            AnnihilationStage = ReadString(parameters, UiAnnihilationStage, false, issues, "fight.annihilation_stage", "Annihilation"),
            UseAlternateStage = ReadBool(parameters, UiUseAlternateStage, false),
            HideUnavailableStage = ReadBool(parameters, UiHideUnavailableStage, true),
            StageResetMode = ReadString(parameters, UiStageResetMode, false, issues, "fight.stage_reset_mode", "Current"),
        };

        return (dto, issues);
    }

    public static TaskCompileOutput CompileFight(
        FightTaskParamsDto dto,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        var issues = new List<TaskValidationIssue>();

        var useAlternateStage = dto.UseAlternateStage;
        var hideUnavailableStage = dto.HideUnavailableStage;
        var stageResetMode = string.IsNullOrWhiteSpace(dto.StageResetMode) ? "Current" : dto.StageResetMode;

        if (useAlternateStage)
        {
            hideUnavailableStage = false;
            stageResetMode = "Ignore";
        }

        if (hideUnavailableStage)
        {
            useAlternateStage = false;
            stageResetMode = "Current";
        }

        var stage = (dto.Stage ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(stage))
        {
            issues.Add(new TaskValidationIssue("FightStageMissing", "fight.stage", "Fight stage cannot be empty."));
        }

        if (dto.Series is < -1 or > 6)
        {
            issues.Add(new TaskValidationIssue("FightSeriesOutOfRange", "fight.series", "Fight series must be between -1 and 6."));
        }

        if (dto.Times < 0)
        {
            issues.Add(new TaskValidationIssue("FightTimesOutOfRange", "fight.times", "Fight times must be greater than or equal to zero."));
        }

        if (dto.EnableTargetDrop && string.IsNullOrWhiteSpace(dto.DropId))
        {
            issues.Add(new TaskValidationIssue("FightDropMissing", "fight.drop_id", "Target drop id cannot be empty when target drop is enabled."));
        }

        if (dto.EnableTimesLimit && dto.Series > 0 && dto.Times > 0 && dto.Times % dto.Series != 0)
        {
            issues.Add(new TaskValidationIssue(
                "FightTimesMayNotExhausted",
                "fight.times",
                "Fight times may not be fully exhausted under current series.",
                Blocking: false));
        }

        if (string.Equals(stage, "Annihilation", StringComparison.OrdinalIgnoreCase)
            && dto.UseCustomAnnihilation
            && !string.IsNullOrWhiteSpace(dto.AnnihilationStage))
        {
            stage = dto.AnnihilationStage.Trim();
        }

        var parameters = new JsonObject
        {
            ["stage"] = stage,
            ["medicine"] = dto.UseMedicine ? Math.Max(0, dto.Medicine) : 0,
            ["expiring_medicine"] = dto.UseExpiringMedicine ? 9999 : 0,
            ["stone"] = dto.UseStone ? Math.Max(0, dto.Stone) : 0,
            ["times"] = dto.EnableTimesLimit ? Math.Max(0, dto.Times) : int.MaxValue,
            ["series"] = dto.Series,
            ["DrGrandet"] = dto.IsDrGrandet,
            ["report_to_penguin"] = ResolveBooleanSetting(profile, config, "EnablePenguin"),
            ["report_to_yituliu"] = ResolveBooleanSetting(profile, config, "EnableYituliu"),
            ["penguin_id"] = ResolveStringSetting(profile, config, "PenguinId") ?? string.Empty,
            ["yituliu_id"] = ResolveStringSetting(profile, config, "YituliuId") ?? string.Empty,
            ["server"] = ResolveStringSetting(profile, config, "ServerType") ?? "CN",
            ["client_type"] = ResolveStringSetting(profile, config, "ClientType") ?? "Official",
            [UiUseAlternateStage] = useAlternateStage,
            [UiHideUnavailableStage] = hideUnavailableStage,
            [UiStageResetMode] = stageResetMode,
            [UiUseCustomAnnihilation] = dto.UseCustomAnnihilation,
            [UiAnnihilationStage] = dto.AnnihilationStage,
        };

        if (dto.EnableTargetDrop && !string.IsNullOrWhiteSpace(dto.DropId))
        {
            parameters["drops"] = new JsonObject
            {
                [dto.DropId.Trim()] = Math.Max(1, dto.DropCount),
            };
        }

        return new TaskCompileOutput
        {
            NormalizedType = "Fight",
            Params = parameters,
            Issues = issues,
        };
    }

    public static (RecruitTaskParamsDto Dto, IReadOnlyList<TaskValidationIssue> Issues) ReadRecruit(
        UnifiedTaskItem task,
        bool strict)
    {
        var issues = new List<TaskValidationIssue>();
        var parameters = task.Params ?? new JsonObject();

        var confirm = ReadIntArray(parameters, "confirm");
        var select = ReadIntArray(parameters, "select");

        var recruitmentTime = parameters["recruitment_time"] as JsonObject ?? new JsonObject();
        var dto = new RecruitTaskParamsDto
        {
            Refresh = ReadBool(parameters, "refresh", strict, issues, "recruit.refresh", true),
            ForceRefresh = ReadBool(parameters, "force_refresh", strict, issues, "recruit.force_refresh", true),
            Times = ReadInt(parameters, "times", strict, issues, "recruit.times", 4),
            SetTime = ReadBool(parameters, "set_time", strict, issues, "recruit.set_time", true),
            UseExpedited = ReadBool(parameters, "expedite", false),
            SkipRobot = ReadBool(parameters, "skip_robot", false, issues, "recruit.skip_robot", true),
            ExtraTagsMode = ReadInt(parameters, "extra_tags_mode", false, issues, "recruit.extra_tags_mode", 0),
            FirstTags = ReadStringArray(parameters, "first_tags"),
            ChooseLevel3 = confirm.Contains(3),
            ChooseLevel4 = confirm.Contains(4),
            ChooseLevel5 = confirm.Contains(5),
            Level3Time = ReadInt(recruitmentTime, "3", strict, issues, "recruit.time.3", 540),
            Level4Time = ReadInt(recruitmentTime, "4", strict, issues, "recruit.time.4", 540),
            Level5Time = ReadInt(recruitmentTime, "5", strict, issues, "recruit.time.5", 540),
        };

        if (confirm.Contains(1))
        {
            dto.SkipRobot = true;
        }

        if (select.Count == 0 && !confirm.Contains(4) && !confirm.Contains(5))
        {
            dto.ChooseLevel4 = false;
            dto.ChooseLevel5 = false;
        }

        return (dto, issues);
    }

    public static TaskCompileOutput CompileRecruit(
        RecruitTaskParamsDto dto,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        var issues = new List<TaskValidationIssue>();

        if (dto.Times < 0)
        {
            issues.Add(new TaskValidationIssue("RecruitTimesOutOfRange", "recruit.times", "Recruit times must be greater than or equal to zero."));
        }

        ValidateRecruitTime(dto.Level3Time, "recruit.time.3", issues);
        ValidateRecruitTime(dto.Level4Time, "recruit.time.4", issues);
        ValidateRecruitTime(dto.Level5Time, "recruit.time.5", issues);

        var refresh = dto.Refresh;
        var forceRefresh = dto.ForceRefresh;
        if (!refresh)
        {
            forceRefresh = false;
        }

        var select = new JsonArray();
        var confirm = new JsonArray();

        if (dto.SkipRobot)
        {
            confirm.Add(1);
        }

        if (dto.ChooseLevel3)
        {
            confirm.Add(3);
        }

        if (dto.ChooseLevel4)
        {
            select.Add(4);
            confirm.Add(4);
        }

        if (dto.ChooseLevel5)
        {
            select.Add(5);
            confirm.Add(5);
        }

        var parameters = new JsonObject
        {
            ["refresh"] = refresh,
            ["force_refresh"] = forceRefresh,
            ["select"] = select,
            ["confirm"] = confirm,
            ["times"] = Math.Max(0, dto.Times),
            ["set_time"] = dto.SetTime,
            ["expedite"] = dto.UseExpedited,
            ["skip_robot"] = dto.SkipRobot,
            ["extra_tags_mode"] = dto.ExtraTagsMode,
            ["first_tags"] = ToJsonArray(dto.FirstTags),
            ["recruitment_time"] = new JsonObject
            {
                ["3"] = ClampRecruitTime(dto.Level3Time),
                ["4"] = ClampRecruitTime(dto.Level4Time),
                ["5"] = ClampRecruitTime(dto.Level5Time),
            },
            ["report_to_penguin"] = ResolveBooleanSetting(profile, config, "EnablePenguin"),
            ["report_to_yituliu"] = ResolveBooleanSetting(profile, config, "EnableYituliu"),
            ["penguin_id"] = ResolveStringSetting(profile, config, "PenguinId") ?? string.Empty,
            ["yituliu_id"] = ResolveStringSetting(profile, config, "YituliuId") ?? string.Empty,
            ["server"] = ResolveStringSetting(profile, config, "ServerType") ?? "CN",
        };

        if (dto.UseExpedited)
        {
            parameters["expedite_times"] = Math.Max(0, dto.Times);
        }

        return new TaskCompileOutput
        {
            NormalizedType = "Recruit",
            Params = parameters,
            Issues = issues,
        };
    }

    public static (RoguelikeTaskParamsDto Dto, IReadOnlyList<TaskValidationIssue> Issues) ReadRoguelike(
        UnifiedTaskItem task,
        bool strict)
    {
        var issues = new List<TaskValidationIssue>();
        var parameters = task.Params ?? new JsonObject();

        var mode = ReadInt(parameters, "mode", strict, issues, "roguelike.mode", 0);

        var dto = new RoguelikeTaskParamsDto
        {
            Mode = mode,
            Theme = ReadString(parameters, "theme", strict, issues, "roguelike.theme", "JieGarden"),
            Difficulty = ReadInt(parameters, "difficulty", strict, issues, "roguelike.difficulty", int.MaxValue),
            StartsCount = ReadInt(parameters, "starts_count", strict, issues, "roguelike.starts_count", 999999),
            InvestmentEnabled = ReadBool(parameters, "investment_enabled", strict, issues, "roguelike.investment_enabled", true),
            InvestmentWithMoreScore = ReadBool(parameters, "investment_with_more_score", false, issues, "roguelike.investment_with_more_score", false),
            InvestmentsCount = ReadInt(parameters, "investments_count", false, issues, "roguelike.investments_count", 999),
            StopWhenInvestmentFull = ReadBool(parameters, "stop_when_investment_full", false, issues, "roguelike.stop_when_investment_full", false),
            Squad = ReadString(parameters, "squad", false, issues, "roguelike.squad", string.Empty),
            Roles = ReadString(parameters, "roles", false, issues, "roguelike.roles", string.Empty),
            CoreChar = ReadString(parameters, "core_char", false, issues, "roguelike.core_char", string.Empty),
            UseSupport = ReadBool(parameters, "use_support", strict, issues, "roguelike.use_support", false),
            UseNonfriendSupport = ReadBool(parameters, "use_nonfriend_support", strict, issues, "roguelike.use_nonfriend_support", false),
            RefreshTraderWithDice = ReadBool(parameters, "refresh_trader_with_dice", strict, issues, "roguelike.refresh_trader_with_dice", false),
            StopAtFinalBoss = ReadBool(parameters, "stop_at_final_boss", false, issues, "roguelike.stop_at_final_boss", false),
            StopAtMaxLevel = ReadBool(parameters, "stop_at_max_level", false, issues, "roguelike.stop_at_max_level", false),
            CollectibleModeShopping = ReadBool(parameters, "collectible_mode_shopping", false, issues, "roguelike.collectible_mode_shopping", false),
            CollectibleModeSquad = ReadString(parameters, "collectible_mode_squad", false, issues, "roguelike.collectible_mode_squad", string.Empty),
            StartWithEliteTwo = ReadBool(parameters, "start_with_elite_two", false, issues, "roguelike.start_with_elite_two", false),
            OnlyStartWithEliteTwo = ReadBool(parameters, "only_start_with_elite_two", false, issues, "roguelike.only_start_with_elite_two", false),
            CollectibleModeStartList = ReadCollectibleStartList(parameters["collectible_mode_start_list"], issues, "roguelike.collectible_mode_start_list"),
            MonthlySquadAutoIterate = ReadBool(parameters, "monthly_squad_auto_iterate", false, issues, "roguelike.monthly_squad_auto_iterate", true),
            MonthlySquadCheckComms = ReadBool(parameters, "monthly_squad_check_comms", false, issues, "roguelike.monthly_squad_check_comms", true),
            DeepExplorationAutoIterate = ReadBool(parameters, "deep_exploration_auto_iterate", false, issues, "roguelike.deep_exploration_auto_iterate", true),
            FindPlayTimeTarget = ReadIntWithAliases(
                parameters,
                ["find_playTime_target", "find_playtime_target"],
                false,
                issues,
                "roguelike.find_playTime_target",
                1),
            FirstFloorFoldartal = ReadString(parameters, "first_floor_foldartal", false, issues, "roguelike.first_floor_foldartal", string.Empty),
            StartFoldartalList = ReadStringArrayCompat(parameters, "start_foldartal_list", false, issues, "roguelike.start_foldartal_list"),
            ExpectedCollapsalParadigms = ReadStringArrayCompat(parameters, "expected_collapsal_paradigms", false, issues, "roguelike.expected_collapsal_paradigms"),
            StartWithSeed = ReadString(parameters, "start_with_seed", false, issues, "roguelike.start_with_seed", string.Empty),
        };

        return (dto, issues);
    }

    public static TaskCompileOutput CompileRoguelike(
        RoguelikeTaskParamsDto dto,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        _ = profile;
        _ = config;

        var issues = new List<TaskValidationIssue>();

        var mode = dto.Mode;
        if (!RoguelikeModes.Contains(mode))
        {
            issues.Add(new TaskValidationIssue("RoguelikeModeInvalid", "roguelike.mode", "Roguelike mode is not supported by current schema."));
            mode = 0;
        }

        var theme = string.IsNullOrWhiteSpace(dto.Theme) ? "JieGarden" : dto.Theme.Trim();
        if (!RoguelikeThemes.Contains(theme))
        {
            issues.Add(new TaskValidationIssue("RoguelikeThemeUnknown", "roguelike.theme", "Unknown roguelike theme, fallback to JieGarden.", Blocking: false));
            theme = "JieGarden";
        }

        if (dto.Difficulty < 0)
        {
            issues.Add(new TaskValidationIssue("RoguelikeDifficultyOutOfRange", "roguelike.difficulty", "Difficulty must be greater than or equal to zero.", Blocking: false));
        }

        if (dto.StartsCount < 0)
        {
            issues.Add(new TaskValidationIssue("RoguelikeStartsCountOutOfRange", "roguelike.starts_count", "Starts count must be greater than or equal to zero.", Blocking: false));
        }

        if (dto.InvestmentsCount < 0)
        {
            issues.Add(new TaskValidationIssue("RoguelikeInvestmentsCountOutOfRange", "roguelike.investments_count", "Investments count must be greater than or equal to zero.", Blocking: false));
        }

        if (!string.Equals(theme, "Mizuki", StringComparison.OrdinalIgnoreCase) && dto.RefreshTraderWithDice)
        {
            issues.Add(new TaskValidationIssue("RoguelikeRefreshTraderThemeMismatch", "roguelike.refresh_trader_with_dice", "Refresh trader with dice is only supported in Mizuki theme.", Blocking: false));
        }

        if (mode == 20001 && !string.Equals(theme, "JieGarden", StringComparison.OrdinalIgnoreCase))
        {
            issues.Add(new TaskValidationIssue("RoguelikeFindPlaytimeThemeMismatch", "roguelike.theme", "FindPlaytime mode requires JieGarden theme, fallback applied.", Blocking: false));
            theme = "JieGarden";
        }

        var findPlayTimeTarget = dto.FindPlayTimeTarget;
        if (mode == 20001 && (findPlayTimeTarget is < 1 or > 3))
        {
            issues.Add(new TaskValidationIssue("RoguelikeFindPlaytimeTargetOutOfRange", "roguelike.find_playTime_target", "FindPlaytime target must be between 1 and 3.", Blocking: false));
            findPlayTimeTarget = 1;
        }

        var startWithEliteTwo = dto.StartWithEliteTwo;
        var onlyStartWithEliteTwo = dto.OnlyStartWithEliteTwo;
        var squad = (dto.Squad ?? string.Empty).Trim();
        var modeAllowsEliteTwo = mode == 4
            && (string.Equals(theme, "Mizuki", StringComparison.OrdinalIgnoreCase) || string.Equals(theme, "Sami", StringComparison.OrdinalIgnoreCase))
            && IsRoguelikeProfessionalSquad(squad);
        if (startWithEliteTwo && !modeAllowsEliteTwo)
        {
            issues.Add(new TaskValidationIssue("RoguelikeEliteTwoModeMismatch", "roguelike.start_with_elite_two", "StartWithEliteTwo is only supported in collectible mode with professional squad.", Blocking: false));
            startWithEliteTwo = false;
        }

        if (onlyStartWithEliteTwo && !startWithEliteTwo)
        {
            issues.Add(new TaskValidationIssue("RoguelikeOnlyEliteTwoRequiresEliteTwo", "roguelike.only_start_with_elite_two", "OnlyStartWithEliteTwo requires StartWithEliteTwo to be enabled.", Blocking: false));
            onlyStartWithEliteTwo = false;
        }

        if (dto.UseSupport && startWithEliteTwo)
        {
            issues.Add(new TaskValidationIssue("RoguelikeEliteTwoSupportConflict", "roguelike.use_support", "UseSupport conflicts with StartWithEliteTwo under current strategy.", Blocking: false));
            startWithEliteTwo = false;
            onlyStartWithEliteTwo = false;
        }

        var startFoldartalList = ParseDelimitedList(dto.StartFoldartalList);
        if (startFoldartalList.Count > 3)
        {
            issues.Add(new TaskValidationIssue("RoguelikeStartFoldartalListTrimmed", "roguelike.start_foldartal_list", "Start foldartal list exceeds max size and will be trimmed to 3.", Blocking: false));
            startFoldartalList = startFoldartalList.Take(3).ToList();
        }

        var expectedCollapsalParadigms = ParseDelimitedList(dto.ExpectedCollapsalParadigms);

        var startWithSeed = (dto.StartWithSeed ?? string.Empty).Trim();
        if (!string.IsNullOrWhiteSpace(startWithSeed) && !RoguelikeSeedRegex.IsMatch(startWithSeed))
        {
            issues.Add(new TaskValidationIssue("RoguelikeStartWithSeedInvalid", "roguelike.start_with_seed", "Seed format is invalid. Expected `<alnum>,rogue_<id>,<step>`."));
        }

        var parameters = new JsonObject
        {
            ["mode"] = mode,
            ["theme"] = theme,
            ["difficulty"] = Math.Max(0, dto.Difficulty),
            ["starts_count"] = Math.Max(0, dto.StartsCount),
            ["investment_enabled"] = dto.InvestmentEnabled,
            ["use_support"] = dto.UseSupport,
            ["use_nonfriend_support"] = dto.UseNonfriendSupport,
            ["refresh_trader_with_dice"] = string.Equals(theme, "Mizuki", StringComparison.OrdinalIgnoreCase) && dto.RefreshTraderWithDice,
        };

        if (dto.InvestmentEnabled)
        {
            parameters["investment_with_more_score"] = dto.InvestmentWithMoreScore && mode == 1;
            parameters["investments_count"] = Math.Max(0, dto.InvestmentsCount);
            parameters["stop_when_investment_full"] = dto.StopWhenInvestmentFull;
        }

        if (!string.IsNullOrWhiteSpace(squad))
        {
            parameters["squad"] = squad;
        }

        if (!string.IsNullOrWhiteSpace(dto.Roles))
        {
            parameters["roles"] = dto.Roles.Trim();
        }

        if (!string.IsNullOrWhiteSpace(dto.CoreChar))
        {
            parameters["core_char"] = dto.CoreChar.Trim();
        }

        if (mode == 0)
        {
            parameters["stop_at_final_boss"] = dto.StopAtFinalBoss;
            parameters["stop_at_max_level"] = dto.StopAtMaxLevel;
        }

        if (mode == 4)
        {
            parameters["collectible_mode_shopping"] = dto.CollectibleModeShopping;
            parameters["collectible_mode_squad"] = dto.CollectibleModeSquad.Trim();
            parameters["start_with_elite_two"] = startWithEliteTwo;
            parameters["only_start_with_elite_two"] = onlyStartWithEliteTwo;
            parameters["collectible_mode_start_list"] = CompileCollectibleStartList(dto.CollectibleModeStartList);
        }

        if (mode == 6)
        {
            parameters["monthly_squad_auto_iterate"] = dto.MonthlySquadAutoIterate;
            parameters["monthly_squad_check_comms"] = dto.MonthlySquadCheckComms;
        }

        if (mode == 7)
        {
            parameters["deep_exploration_auto_iterate"] = dto.DeepExplorationAutoIterate;
        }

        if (mode == 20001)
        {
            parameters["find_playTime_target"] = findPlayTimeTarget;
        }

        if (!string.IsNullOrWhiteSpace(dto.FirstFloorFoldartal))
        {
            parameters["first_floor_foldartal"] = dto.FirstFloorFoldartal.Trim();
        }

        if (startFoldartalList.Count > 0)
        {
            parameters["start_foldartal_list"] = ToJsonArray(startFoldartalList);
        }

        if (mode == 5)
        {
            parameters["expected_collapsal_paradigms"] = ToJsonArray(expectedCollapsalParadigms);
        }

        if (!string.IsNullOrWhiteSpace(startWithSeed))
        {
            parameters["start_with_seed"] = startWithSeed;
        }

        return new TaskCompileOutput
        {
            NormalizedType = "Roguelike",
            Params = parameters,
            Issues = issues,
        };
    }

    public static (ReclamationTaskParamsDto Dto, IReadOnlyList<TaskValidationIssue> Issues) ReadReclamation(
        UnifiedTaskItem task,
        bool strict)
    {
        var issues = new List<TaskValidationIssue>();
        var parameters = task.Params ?? new JsonObject();

        var dto = new ReclamationTaskParamsDto
        {
            Theme = ReadString(parameters, "theme", strict, issues, "reclamation.theme", "Tales"),
            Mode = ReadInt(parameters, "mode", strict, issues, "reclamation.mode", 1),
            IncrementMode = ReadInt(parameters, "increment_mode", strict, issues, "reclamation.increment_mode", 0),
            NumCraftBatches = ReadInt(parameters, "num_craft_batches", strict, issues, "reclamation.num_craft_batches", 1),
            ToolsToCraft = ReadStringArrayCompat(parameters, "tools_to_craft", false, issues, "reclamation.tools_to_craft"),
            ClearStore = ReadBool(parameters, "clear_store", strict, issues, "reclamation.clear_store", true),
        };

        return (dto, issues);
    }

    public static TaskCompileOutput CompileReclamation(
        ReclamationTaskParamsDto dto,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        _ = profile;
        _ = config;

        var issues = new List<TaskValidationIssue>();

        var theme = string.IsNullOrWhiteSpace(dto.Theme) ? "Tales" : dto.Theme.Trim();
        if (!ReclamationThemes.Contains(theme))
        {
            issues.Add(new TaskValidationIssue("ReclamationThemeUnknown", "reclamation.theme", "Unknown reclamation theme, fallback to Tales.", Blocking: false));
            theme = "Tales";
        }

        var mode = dto.Mode;
        if (!ReclamationModes.Contains(mode))
        {
            issues.Add(new TaskValidationIssue("ReclamationModeInvalid", "reclamation.mode", "Reclamation mode is not supported by current schema."));
            mode = 1;
        }

        var incrementMode = dto.IncrementMode;
        if (incrementMode is < 0 or > 1)
        {
            issues.Add(new TaskValidationIssue("ReclamationIncrementModeOutOfRange", "reclamation.increment_mode", "Increment mode must be 0 or 1.", Blocking: false));
            incrementMode = 0;
        }

        var numCraftBatches = dto.NumCraftBatches;
        if (numCraftBatches is < 1 or > 99999)
        {
            issues.Add(new TaskValidationIssue("ReclamationNumCraftBatchesOutOfRange", "reclamation.num_craft_batches", "NumCraftBatches must be between 1 and 99999.", Blocking: false));
            numCraftBatches = Math.Clamp(numCraftBatches, 1, 99999);
        }

        var toolsToCraft = ParseDelimitedList(dto.ToolsToCraft);
        if (toolsToCraft.Any(ContainsStructuredToken))
        {
            issues.Add(new TaskValidationIssue("ReclamationToolNameInvalid", "reclamation.tools_to_craft", "ToolsToCraft contains unparseable structured tokens."));
        }

        if (mode == 0 && toolsToCraft.Count > 0)
        {
            issues.Add(new TaskValidationIssue("ReclamationToolsIgnoredInNoArchive", "reclamation.tools_to_craft", "ToolsToCraft is ignored in no-archive mode and will be cleared.", Blocking: false));
            toolsToCraft = [];
        }

        var clearStore = dto.ClearStore;
        if (mode == 1 && clearStore)
        {
            issues.Add(new TaskValidationIssue(
                "ReclamationClearStoreIgnoredInArchive",
                "reclamation.clear_store",
                "ClearStore is ignored in archive mode and will be disabled.",
                Blocking: false));
            clearStore = false;
        }

        var parameters = new JsonObject
        {
            ["theme"] = theme,
            ["mode"] = mode,
            ["increment_mode"] = incrementMode,
            ["num_craft_batches"] = numCraftBatches,
            ["tools_to_craft"] = ToJsonArray(toolsToCraft),
            ["clear_store"] = clearStore,
        };

        return new TaskCompileOutput
        {
            NormalizedType = "Reclamation",
            Params = parameters,
            Issues = issues,
        };
    }

    public static (CustomTaskParamsDto Dto, IReadOnlyList<TaskValidationIssue> Issues) ReadCustom(
        UnifiedTaskItem task,
        bool strict)
    {
        var issues = new List<TaskValidationIssue>();
        var parameters = task.Params ?? new JsonObject();
        var taskNames = ReadStringArrayCompat(parameters, "task_names", strict, issues, "custom.task_names");

        var dto = new CustomTaskParamsDto
        {
            TaskNames = taskNames,
        };

        return (dto, issues);
    }

    public static TaskCompileOutput CompileCustom(
        CustomTaskParamsDto dto,
        UnifiedProfile profile,
        UnifiedConfig config)
    {
        _ = profile;
        _ = config;

        var issues = new List<TaskValidationIssue>();
        var taskNames = ParseDelimitedList(dto.TaskNames);
        if (taskNames.Count == 0)
        {
            issues.Add(new TaskValidationIssue("CustomTaskNamesEmpty", "custom.task_names", "Custom task names list is empty.", Blocking: false));
        }

        var normalizedTaskNames = new List<string>(taskNames.Count);
        var normalizedChanged = false;
        foreach (var taskName in taskNames)
        {
            if (ContainsStructuredToken(taskName))
            {
                issues.Add(new TaskValidationIssue("CustomTaskNameInvalid", "custom.task_names", $"Custom task name `{taskName}` contains unparseable structured tokens."));
                continue;
            }

            var normalizedName = NormalizeTaskType(taskName);
            if (!CustomKnownTaskTypes.Contains(normalizedName))
            {
                issues.Add(new TaskValidationIssue("CustomTaskNameUnknown", "custom.task_names", $"Custom task name `{taskName}` is not recognized.", Blocking: false));
            }

            normalizedTaskNames.Add(normalizedName);
            normalizedChanged |= !string.Equals(taskName, normalizedName, StringComparison.Ordinal);
        }

        if (normalizedChanged || normalizedTaskNames.Count != taskNames.Count || taskNames.Count != dto.TaskNames.Count)
        {
            issues.Add(new TaskValidationIssue("CustomTaskNamesNormalized", "custom.task_names", "Custom task names were normalized and deduplicated.", Blocking: false));
        }

        var parameters = new JsonObject
        {
            ["task_names"] = ToJsonArray(normalizedTaskNames),
        };

        return new TaskCompileOutput
        {
            NormalizedType = "Custom",
            Params = parameters,
            Issues = issues,
        };
    }

    public static TaskCompileOutput CompileTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var normalized = NormalizeTaskType(task.Type);

        return normalized switch
        {
            "StartUp" => CompileStartUpFromTask(task, profile, config, strict),
            "Fight" => CompileFightFromTask(task, profile, config, strict),
            "Recruit" => CompileRecruitFromTask(task, profile, config, strict),
            "Roguelike" => CompileRoguelikeFromTask(task, profile, config, strict),
            "Reclamation" => CompileReclamationFromTask(task, profile, config, strict),
            "Custom" => CompileCustomFromTask(task, profile, config, strict),
            _ => new TaskCompileOutput
            {
                NormalizedType = normalized,
                Params = task.Params ?? new JsonObject(),
                Issues = [],
            },
        };
    }

    private static TaskCompileOutput CompileStartUpFromTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var (dto, readIssues) = ReadStartUp(task, profile, config, strict);
        var compiled = CompileStartUp(dto, profile, config);
        var allIssues = readIssues.Concat(compiled.Issues).ToList();
        return new TaskCompileOutput
        {
            NormalizedType = compiled.NormalizedType,
            Params = compiled.Params,
            Issues = allIssues,
        };
    }

    private static TaskCompileOutput CompileFightFromTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var (dto, readIssues) = ReadFight(task, strict);
        var compiled = CompileFight(dto, profile, config);
        var allIssues = readIssues.Concat(compiled.Issues).ToList();
        return new TaskCompileOutput
        {
            NormalizedType = compiled.NormalizedType,
            Params = compiled.Params,
            Issues = allIssues,
        };
    }

    private static TaskCompileOutput CompileRecruitFromTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var (dto, readIssues) = ReadRecruit(task, strict);
        var compiled = CompileRecruit(dto, profile, config);
        var allIssues = readIssues.Concat(compiled.Issues).ToList();
        return new TaskCompileOutput
        {
            NormalizedType = compiled.NormalizedType,
            Params = compiled.Params,
            Issues = allIssues,
        };
    }

    private static TaskCompileOutput CompileRoguelikeFromTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var (dto, readIssues) = ReadRoguelike(task, strict);
        var compiled = CompileRoguelike(dto, profile, config);
        var allIssues = readIssues.Concat(compiled.Issues).ToList();
        return new TaskCompileOutput
        {
            NormalizedType = compiled.NormalizedType,
            Params = compiled.Params,
            Issues = allIssues,
        };
    }

    private static TaskCompileOutput CompileReclamationFromTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var (dto, readIssues) = ReadReclamation(task, strict);
        var compiled = CompileReclamation(dto, profile, config);
        var allIssues = readIssues.Concat(compiled.Issues).ToList();
        return new TaskCompileOutput
        {
            NormalizedType = compiled.NormalizedType,
            Params = compiled.Params,
            Issues = allIssues,
        };
    }

    private static TaskCompileOutput CompileCustomFromTask(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        bool strict)
    {
        var (dto, readIssues) = ReadCustom(task, strict);
        var compiled = CompileCustom(dto, profile, config);
        var allIssues = readIssues.Concat(compiled.Issues).ToList();
        return new TaskCompileOutput
        {
            NormalizedType = compiled.NormalizedType,
            Params = compiled.Params,
            Issues = allIssues,
        };
    }

    public static void ApplyStartUpSharedProfileValues(UnifiedProfile profile, StartUpTaskParamsDto dto)
    {
        profile.Values["ConnectConfig"] = JsonValue.Create(dto.ConnectConfig);
        profile.Values["ConnectAddress"] = JsonValue.Create(dto.ConnectAddress);
        profile.Values["AdbPath"] = JsonValue.Create(dto.AdbPath);
        profile.Values["TouchMode"] = JsonValue.Create(dto.TouchMode);
        profile.Values["AutoDetect"] = JsonValue.Create(dto.AutoDetectConnection);
        profile.Values["AttachWindowScreencapMethod"] = JsonValue.Create(dto.AttachWindowScreencapMethod);
        profile.Values["AttachWindowMouseMethod"] = JsonValue.Create(dto.AttachWindowMouseMethod);
        profile.Values["AttachWindowKeyboardMethod"] = JsonValue.Create(dto.AttachWindowKeyboardMethod);
        profile.Values["ClientType"] = JsonValue.Create(dto.ClientType);
        profile.Values["StartGame"] = JsonValue.Create(dto.StartGameEnabled);
    }

    private static void ValidateRecruitTime(int value, string field, ICollection<TaskValidationIssue> issues)
    {
        if (value < 60 || value > 540 || value % 10 != 0)
        {
            issues.Add(new TaskValidationIssue(
                "RecruitTimeOutOfRange",
                field,
                "Recruit time must be between 60 and 540 minutes and aligned to 10-minute steps."));
        }
    }

    private static int ClampRecruitTime(int value)
    {
        var clamped = Math.Clamp(value, 60, 540);
        return (clamped / 10) * 10;
    }

    private static JsonArray ToJsonArray(IEnumerable<string> values)
    {
        var array = new JsonArray();
        foreach (var value in values.Where(v => !string.IsNullOrWhiteSpace(v)).Select(v => v.Trim()).Distinct(StringComparer.Ordinal))
        {
            array.Add(value);
        }

        return array;
    }

    private static List<string> ParseDelimitedList(IEnumerable<string> values)
    {
        return values
            .Where(v => !string.IsNullOrWhiteSpace(v))
            .Select(v => v.Trim())
            .Where(v => !string.IsNullOrWhiteSpace(v))
            .Distinct(StringComparer.Ordinal)
            .ToList();
    }

    private static bool IsRoguelikeProfessionalSquad(string squad)
    {
        return RoguelikeProfessionalSquads.Contains(squad);
    }

    private static bool ContainsStructuredToken(string value)
    {
        return value.IndexOfAny(['[', ']', '{', '}', ':', '"', '\r', '\n']) >= 0
               || value.Any(c => char.IsControl(c) && c != '\t');
    }

    private static JsonObject CompileCollectibleStartList(RoguelikeCollectibleStartListDto? dto)
    {
        dto ??= new RoguelikeCollectibleStartListDto();
        return new JsonObject
        {
            ["hot_water"] = dto.HotWater,
            ["shield"] = dto.Shield,
            ["ingot"] = dto.Ingot,
            ["hope"] = dto.Hope,
            ["random"] = dto.Random,
            ["key"] = dto.Key,
            ["dice"] = dto.Dice,
            ["ideas"] = dto.Ideas,
            ["ticket"] = dto.Ticket,
        };
    }

    private static RoguelikeCollectibleStartListDto ReadCollectibleStartList(
        JsonNode? node,
        ICollection<TaskValidationIssue> issues,
        string field)
    {
        if (node is null)
        {
            return new RoguelikeCollectibleStartListDto();
        }

        if (node is not JsonObject value)
        {
            issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, "Task field has incompatible type."));
            return new RoguelikeCollectibleStartListDto();
        }

        return new RoguelikeCollectibleStartListDto
        {
            HotWater = ReadBool(value, "hot_water", false, issues, $"{field}.hot_water", false),
            Shield = ReadBool(value, "shield", false, issues, $"{field}.shield", false),
            Ingot = ReadBool(value, "ingot", false, issues, $"{field}.ingot", false),
            Hope = ReadBool(value, "hope", false, issues, $"{field}.hope", false),
            Random = ReadBool(value, "random", false, issues, $"{field}.random", false),
            Key = ReadBool(value, "key", false, issues, $"{field}.key", false),
            Dice = ReadBool(value, "dice", false, issues, $"{field}.dice", false),
            Ideas = ReadBool(value, "ideas", false, issues, $"{field}.ideas", false),
            Ticket = ReadBool(value, "ticket", false, issues, $"{field}.ticket", false),
        };
    }

    private static List<int> ReadIntArray(JsonObject obj, string key)
    {
        if (obj[key] is not JsonArray array)
        {
            return [];
        }

        var result = new List<int>();
        foreach (var item in array)
        {
            if (item is JsonValue value && value.TryGetValue(out int parsed))
            {
                result.Add(parsed);
            }
        }

        return result;
    }

    private static List<string> ReadStringArray(JsonObject obj, string key)
    {
        if (obj[key] is not JsonArray array)
        {
            return [];
        }

        var result = new List<string>();
        foreach (var item in array)
        {
            if (item is JsonValue value && value.TryGetValue(out string? parsed) && !string.IsNullOrWhiteSpace(parsed))
            {
                result.Add(parsed.Trim());
            }
        }

        return result;
    }

    private static List<string> ReadStringArrayCompat(
        JsonObject obj,
        string key,
        bool strict,
        ICollection<TaskValidationIssue> issues,
        string field)
    {
        if (!obj.TryGetPropertyValue(key, out var node))
        {
            if (strict)
            {
                issues.Add(new TaskValidationIssue("TaskFieldMissing", field, $"Required task field `{key}` is missing."));
            }

            return [];
        }

        if (node is JsonArray array)
        {
            var result = new List<string>();
            var hasInvalidEntry = false;
            foreach (var item in array)
            {
                if (item is JsonValue jsonValue
                    && jsonValue.TryGetValue(out string? parsed)
                    && !string.IsNullOrWhiteSpace(parsed))
                {
                    result.Add(parsed.Trim());
                    continue;
                }

                if (item is not null)
                {
                    hasInvalidEntry = true;
                }
            }

            if (hasInvalidEntry)
            {
                issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, "Task field has incompatible type."));
            }

            return result
                .Where(v => !string.IsNullOrWhiteSpace(v))
                .Distinct(StringComparer.Ordinal)
                .ToList();
        }

        if (node is JsonValue value && value.TryGetValue(out string? text) && !string.IsNullOrWhiteSpace(text))
        {
            return text
                .Split(new[] { '\r', '\n', ';', ',', '|' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                .Where(v => !string.IsNullOrWhiteSpace(v))
                .Distinct(StringComparer.Ordinal)
                .ToList();
        }

        issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, "Task field has incompatible type."));
        return [];
    }

    private static string ReadString(
        JsonObject obj,
        string key,
        bool strict,
        ICollection<TaskValidationIssue> issues,
        string field,
        string fallback)
    {
        if (obj.TryGetPropertyValue(key, out var value))
        {
            if (value is JsonValue jsonValue
                && jsonValue.TryGetValue(out string? text)
                && text is not null)
            {
                return text;
            }

            issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
            return fallback;
        }

        if (strict)
        {
            issues.Add(new TaskValidationIssue("TaskFieldMissing", field, $"Required task field `{key}` is missing."));
        }

        return fallback;
    }

    private static int ReadInt(
        JsonObject obj,
        string key,
        bool strict,
        ICollection<TaskValidationIssue> issues,
        string field,
        int fallback)
    {
        if (obj.TryGetPropertyValue(key, out var value))
        {
            if (value is not JsonValue jsonValue)
            {
                issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
                return fallback;
            }

            if (jsonValue.TryGetValue(out int parsed))
            {
                return parsed;
            }

            if (jsonValue.TryGetValue(out long parsedLong))
            {
                return Convert.ToInt32(parsedLong);
            }

            if (jsonValue.TryGetValue(out string? text) && int.TryParse(text, out var parsedText))
            {
                return parsedText;
            }

            issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
            return fallback;
        }

        if (strict)
        {
            issues.Add(new TaskValidationIssue("TaskFieldMissing", field, $"Required task field `{key}` is missing."));
        }

        return fallback;
    }

    private static int ReadIntWithAliases(
        JsonObject obj,
        IReadOnlyList<string> keys,
        bool strict,
        ICollection<TaskValidationIssue> issues,
        string field,
        int fallback)
    {
        var foundAlias = false;
        foreach (var key in keys)
        {
            if (!obj.TryGetPropertyValue(key, out var value)
                || value is null)
            {
                continue;
            }

            foundAlias = true;
            if (value is not JsonValue jsonValue)
            {
                issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
                return fallback;
            }

            if (jsonValue.TryGetValue(out int parsed))
            {
                return parsed;
            }

            if (jsonValue.TryGetValue(out long parsedLong))
            {
                return Convert.ToInt32(parsedLong);
            }

            if (jsonValue.TryGetValue(out string? text) && int.TryParse(text, out var parsedText))
            {
                return parsedText;
            }

            issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
            return fallback;
        }

        if (!foundAlias && strict)
        {
            issues.Add(new TaskValidationIssue("TaskFieldMissing", field, $"Required task field `{keys[0]}` is missing."));
        }

        return fallback;
    }

    private static bool ReadBool(
        JsonObject obj,
        string key,
        bool strict,
        ICollection<TaskValidationIssue> issues,
        string field,
        bool fallback)
    {
        if (obj.TryGetPropertyValue(key, out var value))
        {
            if (value is not JsonValue jsonValue)
            {
                issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
                return fallback;
            }

            if (jsonValue.TryGetValue(out bool parsed))
            {
                return parsed;
            }

            if (jsonValue.TryGetValue(out int parsedInt))
            {
                return parsedInt != 0;
            }

            if (jsonValue.TryGetValue(out string? text) && bool.TryParse(text, out var parsedText))
            {
                return parsedText;
            }

            issues.Add(new TaskValidationIssue("TaskFieldTypeInvalid", field, $"Task field `{key}` has incompatible type."));
            return fallback;
        }

        if (strict)
        {
            issues.Add(new TaskValidationIssue("TaskFieldMissing", field, $"Required task field `{key}` is missing."));
        }

        return fallback;
    }

    private static string? ResolveStringSetting(UnifiedProfile profile, UnifiedConfig config, string key)
    {
        if (profile.Values.TryGetValue(key, out var profileValue)
            && profileValue is JsonValue value
            && value.TryGetValue(out string? text)
            && !string.IsNullOrWhiteSpace(text))
        {
            return text;
        }

        if (config.GlobalValues.TryGetValue(key, out var globalValue)
            && globalValue is JsonValue global
            && global.TryGetValue(out string? globalText)
            && !string.IsNullOrWhiteSpace(globalText))
        {
            return globalText;
        }

        if (config.GlobalValues.TryGetValue($"GUI.{key}", out var guiValue)
            && guiValue is JsonValue gui
            && gui.TryGetValue(out string? guiText)
            && !string.IsNullOrWhiteSpace(guiText))
        {
            return guiText;
        }

        return null;
    }

    private static bool ResolveBooleanSetting(UnifiedProfile profile, UnifiedConfig config, string key, bool fallback = false)
    {
        if (TryResolveBooleanSetting(profile, config, key, out var value))
        {
            return value;
        }

        return fallback;
    }

    private static bool TryResolveBooleanSetting(UnifiedProfile profile, UnifiedConfig config, string key, out bool value)
    {
        if (profile.Values.TryGetValue(key, out var profileValue) && TryReadBooleanNode(profileValue, out value))
        {
            return true;
        }

        if (config.GlobalValues.TryGetValue(key, out var globalValue) && TryReadBooleanNode(globalValue, out value))
        {
            return true;
        }

        if (config.GlobalValues.TryGetValue($"GUI.{key}", out var guiValue) && TryReadBooleanNode(guiValue, out value))
        {
            return true;
        }

        value = default;
        return false;
    }

    private static bool ToBoolean(JsonNode? node, bool fallback)
    {
        if (TryReadBooleanNode(node, out var parsed))
        {
            return parsed;
        }

        return fallback;
    }

    private static bool TryReadBooleanNode(JsonNode? node, out bool value)
    {
        if (node is not JsonValue jsonValue)
        {
            value = default;
            return false;
        }

        if (jsonValue.TryGetValue(out bool parsedBool))
        {
            value = parsedBool;
            return true;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            value = parsedInt != 0;
            return true;
        }

        if (jsonValue.TryGetValue(out string? text) && bool.TryParse(text, out var parsedText))
        {
            value = parsedText;
            return true;
        }

        value = default;
        return false;
    }

    private static bool ReadBool(JsonObject obj, string key, bool fallback)
    {
        if (!obj.TryGetPropertyValue(key, out var node))
        {
            return fallback;
        }

        return ToBoolean(node, fallback);
    }
}
