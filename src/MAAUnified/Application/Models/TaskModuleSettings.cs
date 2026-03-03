using System.Text.Json;
using System.Text.Json.Nodes;

namespace MAAUnified.Application.Models;

public static class TaskModuleTypes
{
    public const string StartUp = "StartUp";
    public const string Fight = "Fight";
    public const string Recruit = "Recruit";
    public const string Infrast = "Infrast";
    public const string Mall = "Mall";
    public const string Award = "Award";
    public const string Roguelike = "Roguelike";
    public const string Reclamation = "Reclamation";
    public const string Custom = "Custom";
    public const string PostAction = "PostAction";

    public static string Normalize(string? type)
    {
        if (string.IsNullOrWhiteSpace(type))
        {
            return "Unknown";
        }

        var value = type.Trim();
        var lastDot = value.LastIndexOf('.');
        if (lastDot >= 0)
        {
            value = value[(lastDot + 1)..];
        }

        if (value.EndsWith("Task", StringComparison.OrdinalIgnoreCase))
        {
            value = value[..^4];
        }

        return value;
    }
}

public static class TaskModuleParameterDefaults
{
    public static JsonObject Create(string taskType, string language = "zh-cn")
    {
        var normalized = TaskModuleTypes.Normalize(taskType);
        return normalized switch
        {
            TaskModuleTypes.Infrast => CreateInfrastDefaults(),
            TaskModuleTypes.Mall => CreateMallDefaults(language),
            TaskModuleTypes.Award => CreateAwardDefaults(),
            _ => new JsonObject(),
        };
    }

    public static JsonObject CreateInfrastDefaults()
    {
        return new JsonObject
        {
            ["mode"] = 0,
            ["facility"] = new JsonArray("Mfg", "Trade", "Power", "Control", "Reception", "Office", "Dorm", "Processing", "Training"),
            ["drones"] = "Money",
            ["continue_training"] = false,
            ["threshold"] = 0.3,
            ["dorm_notstationed_enabled"] = true,
            ["dorm_trust_enabled"] = true,
            ["replenish"] = true,
            ["reception_message_board"] = true,
            ["reception_clue_exchange"] = true,
            ["reception_send_clue"] = true,
            ["filename"] = string.Empty,
            ["plan_index"] = -1,
        };
    }

    public static JsonObject CreateMallDefaults(string language)
    {
        var first = new JsonArray();
        var blacklist = new JsonArray();
        if (string.Equals(language, "en-us", StringComparison.OrdinalIgnoreCase))
        {
            first.Add("Recruitment Permit");
            blacklist.Add("Carbon");
            blacklist.Add("Furniture Part");
            blacklist.Add("Expedited Plan");
        }
        else
        {
            first.Add("招聘许可");
            blacklist.Add("碳");
            blacklist.Add("家具");
            blacklist.Add("加急许可");
        }

        return new JsonObject
        {
            ["credit_fight"] = false,
            ["credit_fight_once_a_day"] = true,
            ["formation_index"] = 0,
            ["visit_friends"] = true,
            ["visit_friends_once_a_day"] = false,
            ["shopping"] = true,
            ["buy_first"] = first,
            ["blacklist"] = blacklist,
            ["force_shopping_if_credit_full"] = false,
            ["only_buy_discount"] = false,
            ["reserve_max_credit"] = false,
        };
    }

    public static JsonObject CreateAwardDefaults()
    {
        return new JsonObject
        {
            ["award"] = true,
            ["mail"] = false,
            ["recruit"] = false,
            ["orundum"] = false,
            ["mining"] = false,
            ["specialaccess"] = false,
        };
    }
}

public sealed class InfrastParams
{
    public int Mode { get; set; }

    public List<string> Facility { get; set; } = [];

    public string Drones { get; set; } = "Money";

    public bool ContinueTraining { get; set; }

    public double Threshold { get; set; } = 0.3;

    public bool DormNotStationedEnabled { get; set; } = true;

    public bool DormTrustEnabled { get; set; } = true;

    public bool Replenish { get; set; } = true;

    public bool ReceptionMessageBoard { get; set; } = true;

