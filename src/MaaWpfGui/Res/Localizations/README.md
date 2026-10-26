# WPF localization keys

The five language catalogs use the same keys and sort them in ordinal key order. A
key has the form `Domain[.Topic].Name[.Tip|.Title|.Content|.Description]`, for
example `Connection.Adb.Path`, `Roguelike.BlackFlow.StartRun.Preparing`, or
`Achievement.AfkWatcher.Title`. Choose the domain from the feature that owns
the message. Use `.Tip` for help text, `.Title` for headings, `.Content`
for message bodies, and `.Description` for descriptive copy. If the
corresponding base message exists, keep the role suffix under its key.

The main ownership boundaries are:

| Domain | Owns |
| --- | --- |
| `Connection` | ADB, emulator, PC client, capture and touch modes |
| `TaskQueue` | Queued task controls, execution state and logs |
| `Combat` | Fighting, stages and formations |
| `Copilot` | Copilot plans and execution |
| `Roguelike` / `Reclamation` | Their respective game modes |
| `Infrastructure` / `Depot` / `Recruitment` | Base management, inventory recognition and recruitment |
| `Toolbox` / `Gacha` / `Operator` | Toolbox features, gacha and operator recognition |
| `Settings` / `Appearance` | General application options and visual theme |
| `Update` / `Version` / `ResourceUpdate` | Application updates, version labels and resource updates |
| `Notification` / `ExternalNotification` | In-app and external notifications |
| `Common` | Reused generic words only; prefer a feature domain otherwise |

Other established domains can be found by their sorted prefix in the catalog.
Add a new top-level domain only when existing owners cannot describe the
feature. The dotted `Achievement.<name>.<role>` family predates this migration
and remains unchanged.

Add a new key to all five catalogs at its sorted position. Use the canonical
key directly in XAML `DynamicResource`, C# `LocalizationHelper` calls, and
inline `{key=...}` references. Run `python tools/CheckLocalization.py` from
the repository root. The checker validates XML, unique keys, ordering, language
parity, inline references, and literal C#/XAML references. References assembled
at runtime cannot be checked statically; review their source and target range
when changing them.

Task controls in `Views/UserControl/TaskQueue` and their view models have a
localization ownership policy in `tools/CheckLocalization.py`. Each control
may use its own feature prefix and shared `Common` words. A cross-feature
reference must be listed by its exact key in `TASK_CONTROL_SHARED`; a new
task control needs a `TASK_CONTROL_PRIMARY` entry. Files under these
directories are discovered automatically: the entry defines what a new
control may use, not whether CI checks it. Without an entry, any localization
reference fails. The CI checker reports every reference outside that policy.
Run its regression tests with
`python -m unittest discover -s tools/tests -p 'test_check_localization.py'`.

`legacy-key-map.tsv` is a frozen bridge for enum names, task/core callbacks,
saved configuration, and MaaRelease data that can still supply old keys.
`LocalizationHelper` first tries the canonical key and consults this map only
on a miss. Do not add entries for new messages. Remove the bridge after all
external producers and persisted data have migrated to canonical keys.
