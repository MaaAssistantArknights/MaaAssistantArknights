using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;

namespace MAAUnified.Application.Configuration;

internal static class LegacyTaskSchemaConverter
{
    private static readonly Dictionary<string, string> _legacyTypeMap = new(StringComparer.OrdinalIgnoreCase)
    {
        ["StartUpTask"] = "StartUp",
        ["CloseDownTask"] = "CloseDown",
        ["FightTask"] = "Fight",
        ["RecruitTask"] = "Recruit",
        ["InfrastTask"] = "Infrast",
        ["MallTask"] = "Mall",
        ["AwardTask"] = "Award",
        ["RoguelikeTask"] = "Roguelike",
        ["ReclamationTask"] = "Reclamation",
        ["CustomTask"] = "Custom",
        // Accept already migrated type names.
        ["StartUp"] = "StartUp",
        ["CloseDown"] = "CloseDown",
        ["Fight"] = "Fight",
        ["Recruit"] = "Recruit",
        ["Infrast"] = "Infrast",
        ["Mall"] = "Mall",
        ["Award"] = "Award",
        ["Roguelike"] = "Roguelike",
        ["Reclamation"] = "Reclamation",
        ["Custom"] = "Custom",
    };

    public static bool TryConvertLegacyTask(
        JsonObject legacyTask,
        UnifiedProfile profile,
        UnifiedConfig config,
        out UnifiedTaskItem taskItem,
        out string? error)
    {
        var rawLegacyType = GetString(legacyTask, "$type")
            ?? GetString(legacyTask, "Type")
            ?? "Unknown";
        var legacyType = NormalizeTypeName(rawLegacyType);

        var name = GetString(legacyTask, "Name")
            ?? GetString(legacyTask, "name")
            ?? legacyType;
        var isEnabled = GetBool(legacyTask, "IsEnable", true);

        if (!_legacyTypeMap.TryGetValue(legacyType, out var coreType))
        {
            taskItem = new UnifiedTaskItem
            {
                Type = legacyType,
                Name = name,
                IsEnabled = false,
                Params = [],
                LegacyRawTask = (JsonObject?)legacyTask.DeepClone(),
            };
            error = $"Unsupported legacy task type `{rawLegacyType}`.";
            return false;
        }

        JsonObject @params;
        try
        {
            @params = coreType switch
            {
                "StartUp" => ConvertStartUp(legacyTask, profile, config),
                "CloseDown" => ConvertCloseDown(profile, config),
                "Fight" => ConvertFight(legacyTask, profile, config),
                "Recruit" => ConvertRecruit(legacyTask, profile, config),
                "Infrast" => ConvertInfrast(legacyTask),
                "Mall" => ConvertMall(legacyTask),
                "Award" => ConvertAward(legacyTask),
                "Roguelike" => ConvertRoguelike(legacyTask),
                "Reclamation" => ConvertReclamation(legacyTask),
                "Custom" => ConvertCustom(legacyTask),
                _ => [],
            };
        }
        catch (Exception ex)
        {
            taskItem = new UnifiedTaskItem
            {
                Type = coreType,
                Name = name,
                IsEnabled = false,
                Params = [],
                LegacyRawTask = (JsonObject?)legacyTask.DeepClone(),
            };
            error = $"Failed to convert legacy task `{name}` ({rawLegacyType}): {ex.Message}";
            return false;
        }

        taskItem = new UnifiedTaskItem
        {
            Type = coreType,
            Name = name,
            IsEnabled = isEnabled,
            Params = @params,
            LegacyRawTask = null,
        };
        error = null;
        return true;
    }

    public static bool TryUpgradeTaskToSchemaV2(
        UnifiedTaskItem task,
        UnifiedProfile profile,
        UnifiedConfig config,
        out UnifiedTaskItem upgradedTask,
        out string? error)
    {
        var normalizedType = NormalizeTypeName(task.Type);

        if (task.LegacyRawTask is null && !_legacyTypeMap.ContainsKey(normalizedType))
        {
            upgradedTask = CloneTask(task);
            error = null;
            return true;
        }

        if (task.LegacyRawTask is null && _legacyTypeMap.TryGetValue(normalizedType, out var alreadyCoreType))
        {
            upgradedTask = CloneTask(task);
            upgradedTask.Type = alreadyCoreType;
            upgradedTask.LegacyRawTask = null;
            error = null;
            return true;
        }

        var legacy = task.LegacyRawTask?.DeepClone() as JsonObject;
        if (legacy is null)
        {
            upgradedTask = CloneTask(task);
            upgradedTask.IsEnabled = false;
            upgradedTask.Params = [];
            error = $"Task `{task.Name}` cannot be migrated because no legacy payload was found.";
            return false;
        }

        if (string.IsNullOrWhiteSpace(GetString(legacy, "$type")))
        {
            legacy["$type"] = task.Type;
        }

        if (string.IsNullOrWhiteSpace(GetString(legacy, "Name")))
        {
            legacy["Name"] = task.Name;
        }

        legacy["IsEnable"] = task.IsEnabled;

        var converted = TryConvertLegacyTask(legacy, profile, config, out upgradedTask, out error);
        if (!converted)
        {
            upgradedTask.IsEnabled = false;
        }

        return converted;
    }