    public bool ReceptionClueExchange { get; set; } = true;

    public bool ReceptionSendClue { get; set; } = true;

    public string Filename { get; set; } = string.Empty;

    public int PlanIndex { get; set; } = -1;

    public static InfrastParams FromJson(JsonObject? obj)
    {
        obj ??= TaskModuleParameterDefaults.CreateInfrastDefaults();
        var result = new InfrastParams
        {
            Mode = ReadInt(obj, "mode", 0),
            Drones = ReadString(obj, "drones", "Money"),
            ContinueTraining = ReadBool(obj, "continue_training", false),
            Threshold = ReadDouble(obj, "threshold", 0.3),
            DormNotStationedEnabled = ReadBool(obj, "dorm_notstationed_enabled", true),
            DormTrustEnabled = ReadBool(obj, "dorm_trust_enabled", true),
            Replenish = ReadBool(obj, "replenish", true),
            ReceptionMessageBoard = ReadBool(obj, "reception_message_board", true),
            ReceptionClueExchange = ReadBool(obj, "reception_clue_exchange", true),
            ReceptionSendClue = ReadBool(obj, "reception_send_clue", true),
            Filename = ReadString(obj, "filename", string.Empty),
            PlanIndex = ReadInt(obj, "plan_index", -1),
        };

        if (obj["facility"] is JsonArray facilityArray)
        {
            foreach (var node in facilityArray)
            {
                var value = node?.GetValue<string?>();
                if (!string.IsNullOrWhiteSpace(value))
                {
                    result.Facility.Add(value.Trim());
                }
            }
        }

        if (result.Facility.Count == 0)
        {
            result.Facility.AddRange(new[]
            {
                "Mfg", "Trade", "Power", "Control", "Reception", "Office", "Dorm", "Processing", "Training",
            });
        }

        return result;
    }

    public JsonObject ToJson()
    {
        var facility = new JsonArray();
        foreach (var name in Facility.Where(n => !string.IsNullOrWhiteSpace(n)))
        {
            facility.Add(name.Trim());
        }

        return new JsonObject
        {
            ["mode"] = Mode,
            ["facility"] = facility,
            ["drones"] = Drones,
            ["continue_training"] = ContinueTraining,
            ["threshold"] = Math.Clamp(Threshold, 0, 1),
            ["dorm_notstationed_enabled"] = DormNotStationedEnabled,
            ["dorm_trust_enabled"] = DormTrustEnabled,
            ["replenish"] = Replenish,
            ["reception_message_board"] = ReceptionMessageBoard,
            ["reception_clue_exchange"] = ReceptionClueExchange,
            ["reception_send_clue"] = ReceptionSendClue,
            ["filename"] = Filename,
            ["plan_index"] = PlanIndex,
        };
    }

    private static bool ReadBool(JsonObject obj, string key, bool fallback)
        => TryGetNode(obj, key, out var node) && node is not null && TryReadBool(node, out var value) ? value : fallback;

    private static int ReadInt(JsonObject obj, string key, int fallback)
        => TryGetNode(obj, key, out var node) && node is not null && TryReadInt(node, out var value) ? value : fallback;

    private static double ReadDouble(JsonObject obj, string key, double fallback)
        => TryGetNode(obj, key, out var node) && node is not null && TryReadDouble(node, out var value) ? value : fallback;

    private static string ReadString(JsonObject obj, string key, string fallback)
        => TryGetNode(obj, key, out var node) && node is not null && TryReadString(node, out var value) ? value : fallback;

    private static bool TryGetNode(JsonObject obj, string key, out JsonNode? value)
        => obj.TryGetPropertyValue(key, out value) && value is not null;

