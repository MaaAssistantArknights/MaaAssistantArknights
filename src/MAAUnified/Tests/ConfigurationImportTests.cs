using System.Text.Json.Nodes;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class ConfigurationImportTests
{
    [Fact]
    public async Task AutoImport_UsesGuiNewThenGuiFillMissing()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    { "$type": "FightTask", "Name": "Fight", "IsEnable": true }
                  ],
                  "ConnectAddress": "127.0.0.1:5555"
                }
              },
              "GUI": {
                "Localization": "zh-cn"
              }
            }
            """);

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.0.0.2:1234",
                  "TouchMode": "maatouch"
                }
              },
              "Global": {
                "GUI.Localization": "en-us"
              }
            }
            """);

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.False(result.LoadedFromExistingConfig);
        var import = Assert.IsType<ImportReport>(result.ImportReport);
        Assert.True(import.AppliedConfig);
        Assert.True(import.ImportedGuiNew);
        Assert.True(import.ImportedGui);
        Assert.Contains("gui.new.json", import.ImportedFiles, StringComparer.OrdinalIgnoreCase);
        Assert.Contains("gui.json", import.ImportedFiles, StringComparer.OrdinalIgnoreCase);
        Assert.True(import.ConflictCount > 0);
        Assert.True(service.CurrentConfig.Profiles["Default"].TaskQueue.Count == 1);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Fight", service.CurrentConfig.Profiles["Default"].TaskQueue[0].Type);
        Assert.Equal("127.0.0.1:5555", service.CurrentConfig.Profiles["Default"].Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("maatouch", service.CurrentConfig.Profiles["Default"].Values["TouchMode"]?.GetValue<string>());
        Assert.True(service.CurrentConfig.Profiles["Default"].TaskQueue[0].Params.ContainsKey("stage"));
    }

    [Fact]
    public async Task GuiNewImport_ShouldNormalizeLegacyConnectionKeys_ToCanonicalProfileValues()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "Connect.Address": "10.6.0.6:7555",
                  "Connect.ConnectConfig": "LDPlayer",
                  "Connect.AdbPath": "/tmp/adb-normalized"
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.True(report.Success);
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Equal("10.6.0.6:7555", profile.Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("LDPlayer", profile.Values["ConnectConfig"]?.GetValue<string>());
        Assert.Equal("/tmp/adb-normalized", profile.Values["AdbPath"]?.GetValue<string>());
        Assert.False(profile.Values.ContainsKey("Connect.Address"));
        Assert.False(profile.Values.ContainsKey("Connect.ConnectConfig"));
        Assert.False(profile.Values.ContainsKey("Connect.AdbPath"));
    }

    [Fact]
    public async Task GuiNewImport_FightCurrentOrLastStage_ShouldStoreSentinelValue()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    {
                      "$type": "FightTask",
                      "Name": "Fight",
                      "IsEnable": true,
                      "EnableTimesLimit": true,
                      "TimesLimit": 1,
                      "Series": 1,
                      "StagePlan": [""]
                    }
                  ]
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.True(report.Success);
        var task = Assert.Single(service.CurrentConfig.Profiles["Default"].TaskQueue);
        Assert.Equal(FightStageSelection.CurrentOrLast, task.Params["stage"]?.GetValue<string>());
    }

    [Fact]
    public async Task ExistingAvaloniaConfig_SkipsLegacyRead()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 1,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": { "Values": { "ConnectAddress": "1.1.1.1:5555" }, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": { "ImportedBy": "test" }
            }
            """);

        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.new.json"), "{\"Current\":\"Other\"}");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal("Default", service.CurrentConfig.CurrentProfile);
        Assert.Equal(1, service.CurrentConfig.SchemaVersion);
        var schemaBackupExists = Directory.EnumerateFiles(Path.Combine(root, "config"), "avalonia.json.schema-v1.bak.*").Any();
        Assert.False(schemaBackupExists);
    }

    [Fact]
    public async Task ExistingAvaloniaConfig_LegacyEmptyFightStage_ShouldNormalizeAndPersistSentinel()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": {
                  "Values": {},
                  "TaskQueue": [
                    {
                      "Type": "Fight",
                      "Name": "Fight",
                      "IsEnabled": true,
                      "Params": {
                        "stage": "",
                        "medicine": 0,
                        "stone": 0,
                        "times": 1,
                        "series": 1
                      }
                    }
                  ]
                }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal(
            FightStageSelection.CurrentOrLast,
            service.CurrentConfig.Profiles["Default"].TaskQueue[0].Params["stage"]?.GetValue<string>());

        var persisted = Assert.IsType<JsonObject>(
            JsonNode.Parse(await File.ReadAllTextAsync(Path.Combine(root, "config", "avalonia.json"))));
        var stage = persisted["Profiles"]?["Default"]?["TaskQueue"]?[0]?["Params"]?["stage"]?.GetValue<string>();
        Assert.Equal(FightStageSelection.CurrentOrLast, stage);
    }

    [Fact]
    public async Task CorruptedAvaloniaConfig_RebuildsDefaults_AndDoesNotCrash()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "{ invalid json");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Default", service.CurrentConfig.CurrentProfile);
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
    }

    [Fact]
    public async Task CorruptedAvaloniaConfig_EmitsWarningLog()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "{ invalid json");

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();

        Assert.Contains(
            service.LogService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.Ordinal) &&
                   log.Message.Contains("ConfigRepair.DeserializeException", StringComparison.Ordinal));
    }

    [Fact]
    public async Task NullAvaloniaConfig_RebuildsDefaults_AndEmitsParseNullWarning()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "null");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Default", service.CurrentConfig.CurrentProfile);
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
        Assert.Contains(
            service.LogService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.Ordinal)
                   && log.Message.Contains("ConfigRepair.DeserializeNull", StringComparison.Ordinal));
    }

    [Fact]
    public async Task OutdatedSchema_LoadsWithMigrationWarningIssue()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 1,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": { "Values": {}, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": { "ImportedBy": "test" }
            }
            """);

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        var issue = Assert.Single(result.ValidationIssues.Where(i =>
            string.Equals(i.Scope, "ConfigMigration", StringComparison.Ordinal) &&
            string.Equals(i.Code, "SchemaOutdated", StringComparison.Ordinal)));
        Assert.False(issue.Blocking);
        Assert.Equal("schema_version", issue.Field);
        Assert.NotNull(result.SchemaMigrationNotice);
        Assert.Equal(1, result.SchemaMigrationNotice!.CurrentSchemaVersion);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, result.SchemaMigrationNotice.LatestSchemaVersion);
    }

    [Fact]
    public async Task OutdatedSchema_SaveCreatesSchemaBackup()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 1,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": { "Values": {}, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": { "ImportedBy": "test" }
            }
            """);

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();
        await service.SaveAsync();

        var backupExists = Directory
            .EnumerateFiles(Path.Combine(root, "config"), "avalonia.json.schema-v1.bak.*")
            .Any();
        Assert.True(backupExists);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
    }

    [Fact]
    public async Task LoadOrBootstrapAsync_ShouldSyncValidationIssues_WithServiceState()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": {
                  "Values": {},
                  "TaskQueue": [
                    {
                      "Type": "Recruit",
                      "Name": "Recruit",
                      "IsEnabled": true,
                      "Params": {
                        "times": 4
                      }
                    }
                  ]
                }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        var load = await service.LoadOrBootstrapAsync();

        Assert.NotEmpty(load.ValidationIssues);
        Assert.Equal(service.CurrentValidationIssues.Count, load.ValidationIssues.Count);
        Assert.Equal(service.HasBlockingValidationIssues, load.HasBlockingValidationIssues);
        Assert.True(service.HasBlockingValidationIssues);
    }

    [Fact]
    public async Task LoadOrBootstrapAsync_CurrentProfileMissing_ShouldBeBlockingAndSynced()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Alt": { "Values": {}, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        var load = await service.LoadOrBootstrapAsync();

        var issue = Assert.Single(load.ValidationIssues, i =>
            string.Equals(i.Code, "CurrentProfileMissing", StringComparison.Ordinal));
        Assert.True(issue.Blocking);
        Assert.True(load.HasBlockingValidationIssues);
        Assert.True(service.HasBlockingValidationIssues);
        Assert.Contains(service.CurrentValidationIssues, i => string.Equals(i.Code, "CurrentProfileMissing", StringComparison.Ordinal));
    }

    [Fact]
    public async Task SaveAsync_ShouldRefreshValidationStateAndBlockingFlag()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();
        Assert.False(service.HasBlockingValidationIssues);

        service.CurrentConfig.Profiles.Clear();
        await service.SaveAsync();

        Assert.True(service.HasBlockingValidationIssues);
        Assert.Contains(service.CurrentValidationIssues, issue => issue.Code == "ProfileMissing");
    }

    [Fact]
    public async Task LoadOrBootstrap_AutoImportFailure_WritesDebugReport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.new.json"), "{ invalid json");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.False(result.LoadedFromExistingConfig);
        var report = Assert.IsType<ImportReport>(result.ImportReport);
        Assert.False(report.Success);
        Assert.True(report.AppliedConfig);
        Assert.NotEmpty(report.Errors);
        var reportPath = Path.Combine(root, "debug", "config-import-report.json");
        Assert.True(File.Exists(reportPath));
        var reportJson = await File.ReadAllTextAsync(reportPath);
        Assert.Contains("errors", reportJson, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task LoadOrBootstrap_AutoImportFailure_DoesNotCrash_AndCanSave()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.new.json"), "{ invalid json");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.NotNull(result.ImportReport);
        Assert.False(result.ImportReport!.Success);
        Assert.True(result.ImportReport.AppliedConfig);
        await service.SaveAsync();
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
    }

    [Fact]
    public async Task LoadOrBootstrap_WhenNoLegacyFileExists_ShouldCreateDefaultConfigAndReportIt()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        var report = Assert.IsType<ImportReport>(result.ImportReport);
        Assert.True(report.AppliedConfig);
        Assert.True(report.CreatedDefaultConfig);
        Assert.Contains("gui.new.json", report.MissingFiles, StringComparer.OrdinalIgnoreCase);
        Assert.Contains("gui.json", report.MissingFiles, StringComparer.OrdinalIgnoreCase);
        Assert.Contains(
            service.LogService.Snapshot,
            log => log.Message.Contains("已自动创建默认配置 avalonia.json", StringComparison.Ordinal));
    }

    [Fact]
    public async Task ImportLegacy_Auto_WhenOnlyGuiExists_ShouldImportAndProduceCorrectReportFlags()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.0.0.7:5555",
                  "TouchMode": "maatouch"
                }
              },
              "Global": {
                "GUI.Localization": "en-us"
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.Auto, manualImport: false);

        Assert.True(report.Success);
        Assert.True(report.ImportedGui);
        Assert.False(report.ImportedGuiNew);
        Assert.Empty(report.Errors);
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Equal("10.0.0.7:5555", profile.Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("maatouch", profile.Values["TouchMode"]?.GetValue<string>());
    }

    [Fact]
    public async Task ImportLegacy_GuiPostActionsLegacyValue_RemainsReadable_AndMigratesOnLoad()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.0.0.7:5555"
                }
              },
              "Global": {
                "MainFunction.PostActions": "136"
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiOnly, manualImport: false);

        Assert.True(report.Success);
        var diagnostics = new UiDiagnosticsService(root, service.LogService);
        var feature = new PostActionFeatureService(service, diagnostics, new NoOpPostActionExecutorService());

        var load = await feature.LoadAsync();

        Assert.True(load.Success);
        Assert.NotNull(load.Value);
        Assert.True(load.Value!.ExitSelf);
        Assert.True(load.Value.Sleep);

        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.True(profile.Values.ContainsKey("TaskQueue.PostAction"));
        Assert.False(profile.Values.ContainsKey("MainFunction.PostActions"));
        Assert.False(service.CurrentConfig.GlobalValues.ContainsKey("MainFunction.PostActions"));
    }

    [Fact]
    public async Task ImportLegacy_Auto_GuiNewValidAndGuiCorrupted_ShouldKeepImportResultSaveable_AndReportError()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    { "$type": "FightTask", "Name": "Fight", "IsEnable": true }
                  ],
                  "ConnectAddress": "127.0.0.1:6000"
                }
              }
            }
            """);
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{ invalid json");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.Auto, manualImport: false);

        Assert.False(report.Success);
        Assert.NotEmpty(report.Errors);
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Single(profile.TaskQueue);
        Assert.Equal("127.0.0.1:6000", profile.Values["ConnectAddress"]?.GetValue<string>());
        await service.SaveAsync();
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
    }

    [Fact]
    public async Task GuiNewImport_TaskQueueWithNonObjectEntries_ShouldSkipInvalidRowsAndWarnWithoutBlockingImport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    1,
                    "invalid",
                    { "$type": "FightTask", "Name": "Fight", "IsEnable": true }
                  ]
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.True(report.Success);
        Assert.Empty(report.Errors);
        Assert.Contains(report.Warnings, warning => warning.Contains("non-object entry", StringComparison.OrdinalIgnoreCase));
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Single(profile.TaskQueue);
    }

    [Fact]
    public async Task ManualImport_CreatesBackupAndReport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "{\"SchemaVersion\":1,\"CurrentProfile\":\"Default\",\"Profiles\":{\"Default\":{\"Values\":{},\"TaskQueue\":[]}},\"GlobalValues\":{},\"Migration\":{}}");
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{\"Current\":\"Default\",\"Configurations\":{\"Default\":{\"TouchMode\":\"maatouch\"}},\"Global\":{}}");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiOnly, manualImport: true);

        Assert.True(report.Success);
        Assert.True(report.AppliedConfig);
        var bakExists = Directory.EnumerateFiles(Path.Combine(root, "config"), "avalonia.json.bak.*").Any();
        Assert.True(bakExists);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.True(File.Exists(Path.Combine(root, "debug", "config-import-report.json")));
    }

    [Fact]
    public async Task ManualImport_SingleGuiOnly_WithForceImport_ShouldStartFromDefaults()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TouchMode": "maatouch"
                }
              },
              "Global": {}
            }
            """);

        var service = CreateService(root);
        service.CurrentConfig.GlobalValues["Legacy.Leftover"] = JsonValue.Create("should-be-removed");
        await service.SaveAsync();

        var report = await service.ImportLegacyAsync(
            new LegacyImportRequest(
                LegacyConfigSnapshot.FromPaths(null, Path.Combine(root, "config", "gui.json")),
                ImportSource.GuiOnly,
                ManualImport: true,
                AllowPartialImport: true));

        Assert.True(report.AppliedConfig);
        Assert.True(report.Success);
        Assert.Contains("gui.new.json", report.MissingFiles, StringComparer.OrdinalIgnoreCase);
        Assert.False(service.CurrentConfig.GlobalValues.ContainsKey("Legacy.Leftover"));
        Assert.Equal("maatouch", service.CurrentConfig.Profiles["Default"].Values["TouchMode"]?.GetValue<string>());
    }

    [Fact]
    public async Task ManualImport_DamagedGuiJson_WithAllowPartialFalse_ShouldNotApplyConfig()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.1.2.3:5555"
                }
              }
            }
            """);
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{ invalid json");
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": { "Values": { "ConnectAddress": "1.1.1.1:5555" }, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();

        var report = await service.ImportLegacyAsync(
            new LegacyImportRequest(
                LegacyConfigSnapshot.FromPaths(
                    Path.Combine(root, "config", "gui.new.json"),
                    Path.Combine(root, "config", "gui.json")),
                ImportSource.Auto,
                ManualImport: true,
                AllowPartialImport: false));

        Assert.False(report.AppliedConfig);
        Assert.False(report.Success);
        Assert.Contains("gui.new.json", report.ImportedFiles, StringComparer.OrdinalIgnoreCase);
        Assert.Contains("gui.json", report.DamagedFiles, StringComparer.OrdinalIgnoreCase);
        Assert.Equal("1.1.1.1:5555", service.CurrentConfig.Profiles["Default"].Values["ConnectAddress"]?.GetValue<string>());
    }

    [Fact]
    public async Task ManualImport_DamagedGuiJson_WithAllowPartialTrue_ShouldApplyUsableContent()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.5.6.7:5555"
                }
              }
            }
            """);
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{ invalid json");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(
            new LegacyImportRequest(
                LegacyConfigSnapshot.FromPaths(
                    Path.Combine(root, "config", "gui.new.json"),
                    Path.Combine(root, "config", "gui.json")),
                ImportSource.Auto,
                ManualImport: true,
                AllowPartialImport: true));

        Assert.True(report.AppliedConfig);
        Assert.False(report.Success);
        Assert.Contains("gui.json", report.DamagedFiles, StringComparer.OrdinalIgnoreCase);
        Assert.Equal("10.5.6.7:5555", service.CurrentConfig.Profiles["Default"].Values["ConnectAddress"]?.GetValue<string>());
    }

    [Fact]
    public async Task ManualImport_WhenNoUsableLegacyContentExists_ShouldNotOverwriteExistingConfig()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{ invalid json");
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": { "Values": { "TouchMode": "adb" }, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();
        var before = await File.ReadAllTextAsync(Path.Combine(root, "config", "avalonia.json"));

        var report = await service.ImportLegacyAsync(
            new LegacyImportRequest(
                LegacyConfigSnapshot.FromPaths(null, Path.Combine(root, "config", "gui.json")),
                ImportSource.GuiOnly,
                ManualImport: true,
                AllowPartialImport: true));

        Assert.False(report.AppliedConfig);
        Assert.False(report.Success);
        var after = await File.ReadAllTextAsync(Path.Combine(root, "config", "avalonia.json"));
        Assert.Equal(before, after);
        Assert.Equal("adb", service.CurrentConfig.Profiles["Default"].Values["TouchMode"]?.GetValue<string>());
    }

    [Fact]
    public async Task UnsupportedLegacyTask_IsDisabledAndReported()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    { "$type": "UnknownLegacyTask", "Name": "Unsupported", "IsEnable": true }
                  ]
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.False(report.Success);
        var task = service.CurrentConfig.Profiles["Default"].TaskQueue.Single();
        Assert.False(task.IsEnabled);
        Assert.Equal("UnknownLegacyTask", task.Type);
        Assert.NotEmpty(report.Errors);
    }

    [Fact]
    public async Task CorruptedGuiFile_FallsBackToDefaultsWithErrorInReport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{invalid json");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiOnly, manualImport: false);

        Assert.False(report.Success);
        Assert.NotEmpty(report.Errors);
    }

    private static UnifiedConfigurationService CreateService(string baseDirectory)
    {
        var store = new AvaloniaJsonConfigStore(baseDirectory);
        var log = new UiLogService();
        return new UnifiedConfigurationService(store, new GuiNewJsonConfigImporter(), new GuiJsonConfigImporter(), log, baseDirectory);
    }

    private static string CreateTempRoot()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        return root;
    }
}
