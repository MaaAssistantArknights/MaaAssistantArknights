// <copyright file="Program.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Globalization;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.DailyReport;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

var tests = new (string Name, Action Run)[]
{
    ("Two accounts on the same day retain separate identities and totals", TwoAccountsOnSameDay),
    ("Selection changes before the first drop do not relabel a running task", SelectionSnapshot),
    ("Renaming an account preserves its stable identity and earlier snapshot", RenameSnapshot),
    ("Multi-stage tasks inherit their original account and pending consumables", MultiStageSnapshot),
    ("Legacy JSON remains unassigned when new account data is appended", LegacyJson),
    ("Blank account IDs load as unassigned without a misleading name", BlankAccountIds),
    ("Unknown current account IDs do not assign another account", UnknownCurrentAccount),
    ("Historical accounts absent from configuration keep their identity", HistoricalOrphan),
    ("Default and explicitly cleared selections remain unassigned", NoAccountSelected),
    ("Save and load preserve account metadata, drops and consumables", StoreRoundTrip),
    ("Empty and non-combat tasks produce no account-only records", EmptyAndNonCombatTasks),
    ("Queue stop finalizes the original account and subsequent tasks use the new one", QueueStop),
    ("Account configuration round trips retain stable IDs independently of names", AccountConfigurationRoundTrip),
};

DailyReportCollector.Initialize(Instances.AsstProxy);
int failed = 0;
for (int index = 0; index < tests.Length; index++)
{
    DailyReportCollector.OnQueueStopped();
    DateTimeExtension.GameDay = new DateTime(2030, 1, 1).AddDays(index);
    Instances.AsstProxy.TasksStatus.Clear();
    SettingsViewModel.GuiSettings.DailyReportAccounts =
    [
        new() { Id = "primary", Name = "Main account" },
        new() { Id = "secondary", Name = "Second account" },
    ];
    SettingsViewModel.GuiSettings.DailyReportAccountId = null;
    try
    {
        tests[index].Run();
        Console.WriteLine("PASS " + tests[index].Name);
    }
    catch (Exception ex)
    {
        failed++;
        Console.Error.WriteLine("FAIL " + tests[index].Name + ": " + ex.Message);
    }
}

DailyReportCollector.OnQueueStopped();
Console.WriteLine($"{tests.Length - failed}/{tests.Length} tests passed.");

// Only this run's files exist in the newly generated temporary directory.
if (Directory.Exists(DailyReportStore.DirectoryPath))
{
    foreach (var file in Directory.EnumerateFiles(DailyReportStore.DirectoryPath))
    {
        File.Delete(file);
    }

    Directory.Delete(DailyReportStore.DirectoryPath);
    Directory.Delete(PathsHelper.DataDir);
}

return failed == 0 ? 0 : 1;

static void TwoAccountsOnSameDay()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    Drop(1, "1-7", 3, 18, 5);
    Finish(1);
    SettingsViewModel.GuiSettings.DailyReportAccountId = "secondary";
    Start(2);
    Drop(2, "1-7", 6, 36, 11);
    Finish(2);

    var sessions = ReadSessions();
    Equal(2, sessions.Count);
    Equal("primary", sessions[0].AccountId);
    Equal("Main account", sessions[0].AccountName);
    Equal(3, sessions[0].Times);
    Equal(18, sessions[0].Sanity);
    Equal(5, sessions[0].Drops["30012"]);
    Equal("secondary", sessions[1].AccountId);
    Equal("Second account", sessions[1].AccountName);
    Equal(6, sessions[1].Times);
    Equal(36, sessions[1].Sanity);
    Equal(11, sessions[1].Drops["30012"]);
}

static void SelectionSnapshot()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    SettingsViewModel.GuiSettings.DailyReportAccountId = "secondary";
    Drop(1, "1-7");
    Finish(1);
    Start(2);
    Drop(2, "1-7");
    Finish(2);

    var sessions = ReadSessions();
    Equal("primary", sessions[0].AccountId);
    Equal("secondary", sessions[1].AccountId);
}

static void RenameSnapshot()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    SettingsViewModel.GuiSettings.DailyReportAccounts[0].Name = "Renamed account";
    Drop(1, "1-7");
    Finish(1);
    Start(2);
    Drop(2, "1-7");
    Finish(2);

    var sessions = ReadSessions();
    Equal("primary", sessions[0].AccountId);
    Equal("primary", sessions[1].AccountId);
    Equal("Main account", sessions[0].AccountName);
    Equal("Renamed account", sessions[1].AccountName);
}

