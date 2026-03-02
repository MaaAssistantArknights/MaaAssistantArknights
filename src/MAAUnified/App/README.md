# MAAUnified App

Avalonia UI entry for the unified cross-platform frontend.

## Hard constraints

- New files are restricted to `src/MAAUnified/**`.
- Main config file is `config/avalonia.json`.
- Legacy config files are imported read-only (`gui.new.json`, `gui.json`).
- If `avalonia.json` already exists, startup will not read legacy files.

## Structure

- `ViewModels`: Root navigation, module state, orchestration actions.
- `Views`: Main shell and feature pages.
- `Features`: Full module surfaces mapped from existing frontend baseline.
