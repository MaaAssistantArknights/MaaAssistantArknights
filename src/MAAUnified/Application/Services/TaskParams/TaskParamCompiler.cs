using System.Text.Json.Nodes;
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

        var dto = new StartUpTaskParamsDto
        {
            ClientType = ReadString(parameters, "client_type", strict, issues, "start_up.client_type", ResolveStringSetting(profile, config, "ClientType") ?? "Official"),
            StartGameEnabled = ReadBool(parameters, "start_game_enabled", strict, issues, "start_up.start_game_enabled", ResolveBooleanSetting(profile, config, "StartGame", true)),
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

    private static string ReadString(
        JsonObject obj,
        string key,
        bool strict,
        ICollection<TaskValidationIssue> issues,
        string field,
        string fallback)
    {
        if (obj.TryGetPropertyValue(key, out var value)
            && value is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? text)
            && text is not null)
        {
            return text;
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
        if (obj.TryGetPropertyValue(key, out var value)
            && value is JsonValue jsonValue)
        {
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
        }

        if (strict)
        {
            issues.Add(new TaskValidationIssue("TaskFieldMissing", field, $"Required task field `{key}` is missing."));
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
        if (obj.TryGetPropertyValue(key, out var value)
            && value is JsonValue jsonValue)
        {
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
        if (profile.Values.TryGetValue(key, out var profileValue))
        {
            return ToBoolean(profileValue, fallback);
        }

        if (config.GlobalValues.TryGetValue(key, out var globalValue))
        {
            return ToBoolean(globalValue, fallback);
        }

        if (config.GlobalValues.TryGetValue($"GUI.{key}", out var guiValue))
        {
            return ToBoolean(guiValue, fallback);
        }

        return fallback;
    }

    private static bool ToBoolean(JsonNode? node, bool fallback)
    {
        if (node is not JsonValue value)
        {
            return fallback;
        }

        if (value.TryGetValue(out bool parsedBool))
        {
            return parsedBool;
        }

        if (value.TryGetValue(out int parsedInt))
        {
            return parsedInt != 0;
        }

        if (value.TryGetValue(out string? text) && bool.TryParse(text, out var parsedText))
        {
            return parsedText;
        }

        return fallback;
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
