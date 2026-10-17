---
order: 9
icon: material-symbols:route
---

# Activity navigation

`ActivityNavigation` only enters an event partition. It does not select a stage, form a squad or start a battle. Parameters: required `stage_id` (internal Tile-Pos ID), optional `client_type` (otherwise the connected client) and `is_raid` (false by default; validates the mode only). Connect and log in before running it.

`Copilot` in `copilot_list` mode and `Fight` can enable the same entry with `navigate_to_activity: true` (default false). Copilot also requires an explicit `client_type`; use a one-element list for a single job. Enabling it with `filename` is rejected. Registered list entries are checked when accepting the task and again during execution. After entering the partition, existing stage selection, exact code confirmation, raid selection, formation and battle continue. WPF enables this for job lists; file mode is unchanged.

`resource/activity_navigation.json` has `schema_version: 1` and `activities`. Each activity records `id`, `enabled`, `clients`, `routes`, `stages`, plus descriptive `name` and `sources`. Routes contain a standard task name `task` and timezone-qualified ISO 8601 `opens_at` / `closes_at` with whole seconds. Each internal stage ID maps to `code`, `route`, and `modes` (normal / raid). Opening is inclusive; closing is exclusive. Tasks and codes must match Tile-Pos. Registered but closed, disabled, wrong-client or unsupported-mode entries fail without fallback. Unknown activities keep existing behavior. A valid complete manifest replaces the previous one; empty `activities` clears it; invalid data does not replace it.

Compatible tasks belong in `resource/tasks/hotfix/activity_navigation/`. MaaResource must allow the manifest and `template/StageNavigation/`; the main repository sync workflow bumps the resource version for these changes. New task fields or core features still require a core upgrade. `AsstGetActivityNavigationVersion()` returns the supported protocol (1 here); check symbol availability before calling it. Older cores can retain external Custom navigation. Callbacks `ActivityNavigationStarted`, `ActivityNavigationCompleted` and `ActivityNavigationFailed` include stage, partition or failure details.

With activity navigation enabled, Fight accepts `is_raid: true` to select raid mode (default false). For registered activity stages, this option is used both for partition validation and for difficulty selection after locating the stage.

Copilot retains the registered route metadata of each accepted list entry. If a route is removed or its metadata changes before execution, it stops with `MissingRoute` or `RouteChanged`; submit the list again to accept the new routes. Activities unknown at acceptance retain legacy selection. A reader/writer lock synchronizes route copies and full-map replacement. Preflight failure callbacks include `phase: Preflight` and the specific `reason`: `Closed`, `Disabled`, `WrongClient`, `StageCodeMismatch` or `UnsupportedMode`, matching execution-time availability checks.
