"""Check the actual craft parameter builder without loading WPF or user config."""

from pathlib import Path
import subprocess

from test_material_craft_startup import method

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/craft-params-check"

FIXTURE = r"""
using System;
using System.Collections.Generic;
using System.Linq;
class JObject : Dictionary<string, object> {}
class JArray(IEnumerable<JObject> values) : List<JObject>(values) {}
class BaseTask {}
class InfrastTask : BaseTask { public bool OriginiumShardAutoReplenishment = true; }
class Config { public List<BaseTask> TaskQueue = []; }
static class ConfigFactory { public static Config CurrentConfig = new(); }
record Item(string Id, int Count);
class Vm {
    List<Item> MaterialCraftPlanItems = [new("3283", 1)];
    List<Item> DepotResult = [new("3282", 2), new("32001", 1)];
    public JObject Build() => BuildMaterialCraftTaskParams();
__METHOD__
}
class Program {
    static void Main() {
        int checks = 0;
        void Check(bool ok) { ++checks; if (!ok) throw new Exception($"Check {checks}"); }
        var vm = new Vm();
        Check((bool)vm.Build()["replenish"] == false);
        ConfigFactory.CurrentConfig.TaskQueue.Add(new BaseTask());
        Check((bool)vm.Build()["replenish"] == false);
        var infrast = new InfrastTask { OriginiumShardAutoReplenishment = false };
        ConfigFactory.CurrentConfig.TaskQueue.Add(infrast);
        Check((bool)vm.Build()["replenish"] == false);
        infrast.OriginiumShardAutoReplenishment = true;
        var submitted = vm.Build();
        Check((bool)submitted["replenish"] == true);
        infrast.OriginiumShardAutoReplenishment = false;
        Check((bool)submitted["replenish"] == true);
        Check((bool)vm.Build()["replenish"] == false);
        ConfigFactory.CurrentConfig.TaskQueue.Add(new InfrastTask());
        Check((bool)vm.Build()["replenish"] == false);
        ConfigFactory.CurrentConfig = new Config();
        Check((bool)vm.Build()["replenish"] == false);
        var targets = (JArray)submitted["items"];
        Check(targets.Count == 1 && (string)targets[0]["itemId"] == "3283" && (int)targets[0]["count"] == 1);
        var inventory = (JObject)submitted["inventory"];
        Check((int)inventory["3282"] == 2 && (int)inventory["32001"] == 1);
        Console.WriteLine($"MaterialCraft parameters: {checks} checks passed");
    }
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = (
        ROOT / "src/MaaWpfGui/ViewModels/UserControl/MaterialCraftViewModel.cs"
    ).read_text(encoding="utf-8-sig")
    (OUT / "Program.cs").write_text(
        FIXTURE.replace("__METHOD__", method(source, "BuildMaterialCraftTaskParams")),
        encoding="utf-8",
    )
    (OUT / "test.csproj").write_text(
        '<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType>'
        "<TargetFramework>net10.0</TargetFramework><Nullable>enable</Nullable>"
        "</PropertyGroup></Project>",
        encoding="utf-8",
    )
    config = OUT / "NuGet.Config"
    config.write_text(
        "<configuration><packageSources><clear /></packageSources></configuration>",
        encoding="utf-8",
    )
    subprocess.run(
        ["dotnet", "restore", str(OUT / "test.csproj"), "--configfile", str(config)],
        check=True,
    )
    subprocess.run(
        ["dotnet", "run", "--no-restore", "--project", str(OUT / "test.csproj")],
        check=True,
    )


if __name__ == "__main__":
    main()