    private static UnifiedTaskItem CloneTask(UnifiedTaskItem task)
    {
        return new UnifiedTaskItem
        {
            Type = task.Type,
            Name = task.Name,
            IsEnabled = task.IsEnabled,
            Params = (JsonObject)(task.Params.DeepClone() ?? new JsonObject()),
            LegacyRawTask = (JsonObject?)task.LegacyRawTask?.DeepClone(),
        };
    }

    private static JsonObject ConvertStartUp(JsonObject task, UnifiedProfile profile, UnifiedConfig config)
    {
        var clientType = ResolveClientType(profile, config);
        var accountName = GetString(task, "AccountName") ?? string.Empty;
        var startGameEnabled = ResolveBooleanSetting(profile, config, "StartGame");
        return new JsonObject
        {
            ["client_type"] = clientType,
            ["start_game_enabled"] = startGameEnabled,
            ["account_name"] = accountName,
        };
    }

    private static JsonObject ConvertCloseDown(UnifiedProfile profile, UnifiedConfig config)
    {
        return new JsonObject
        {
            ["client_type"] = ResolveClientType(profile, config),
        };
    }

    private static JsonObject ConvertFight(JsonObject task, UnifiedProfile profile, UnifiedConfig config)
    {
        var stage = FightStageSelection.NormalizeStoredValue(ResolveFightStage(task));
        var useMedicine = GetBool(task, "UseMedicine", false);
        var useStone = GetBool(task, "UseStone", false);
        var useExpiringMedicine = GetBool(task, "UseExpiringMedicine", false);
        var enableTimesLimit = GetBool(task, "EnableTimesLimit", false);
        var enableTargetDrop = GetBool(task, "EnableTargetDrop", false);
        var useCustomAnnihilation = GetBool(task, "UseCustomAnnihilation", false);
        var annihilationStage = GetString(task, "AnnihilationStage") ?? string.Empty;

        if (string.Equals(stage, "Annihilation", StringComparison.OrdinalIgnoreCase)
            && useCustomAnnihilation
            && !string.IsNullOrWhiteSpace(annihilationStage))
        {
            stage = annihilationStage;
        }

        var result = new JsonObject
        {
            ["stage"] = stage,
            ["medicine"] = useMedicine ? GetInt(task, "MedicineCount", 0) : 0,
            ["expiring_medicine"] = useExpiringMedicine ? 9999 : 0,
            ["stone"] = useStone ? GetInt(task, "StoneCount", 0) : 0,
            ["times"] = enableTimesLimit ? GetInt(task, "TimesLimit", int.MaxValue) : int.MaxValue,
            ["series"] = Math.Max(1, GetInt(task, "Series", 1)),
            ["DrGrandet"] = GetBool(task, "IsDrGrandet", false),
            ["report_to_penguin"] = ResolveBooleanSetting(profile, config, "EnablePenguin"),
            ["report_to_yituliu"] = ResolveBooleanSetting(profile, config, "EnableYituliu"),
            ["penguin_id"] = ResolveStringSetting(profile, config, "PenguinId"),
            ["yituliu_id"] = ResolveStringSetting(profile, config, "YituliuId"),
            ["server"] = ResolveServerType(profile, config),
            ["client_type"] = ResolveClientType(profile, config),
        };

        if (enableTargetDrop)
        {
            var dropId = GetString(task, "DropId");
            if (!string.IsNullOrWhiteSpace(dropId))
            {
                var drops = new JsonObject
                {
                    [dropId] = GetInt(task, "DropCount", 1),
                };
                result["drops"] = drops;
            }
        }

        return result;
    }

