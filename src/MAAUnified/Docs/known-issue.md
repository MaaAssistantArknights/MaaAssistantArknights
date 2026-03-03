# Module B Known Issues

## Mall `credit_fight` downgrade is log-only

- Scope: `TaskQueue` + `Mall`
- Status: Accepted limitation in Package I / Module B
- Current behavior:
  - When there is at least one enabled `Fight` task with empty `stage`, `Mall.credit_fight` is automatically downgraded to `false` before queue append.
  - The downgrade is recorded in logs only (no dedicated UI warning banner/dialog).
- Trigger condition:
  - `profile.taskQueue` contains enabled `Fight` with `stage == ""` (or missing stage), and enabled `Mall` with `credit_fight == true`.
- Diagnostic path:
  - `debug/avalonia-ui-events.log`
  - grep keyword: `Mall credit fight disabled`
- Why kept as-is:
  - This iteration explicitly keeps downgrade semantics unchanged and avoids introducing additional UI interruption.
- Follow-up suggestion:
  - If product policy changes, add non-blocking warning surface in TaskQueue start precheck, while preserving current downgrade behavior.
