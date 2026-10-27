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

using System.IO;
using System.Runtime.Loader;
using System.Text.Json;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;

internal static class Program
{
    private static int _checks;

    private static void Main()
    {
        AssemblyLoadContext.Default.Resolving += (_, name) => {
            var root = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "../../../../../build/bin/Debug"));
            var path = Path.Combine(root, name.Name + ".dll");
            return File.Exists(path) ? AssemblyLoadContext.Default.LoadFromAssemblyPath(path) : null;
        };
        Run();
        Console.WriteLine($"Passed {_checks} rotation/dorm safety checks.");
    }

    private static void Check(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
        ++_checks;
    }

    private static void Run()
    {
        foreach (var mode in Enum.GetValues<InfrastMode>())
        {
            foreach (var enabled in new[] { false, true })
            {
                var task = new AsstInfrastTask { Mode = mode, Facilitys = ["Mfg", "Trade"], DormThreshold = 0.3 };
                var before = task.Serialize().Params.ToString();
                var tasks = task.BuildExecutionTasks(enabled).ToArray();
                Check(tasks.Length == (mode == InfrastMode.Rotation && enabled ? 2 : 1), "Unexpected task count");
                Check(ReferenceEquals(tasks[0], task) && task.Serialize().Params.ToString() == before, "Primary task changed");
                if (tasks.Length != 2)
                {
                    continue;
                }

                var dorm = tasks[1].Serialize().Params;
                Check((int)dorm["mode"]! == 0 && dorm["facility"]!.Count() == 1 && (string)dorm["facility"]![0]! == "Dorm", "Dorm task must manage only dormitories");
                Check((bool)dorm["dorm_notstationed_enabled"]! && (bool)dorm["dorm_trust_enabled"]!, "Dorm safety filters missing");
                Check(!(bool)dorm["fiammetta_recovery_enabled"]! && !(bool)dorm["continue_training"]!, "Training or Fiammetta unexpectedly enabled");
                Check((string)dorm["drones"]! == "_NotUse", "Dorm task must not consume drones");
            }
        }

        var config = new InfrastTask();
        Check(!config.RotationDormTrustEnabled, "New option must default off");
        foreach (var enabled in new[] { true, false })
        {
            config.RotationDormTrustEnabled = enabled;
            var loaded = JsonSerializer.Deserialize<InfrastTask>(JsonSerializer.Serialize(config))!;
            Check(loaded.RotationDormTrustEnabled == enabled, "Option not preserved by config serialization");
            var task = new AsstInfrastTask { Mode = InfrastMode.Rotation };
            Check(task.BuildExecutionTasks(loaded.RotationDormTrustEnabled).Count() == (enabled ? 2 : 1), "Saved option does not control task composition");
        }
    }
}