    private static JsonObject ConvertRecruit(JsonObject task, UnifiedProfile profile, UnifiedConfig config)
    {
        var maxTimes = GetInt(task, "MaxTimes", 4);
        var level1NotChoose = GetBool(task, "Level1NotChoose", true);
        var level3Choose = GetBool(task, "Level3Choose", true);
        var level4Choose = GetBool(task, "Level4Choose", true);
        var level5Choose = GetBool(task, "Level5Choose", false);
        var useExpedited = GetBool(task, "UseExpedited", true);

        var selectList = new JsonArray();
        var confirmList = new JsonArray();

        if (level1NotChoose)
        {
            confirmList.Add(1);
        }

        if (level3Choose)
        {
            confirmList.Add(3);
        }

        if (level4Choose)
        {
            selectList.Add(4);
            confirmList.Add(4);
        }

        if (level5Choose)
        {
            selectList.Add(5);
            confirmList.Add(5);
        }

        var firstTags = new JsonArray();
        if (task["Level3PreferTags"] is JsonArray tagsArray)
        {
            foreach (var tag in tagsArray)
            {
                if (tag is null)
                {
                    continue;
                }

                var tagText = tag.GetValue<string?>();
                if (!string.IsNullOrWhiteSpace(tagText))
                {
                    firstTags.Add(tagText.Trim());
                }
            }
        }

        var result = new JsonObject
        {
            ["refresh"] = GetBool(task, "RefreshLevel3", true),
            ["force_refresh"] = GetBool(task, "ForceRefresh", true),
            ["select"] = selectList,
            ["confirm"] = confirmList,
            ["times"] = maxTimes,
            ["set_time"] = true,
            ["expedite"] = useExpedited,
            ["skip_robot"] = level1NotChoose,
            ["extra_tags_mode"] = GetInt(task, "ExtraTagMode", 0),
            ["first_tags"] = firstTags,
            ["recruitment_time"] = new JsonObject
            {
                ["3"] = GetInt(task, "Level3Time", 540),
                ["4"] = GetInt(task, "Level4Time", 540),
                ["5"] = GetInt(task, "Level5Time", 540),
            },
            ["report_to_penguin"] = ResolveBooleanSetting(profile, config, "EnablePenguin"),
            ["report_to_yituliu"] = ResolveBooleanSetting(profile, config, "EnableYituliu"),
            ["penguin_id"] = ResolveStringSetting(profile, config, "PenguinId"),
            ["yituliu_id"] = ResolveStringSetting(profile, config, "YituliuId"),
            ["server"] = ResolveServerType(profile, config),
        };

        if (useExpedited)
        {
            result["expedite_times"] = maxTimes;
        }

        return result;
    }

    private static JsonObject ConvertInfrast(JsonObject task)
    {
        var facilities = new JsonArray();
        if (task["RoomList"] is JsonArray roomList)
        {
            foreach (var roomNode in roomList)
            {
                if (roomNode is not JsonObject room)
                {
                    continue;
                }

                if (!GetBool(room, "IsEnabled", true))
                {
                    continue;
                }

                var roomName = GetString(room, "Room");
                if (!string.IsNullOrWhiteSpace(roomName))
                {
                    facilities.Add(roomName);
                }
            }
        }

        var mode = GetInt(task, "Mode", 0);
        var result = new JsonObject
        {
            ["facility"] = facilities,
            ["drones"] = GetString(task, "UsesOfDrones") ?? "_NotUse",
            ["continue_training"] = GetBool(task, "ContinueTraining", false),
            ["threshold"] = GetInt(task, "DormThreshold", 30) / 100.0,
            ["dorm_notstationed_enabled"] = GetBool(task, "DormFilterNotStationed", true),
            ["dorm_trust_enabled"] = GetBool(task, "DormTrustEnabled", true),
            ["replenish"] = GetBool(task, "OriginiumShardAutoReplenishment", true),
            ["reception_message_board"] = GetBool(task, "ReceptionMessageBoard", true),
            ["reception_clue_exchange"] = GetBool(task, "ReceptionClueExchange", true),
            ["reception_send_clue"] = GetBool(task, "SendClue", true),
            ["mode"] = mode,
        };

        if (mode == 10000)
        {
            result["filename"] = GetString(task, "Filename") ?? string.Empty;
            result["plan_index"] = Math.Max(0, GetInt(task, "PlanSelect", 0));
        }

        return result;
    }

