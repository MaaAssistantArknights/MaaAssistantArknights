# Module B Known Issues

## Mall `credit_fight` downgrade visibility gap (closed)

- Scope: `TaskQueue` + `Mall`
- Status: Closed on 2026-03-04
- Current behavior:
  - When there is at least one enabled `Fight` task with empty `stage`, `Mall.credit_fight` is automatically downgraded to `false` before queue append.
  - A non-blocking warning is shown on TaskQueue start precheck.
  - Even when `StartAsync` is later blocked by task validation, the precheck warning remains visible and downgrade log is still recorded.
  - The downgrade is still recorded in logs.
- Trigger condition:
  - `profile.taskQueue` contains enabled `Fight` with `stage == ""` (or missing stage), and enabled `Mall` with `credit_fight == true`.
- Diagnostic path:
  - `debug/avalonia-ui-events.log`
  - grep keyword: `Mall credit fight disabled`
- Notes:
  - Downgrade semantics remain unchanged; only UI visibility has been added.