static void MultiStageSnapshot()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    Drop(1, "1-7");
    SettingsViewModel.GuiSettings.DailyReportAccountId = "secondary";
    Publish(1, "UseMedicine", new JObject { ["count"] = 2, ["is_expiring"] = true });
    Instances.AsstProxy.Publish(AsstMsg.SubTaskStart, new AsstSubTaskMsg
    {
        TaskId = 1,
        SubTask = "ProcessTask",
        Details = new JObject { ["task"] = "StoneConfirm" },
    });
    Drop(1, "S2-1", 2, 12, 7);
    Finish(1);

    var sessions = ReadSessions();
    Equal(2, sessions.Count);
    Equal("primary", sessions[0].AccountId);
    Equal("primary", sessions[1].AccountId);
    Equal("Main account", sessions[1].AccountName);
    Equal("1-7", sessions[0].Stage);
    Equal("S2-1", sessions[1].Stage);
    Equal(0, sessions[0].ExpiringMedicinesUsed);
    Equal(0, sessions[0].StonesUsed);
    Equal(2, sessions[1].ExpiringMedicinesUsed);
    Equal(1, sessions[1].StonesUsed);
    Equal(12, sessions[1].Sanity);
    Equal(7, sessions[1].Drops["30012"]);
    Check(sessions.All(s => s.EndTime is not null), "Both stage sessions must be finalized.");
}

static void LegacyJson()
{
    WriteJson("""
        {"version":1,"sessions":[{"stage":"1-7","startTime":"2026-11-10T01:00:00Z","endTime":null,"times":2,"rounds":1,"sanity":12,"medicinesUsed":0,"expiringMedicinesUsed":0,"stonesUsed":0,"drops":{"30012":4}}]}
        """);
    var legacy = ReadSessions().Single();
    Equal(null, legacy.AccountId);
    Equal(null, legacy.AccountName);
    Equal(null, legacy.EndTime);

    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    Drop(1, "1-7");
    Finish(1);
    var sessions = ReadSessions();
    Equal(2, sessions.Count);
    Equal(null, sessions[0].AccountId);
    Equal(4, sessions[0].Drops["30012"]);
    Equal("primary", sessions[1].AccountId);
}

static void BlankAccountIds()
{
    var data = new DailyReportDayData
    {
        Sessions = new string?[] { null, string.Empty, " \t " }
            .Select(id => new CombatSession { AccountId = id, AccountName = "Misleading name", Times = 1 })
            .ToList(),
    };
    WriteJson(JsonConvert.SerializeObject(data));
    var sessions = ReadSessions();
    Equal(3, sessions.Count);
    foreach (var session in sessions)
    {
        Equal(null, session.AccountId);
        Equal(null, session.AccountName);
    }
}

static void UnknownCurrentAccount()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "missing-id";
    Start(1);
    Drop(1, "1-7");
    Finish(1);
    var session = ReadSessions().Single();
    Equal(null, session.AccountId);
    Equal(null, session.AccountName);
}

static void HistoricalOrphan()
{
    DailyReportStore.Save(Today(), new DailyReportDayData
    {
        Sessions = [new() { AccountId = "removed-id", AccountName = "Old name", Times = 2 }],
    });
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    Drop(1, "1-7");
    Finish(1);
    var sessions = ReadSessions();
    Equal("removed-id", sessions[0].AccountId);
    Equal("Old name", sessions[0].AccountName);
    Equal("primary", sessions[1].AccountId);
}

static void NoAccountSelected()
{
    Start(1);
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Drop(1, "1-7");
    Finish(1);
    SettingsViewModel.GuiSettings.DailyReportAccountId = string.Empty;
    Start(2);
    Drop(2, "1-7");
    Finish(2);
    SettingsViewModel.GuiSettings.DailyReportAccountId = " \t ";
    Start(3);
    Drop(3, "1-7");
    Finish(3);
    var sessions = ReadSessions();
    Equal(3, sessions.Count);
    Check(sessions.All(s => s.AccountId is null && s.AccountName is null), "Unassigned tasks must remain unassigned.");
    var saved = JObject.Parse(File.ReadAllText(FilePath()));
    Check(saved["sessions"]!.All(s => s["accountId"] is null && s["accountName"] is null), "Unassigned JSON must omit optional account metadata.");
}