    private static JsonObject ConvertMall(JsonObject task)
    {
        return new JsonObject
        {
            ["credit_fight"] = GetBool(task, "CreditFight", false),
            ["credit_fight_once_a_day"] = GetBool(task, "CreditFightOnceADay", true),
            ["_ui_mall_credit_fight_last_time"] = GetString(task, "CreditFightLastTime") ?? string.Empty,
            ["formation_index"] = GetInt(task, "CreditFightFormation", 0),
            ["visit_friends"] = GetBool(task, "VisitFriends", true),
            ["visit_friends_once_a_day"] = GetBool(task, "VisitFriendsOnceADay", false),
            ["_ui_mall_visit_friends_last_time"] = GetString(task, "VisitFriendsLastTime") ?? string.Empty,
            ["shopping"] = GetBool(task, "Shopping", true),
            ["buy_first"] = ToJsonArray(SplitNonEmpty(GetString(task, "FirstList"), ';')),
            ["blacklist"] = ToJsonArray(SplitNonEmpty(GetString(task, "BlackList"), ';')),
            ["force_shopping_if_credit_full"] = GetBool(task, "ShoppingIgnoreBlackListWhenFull", false),
            ["only_buy_discount"] = GetBool(task, "OnlyBuyDiscount", false),
            ["reserve_max_credit"] = GetBool(task, "ReserveMaxCredit", false),
        };
    }

    private static JsonObject ConvertAward(JsonObject task)
    {
        return new JsonObject
        {
            ["award"] = GetBool(task, "Award", true),
            ["mail"] = GetBool(task, "Mail", false),
            ["recruit"] = GetBool(task, "FreeGacha", false),
            ["orundum"] = GetBool(task, "Orundum", false),
            ["mining"] = GetBool(task, "Mining", false),
            ["specialaccess"] = GetBool(task, "SpecialAccess", false),
        };
    }

    private static JsonObject ConvertRoguelike(JsonObject task)
    {
        var mode = GetInt(task, "Mode", 0);
        var theme = ResolveRoguelikeTheme(task["Theme"]);
        var collectibleAwardsMask = GetInt(task, "CollectibleStartAwards", 0);

        var result = new JsonObject
        {
            ["mode"] = mode,
            ["theme"] = theme,
            ["difficulty"] = GetInt(task, "Difficulty", int.MaxValue),
            ["starts_count"] = GetInt(task, "StartCount", 999999),
            ["investment_enabled"] = GetBool(task, "Investment", true),
            ["use_support"] = GetBool(task, "UseSupport", false),
            ["use_nonfriend_support"] = GetBool(task, "UseSupportNonFriend", false),
            ["refresh_trader_with_dice"] = string.Equals(theme, "Mizuki", StringComparison.OrdinalIgnoreCase)
                && GetBool(task, "RefreshTraderWithDice", false),
        };

        if (GetBool(task, "Investment", true))
        {
            result["investment_with_more_score"] = GetBool(task, "InvestWithMoreScore", false) && mode == 1;
            result["investments_count"] = GetInt(task, "InvestCount", 999);
            result["stop_when_investment_full"] = GetBool(task, "StopWhenDepositFull", false);
        }

        var squad = GetString(task, "Squad");
        if (!string.IsNullOrWhiteSpace(squad))
        {
            result["squad"] = squad;
        }

        var roles = GetString(task, "Roles");
        if (!string.IsNullOrWhiteSpace(roles))
        {
            result["roles"] = roles;
        }

        var coreChar = GetString(task, "CoreChar");
        if (!string.IsNullOrWhiteSpace(coreChar))
        {
            result["core_char"] = coreChar;
        }

        if (mode == 0)
        {
            result["stop_at_final_boss"] = GetBool(task, "StopAtFinalBoss", false);
            result["stop_at_max_level"] = GetBool(task, "StopWhenLevelMax", false);
        }

        if (mode == 4)
        {
            result["collectible_mode_shopping"] = GetBool(task, "CollectibleShopping", false);
            result["collectible_mode_squad"] = GetString(task, "SquadCollectible") ?? string.Empty;
            result["start_with_elite_two"] = GetBool(task, "StartWithEliteTwo", false);
            result["only_start_with_elite_two"] = GetBool(task, "StartWithEliteTwoOnly", false);
            result["collectible_mode_start_list"] = BuildCollectibleStartList(collectibleAwardsMask);
        }

        if (mode == 6)
        {
            result["monthly_squad_auto_iterate"] = GetBool(task, "MonthlySquadAutoIterate", true);
            result["monthly_squad_check_comms"] = GetBool(task, "MonthlySquadCheckComms", true);
        }

        if (mode == 7)
        {
            result["deep_exploration_auto_iterate"] = GetBool(task, "DeepExplorationAutoIterate", true);
        }

        if (mode == 20001)
        {
            result["find_playTime_target"] = GetInt(task, "FindPlaytimeTarget", 1);
        }

        if (GetBool(task, "SamiFirstFloorFoldartal", false))
        {
            var firstFloorFoldartal = GetString(task, "SamiFirstFloorFoldartals");
            if (!string.IsNullOrWhiteSpace(firstFloorFoldartal))
            {
                result["first_floor_foldartal"] = firstFloorFoldartal;
            }
        }

        if (GetBool(task, "SamiNewSquad2StartingFoldartal", false))
        {
            var foldartalList = SplitNonEmpty(GetString(task, "SamiNewSquad2StartingFoldartals"), ';')
                .Take(3);
            result["start_foldartal_list"] = ToJsonArray(foldartalList);
        }

        if (mode == 5)
        {
            var paradigms = SplitNonEmpty(GetString(task, "ExpectedCollapsalParadigms"), ';');
            result["expected_collapsal_paradigms"] = ToJsonArray(paradigms);
        }

        if (GetBool(task, "StartWithSeed", false))
        {
            var seed = GetString(task, "Seed");
            if (!string.IsNullOrWhiteSpace(seed))
            {
                result["start_with_seed"] = seed;
            }
        }

        return result;
    }

