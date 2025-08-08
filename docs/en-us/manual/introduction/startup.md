---
order: 1
icon: ic:round-rocket-launch
---

# Startup

Automatically starts the emulator, launches the client, and enters the game.

Automatic emulator startup requires additional configuration in the startup settings and enabling `Retry launching the emulator when ADB connection fails` in the connection settings.

## Account Switch

This feature needs to be used together with configuration switching or scheduled execution. An account switch operation will be performed each time the `Startup` task is executed.

Only supports switching to already logged-in accounts, using the login name for identification. Please ensure that the input content is unique among all logged-in accounts.

- Examples:
  - Official server: `123****8901` can be entered as `123****8901`, `123`, `8901`, or `3****8`
  - Bilibili server: `Zhang San` can be entered as `Zhang San`, `Zhang`, or `San`

## Connection Settings

[Click here](../connection.md)