    private static bool TryReadBool(JsonNode node, out bool value)
    {
        value = false;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out bool b))
        {
            value = b;
            return true;
        }

        if (jsonValue.TryGetValue(out string? s) && bool.TryParse(s, out var parsed))
        {
            value = parsed;
            return true;
        }

        if (jsonValue.TryGetValue(out int i))
        {
            value = i != 0;
            return true;
        }

        return false;
    }

    private static bool TryReadInt(JsonNode node, out int value)
    {
        value = 0;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out int i))
        {
            value = i;
            return true;
        }

        if (jsonValue.TryGetValue(out string? s) && int.TryParse(s, out var parsed))
        {
            value = parsed;
            return true;
        }

        return false;
    }

    private static bool TryReadDouble(JsonNode node, out double value)
    {
        value = 0;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out double d))
        {
            value = d;
            return true;
        }

        if (jsonValue.TryGetValue(out int i))
        {
            value = i;
            return true;
        }

        if (jsonValue.TryGetValue(out string? s) && double.TryParse(s, out var parsed))
        {
            value = parsed;
            return true;
        }

        return false;
    }

    private static bool TryReadString(JsonNode node, out string value)
    {
        value = string.Empty;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out string? s))
        {
            value = s ?? string.Empty;
            return true;
        }

        try
        {
            value = node.ToJsonString();
            return true;
        }
        catch
        {
            return false;
        }
    }
}

public sealed class MallParams
{
    public bool CreditFight { get; set; }

    public bool CreditFightOnceADay { get; set; } = true;

    public int FormationIndex { get; set; }

    public bool VisitFriends { get; set; } = true;

    public bool VisitFriendsOnceADay { get; set; }

    public bool Shopping { get; set; } = true;

    public List<string> BuyFirst { get; set; } = [];

    public List<string> Blacklist { get; set; } = [];

    public bool ForceShoppingIfCreditFull { get; set; }

    public bool OnlyBuyDiscount { get; set; }

    public bool ReserveMaxCredit { get; set; }

    public static MallParams FromJson(JsonObject? obj)
    {
        obj ??= TaskModuleParameterDefaults.CreateMallDefaults("zh-cn");
        var result = new MallParams
        {
            CreditFight = ReadBool(obj, "credit_fight", false),
            CreditFightOnceADay = ReadBool(obj, "credit_fight_once_a_day", true),
            FormationIndex = ReadInt(obj, "formation_index", 0),
            VisitFriends = ReadBool(obj, "visit_friends", true),
            VisitFriendsOnceADay = ReadBool(obj, "visit_friends_once_a_day", false),
            Shopping = ReadBool(obj, "shopping", true),
            ForceShoppingIfCreditFull = ReadBool(obj, "force_shopping_if_credit_full", false),
            OnlyBuyDiscount = ReadBool(obj, "only_buy_discount", false),
            ReserveMaxCredit = ReadBool(obj, "reserve_max_credit", false),
        };

        result.BuyFirst.AddRange(ReadStringList(obj, "buy_first"));
        result.Blacklist.AddRange(ReadStringList(obj, "blacklist"));
        return result;
    }

    public JsonObject ToJson()
    {
        return new JsonObject
        {
            ["credit_fight"] = CreditFight,
            ["credit_fight_once_a_day"] = CreditFightOnceADay,
            ["formation_index"] = Math.Clamp(FormationIndex, 0, 4),
            ["visit_friends"] = VisitFriends,
            ["visit_friends_once_a_day"] = VisitFriendsOnceADay,
            ["shopping"] = Shopping,
            ["buy_first"] = ToJsonArray(BuyFirst),
            ["blacklist"] = ToJsonArray(Blacklist),
            ["force_shopping_if_credit_full"] = ForceShoppingIfCreditFull,
            ["only_buy_discount"] = OnlyBuyDiscount,
            ["reserve_max_credit"] = ReserveMaxCredit,
        };
    }

    private static bool ReadBool(JsonObject obj, string key, bool fallback)
        => obj.TryGetPropertyValue(key, out var node) && node is JsonValue v && (
            v.TryGetValue(out bool b) ? b :
            v.TryGetValue(out int i) ? i != 0 :
            v.TryGetValue(out string? s) && bool.TryParse(s, out var parsed) ? parsed : fallback);

    private static int ReadInt(JsonObject obj, string key, int fallback)
    {
        if (!obj.TryGetPropertyValue(key, out var node) || node is not JsonValue value)
        {
            return fallback;
        }

        if (value.TryGetValue(out int parsedInt))
        {
            return parsedInt;
        }

        if (value.TryGetValue(out string? parsedText) && int.TryParse(parsedText, out parsedInt))
        {
            return parsedInt;
        }

        return fallback;
    }

