"""Run actual ViewModel startup methods with a gated, non-game Core substitute.

Usage: python unit_test/MaaWpfGui/test_material_craft_startup.py
Generated C# stays in build/craft-startup-check. No emulator or materials are used.
"""

from pathlib import Path
import re
import subprocess
import sys

REPO = Path(__file__).resolve().parents[2]
SOURCE = REPO / "src/MaaWpfGui/ViewModels/UserControl/MaterialCraftViewModel.cs"


def method(source, name):
    """Extract the existing method body, so the test does not maintain a copy."""
    declaration = re.search(
        rf"^    (?:public|private) [^\n]*\b{re.escape(name)}\(", source, re.MULTILINE
    )
    if declaration is None:
        raise ValueError(f"Method not found: {name}")
    start = declaration.start()
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


FIXTURE = r"""
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Models;

class JObject {}
static class LocalizationHelper { public static string GetString(string key) => key; }
class Logger { public void Error(Exception e, string message) {} }
namespace Services { enum AsstTaskType { MaterialCraft, MaterialRequirement } }
static class Instances {
    public static AsstProxy AsstProxy = null!;
    public static TaskQueue TaskQueueViewModel = null!;
}
class AsstProxy {
    public enum TaskType { MaterialCraft, MaterialRequirement }
    public readonly ManualResetEventSlim Connecting = new(false), Release = new(false);
    public int Outcome, Appends, Starts, Stops;
    public bool AsstConnect(ref string message) {
        Connecting.Set();
        if (!Release.Wait(TimeSpan.FromSeconds(10))) throw new TimeoutException();
        if (Outcome == 2) throw new InvalidOperationException("Connection failed");
        message = "Connection failed";
        return Outcome == 0;
    }
    public (bool, int) AsstAppendTaskWithEncoding(TaskType type, (Services.AsstTaskType, JObject?) args) {
        Appends++; return (true, Appends);
    }
    public bool AsstStart() { Starts++; return true; }
    public bool AsstStop() { Stops++; return true; }
}
class State(Vm vm) {
    public void SetIdle(bool value) { vm.Idle = value; if(value)vm.CompleteStop(); }
    public void SetStopping(bool value) {
        vm.Stopping = value;
        if (value) vm.CancelMaterialCraftPlan();
    }
}
class TaskQueue(Vm vm) {
    public int Resets;
    public void ManualStop() {
        vm.Stopping = true;
        vm.CancelMaterialCraftPlan();
        Instances.AsstProxy.AsstStop();
        // Core has no TaskChainStopped callback if no chain was started.
    }
    public bool SetStopped() {
        Resets++; vm.Stopping = false; vm.Idle = true; vm.CompleteStop(); return true;
    }
}
class Toolbox {
    public bool DepotInventoryNeedsRecognition;
    public int Saves;
    public void SaveDepotDetails() => Saves++;
}
class Target { public string Id = "A"; public int Count = 1; }
class Vm {
    public bool Idle = true, Stopping;
    public string MaterialCraftResultInfo = "";
    public readonly List<Target> MaterialCraftPlanItems = [new()];
    public readonly Toolbox _toolbox = new();
    readonly State _runningState;
    readonly Logger _logger = new();
    MaterialCraftExecution? _materialCraftExecution;
    Dictionary<string, long> _materialCraftPlannedOutputs = [];
    public List<string> MaterialCraftChanges = [];
    CancellationTokenSource? _materialCraftCancellation;
    int _requirementTaskId;
    bool _materialCraftStopRequested;
    public Vm() => _runningState = new(this);
    public bool CanEditMaterialCraftPlan => _materialCraftExecution is null && _materialCraftCancellation is null;
    bool CheckMaterialCraftPlanCore(bool updatePreview) => true;
    JObject BuildMaterialCraftTaskParams() => new();
    void NotifyOfPropertyChange(string property) {}
    public bool CanToggleMaterialCraft => !Stopping && (Idle || _materialCraftExecution is not null || _materialCraftCancellation is not null || _requirementTaskId != 0);
    void FinishMaterialCraft(int taskId, bool completed) { _materialCraftExecution = null; }
    public void Stop() => StopMaterialCraft();
    public void CompleteStop() => CompleteMaterialCraftStop();
__METHODS__
}
class Program {
    static int checks;
    static void Check(bool value, string message) {
        if (!value) throw new Exception(message);
        checks++;
    }
    static Task Start(Vm vm, bool requirement) => requirement ? vm.RecognizeMaterialRequirement() : vm.StartMaterialCraft();
    static (Vm, AsstProxy, TaskQueue) Setup() {
        var vm = new Vm(); var core = new AsstProxy(); var queue = new TaskQueue(vm);
        Instances.AsstProxy = core; Instances.TaskQueueViewModel = queue;
        return (vm, core, queue);
    }
    static async Task Main() {
        foreach (bool requirement in new[] { false, true }) {
            foreach (int outcome in new[] { 0, 1, 2 }) {
                var (vm, core, queue) = Setup(); core.Outcome = outcome;
                var pending = Start(vm, requirement);
                Check(core.Connecting.Wait(TimeSpan.FromSeconds(5)), "Connection must be in flight");
                vm.Stop();
                Check(vm.Stopping && !vm.Idle, "Keep startup occupied until connection exits");
                Check(!vm.CanToggleMaterialCraft, "Do not restart while cancellation is pending");
                core.Release.Set(); await pending.WaitAsync(TimeSpan.FromSeconds(5));
                Check(vm.Idle && !vm.Stopping, "Cancelled connection must restore idle without a Core callback");
                Check(core.Stops == 0, "Do not queue a late Core stop during connection");
                Check(vm.CanEditMaterialCraftPlan && vm.CanToggleMaterialCraft, "Restore editing and start controls");
                Check(core.Appends == 0 && core.Starts == 0, "Cancelled startup must never submit a task");
                Check(vm.MaterialCraftResultInfo == "Stopped", "A cancelled failure is still a stop");
                Check(vm.MaterialCraftPlanItems.Single().Count == 1 && !vm._toolbox.DepotInventoryNeedsRecognition && vm._toolbox.Saves == 0,
                    "Connection cancellation must preserve the queue and inventory");
                core.Outcome = 0;
                await Start(vm, requirement).WaitAsync(TimeSpan.FromSeconds(5));
                Check(core.Starts == 1 && !vm.Idle && !vm.Stopping, "The next startup must succeed");
                int resets = queue.Resets;
                vm.Stop();
                Check(core.Stops == 1 && vm.Stopping && !vm.Idle && queue.Resets == resets,
                    "An active chain still waits for its Core stop callback");
                vm.CompleteStop();
                Check(vm.MaterialCraftResultInfo == "Stopping", "Do not report stopped before idle");
                // A terminal task callback may set an error before the idle notification.
                vm.MaterialCraftResultInfo = "Task failed";
                queue.SetStopped();
                Check(vm.MaterialCraftResultInfo == "Stopped", "Finished cancellation must replace the pending/error text");
                vm.MaterialCraftResultInfo = "Next result";
                queue.SetStopped();
                Check(vm.MaterialCraftResultInfo == "Next result", "Duplicate idle must not overwrite subsequent results");
            }
            foreach (int outcome in new[] { 1, 2 }) {
                var (vm, core, queue) = Setup(); core.Outcome = outcome; core.Release.Set();
                await Start(vm, requirement).WaitAsync(TimeSpan.FromSeconds(5));
                Check(vm.Idle && !vm.Stopping && vm.CanEditMaterialCraftPlan, "Ordinary connection failures recover");
                Check(vm.MaterialCraftResultInfo != "Stopped" && queue.Resets == 0, "Preserve ordinary failure reporting");
            }
        }
        Console.WriteLine($"MaterialCraft startup: {checks} checks passed (10 scenarios)");
    }
}
"""