    private static JsonObject ConvertReclamation(JsonObject task)
    {
        return new JsonObject
        {
            ["theme"] = ResolveReclamationTheme(task["Theme"]),
            ["mode"] = GetInt(task, "Mode", 1),
            ["increment_mode"] = GetInt(task, "IncrementMode", 0),
            ["num_craft_batches"] = GetInt(task, "MaxCraftCountPerRound", 16),
            ["tools_to_craft"] = ToJsonArray(SplitNonEmpty(GetString(task, "ToolToCraft"), ';')),
            ["clear_store"] = GetBool(task, "ClearStore", true),
        };
    }

    private static JsonObject ConvertCustom(JsonObject task)
    {
        return new JsonObject
        {
            ["task_names"] = ToJsonArray(SplitNonEmpty(GetString(task, "CustomTaskName"), ',')),
        };
    }

    private static JsonObject BuildCollectibleStartList(int mask)
    {
        return new JsonObject
        {
            ["hot_water"] = HasFlag(mask, 1 << 0),
            ["shield"] = HasFlag(mask, 1 << 1),
            ["ingot"] = HasFlag(mask, 1 << 2),
            ["hope"] = HasFlag(mask, 1 << 3),
            ["random"] = HasFlag(mask, 1 << 4),
            ["key"] = HasFlag(mask, 1 << 5),
            ["dice"] = HasFlag(mask, 1 << 6),
            ["ideas"] = HasFlag(mask, 1 << 7),
            ["ticket"] = HasFlag(mask, 1 << 8),
        };
    }

    private static bool HasFlag(int value, int flag) => (value & flag) == flag;

    private static string ResolveFightStage(JsonObject task)
    {
        if (task["StagePlan"] is JsonArray stagePlan)
        {
            foreach (var stageNode in stagePlan)
            {
                var stage = stageNode?.GetValue<string?>();
                if (!string.IsNullOrWhiteSpace(stage))
                {
                    return stage.Trim();
                }
            }
        }

        return string.Empty;
    }

    private static string ResolveRoguelikeTheme(JsonNode? themeNode)
    {
        if (themeNode is JsonValue value)
        {
            if (value.TryGetValue(out string? themeText) && !string.IsNullOrWhiteSpace(themeText))
            {
                return themeText;
            }

            if (value.TryGetValue(out int themeNumber))
            {
                return themeNumber switch
                {
                    0 => "Phantom",
                    1 => "Mizuki",
                    2 => "Sami",
                    3 => "Sarkaz",
                    4 => "JieGarden",
                    _ => "JieGarden",
                };
            }
        }

        return "JieGarden";
    }

    private static string ResolveReclamationTheme(JsonNode? themeNode)
    {
        if (themeNode is JsonValue value)
        {
            if (value.TryGetValue(out string? themeText) && !string.IsNullOrWhiteSpace(themeText))
            {
                return themeText;
            }

            if (value.TryGetValue(out int themeNumber))
            {
                return themeNumber == 0 ? "Fire" : "Tales";
            }
        }

        return "Tales";
    }

