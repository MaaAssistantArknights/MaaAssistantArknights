# MAAUnified

Single-directory implementation root for the new Avalonia frontend replacement.

## Directory map

- `App/`: Avalonia startup, shell, module navigation, feature views.
- `Application/`: orchestration, config facade, session state machine, feature services.
- `CoreBridge/`: MaaCore bridge interface and stub implementation.
- `Platform/`: tray/notification/hotkey/autostart/file-dialog/overlay capability layer.
- `Compat/`: WPF baseline compatibility mapping and legacy configuration keys.
- `Tests/`: config import and contract tests.
- `CI/`: standalone CI workflow template.
- `Docs/`: parity matrix, migration, preview, rollback, and import report sample.

## Locked behavior

- Config write target: `config/avalonia.json`
- Auto import only when `avalonia.json` does not exist
- Import order: `gui.new.json` -> `gui.json` -> defaults
- Manual import supports: auto / gui.new only / gui only
- Legacy config files are read-only and never overwritten