static void StoreRoundTrip()
{
    var start = new DateTimeOffset(2026, 11, 10, 1, 2, 3, TimeSpan.Zero);
    DailyReportStore.Save(Today(), new DailyReportDayData
    {
        Sessions =
        [
            new()
            {
                AccountId = "primary",
                AccountName = "大号 <&> / \\ Unicode",
                Stage = "1-7",
                StartTime = start,
                EndTime = start.AddMinutes(3),
                Times = 6,
                Rounds = 1,
                Sanity = 36,
                MedicinesUsed = 2,
                ExpiringMedicinesUsed = 1,
                StonesUsed = 3,
                Drops = new() { ["30012"] = 8, ["4001"] = 432 },
            },
        ],
    });
    var session = ReadSessions().Single();
    Equal("primary", session.AccountId);
    Equal("大号 <&> / \\ Unicode", session.AccountName);
    Equal(start, session.StartTime);
    Equal(start.AddMinutes(3), session.EndTime);
    Equal(6, session.Times);
    Equal(1, session.Rounds);
    Equal(36, session.Sanity);
    Equal(2, session.MedicinesUsed);
    Equal(1, session.ExpiringMedicinesUsed);
    Equal(3, session.StonesUsed);
    Equal(8, session.Drops["30012"]);
    Equal(432, session.Drops["4001"]);
    Equal(1, DailyReportStore.LoadRange(Today(), Today()).Count);
}

static void EmptyAndNonCombatTasks()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    Finish(1);
    Instances.AsstProxy.SetTaskStatus(2, AsstProxy.TaskType.StartUp, TaskItemStatus.InProgress);
    Drop(2, "1-7");
    Instances.AsstProxy.SetTaskStatus(2, AsstProxy.TaskType.StartUp, TaskItemStatus.Completed);
    Equal(null, DailyReportStore.Load(Today()));
}

static void QueueStop()
{
    SettingsViewModel.GuiSettings.DailyReportAccountId = "primary";
    Start(1);
    Drop(1, "1-7");
    SettingsViewModel.GuiSettings.DailyReportAccountId = "secondary";
    DailyReportCollector.OnQueueStopped();
    var first = ReadSessions().Single();
    Equal("primary", first.AccountId);
    Check(first.EndTime is not null, "Stopping must finalize the current account's session.");
    Start(2);
    Drop(2, "1-7");
    Finish(2);
    Equal("secondary", ReadSessions()[1].AccountId);
}

static void AccountConfigurationRoundTrip()
{
    var first = new DailyReportAccount { Name = "Same label" };
    var second = new DailyReportAccount { Name = "Same label" };
    Check(!string.IsNullOrWhiteSpace(first.Id), "New accounts need a stable identifier.");
    Check(first.Id != second.Id, "Display names must not determine identity.");
    var json = System.Text.Json.JsonSerializer.Serialize(new[] { first, second });
    var reloaded = System.Text.Json.JsonSerializer.Deserialize<DailyReportAccount[]>(json)
        ?? throw new InvalidOperationException("Account configuration did not deserialize.");
    Equal(first.Id, reloaded[0].Id);
    Equal(second.Id, reloaded[1].Id);
    reloaded[0].Name = "Renamed after reload";
    Equal(first.Id, reloaded[0].Id);
}

static void Start(int taskId) => Instances.AsstProxy.SetTaskStatus(taskId, AsstProxy.TaskType.Fight, TaskItemStatus.InProgress);

static void Finish(int taskId) => Instances.AsstProxy.SetTaskStatus(taskId, AsstProxy.TaskType.Fight, TaskItemStatus.Completed);

static void Drop(int taskId, string stage, int series = 1, int sanity = 6, int quantity = 1)
{
    Publish(taskId, "FightTimes", new JObject { ["series"] = series, ["sanity_cost"] = sanity });
    Publish(taskId, "StageDrops", new JObject
    {
        ["stage"] = new JObject { ["stageCode"] = stage },
        ["cur_times"] = series,
        ["stats"] = new JArray(new JObject { ["itemId"] = "30012", ["quantity"] = quantity }),
    });
}

static void Publish(int taskId, string what, JObject details) => Instances.AsstProxy.Publish(AsstMsg.SubTaskExtraInfo, new AsstSubTaskMsg
{
    TaskId = taskId,
    What = what,
    Details = details,
});

static DateOnly Today() => DateOnly.FromDateTime(DateTimeExtension.GameDay);

static string FilePath() => Path.Combine(DailyReportStore.DirectoryPath, Today().ToString("yyyy-MM-dd", CultureInfo.InvariantCulture) + ".json");

static void WriteJson(string json)
{
    Directory.CreateDirectory(DailyReportStore.DirectoryPath);
    File.WriteAllText(FilePath(), json);
}

static List<CombatSession> ReadSessions() => DailyReportStore.Load(Today())?.Sessions ?? throw new InvalidOperationException("Expected a saved daily report.");

static void Equal(object? expected, object? actual)
{
    if (!Equals(expected, actual))
    {
        throw new InvalidOperationException($"Expected {expected ?? "<null>"}, got {actual ?? "<null>"}.");
    }
}

static void Check(bool condition, string message)
{
    if (!condition)
    {
        throw new InvalidOperationException(message);
    }
}