    private static string ResolveClientType(UnifiedProfile profile, UnifiedConfig config)
    {
        return ResolveStringSetting(profile, config, "ClientType")
            ?? ResolveStringSetting(profile, config, "GameSettings.ClientType")
            ?? "Official";
    }

    private static string ResolveServerType(UnifiedProfile profile, UnifiedConfig config)
    {
        return ResolveStringSetting(profile, config, "ServerType")
            ?? ResolveStringSetting(profile, config, "Server")
            ?? "CN";
    }

    private static string? ResolveStringSetting(UnifiedProfile profile, UnifiedConfig config, string key)
    {
        if (profile.Values.TryGetValue(key, out var profileValue))
        {
            if (profileValue is JsonValue value && value.TryGetValue(out string? text))
            {
                return text;
            }
        }

        if (config.GlobalValues.TryGetValue(key, out var globalValue))
        {
            if (globalValue is JsonValue value && value.TryGetValue(out string? text))
            {
                return text;
            }
        }

        if (config.GlobalValues.TryGetValue($"GUI.{key}", out var guiGlobal))
        {
            if (guiGlobal is JsonValue value && value.TryGetValue(out string? text))
            {
                return text;
            }
        }

        return null;
    }

    private static bool ResolveBooleanSetting(UnifiedProfile profile, UnifiedConfig config, string key)
    {
        if (profile.Values.TryGetValue(key, out var profileValue))
        {
            return ToBoolean(profileValue, false);
        }

        if (config.GlobalValues.TryGetValue(key, out var globalValue))
        {
            return ToBoolean(globalValue, false);
        }

        if (config.GlobalValues.TryGetValue($"GUI.{key}", out var guiGlobal))
        {
            return ToBoolean(guiGlobal, false);
        }

        return false;
    }

    private static bool ToBoolean(JsonNode? node, bool fallback)
    {
        if (node is JsonValue value)
        {
            if (value.TryGetValue(out bool b))
            {
                return b;
            }

            if (value.TryGetValue(out string? s) && bool.TryParse(s, out var parsed))
            {
                return parsed;
            }

            if (value.TryGetValue(out int i))
            {
                return i != 0;
            }
        }

        return fallback;
    }

    private static string NormalizeTypeName(string type)
    {
        if (string.IsNullOrWhiteSpace(type))
        {
            return "Unknown";
        }

        var withoutAssembly = type.Split(',')[0].Trim();
        var lastDot = withoutAssembly.LastIndexOf('.');
        return lastDot >= 0 ? withoutAssembly[(lastDot + 1)..] : withoutAssembly;
    }

    private static string? GetString(JsonObject obj, string key)
    {
        if (!obj.TryGetPropertyValue(key, out var value) || value is null)
        {
            return null;
        }

        return value switch
        {
            JsonValue v when v.TryGetValue(out string? text) => text,
            JsonValue v => v.ToJsonString(),
            _ => value.ToJsonString(),
        };
    }

    private static bool GetBool(JsonObject obj, string key, bool fallback)
    {
        if (!obj.TryGetPropertyValue(key, out var value) || value is null)
        {
            return fallback;
        }

        if (value is JsonValue jsonValue)
        {
            if (jsonValue.TryGetValue(out bool b))
            {
                return b;
            }

            if (jsonValue.TryGetValue(out string? text) && bool.TryParse(text, out var parsed))
            {
                return parsed;
            }

            if (jsonValue.TryGetValue(out int i))
            {
                return i != 0;
            }
        }

        return fallback;
    }

    private static int GetInt(JsonObject obj, string key, int fallback)
    {
        if (!obj.TryGetPropertyValue(key, out var value) || value is null)
        {
            return fallback;
        }

        if (value is JsonValue jsonValue)
        {
            if (jsonValue.TryGetValue(out int number))
            {
                return number;
            }

            if (jsonValue.TryGetValue(out long number64))
            {
                return (int)Math.Clamp(number64, int.MinValue, int.MaxValue);
            }

            if (jsonValue.TryGetValue(out string? text) && int.TryParse(text, out var parsed))
            {
                return parsed;
            }
        }

        return fallback;
    }

    private static IEnumerable<string> SplitNonEmpty(string? input, char separator)
    {
        if (string.IsNullOrWhiteSpace(input))
        {
            return [];
        }

        return input
            .Split(separator, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Where(s => !string.IsNullOrWhiteSpace(s));
    }

    private static JsonArray ToJsonArray(IEnumerable<string> values)
    {
        var array = new JsonArray();
        foreach (var value in values)
        {
            array.Add(value);
        }

        return array;
    }
}