def main():
    source = SOURCE.read_text(encoding="utf-8-sig")
    # Optional source override supports checking that the regression fails before a fix.
    if len(sys.argv) > 1:
        source = Path(sys.argv[1]).read_text(encoding="utf-8-sig")
    methods = "\n".join(
        method(source, name)
        for name in (
            "StartMaterialCraft",
            "RecognizeMaterialRequirement",
            "StopMaterialCraft",
            "CancelMaterialCraftPlan",
            "CompleteMaterialCraftStop",
        )
    )
    output = REPO / "build/craft-startup-check"
    output.mkdir(parents=True, exist_ok=True)
    (output / "Program.cs").write_text(
        FIXTURE.replace("__METHODS__", methods), encoding="utf-8"
    )
    execution = REPO / "src/MaaWpfGui/Models/MaterialCraftExecution.cs"
    (output / "test.csproj").write_text(
        f'''<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><Nullable>enable</Nullable></PropertyGroup>
  <ItemGroup>
    <Compile Include="{execution.as_posix()}" Link="MaterialCraftExecution.cs" />
    <Compile Include="{execution.with_name("MaterialCraftInventoryChange.cs").as_posix()}" Link="MaterialCraftInventoryChange.cs" />
  </ItemGroup>
</Project>''',
        encoding="utf-8",
    )
    # The fixture has no packages and can run fully offline.
    config = output / "NuGet.Config"
    config.write_text(
        "<configuration><packageSources><clear /></packageSources></configuration>",
        encoding="utf-8",
    )
    subprocess.run(
        ["dotnet", "restore", str(output / "test.csproj"), "--configfile", str(config)],
        check=True,
        cwd=REPO,
    )
    subprocess.run(
        ["dotnet", "run", "--no-restore", "--project", str(output / "test.csproj")],
        check=True,
        cwd=REPO,
    )


if __name__ == "__main__":
    main()