    private static List<string> ReadStringList(JsonObject obj, string key)
    {
        var result = new List<string>();
        if (!obj.TryGetPropertyValue(key, out var node) || node is not JsonArray array)
        {
            return result;
        }

        foreach (var item in array)
        {
            if (item is JsonValue value && value.TryGetValue(out string? text) && !string.IsNullOrWhiteSpace(text))
            {
                result.Add(text.Trim());
            }
        }

        return result;
    }

    private static JsonArray ToJsonArray(IEnumerable<string> values)
    {
        var result = new JsonArray();
        foreach (var value in values.Where(v => !string.IsNullOrWhiteSpace(v)))
        {
            result.Add(value.Trim());
        }

        return result;
    }
}

public sealed class AwardParams
{
    public bool Award { get; set; } = true;

    public bool Mail { get; set; }

    public bool Recruit { get; set; }

    public bool Orundum { get; set; }

    public bool Mining { get; set; }

    public bool SpecialAccess { get; set; }

    public static AwardParams FromJson(JsonObject? obj)
    {
        obj ??= TaskModuleParameterDefaults.CreateAwardDefaults();
        return new AwardParams
        {
            Award = ReadBool(obj, "award", true),
            Mail = ReadBool(obj, "mail", false),
            Recruit = ReadBool(obj, "recruit", false),
            Orundum = ReadBool(obj, "orundum", false),
            Mining = ReadBool(obj, "mining", false),
            SpecialAccess = ReadBool(obj, "specialaccess", false),
        };
    }

    public JsonObject ToJson()
    {
        return new JsonObject
        {
            ["award"] = Award,
            ["mail"] = Mail,
            ["recruit"] = Recruit,
            ["orundum"] = Orundum,
            ["mining"] = Mining,
            ["specialaccess"] = SpecialAccess,
        };
    }

    private static bool ReadBool(JsonObject obj, string key, bool fallback)
        => obj.TryGetPropertyValue(key, out var node) && node is JsonValue v && (
            v.TryGetValue(out bool b) ? b :
            v.TryGetValue(out int i) ? i != 0 :
            v.TryGetValue(out string? s) && bool.TryParse(s, out var parsed) ? parsed : fallback);
}

public sealed class PostActionConfig
{
    public bool Once { get; set; }

    public bool ExitArknights { get; set; }

    public bool BackToAndroidHome { get; set; }

    public bool ExitEmulator { get; set; }

    public bool ExitSelf { get; set; }

    public bool IfNoOtherMaa { get; set; }

    public bool Hibernate { get; set; }

    public bool Shutdown { get; set; }

    public bool Sleep { get; set; }

    public PostActionCommandConfig Commands { get; set; } = PostActionCommandConfig.Default;

    public static PostActionConfig Default => new();

    public static PostActionConfig FromJson(JsonNode? node)
    {
        if (node is not JsonObject obj)
        {
            return Default;
        }

        return new PostActionConfig
        {
            Once = ReadBool(obj, "once", false),
            ExitArknights = ReadBool(obj, "exit_arknights", false),
            BackToAndroidHome = ReadBool(obj, "back_to_android_home", false),
            ExitEmulator = ReadBool(obj, "exit_emulator", false),
            ExitSelf = ReadBool(obj, "exit_self", false),
            IfNoOtherMaa = ReadBool(obj, "if_no_other_maa", false),
            Hibernate = ReadBool(obj, "hibernate", false),
            Shutdown = ReadBool(obj, "shutdown", false),
            Sleep = ReadBool(obj, "sleep", false),
            Commands = PostActionCommandConfig.FromJson(obj["commands"]),
        };
    }

    public JsonObject ToJson()
    {
        return new JsonObject
        {
            ["once"] = Once,
            ["exit_arknights"] = ExitArknights,
            ["back_to_android_home"] = BackToAndroidHome,
            ["exit_emulator"] = ExitEmulator,
            ["exit_self"] = ExitSelf,
            ["if_no_other_maa"] = IfNoOtherMaa,
            ["hibernate"] = Hibernate,
            ["shutdown"] = Shutdown,
            ["sleep"] = Sleep,
            ["commands"] = Commands.ToJson(),
        };
    }

    public bool HasAnyAction()
    {
        return ExitArknights || BackToAndroidHome || ExitEmulator || ExitSelf || Hibernate || Shutdown || Sleep;
    }

    public PostActionConfig Clone()
    {
        return new PostActionConfig
        {
            Once = Once,
            ExitArknights = ExitArknights,
            BackToAndroidHome = BackToAndroidHome,
            ExitEmulator = ExitEmulator,
            ExitSelf = ExitSelf,
            IfNoOtherMaa = IfNoOtherMaa,
            Hibernate = Hibernate,
            Shutdown = Shutdown,
            Sleep = Sleep,
            Commands = Commands.Clone(),
        };
    }

    private static bool ReadBool(JsonObject obj, string key, bool fallback)
        => obj.TryGetPropertyValue(key, out var node) && node is JsonValue v && (
            v.TryGetValue(out bool b) ? b :
            v.TryGetValue(out int i) ? i != 0 :
            v.TryGetValue(out string? s) && bool.TryParse(s, out var parsed) ? parsed : fallback);
}

public sealed class PostActionCommandConfig
{
    public string ExitArknights { get; set; } = string.Empty;

    public string BackToAndroidHome { get; set; } = string.Empty;

    public string ExitEmulator { get; set; } = string.Empty;

    public string ExitSelf { get; set; } = string.Empty;

    public static PostActionCommandConfig Default => new();

    public static PostActionCommandConfig FromJson(JsonNode? node)
    {
        if (node is not JsonObject obj)
        {
            return Default;
        }

        return new PostActionCommandConfig
        {
            ExitArknights = ReadString(obj, "exit_arknights"),
            BackToAndroidHome = ReadString(obj, "back_to_android_home"),
            ExitEmulator = ReadString(obj, "exit_emulator"),
            ExitSelf = ReadString(obj, "exit_self"),
        };
    }

    public JsonObject ToJson()
    {
        return new JsonObject
        {
            ["exit_arknights"] = ExitArknights,
            ["back_to_android_home"] = BackToAndroidHome,
            ["exit_emulator"] = ExitEmulator,
            ["exit_self"] = ExitSelf,
        };
    }

    public PostActionCommandConfig Clone()
    {
        return new PostActionCommandConfig
        {
            ExitArknights = ExitArknights,
            BackToAndroidHome = BackToAndroidHome,
            ExitEmulator = ExitEmulator,
            ExitSelf = ExitSelf,
        };
    }

    private static string ReadString(JsonObject obj, string key)
    {
        if (!obj.TryGetPropertyValue(key, out var node) || node is not JsonValue value)
        {
            return string.Empty;
        }

        return value.TryGetValue(out string? text)
            ? text ?? string.Empty
            : string.Empty;
    }
}

public sealed record PostActionPreview(
    bool HasBlockingError,
    IReadOnlyList<string> Warnings,
    IReadOnlyList<string> UnsupportedActions);

public sealed record PostActionExecutionContext(
    string TriggerMessage,
    bool WasSuccessfulTaskChain,
    string RunId = "-",
    int? TaskIndex = null);

public sealed record PostActionCapabilitySnapshot(
    bool ExitArknightsSupported,
    bool BackToAndroidHomeSupported,
    bool ExitEmulatorSupported,
    bool ExitSelfSupported,
    bool HibernateSupported,
    bool ShutdownSupported,
    bool SleepSupported);

public sealed record PostActionExecutionPlan(
    IReadOnlyList<string> PlannedActions,
    IReadOnlyList<string> SkippedActions,
    bool SkippedSystemActionsForOtherMaa);

public sealed record TaskRuntimeStatusSnapshot(
    string RunId,
    int? TaskIndex,
    string Module,
    string Action,
    string Status,
    string Message,
    DateTimeOffset Timestamp);
